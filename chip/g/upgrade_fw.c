/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "byteorder.h"
#include "compile_time_macros.h"
#include "console.h"
#include "dcrypto/dcrypto.h"
#include "extension.h"
#include "flash.h"
#include "hooks.h"
#include "system.h"
#include "system_chip.h"
#include "registers.h"
#include "uart.h"

#include "signed_header.h"
#include "upgrade_fw.h"
#include "cryptoc/sha.h"

#define CPRINTF(format, args...) cprintf(CC_EXTENSION, format, ## args)

/*
 * This structure defines flash offset ranges of the RO and RW images which
 * are not currently active and as such could be overwritten with an update.
 */
struct {
	uint32_t ro_base_offset;
	uint32_t ro_top_offset;
	uint32_t rw_base_offset;
	uint32_t rw_top_offset;
} valid_sections;

/* Pick sections where updates can go to based on current code addresses. */
static void set_valid_sections(void)
{
	 switch (system_get_ro_image_copy()) {
	 case SYSTEM_IMAGE_RO:
		 valid_sections.ro_base_offset = CHIP_RO_B_MEM_OFF;
		 break;
	 case SYSTEM_IMAGE_RO_B:
		 valid_sections.ro_base_offset = CONFIG_RO_MEM_OFF;
		 break;
	 default:
		 CPRINTF("Failed to set RO image offsets\n");
		 return;
	 }

	switch (system_get_image_copy()) {
	case SYSTEM_IMAGE_RW:
		valid_sections.rw_base_offset = CONFIG_RW_B_MEM_OFF;
		break;
	case SYSTEM_IMAGE_RW_B:
		valid_sections.rw_base_offset = CONFIG_RW_MEM_OFF;
		break;
	default:
		CPRINTF("Failed to set RW image offsets\n");
		return;
	}

	valid_sections.ro_top_offset = valid_sections.ro_base_offset +
		CONFIG_RO_SIZE - 0x800; /* 2K for certs! */

	valid_sections.rw_top_offset = valid_sections.rw_base_offset +
		CONFIG_RW_SIZE;
}

/* Enable write access to the backup RO section. */
static void open_ro_window(uint32_t offset, size_t size_b)
{
	GREG32(GLOBALSEC, FLASH_REGION6_BASE_ADDR) =
		offset + CONFIG_PROGRAM_MEMORY_BASE;
	GREG32(GLOBALSEC, FLASH_REGION6_SIZE) = size_b - 1;
	GWRITE_FIELD(GLOBALSEC, FLASH_REGION6_CTRL, EN, 1);
	GWRITE_FIELD(GLOBALSEC, FLASH_REGION6_CTRL, RD_EN, 1);
	GWRITE_FIELD(GLOBALSEC, FLASH_REGION6_CTRL, WR_EN, 1);
}

/*
 * Verify that the passed in block fits into the valid area. If it does, and
 * is destined to the base address of the area - erase the area contents.
 *
 * Return success, or indication of an erase failure or chunk not fitting into
 * valid area.
 */
static uint8_t check_update_chunk(uint32_t block_offset, size_t body_size)
{
	uint32_t base;
	uint32_t size;

	/* Is this an RW chunk? */
	if (valid_sections.rw_top_offset &&
	    (block_offset >= valid_sections.rw_base_offset) &&
	    ((block_offset + body_size) <= valid_sections.rw_top_offset)) {

		base = valid_sections.rw_base_offset;
		size = valid_sections.rw_top_offset -
			 valid_sections.rw_base_offset;
		/*
		 * If this is the first chunk for this section, it needs to
		 * be erased.
		 */
		if (block_offset == valid_sections.rw_base_offset) {
			if (flash_physical_erase(base, size) != EC_SUCCESS) {
				CPRINTF("%s:%d erase failure of 0x%x..+0x%x\n",
					__func__, __LINE__, base, size);
				return UPGRADE_ERASE_FAILURE;
			}
		}

		return UPGRADE_SUCCESS;
	}

	/* Is this an RO chunk? */
	if (valid_sections.ro_top_offset &&
	    (block_offset >= valid_sections.ro_base_offset) &&
	    ((block_offset + body_size) <= valid_sections.ro_top_offset)) {
		/*
		 * If this is the first chunk for this section, it needs to
		 * be erased.
		 */
		if (block_offset == valid_sections.ro_base_offset) {
			uint32_t base;
			uint32_t size;

			base = valid_sections.ro_base_offset;
			size = valid_sections.ro_top_offset -
				valid_sections.ro_base_offset;
			/* backup RO area write access needs to be enabled. */
			open_ro_window(base, size);
			if (flash_physical_erase(base, size) != EC_SUCCESS) {
				CPRINTF("%s:%d erase failure of 0x%x..+0x%x\n",
					__func__, __LINE__, base, size);
				return UPGRADE_ERASE_FAILURE;
			}
		}
		return UPGRADE_SUCCESS;
	}

	CPRINTF("%s:%d %x, %d ro base %x top %x, rw base %x top %x\n",
		__func__, __LINE__,
		block_offset, body_size,
		valid_sections.ro_base_offset,
		valid_sections.ro_top_offset,
		valid_sections.rw_base_offset,
		valid_sections.rw_top_offset);

	return UPGRADE_BAD_ADDR;
}

int usb_pdu_valid(struct upgrade_command *cmd_body,  size_t cmd_size)
{
	uint8_t sha1_digest[SHA_DIGEST_SIZE];
	size_t body_size = cmd_size - offsetof(struct update_frame_header,
					       cmd.block_base);

	/* Check if the block was received properly. */
	DCRYPTO_SHA1_hash((uint8_t *)&cmd_body->block_base,
			  body_size + sizeof(cmd_body->block_base),
			  sha1_digest);
	if (memcmp(sha1_digest, &cmd_body->block_digest,
		   sizeof(cmd_body->block_digest))) {
		CPRINTF("%s:%d sha1 %x not equal received %x\n",
			__func__, __LINE__,
			*(uint32_t *)sha1_digest, cmd_body->block_digest);
		return 0;
	}

	return 1;
}

#ifndef CR50_DEV

/* Compare two versions, return True if the new version is older. */
static int new_is_older(const struct SignedHeader *new,
			const struct SignedHeader *old)
{
	if (new->epoch_ != old->epoch_)
		return new->epoch_ < old->epoch_;

	if (new->major_ != old->major_)
		return new->major_ < old->major_;


	return new->minor_ < old->minor_;
}

/*
 * Check if this chunk of data is a rollback attempt, or is unaligned and
 * overlaps RO or RW header.
 *
 * Return False if this is such an attempt or an overlap, when in prod mode;
 * otherwise return True.
 */
static int contents_allowed(uint32_t block_offset,
			    size_t body_size, void *upgrade_data)
{
	/* Pointer to RO or RW header in flash, to compare against. */
	const struct SignedHeader *header;

	if (block_offset == valid_sections.ro_base_offset) {
		header = (const struct SignedHeader *)
			get_program_memory_addr(system_get_ro_image_copy());
	} else if (block_offset == valid_sections.rw_base_offset) {
		header = (const struct SignedHeader *)
			get_program_memory_addr(system_get_image_copy());
	} else {

		/*
		 * The received block is not destined to a header directly,
		 * but does it overlap with a header by any chance?
		 */
		int i;
		/* Base offsets of valid headers in flash. */
		uint32_t bases[] = { valid_sections.ro_base_offset,
				     valid_sections.rw_base_offset };
		/* Range of offsets this block is covering. */
		uint32_t range[] = { block_offset, block_offset + body_size };

		for (i = 0; i < ARRAY_SIZE(bases); i++) {
			int j;

			for (j = 0; j < ARRAY_SIZE(range); j++) {
				if ((range[j] >= bases[i]) &&
				    (range[j] <
				     (bases[i] +
				      sizeof(struct SignedHeader)))) {
					CPRINTF("%s:"
						" unaligned block overlaps\n",
						__func__);
					return 0;
				}
			}
		}

		return 1;
	}

	/* This block is a header (ro or rw) of the new image. */
	if (body_size < sizeof(struct SignedHeader)) {
		CPRINTF("%s: block too short\n", __func__);
		return 0;
	}

	/* upgrade_data is the new header. */
	if (new_is_older(upgrade_data, header)) {
		CPRINTF("%s: rejecting an older header.\n", __func__);
		return 0;
	}

	return 1;
}


static uint32_t prev_offset;
static uint64_t prev_timestamp;
#define BACKOFF_TIME (60 * SECOND)

static int chunk_came_too_soon(uint32_t block_offset)
{
	if (!prev_timestamp ||
	    ((get_time().val - prev_timestamp) > BACKOFF_TIME))
		return 0;

	if (!prev_offset ||
	    (block_offset >= (prev_offset + SIGNED_TRANSFER_SIZE)))
		return 0;

	CPRINTF("%s: rejecting a write to the same block\n", __func__);
	return 1;
}

static void new_chunk_written(uint32_t block_offset)
{
	prev_timestamp = get_time().val;
	prev_offset = block_offset;
}
#else
static int chunk_came_too_soon(uint32_t block_offset)
{
	return 0;
}

static void new_chunk_written(uint32_t block_offset)
{
}

static int contents_allowed(uint32_t block_offset,
			    size_t body_size, void *upgrade_data)
{
	return 1;
}
#endif

void fw_upgrade_command_handler(void *body,
				size_t cmd_size,
				size_t *response_size)
{
	struct upgrade_command *cmd_body = body;
	void *upgrade_data;
	uint8_t *error_code = body;  /* Cache the address for code clarity. */
	size_t body_size;
	uint32_t block_offset;

	*response_size = 1; /* One byte response unless this is a start PDU. */

	if (cmd_size < sizeof(struct upgrade_command)) {
		CPRINTF("%s:%d\n", __func__, __LINE__);
		*error_code = UPGRADE_GEN_ERROR;
		return;
	}
	body_size = cmd_size - sizeof(struct upgrade_command);

	if (!cmd_body->block_base && !body_size) {
		struct first_response_pdu *rpdu = body;
		const struct SignedHeader *header;

		/*
		 * This is the connection establishment request, the response
		 * allows the server to decide what sections of the image to
		 * send to program into the flash.
		 */

		/* First, prepare the response structure. */
		memset(rpdu, 0, sizeof(*rpdu));
		*response_size = sizeof(*rpdu);
		rpdu->protocol_version = htobe32(UPGRADE_PROTOCOL_VERSION);

		/*
		 * Determine the valid upgrade sections.
		 */
		set_valid_sections();

		/*
		 * If there have been any problems when determining the valid
		 * Sections offsets/sizes - return an error code.
		 */
		if (!valid_sections.ro_top_offset ||
		    !valid_sections.rw_top_offset) {
			CPRINTF("%s:%d\n", __func__, __LINE__);
			rpdu->return_value = htobe32(UPGRADE_GEN_ERROR);
			return;
		}

		rpdu->backup_ro_offset =
			htobe32(valid_sections.ro_base_offset);

		rpdu->backup_rw_offset =
			htobe32(valid_sections.rw_base_offset);

		/* RO header information. */
		header = (const struct SignedHeader *)
			get_program_memory_addr(system_get_ro_image_copy());
		rpdu->shv[0].minor = htobe32(header->minor_);
		rpdu->shv[0].major = htobe32(header->major_);
		rpdu->shv[0].epoch = htobe32(header->epoch_);
		/* New with protocol version 5 */
		rpdu->keyid[0] = htobe32(header->keyid);

		/* RW header information. */
		header = (const struct SignedHeader *)
			get_program_memory_addr(system_get_image_copy());
		rpdu->shv[1].minor = htobe32(header->minor_);
		rpdu->shv[1].major = htobe32(header->major_);
		rpdu->shv[1].epoch = htobe32(header->epoch_);
		/* New with protocol version 5 */
		rpdu->keyid[1] = htobe32(header->keyid);
		return;
	}

	block_offset = be32toh(cmd_body->block_base);

	if (!usb_pdu_valid(cmd_body, cmd_size)) {
		*error_code = UPGRADE_DATA_ERROR;
		return;
	}

	upgrade_data = cmd_body + 1;
	if (!contents_allowed(block_offset, body_size, upgrade_data)) {
		*error_code = UPGRADE_ROLLBACK_ERROR;
		return;
	}

	/* Check if the block will fit into the valid area. */
	*error_code = check_update_chunk(block_offset, body_size);
	if (*error_code)
		return;

	if (chunk_came_too_soon(block_offset)) {
		*error_code = UPGRADE_RATE_LIMIT_ERROR;
		return;
	}

	CPRINTF("%s: programming at address 0x%x\n", __func__,
		block_offset + CONFIG_PROGRAM_MEMORY_BASE);
	if (flash_physical_write(block_offset, body_size, upgrade_data)
	    != EC_SUCCESS) {
		*error_code = UPGRADE_WRITE_FAILURE;
		CPRINTF("%s:%d upgrade write error\n",	__func__, __LINE__);
		return;
	}

	new_chunk_written(block_offset);

	/* Verify that data was written properly. */
	if (memcmp(upgrade_data, (void *)
		   (block_offset + CONFIG_PROGRAM_MEMORY_BASE),
		   body_size)) {
		*error_code = UPGRADE_VERIFY_ERROR;
		CPRINTF("%s:%d upgrade verification error\n",
			__func__, __LINE__);
		return;
	}

	*error_code = UPGRADE_SUCCESS;
}

void fw_upgrade_complete(void)
{
	system_clear_retry_counter();
}
