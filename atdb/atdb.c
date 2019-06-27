#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <assert.h>

#include "atdb.h"
#include "atdb_io.h"

struct atdb_context *atdb_init(void *ptr, uint32_t length, bool create)
{
	struct atdb_context *atdb;

	atdb = malloc(sizeof(*atdb));

	atdb->ptr = ptr;
	atdb->length = length;

	if (create) {
		atdb->header = (struct atdb_header) {
			.magic = ATDB_MAGIC,
			.header_size = sizeof(struct atdb_header),
		};

		atdb_header_write(atdb, &atdb->header);
	} else {
		atdb_header_read(atdb, &atdb->header);
	}

	atdb->offset = 0;

	return atdb;
}

uint32_t atdb_size(struct atdb_header *header)
{
	return header->header_size +
		header->attr_index_size +
		header->dtree_index_size +
		header->attr_map_size +
		header->attr_value_size +
		header->dtree_fdt_size;
}

void atdb_get_header(struct atdb_context *atdb, struct atdb_header *header)
{
	memcpy(header, &atdb->header, sizeof(struct atdb_header));
}

bool atdb_set_header(struct atdb_context *atdb, struct atdb_header *header)
{
	uint32_t size;

	header->magic = ATDB_MAGIC;
	header->header_size = sizeof(struct atdb_header);

	size = atdb_size(header);
	if (size > atdb->length) {
		return false;
	}

	memcpy(&atdb->header, header, sizeof(struct atdb_header));
	atdb_header_write(atdb, header);
	return true;
}

int atdb_get_attribute(struct atdb_context *atdb,
		       struct pdbg_target *target,
		       const char *attr_name,
		       uint8_t *data,
		       uint32_t *len)
{
	struct atdb_attr_index_entry entry1;
	struct atdb_dtree_index_entry entry2;
	struct atdb_attr_map_entry entry3;
	uint32_t offset;

	if (!atdb_attr_index_find(atdb, attr_name, &entry1, &offset))
		return 1;

	if (*len < (entry1.attr_data_len * entry1.attr_dim)) {
		*len = entry1.attr_data_len * entry1.attr_dim;
		return 2;
	}

	if (!atdb_dtree_index_find(atdb, target, &entry2, &offset))
		return 3;

	if (!atdb_attr_map_find(atdb,
				entry2.attr_map_offset,
				entry2.attr_map_node_size,
				entry1.attr_id,
				&entry3,
				&offset))
		return 4;

	if (entry3.value_flags & ATDB_VALUE_UNDEFINED)
		return 5;

	atdb_attr_value_read(atdb,
			     entry3.value_offset,
			     data,
			     entry1.attr_data_len,
			     entry1.attr_dim);

	return 0;
}

int atdb_set_attribute(struct atdb_context *atdb,
		       struct pdbg_target *target,
		       const char *attr_name,
		       uint8_t *data,
		       uint32_t len)
{
	struct atdb_attr_index_entry entry1;
	struct atdb_dtree_index_entry entry2;
	struct atdb_attr_map_entry entry3;
	uint32_t offset;

	if (!atdb_attr_index_find(atdb, attr_name, &entry1, &offset))
		return 1;

	if (len != (entry1.attr_data_len * entry1.attr_dim)) {
		return 2;
	}

	if (!atdb_dtree_index_find(atdb, target, &entry2, &offset))
		return 3;

	if (!atdb_attr_map_find(atdb,
				entry2.attr_map_offset,
				entry2.attr_map_node_size,
				entry1.attr_id,
				&entry3,
				&offset))
		return 4;

	if (entry3.value_flags & ATDB_VALUE_UNDEFINED) {
		entry3.value_flags &= (~ATDB_VALUE_UNDEFINED);
		atdb_attr_map_write(atdb, offset, &entry3);
	}

	atdb_attr_value_write(atdb,
			      entry3.value_offset,
			      data,
			      entry1.attr_data_len,
			      entry1.attr_dim);
	return 0;
}
