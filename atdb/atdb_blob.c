#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "common/mmap_file.h"

#include "atdb/atdb.h"
#include "atdb/atdb_io.h"
#include "atdb/atdb_blob.h"

struct atdb_blob_info {
	struct mmap_file_context *mfile;
	struct atdb_context *atdb;
};

struct atdb_blob_info *atdb_blob_open(const char *blob, bool allow_write)
{
	struct atdb_blob_info *binfo;
	void *fdt;

	binfo = (struct atdb_blob_info *)malloc(sizeof(struct atdb_blob_info));
	if (!binfo)
		return NULL;

	*binfo = (struct atdb_blob_info) {
		.mfile = NULL,
		.atdb = NULL,
	};

	binfo->mfile = mmap_file_open(blob, allow_write);
	if (!binfo->mfile)
		goto fail;

	binfo->atdb = atdb_init(mmap_file_ptr(binfo->mfile),
				mmap_file_len(binfo->mfile),
				false);
	if (!binfo->atdb) {
		fprintf(stderr, "Failed to allocate atdb\n");
		goto fail;
	}

	fdt = atdb_dtree_fdt_ptr(binfo->atdb);
	pdbg_targets_init(fdt);

	return binfo;

fail:
	atdb_blob_close(binfo);
	return NULL;
}

void atdb_blob_close(struct atdb_blob_info *binfo)
{
	assert(binfo);

	if (binfo->mfile)
		mmap_file_close(binfo->mfile);

	free(binfo);
}

struct atdb_context *atdb_blob_atdb(struct atdb_blob_info *binfo)
{
	return binfo->atdb;
}

void atdb_blob_iterate(struct atdb_blob_info *binfo,
		       struct pdbg_target *target,
		       bool iterate,
		       void (*func1)(struct pdbg_target *, void *),
		       void (*func2)(const char *, uint8_t, uint8_t *, uint32_t, void *),
		       void *private_data)
{
	struct atdb_context *atdb = binfo->atdb;
	struct pdbg_target *child;
	struct atdb_attr_index_entry entry1;
	struct atdb_dtree_index_entry entry2;
	struct atdb_attr_map_entry entry3;
	uint32_t tmp_offset, offset;

	if (!atdb_dtree_index_find(atdb, target, &entry2, &tmp_offset))
		goto end;

	if (entry2.attr_map_node_size == 0)
		goto end;

	func1(target, private_data);

	offset = 0;
	while (offset < entry2.attr_map_node_size) {
		uint8_t *value;
		uint32_t len;
		int ret;

		offset += atdb_attr_map_read(atdb, entry2.attr_map_offset + offset, &entry3);
		atdb_attr_index_by_id(atdb, entry3.attr_id, &entry1, &tmp_offset);

		len = entry1.attr_data_len * entry1.attr_dim;

		value = (uint8_t *)malloc(len);
		assert(value);

		ret = atdb_get_attribute(atdb, target, entry1.attr_name, value, &len);
		assert(ret == 0 || ret == 5);
		if (ret == 5) {
			free(value);
			value = NULL;
		}

		func2(entry1.attr_name, entry1.attr_data_type, value, len, private_data);
		free(value);
	}

end:
	if (!iterate)
		return;

	pdbg_for_each_child_target(target, child) {
		atdb_blob_iterate(binfo, child, iterate, func1, func2, private_data);
	}
}

void atdb_blob_info(struct atdb_blob_info *binfo)
{
	struct atdb_header header;
	uint32_t size;

	atdb_get_header(binfo->atdb, &header);
	size = atdb_size(&header);
	assert(size == mmap_file_len(binfo->mfile));

	printf("  %8u  total size\n", size);
	printf("  %8u  header_size\n", header.header_size);
	printf("  %8u  attr_index_size\n", header.attr_index_size);
	printf("  %8u  dtree_index_size\n", header.dtree_index_size);
	printf("  %8u  attr_map_size\n", header.attr_map_size);
	printf("  %8u  attr_value_size\n", header.attr_value_size);
	printf("  %8u  dtree_fdt_size\n", header.dtree_fdt_size);
}
