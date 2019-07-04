#ifndef __ATDB_H
#define __ATDB_H

#include <stdbool.h>
#include <libpdbg.h>

#define ATDB_MAGIC	0x41544442 /* ATDB */

#define ATDB_ATTR_LEN_MAX	72

struct atdb_context;

struct atdb_header {
	uint32_t magic;
	uint32_t header_size;
	char machine[16];
	uint32_t attr_index_size;
	uint32_t dtree_index_size;
	uint32_t attr_map_size;
	uint32_t attr_value_size;
};

struct atdb_attr_index_entry {
	uint16_t attr_id;
	uint8_t attr_data_type;
	uint8_t attr_data_len;
	uint32_t attr_dim;
	char attr_name[72];
};

struct atdb_dtree_index_entry {
	uint32_t node_hash;
	uint32_t attr_map_offset;
	uint32_t attr_map_node_size;
};

#define ATDB_VALUE_UNDEFINED	0x0001

struct atdb_attr_map_entry {
	uint16_t attr_id;
	uint16_t value_flags;
	uint32_t value_offset;
};

struct atdb_context *atdb_init(void *ptr, uint32_t length, bool create);

uint32_t atdb_size(struct atdb_header *header);

void atdb_get_header(struct atdb_context *atdb, struct atdb_header *header);
bool atdb_set_header(struct atdb_context *atdb, struct atdb_header *header);

int atdb_get_attribute(struct atdb_context *atdb,
		       struct pdbg_target *target,
		       const char *attr_name,
		       uint8_t *data,
		       uint32_t *len);
int atdb_set_attribute(struct atdb_context *atdb,
		       struct pdbg_target *target,
		       const char *attr_name,
		       uint8_t *data,
		       uint32_t len);

#endif /* __ATDB_H */
