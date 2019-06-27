#ifndef __ATDB_BLOB_H
#define __ATDB_BLOB_H

#include <stdbool.h>
#include <stdint.h>

struct atdb_blob_info;

struct atdb_blob_info *atdb_blob_open(const char *blob, bool allow_write);
void atdb_blob_close(struct atdb_blob_info *binfo);

struct atdb_context *atdb_blob_atdb(struct atdb_blob_info *binfo);

void atdb_blob_iterate(struct atdb_blob_info *binfo,
		       struct pdbg_target *target,
		       bool iterate,
		       void (*func1)(struct pdbg_target *, void *),
		       void (*func2)(const char *, uint8_t, uint8_t *, uint32_t, void *),
		       void *private_data);

void atdb_blob_info(struct atdb_blob_info *binfo);

#endif
