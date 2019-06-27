#ifndef __ATDB_IO_H
#define __ATDB_IO_H

#include <stdbool.h>

#include "atdb.h"

struct atdb_context {
	void *ptr;
	uint32_t length;

	struct atdb_header header;
	uint32_t offset;
};

/* From atdb_lowlevel.c */

uint32_t atdb_set_offset(struct atdb_context *atdb, uint32_t offset);
uint32_t atdb_get_offset(struct atdb_context *atdb);

void atdb_write_bytes(struct atdb_context *atdb, uint8_t *bytes, uint32_t count);
void atdb_read_bytes(struct atdb_context *atdb, uint8_t *bytes, uint32_t count);

void atdb_write_uint8(struct atdb_context *atdb, uint8_t value);
void atdb_read_uint8(struct atdb_context *atdb, uint8_t *value);

void atdb_write_uint16(struct atdb_context *atdb, uint16_t value);
void atdb_read_uint16(struct atdb_context *atdb, uint16_t *value);

void atdb_write_uint32(struct atdb_context *atdb, uint32_t value);
void atdb_read_uint32(struct atdb_context *atdb, uint32_t *value);

void atdb_write_uint64(struct atdb_context *atdb, uint64_t value);
void atdb_read_uint64(struct atdb_context *atdb, uint64_t *value);

/* From atdb_io.c */

uint32_t atdb_header_read(struct atdb_context *atdb,
			  struct atdb_header *header);
uint32_t atdb_header_write(struct atdb_context *atdb,
			   struct atdb_header *header);

uint32_t atdb_attr_index_read(struct atdb_context *atdb,
			      uint32_t offset,
			      struct atdb_attr_index_entry *entry);
uint32_t atdb_attr_index_write(struct atdb_context *atdb,
			       uint32_t offset,
			       struct atdb_attr_index_entry *entry);
bool atdb_attr_index_find(struct atdb_context *atdb,
			  const char *attr_name,
			  struct atdb_attr_index_entry *out,
			  uint32_t *attr_index_offset);
void atdb_attr_index_by_id(struct atdb_context *atdb,
			   uint16_t attr_id,
			   struct atdb_attr_index_entry *out,
			   uint32_t *attr_index_offset);
void atdb_attr_index_iterate(struct atdb_context *atdb,
			     void (*callback)(
				     struct atdb_context *atdb,
				     struct atdb_attr_index_entry *entry,
				     void *private_data),
			     void *private_data);

uint32_t atdb_dtree_index_read(struct atdb_context *atdb,
			       uint32_t offset,
			       struct atdb_dtree_index_entry *entry);
uint32_t atdb_dtree_index_write(struct atdb_context *atdb,
				uint32_t offset,
				struct atdb_dtree_index_entry *entry);
bool atdb_dtree_index_find(struct atdb_context *atdb,
			   struct pdbg_target *target,
			   struct atdb_dtree_index_entry *out,
			   uint32_t *dtree_index_offset);
void atdb_dtree_index_iterate(struct atdb_context *atdb,
			      void (*callback)(
				      struct atdb_context *atdb,
				      struct atdb_dtree_index_entry *entry,
				      void *private_data),
			      void *private_data);

uint32_t atdb_attr_map_read(struct atdb_context *atdb,
			    uint32_t offset,
			    struct atdb_attr_map_entry *entry);
uint32_t atdb_attr_map_write(struct atdb_context *atdb,
			     uint32_t offset,
			     struct atdb_attr_map_entry *entry);
bool atdb_attr_map_find(struct atdb_context *atdb,
			uint32_t offset,
			uint32_t node_size,
			uint16_t attr_id,
			struct atdb_attr_map_entry *out,
			uint32_t *attr_map_offset);
void atdb_attr_map_iterate(struct atdb_context *atdb,
			   uint32_t map_offset,
			   uint32_t map_size,
			   void (*callback)(
				   struct atdb_context *atdb,
				   struct atdb_attr_map_entry *entry,
				   void *private_data),
			   void *private_data);

uint32_t atdb_attr_value_read(struct atdb_context *atdb,
			      uint32_t offset,
			      uint8_t *data,
			      uint16_t data_size,
			      uint32_t data_dim);
uint32_t atdb_attr_value_write(struct atdb_context *atdb,
			       uint32_t offset,
			       uint8_t *data,
			       uint16_t data_size,
			       uint32_t data_dim);

void *atdb_dtree_fdt_ptr(struct atdb_context *atdb);
void atdb_dtree_fdt_write(struct atdb_context *atdb, void *fdt, uint32_t length);

void atdb_validate(struct atdb_context *atdb);

#endif /* __ATDB_IO_H */
