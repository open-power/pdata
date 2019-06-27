#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <assert.h>

#include "atdb.h"
#include "atdb_io.h"

uint32_t atdb_set_offset(struct atdb_context *atdb, uint32_t offset)
{
	assert(offset < atdb->length);
	atdb->offset = offset;

	return atdb->offset;
}

uint32_t atdb_get_offset(struct atdb_context *atdb)
{
	return atdb->offset;
}

void atdb_write_bytes(struct atdb_context *atdb, uint8_t *bytes, uint32_t count)
{
	assert(atdb->offset + count <= atdb->length);
	memcpy(atdb->ptr + atdb->offset, bytes, count);
	atdb->offset += count;
}

void atdb_read_bytes(struct atdb_context *atdb, uint8_t *bytes, uint32_t count)
{
	assert(atdb->offset + count <= atdb->length);
	memcpy(bytes, atdb->ptr + atdb->offset, count);
	atdb->offset += count;
}

void atdb_write_uint8(struct atdb_context *atdb, uint8_t value)
{
	atdb_write_bytes(atdb, &value, 1);
}

void atdb_read_uint8(struct atdb_context *atdb, uint8_t *value)
{
	atdb_read_bytes(atdb, value, 1);
}

void atdb_write_uint16(struct atdb_context *atdb, uint16_t value)
{
	uint16_t data = htobe16(value);
	atdb_write_bytes(atdb, (uint8_t *)&data, 2);
}

void atdb_read_uint16(struct atdb_context *atdb, uint16_t *value)
{
	uint16_t data;

	atdb_read_bytes(atdb, (uint8_t *)&data, 2);
	*value = be16toh(data);
}

void atdb_write_uint32(struct atdb_context *atdb, uint32_t value)
{
	uint32_t data = htobe32(value);
	atdb_write_bytes(atdb, (uint8_t *)&data, 4);
}

void atdb_read_uint32(struct atdb_context *atdb, uint32_t *value)
{
	uint32_t data;

	atdb_read_bytes(atdb, (uint8_t *)&data, 4);
	*value = be32toh(data);
}

void atdb_write_uint64(struct atdb_context *atdb, uint64_t value)
{
	uint64_t data = htobe64(value);
	atdb_write_bytes(atdb, (uint8_t *)&data, 8);
}

void atdb_read_uint64(struct atdb_context *atdb, uint64_t *value)
{
	uint64_t data;

	atdb_read_bytes(atdb, (uint8_t *)&data, 8);
	*value = be64toh(data);
}
