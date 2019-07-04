#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ccan/hash/hash.h>

#include "atdb.h"
#include "atdb_io.h"

#ifdef DEBUG_ATDB_IO
#define PRINT	printf
#else
#define PRINT(...)
#endif

/*
 * Section: header
 */

uint32_t atdb_header_read(struct atdb_context *atdb,
			  struct atdb_header *header)
{
	uint32_t begin, end;

	begin = atdb_set_offset(atdb, 0);

	atdb_read_uint32(atdb, &header->magic);
	assert(header->magic == ATDB_MAGIC);
	atdb_read_uint32(atdb, &header->header_size);
	assert(header->header_size == sizeof(struct atdb_header));

	atdb_read_bytes(atdb, (uint8_t *)header->machine, 16);

	atdb_read_uint32(atdb, &header->attr_index_size);
	atdb_read_uint32(atdb, &header->dtree_index_size);
	atdb_read_uint32(atdb, &header->attr_map_size);
	atdb_read_uint32(atdb, &header->attr_value_size);

	end = atdb_get_offset(atdb);

	PRINT("ATDB READ: header %u %u\n", begin, end);

	return end - begin;
}

uint32_t atdb_header_write(struct atdb_context *atdb,
			   struct atdb_header *header)
{
	uint32_t begin, end;

	begin = atdb_set_offset(atdb, 0);

	atdb_write_uint32(atdb, ATDB_MAGIC);
	atdb_write_uint32(atdb, sizeof(struct atdb_header));

	atdb_write_bytes(atdb, (uint8_t *)header->machine, 16);

	atdb_write_uint32(atdb, header->attr_index_size);
	atdb_write_uint32(atdb, header->dtree_index_size);
	atdb_write_uint32(atdb, header->attr_map_size);
	atdb_write_uint32(atdb, header->attr_value_size);

	end = atdb_get_offset(atdb);

	PRINT("ATDB WRITE: header %u %u\n", begin, end);

	return end - begin;
}

/*
 * Section: attribute index
 */

static uint32_t atdb_attr_index_offset(struct atdb_context *atdb,
				       uint32_t offset)
{
	uint32_t base;

	assert(offset < atdb->header.attr_index_size);

	base = atdb->header.header_size;
	return atdb_set_offset(atdb, base + offset);
}

uint32_t atdb_attr_index_read(struct atdb_context *atdb,
			      uint32_t offset,
			      struct atdb_attr_index_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_attr_index_offset(atdb, offset);

	atdb_read_uint16(atdb, &entry->attr_id);
	atdb_read_uint8(atdb, &entry->attr_data_type);
	atdb_read_uint8(atdb, &entry->attr_data_len);
	atdb_read_uint32(atdb, &entry->attr_dim);
	atdb_read_bytes(atdb, (uint8_t *)entry->attr_name, sizeof(entry->attr_name));

	end = atdb_get_offset(atdb);

	PRINT("ATDB READ: attr_index %u %u\n", begin, end);

	return end - begin;
}

uint32_t atdb_attr_index_write(struct atdb_context *atdb,
			       uint32_t offset,
			       struct atdb_attr_index_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_attr_index_offset(atdb, offset);

	atdb_write_uint16(atdb, entry->attr_id);
	atdb_write_uint8(atdb, entry->attr_data_type);
	atdb_write_uint8(atdb, entry->attr_data_len);
	atdb_write_uint32(atdb, entry->attr_dim);
	atdb_write_bytes(atdb, (uint8_t *)entry->attr_name, sizeof(entry->attr_name));

	end = atdb_get_offset(atdb);

	PRINT("ATDB WRITE: attr_index %u %u\n", begin, end);

	return end - begin;
}

bool atdb_attr_index_find(struct atdb_context *atdb,
			  const char *attr_name,
			  struct atdb_attr_index_entry *out,
			  uint32_t *attr_index_offset)
{
	struct atdb_attr_index_entry entry;
	uint32_t reclen = sizeof(struct atdb_attr_index_entry);
	uint32_t offset;
	uint16_t first, middle, last;
	int cmp;

	first = 0;
	last = (atdb->header.attr_index_size / reclen) - 1;

	while (first <= last) {
		middle = (first + last) / 2;
		offset = (reclen * middle);
		atdb_attr_index_read(atdb, offset, &entry);

		cmp = strcmp(attr_name, entry.attr_name);
		if (cmp < 0) {
			if (first == last)
				break;
			last = middle - 1;
		} else if (cmp > 0) {
			if (first == last)
				break;
			first = middle + 1;
		} else {
			*out = entry;
			*attr_index_offset = offset;
			return true;
		}
	}

	return false;
}

void atdb_attr_index_by_id(struct atdb_context *atdb,
			   uint16_t attr_id,
			   struct atdb_attr_index_entry *out,
			   uint32_t *attr_index_offset)
{
	struct atdb_attr_index_entry entry;
	uint32_t offset;

	offset = attr_id * sizeof(struct atdb_attr_index_entry);
	atdb_attr_index_read(atdb, offset, &entry);

	*out = entry;
	*attr_index_offset = offset;
}

void atdb_attr_index_iterate(struct atdb_context *atdb,
			     void (*callback)(
				     struct atdb_context *atdb,
				     struct atdb_attr_index_entry *entry,
				     void *private_data),
			     void *private_data)
{
	struct atdb_attr_index_entry entry;
	uint32_t offset = 0;

	while (offset < atdb->header.attr_index_size) {
		offset += atdb_attr_index_read(atdb, offset, &entry);
		callback(atdb, &entry, private_data);
	}
}

/*
 * Section: device tree index
 */

static uint32_t atdb_dtree_index_offset(struct atdb_context *atdb,
					uint32_t offset)
{
	uint32_t base;

	assert(offset < atdb->header.dtree_index_size);

	base = atdb->header.header_size +
	       atdb->header.attr_index_size;
	return atdb_set_offset(atdb, base + offset);
}

uint32_t atdb_dtree_index_read(struct atdb_context *atdb,
			       uint32_t offset,
			       struct atdb_dtree_index_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_dtree_index_offset(atdb, offset);

	atdb_read_uint32(atdb, &entry->node_hash);
	atdb_read_uint32(atdb, &entry->attr_map_offset);
	atdb_read_uint32(atdb, &entry->attr_map_node_size);

	end = atdb_get_offset(atdb);

	PRINT("ATDB READ: dtree_index %u %u\n", begin, end);

	return end - begin;
}

uint32_t atdb_dtree_index_write(struct atdb_context *atdb,
				uint32_t offset,
				struct atdb_dtree_index_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_dtree_index_offset(atdb, offset);

	atdb_write_uint32(atdb, entry->node_hash);
	atdb_write_uint32(atdb, entry->attr_map_offset);
	atdb_write_uint32(atdb, entry->attr_map_node_size);

	end = atdb_get_offset(atdb);

	PRINT("ATDB WRITE: dtree_index %u %u\n", begin, end);

	return end - begin;
}

bool atdb_dtree_index_find(struct atdb_context *atdb,
			   struct pdbg_target *target,
			   struct atdb_dtree_index_entry *out,
			   uint32_t *dtree_index_offset)
{
	struct atdb_dtree_index_entry entry;
	char *path;
	uint32_t reclen = sizeof(struct atdb_dtree_index_entry);
	uint32_t node_hash, offset;
	uint16_t first, last, i;

	path = pdbg_target_path(target);
	if (!path)
		return false;

	node_hash = hash_stable_8(path, strlen(path), 0);
	free(path);

	first = 0;
	last = (atdb->header.dtree_index_size / reclen) - 1;

	for (i=first; i<=last; i++) {
		offset = reclen * i;
		atdb_dtree_index_read(atdb, offset, &entry);
		if (node_hash == entry.node_hash) {
			*out = entry;
			*dtree_index_offset = offset;
			return true;
		}
	}

	return false;
}

void atdb_dtree_index_iterate(struct atdb_context *atdb,
			      void (*callback)(
				      struct atdb_context *atdb,
				      struct atdb_dtree_index_entry *entry,
				      void *private_data),
			      void *private_data)
{
	struct atdb_dtree_index_entry entry;
	uint32_t offset = 0;

	while (offset < atdb->header.dtree_index_size) {
		offset += atdb_dtree_index_read(atdb, offset, &entry);
		callback(atdb, &entry, private_data);
	}
}

/*
 * Section: attribute map
 */

static uint32_t atdb_attr_map_offset(struct atdb_context *atdb,
				     uint32_t offset)
{
	uint32_t base;

	assert(offset < atdb->header.attr_map_size);

	base = atdb->header.header_size +
	       atdb->header.attr_index_size +
	       atdb->header.dtree_index_size;
	return atdb_set_offset(atdb, base + offset);
}

uint32_t atdb_attr_map_read(struct atdb_context *atdb,
			    uint32_t offset,
			    struct atdb_attr_map_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_attr_map_offset(atdb, offset);

	atdb_read_uint16(atdb, &entry->attr_id);
	atdb_read_uint16(atdb, &entry->value_flags);
	atdb_read_uint32(atdb, &entry->value_offset);

	end = atdb_get_offset(atdb);

	PRINT("ATDB READ: attr_map %u %u\n", begin, end);

	return end - begin;
}

uint32_t atdb_attr_map_write(struct atdb_context *atdb,
			     uint32_t offset,
			     struct atdb_attr_map_entry *entry)
{
	uint32_t begin, end;

	begin = atdb_attr_map_offset(atdb, offset);

	atdb_write_uint16(atdb, entry->attr_id);
	atdb_write_uint16(atdb, entry->value_flags);
	atdb_write_uint32(atdb, entry->value_offset);

	end = atdb_get_offset(atdb);

	PRINT("ATDB WRITE: attr_map %u %u\n", begin, end);

	return end - begin;
}

bool atdb_attr_map_find(struct atdb_context *atdb,
			uint32_t offset,
			uint32_t node_size,
			uint16_t attr_id,
			struct atdb_attr_map_entry *out,
			uint32_t *attr_map_offset)
{
	struct atdb_attr_map_entry entry;
	uint32_t reclen = sizeof(struct atdb_attr_map_entry);
	uint32_t offset2;
	uint16_t first, middle, last;

	if (node_size == 0)
		return false;

	first = 0;
	last = (node_size / reclen) - 1;

	while (first <= last) {
		middle = (first + last) / 2;
		offset2 = reclen * middle;
		atdb_attr_map_read(atdb, offset + offset2, &entry);

		if (attr_id < entry.attr_id) {
			if (first == last)
				break;
			last = middle - 1;
		} else if (attr_id > entry.attr_id) {
			if (first == last)
				break;
			first = middle + 1;
		} else {
			*out = entry;
			*attr_map_offset = offset + offset2;
			return true;
		}
	}

	return false;
}

void atdb_attr_map_iterate(struct atdb_context *atdb,
			   uint32_t map_offset,
			   uint32_t map_size,
			   void (*callback)(
				   struct atdb_context *atdb,
				   struct atdb_attr_map_entry *entry,
				   void *private_data),
			   void *private_data)
{
	struct atdb_attr_map_entry entry;
	uint32_t offset = 0;

	while (offset < map_size) {
		offset += atdb_attr_map_read(atdb, map_offset + offset, &entry);
		callback(atdb, &entry, private_data);
	}
}

/*
 * Section: attribute values
 */

static uint32_t atdb_attr_value_offset(struct atdb_context *atdb,
				       uint32_t offset)
{
	uint32_t base;

	assert(offset < atdb->header.attr_value_size);

	base = atdb->header.header_size +
	       atdb->header.attr_index_size +
	       atdb->header.dtree_index_size +
	       atdb->header.attr_map_size;
	return atdb_set_offset(atdb, base + offset);
}

uint32_t atdb_attr_value_read(struct atdb_context *atdb,
			      uint32_t offset,
			      uint8_t *data,
			      uint16_t data_size,
			      uint32_t data_dim)
{
	uint32_t begin, end, i;
	uint8_t *ptr;

	begin = atdb_attr_value_offset(atdb, offset);

	ptr = data;
	for (i=0; i<data_dim; i++) {
		if (data_size == 1) {
			atdb_read_uint8(atdb, ptr);
		} else if (data_size == 2) {
			atdb_read_uint16(atdb, (uint16_t *)ptr);
		} else if (data_size == 4) {
			atdb_read_uint32(atdb, (uint32_t *)ptr);
		} else if (data_size == 8) {
			atdb_read_uint64(atdb, (uint64_t *)ptr);
		}

		ptr += data_size;
	}

	end = atdb_get_offset(atdb);

	return end - begin;
}

uint32_t atdb_attr_value_write(struct atdb_context *atdb,
			       uint32_t offset,
			       uint8_t *data,
			       uint16_t data_size,
			       uint32_t data_dim)
{
	uint32_t begin, end, i;
	uint8_t *ptr;

	begin = atdb_attr_value_offset(atdb, offset);

	ptr = data;
	for (i=0; i<data_dim; i++) {
		if (data_size == 1) {
			atdb_write_uint8(atdb, *ptr);
		} else if (data_size == 2) {
			atdb_write_uint16(atdb, *(uint16_t *)ptr);
		} else if (data_size == 4) {
			atdb_write_uint32(atdb, *(uint32_t *)ptr);
		} else if (data_size == 8) {
			atdb_write_uint64(atdb, *(uint64_t *)ptr);
		}

		ptr += data_size;
	}

	end = atdb_get_offset(atdb);

	return end - begin;
}

/*
 * Section: device tree fdt
 */

static uint32_t atdb_dtree_fdt_offset(struct atdb_context *atdb)
{
	uint32_t base;

	base = atdb->header.header_size +
	       atdb->header.attr_index_size +
	       atdb->header.dtree_index_size +
	       atdb->header.attr_map_size +
	       atdb->header.attr_value_size;
	return atdb_set_offset(atdb, base);
}

void *atdb_dtree_fdt_ptr(struct atdb_context *atdb)
{
	uint32_t offset;

	offset = atdb_dtree_fdt_offset(atdb);

	return atdb->ptr + offset;
}

void atdb_validate(struct atdb_context *atdb)
{
	struct atdb_attr_index_entry entry1;
	struct atdb_dtree_index_entry entry2;
	struct atdb_attr_map_entry entry3;
	uint32_t offset, offset2;
	uint32_t count, tmp;

	count = 0;
	offset = 0;
	while (offset < atdb->header.attr_index_size) {
		offset += atdb_attr_index_read(atdb, offset, &entry1);
		assert(entry1.attr_id == count);
		count += 1;
	}
	assert(offset == atdb->header.attr_index_size);

	offset = 0;
	offset2 = 0;
	while (offset < atdb->header.dtree_index_size) {
		offset += atdb_dtree_index_read(atdb, offset, &entry2);
		assert(entry2.node_hash != 0);
		if (entry2.attr_map_node_size == 0) {
			assert(entry2.attr_map_offset == 0);
		} else {
			assert(entry2.attr_map_offset == offset2);
		}
		offset2 += entry2.attr_map_node_size;
	}
	assert(offset == atdb->header.dtree_index_size);

	offset = 0;
	offset2 = 0;
	while (offset < atdb->header.attr_map_size) {
		offset += atdb_attr_map_read(atdb, offset, &entry3);
		assert(entry3.value_offset == offset2);

		atdb_attr_index_by_id(atdb, entry3.attr_id, &entry1, &tmp);
		offset2 += (entry1.attr_data_len * entry1.attr_dim);
	}
	assert(offset == atdb->header.attr_map_size);
}
