#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <gdbm.h>

#include <ccan/hash/hash.h>
#include <libpdbg.h>

#include "common/mmap_file.h"
#include "common/dtree.h"

#include "atdb/atdb.h"
#include "atdb/atdb_io.h"
#include "atdb/atdb_blob.h"
#include "attribute.h"
#include "translate.h"

#define	NUM_CHIPLETS	64

struct {
	enum attr_type type;
	char *label;
} attr_type_map[] = {
	{ ATTR_TYPE_UINT8,  "uint8" },
	{ ATTR_TYPE_UINT16, "uint16" },
	{ ATTR_TYPE_UINT32, "uint32" },
	{ ATTR_TYPE_UINT64, "uint64" },
	{ ATTR_TYPE_INT8,   "int8" },
	{ ATTR_TYPE_INT16,  "int16" },
	{ ATTR_TYPE_INT32,  "int32" },
	{ ATTR_TYPE_INT64,  "int64" },
	{ ATTR_TYPE_UNKNOWN, NULL },
};

static enum attr_type attr_type_from_string(char *label)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (strcmp(label, attr_type_map[i].label) == 0)
			return attr_type_map[i].type;
	}

	return ATTR_TYPE_UNKNOWN;
}

static const char *attr_type_to_string(uint8_t type)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (type == attr_type_map[i].type)
			return attr_type_map[i].label;
	}

	return "<NULL>";
}

static int attr_type_size(enum attr_type type)
{
	int size = 0;

	if (type == ATTR_TYPE_UINT8 || type == ATTR_TYPE_INT8) {
		size = 1;
	} else if (type == ATTR_TYPE_UINT16 || type == ATTR_TYPE_INT16) {
		size = 2;
	} else if (type == ATTR_TYPE_UINT32 || type == ATTR_TYPE_INT32) {
		size = 4;
	} else if (type == ATTR_TYPE_UINT64 || type == ATTR_TYPE_INT64) {
		size = 8;
	}

	return size;
}

static char *attr_db_get_value(GDBM_FILE db, char *keystr)
{
	datum key, data;
	char *value;

	key.dptr = keystr;
	key.dsize = strlen(keystr);

	data = gdbm_fetch(db, key);
	if (!data.dptr) {
		fprintf(stderr, "Failed to read key '%s'\n", keystr);
		return NULL;
	}

	value = malloc(data.dsize + 1);
	if (!value) {
		free(data.dptr);
		return NULL;
	}

	strncpy(value, data.dptr, data.dsize);
	value[data.dsize] = '\0';

	free(data.dptr);
	return value;
}

static int count_values(const char *tok)
{
	int count = 0, i;

	for (i=0; i<strlen(tok); i++) {
		if (tok[i] == ' ')
			count++;
	}

	return count + 1;
}

static bool attr_db_read_all(GDBM_FILE db, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int count, i;

	data = attr_db_get_value(db, "all");
	if (!data)
		return false;

	count = count_values(data);

	info->alist.count = count;
	info->alist.attr = (struct attr *)calloc(count, sizeof(struct attr));
	if(!info->alist.attr)
		return false;

	tmp = data;
	for (i=0; i<info->alist.count; i++) {
		tok = strtok(tmp, " ");
		if (!tok)
			return false;

		assert(strlen(tok) < ATTR_MAX_LEN);
		strcpy(info->alist.attr[i].name, tok);

		tmp = NULL;
	}

	free(data);
	return true;
}

static void attr_set_value(char *tok, enum attr_type type, uint8_t *ptr)
{
	unsigned long long int data;

	data = strtoull(tok, NULL, 0);

	if (type == ATTR_TYPE_UINT8 || type == ATTR_TYPE_INT8) {
		uint8_t value = data & 0xff;
		memcpy(ptr, &value, 1);
	} else if (type == ATTR_TYPE_UINT16 || type == ATTR_TYPE_INT16) {
		uint16_t value = data & 0xffff;
		memcpy(ptr, &value, 2);
	} else if (type == ATTR_TYPE_UINT32 || type == ATTR_TYPE_INT32) {
		uint32_t value = data & 0xffffffff;
		memcpy(ptr, &value, 4);
	} else if (type == ATTR_TYPE_UINT64 || type == ATTR_TYPE_INT64) {
		uint64_t value = data;
		memcpy(ptr, &value, 8);
	}
}

static bool attr_db_read_attr(GDBM_FILE db, struct attr_info *info)
{
	char *data, *tok;
	int i, j;

	for (i=0; i<info->alist.count; i++) {
		struct attr *attr = &info->alist.attr[i];
		int defined;

		data = attr_db_get_value(db, attr->name);
		if (!data)
			return false;

		tok = strtok(data, " ");
		if (!tok)
			return false;

		attr->type = attr_type_from_string(tok);
		assert(attr->type != ATTR_TYPE_UNKNOWN);

		attr->data_size = attr_type_size(attr->type);
		assert(attr->data_size > 0);

		tok = strtok(NULL, " ");
		if (!tok)
			return false;

		attr->dim_count = atoi(tok);
		assert(attr->dim_count >= 0 && attr->dim_count < 4);

		attr->size = 1;
		if (attr->dim_count > 0) {
			attr->dim = (int *)malloc(attr->dim_count * sizeof(int));
			assert(attr->dim);

			for (j=0; j<attr->dim_count; j++) {
				tok = strtok(NULL, " ");
				if (!tok)
					return false;

				attr->dim[j] = atoi(tok);
				attr->size *= attr->dim[j];
			}
			assert(attr->size >= 1);
		}

		tok = strtok(NULL, " ");
		if (!tok)
			return false;

		attr->enum_count = atoi(tok);
		attr->aenum = (struct attr_enum *)malloc(attr->enum_count * sizeof(struct attr_enum));
		assert(attr->aenum);

		for (j=0; j<attr->enum_count; j++) {
			tok = strtok(NULL, " ");
			if (!tok)
				return false;

			attr->aenum[j].key = strdup(tok);
			assert(attr->aenum[j].key);

			tok = strtok(NULL, " ");
			if (!tok)
				return false;

			attr->aenum[j].value = strtoull(tok, NULL, 0);
		}

		tok = strtok(NULL, " ");
		if (!tok)
			return false;

		defined = atoi(tok);
		assert(defined == 0 || defined == 1);
		attr->defined = (defined == 1);

		if (defined == 1) {
			uint8_t *ptr;
			int count;

			/* Special case of instance attribute ATTR_PG */
			if (!strcmp(attr->name, "ATTR_PG"))
				count = NUM_CHIPLETS;
			else
				count = attr->size;

			attr->default_value = (uint8_t *)malloc(count * attr->data_size);
			if (!attr->default_value)
				return false;

			ptr = attr->default_value;

			for (j=0; j<count; j++) {
				tok = strtok(NULL, " ");
				if (!tok)
					return false;

				attr_set_value(tok, attr->type, ptr);
				ptr += attr->data_size;
			}
		}

		free(data);
	}

	return true;

}

static bool attr_db_read_targets(GDBM_FILE db, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int count, i;

	data = attr_db_get_value(db, "targets");
	if (!data)
		return false;

	count = count_values(data);

	info->tlist.count = count;
	info->tlist.target = (struct target *)malloc(count * sizeof(struct target));
	if(!info->tlist.target)
		return false;

	tmp = data;
	for (i=0; i<info->tlist.count; i++) {
		tok = strtok(tmp, " ");
		if (!tok)
			return false;

		assert(strlen(tok) < TARGET_MAX_LEN);
		strcpy(info->tlist.target[i].name, tok);

		tmp = NULL;
	}

	free(data);
	return true;
}

static bool attr_db_read_target(GDBM_FILE db, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int i, j;

	for (i=0; i<info->tlist.count; i++) {
		struct target *target = &info->tlist.target[i];

		data = attr_db_get_value(db, target->name);
		if (!data)
			return false;

		target->id_count = count_values(data);
		target->id = (int *)malloc(target->id_count * sizeof(int));
		if (!target->id)
			return false;

		tmp = data;
		for (j=0; j<target->id_count; j++) {
			tok = strtok(tmp, " ");
			if (!tok)
				return false;

			target->id[j] = atoi(tok);
			tmp = NULL;
		}

		free(data);
	}

	return true;
}

bool attr_db_load(const char *filename, struct attr_info *info)
{
	GDBM_FILE db;
	bool rc;

	db = gdbm_open(filename, 0, GDBM_READER, 0, NULL);
	if (!db) {
		fprintf(stderr, "Failed to open db: %s\n", filename);
		return false;
	}

	rc = attr_db_read_all(db, info);
	if (!rc) {
		fprintf(stderr, "Failed to read all\n");
		goto done;
	}

	rc = attr_db_read_attr(db, info);
	if (!rc) {
		fprintf(stderr, "Failed to read attributes\n");
		goto done;
	}

	rc = attr_db_read_targets(db, info);
	if (!rc) {
		fprintf(stderr, "Failed to read targets\n");
		goto done;
	}

	rc = attr_db_read_target(db, info);
	if (!rc) {
		fprintf(stderr, "Failed to read per target attributes\n");
		goto done;
	}

done:
	gdbm_close(db);
	return rc;
}

static void device_tree_set_target(struct attr_info *ainfo,
				   struct dtree_context *dtree)
{
	char root_classname[] = "root";
	struct dtree_node *node;

	for_each_dtree_node(dtree, node) {
		const char *classname;
		struct pdbg_target *target = dtree_node_target(node);
		int i;

		classname = pdbg_target_class_name(target);
		if (!classname && target == pdbg_target_root())
			classname = root_classname;

		if (!classname) {
			char *path;

			path = pdbg_target_path(target);
			assert(path);

			fprintf(stderr, "%s does not have classname\n", path);
			free(path);

			continue;
		}

		for (i=0; i<ainfo->tlist.count; i++) {
			struct target *attr_target = &ainfo->tlist.target[i];

			if (strcmp(classname, attr_target->name) == 0) {
				dtree_node_set_private(node, attr_target);
				break;
			}
		}

		/*
		if (!dtree_node_get_private(node))
			fprintf(stderr, "No attributes for %s\n",
				pdbg_target_dn_name(target));
		*/
	}
}

static void fill_atdb_header(struct attr_info *ainfo,
			     struct dtree_context *dtree,
			     const char *machine,
			     struct atdb_header *header)
{
	struct dtree_node *node;
	uint32_t map_size = 0, value_size = 0;
	int count;

	memset(header, 0, sizeof(*header));

	header->header_size = sizeof(struct atdb_header);

	strncpy(header->machine, machine, 15);
	header->attr_index_size =
		ainfo->alist.count * sizeof(struct atdb_attr_index_entry);
	count = dtree_num_nodes(dtree);
	header->dtree_index_size =
		count * sizeof(struct atdb_dtree_index_entry);

	for_each_dtree_node(dtree, node) {
		struct target *attr_target = (struct target *)dtree_node_get_private(node);
		int i;

		if (!attr_target)
			continue;

		map_size += attr_target->id_count * sizeof(struct atdb_attr_map_entry);

		for (i=0; i<attr_target->id_count; i++) {
			int attr_id = attr_target->id[i];
			struct attr *attr = &ainfo->alist.attr[attr_id];

			value_size += attr->data_size * attr->size;
		}
	}

	header->attr_map_size = map_size;
	header->attr_value_size = value_size;
}

static bool blob_write(const char *filename,
		       struct attr_info *ainfo,
		       struct dtree_context *dtree,
		       const char *machine)
{
	struct atdb_context *atdb;
	struct mmap_file_context *mfile;
	struct dtree_node *node;
	struct atdb_header header;
	struct atdb_attr_index_entry entry1;
	struct atdb_dtree_index_entry entry2;
	struct atdb_attr_map_entry entry3;
	uint32_t size, offset;
	int i;

	fill_atdb_header(ainfo, dtree, machine, &header);
	size = atdb_size(&header);
	printf("Blob size = %u\n", size);

	mfile = mmap_file_create(filename, size);
	if (!mfile)
		return false;

	atdb = atdb_init(mmap_file_ptr(mfile), mmap_file_len(mfile), true);
	if (!atdb_set_header(atdb, &header)) {
		fprintf(stderr, "Failed to write header\n");
		return false;
	}

	/* Attribute index */
	offset = 0;
	for (i=0; i<ainfo->alist.count; i++) {
		struct attr *attr = &ainfo->alist.attr[i];

		entry1 = (struct atdb_attr_index_entry) {
			.attr_id = i,
			.attr_data_type = attr->type,
			.attr_data_len = attr->data_size,
			.attr_dim = attr->size,
		};

		strcpy(entry1.attr_name, attr->name);

		offset += atdb_attr_index_write(atdb, offset, &entry1);
	}

	/* Device tree index */
	offset = 0;
	for_each_dtree_node(dtree, node) {
		entry2 = (struct atdb_dtree_index_entry) {
			.node_hash = dtree_node_hash(node),
		};

		offset += atdb_dtree_index_write(atdb, offset, &entry2);
	}

	/* Attribute map */
	offset = 0;
	for_each_dtree_node(dtree, node) {
		struct pdbg_target *target = dtree_node_target(node);
		struct target *attr_target = dtree_node_get_private(node);
		uint32_t dtree_offset;

		if (!attr_target)
			continue;

		if (!atdb_dtree_index_find(atdb, target, &entry2, &dtree_offset)) {
			fprintf(stderr, "Failed to find dtree index for %s\n",
				pdbg_target_dn_name(target));
			return false;
		}

		entry2.attr_map_offset = offset;

		for (i=0; i<attr_target->id_count; i++) {
			int id = attr_target->id[i];

			entry3 = (struct atdb_attr_map_entry) {
				.attr_id = id,
			};

			offset += atdb_attr_map_write(atdb, offset, &entry3);
		}

		entry2.attr_map_node_size = offset - entry2.attr_map_offset;

		atdb_dtree_index_write(atdb, dtree_offset, &entry2);
	}

	/* Attribute value block */
	offset = 0;
	for_each_dtree_node(dtree, node) {
		struct pdbg_target *target = dtree_node_target(node);
		struct target *attr_target = (struct target *)dtree_node_get_private(node);
		uint32_t dtree_offset, map_offset;

		if (!attr_target)
			continue;

		if (!atdb_dtree_index_find(atdb, target, &entry2, &dtree_offset)) {
			fprintf(stderr, "Failed to find dtree index for %s\n",
				pdbg_target_dn_name(target));
			return false;
		}

		for (i=0; i<attr_target->id_count; i++) {
			int id = attr_target->id[i];
			struct attr *attr = &ainfo->alist.attr[id];

			if (!atdb_attr_map_find(atdb, entry2.attr_map_offset, entry2.attr_map_node_size, id, &entry3, &map_offset)) {
				fprintf(stderr, "Failed to find attr map for %s,%s\n",
				pdbg_target_dn_name(target), attr->name);
				return false;
			}

			if (!attr->defined) {
				entry3.value_flags = ATDB_VALUE_UNDEFINED;
			}
			entry3.value_offset = offset;

			atdb_attr_map_write(atdb, map_offset, &entry3);

			if (attr->defined) {
				uint8_t *ptr;

				if (!strcmp(attr->name, "ATTR_PG")) {
					uint32_t index = pdbg_target_index(target);
					assert(index < NUM_CHIPLETS);

					ptr = attr->default_value + index * attr->data_size;
				} else {
					ptr = attr->default_value;
				}
				offset += atdb_attr_value_write(atdb, offset, ptr, attr->data_size, attr->size);
			} else {
				offset += (attr->data_size * attr->size);
			}
		}
	}

	mmap_file_close(mfile);

	return true;
}

static int do_create(const char *machine, const char *infodb)
{
	struct attr_info ainfo;
	struct dtree_context *dtree;
	char path[1024];

	sprintf(path, "attributes.atdb");

	if (!attr_db_load(infodb, &ainfo)) {
		return 1;
	}

	dtree = dtree_init(pdbg_default_dtb());
	if (!dtree)
		return 3;

	device_tree_set_target(&ainfo, dtree);

	if (!blob_write(path, &ainfo, dtree, machine))
		return 4;

	dtree_free(dtree);

	printf("Created %s\n", path);
	return 0;
}

static int do_info(const char *blob)
{
	struct atdb_blob_info *binfo;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 1;

	atdb_blob_info(binfo);
	atdb_blob_close(binfo);

	return 0;
}

static void do_debug_attr_index(struct atdb_context *atdb,
				struct atdb_attr_index_entry *entry,
				void *private_data)
{
	uint32_t *count = (uint32_t *)private_data;
	*count += 1;
}

static void do_debug_dtree_index(struct atdb_context *atdb,
				 struct atdb_dtree_index_entry *entry,
				 void *private_data)
{
	uint32_t *count = (uint32_t *)private_data;
	*count += 1;
}

static void do_debug_attr_map(struct atdb_context *atdb,
			      struct atdb_attr_map_entry *entry,
			      void *private_data)
{
	uint32_t *count = (uint32_t *)private_data;
	*count += 1;
}

static void do_debug_target(struct atdb_context *atdb,
			    struct pdbg_target *target)
{
	struct pdbg_target *child;
	char *path;
	struct atdb_dtree_index_entry entry;
	uint32_t offset, count = 0;

	path = pdbg_target_path(target);
	assert(path);

	assert(atdb_dtree_index_find(atdb, target, &entry, &offset));

	atdb_attr_map_iterate(atdb,
			      entry.attr_map_offset,
			      entry.attr_map_node_size,
			      do_debug_attr_map,
			      &count);
	printf("  %s: %u attributes\n", path, count);

	pdbg_for_each_child_target(target, child) {
		do_debug_target(atdb, child);
	}
}

static int do_debug(const char *blob)
{
	struct atdb_blob_info *binfo;
	struct atdb_context *atdb;
	uint32_t count;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 1;

	atdb = atdb_blob_atdb(binfo);
	atdb_validate(atdb);

	count = 0;
	atdb_attr_index_iterate(atdb, do_debug_attr_index, &count);
	printf("Total attributes = %u\n", count);

	count = 0;
	atdb_dtree_index_iterate(atdb, do_debug_dtree_index, &count);
	printf("Total dtree nodes = %u\n", count);

	do_debug_target(atdb, pdbg_target_root());

	atdb_blob_close(binfo);

	return 0;
}

static void do_dump_value(uint8_t *value, uint32_t len, uint8_t type, uint16_t dim)
{
	enum attr_type atype = type;
	uint8_t *ptr;
	int i;

	ptr = value;
	for (i=0; i<dim; i++) {
		switch (atype) {
		case ATTR_TYPE_UINT8:
			printf(" 0x%x", *ptr);
			ptr += 1;
			break;

		case ATTR_TYPE_UINT16:
			printf(" 0x%x", *(uint16_t *)ptr);
			ptr += 2;
			break;

		case ATTR_TYPE_UINT32:
			printf(" 0x%"PRIx32, *(uint32_t *)ptr);
			ptr += 4;
			break;

		case ATTR_TYPE_UINT64:
			printf(" 0x%"PRIx64, *(uint64_t *)ptr);
			ptr += 8;
			break;

		case ATTR_TYPE_INT8:
			printf(" %d", *(int8_t *)ptr);
			ptr += 1;
			break;

		case ATTR_TYPE_INT16:
			printf(" %d", *(int16_t *)ptr);
			ptr += 2;
			break;

		case ATTR_TYPE_INT32:
			printf(" %d", *(int32_t *)ptr);
			ptr += 4;
			break;

		case ATTR_TYPE_INT64:
			printf(" 0x%"PRIx64, *(int64_t *)ptr);
			ptr += 8;
			break;

		default:
			break;
		}
	}

	printf("\n");
}

static void do_dump_func1(struct pdbg_target *target, void *private_data)
{
	char *path;

	path = pdbg_target_path(target);
	assert(path);
	printf("%s\n", path);
	free(path);
}

static void do_dump_func2(const char *attr_name, uint8_t data_type, uint8_t *value, uint32_t len, void *private_data)
{
	printf("  %s: %s", attr_name, attr_type_to_string(data_type));
	if (value == NULL) {
		printf(" UNDEFINED\n");
	} else {
		uint16_t dim = len / data_type;

		if (dim <= 4) {
			do_dump_value(value, len, data_type, dim);
		} else {
			printf(" [%u]\n", dim);
		}
	}
}

static int do_dump(const char *blob, const char *arg)
{
	struct pdbg_target *target;
	struct atdb_blob_info *binfo;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 1;

	if (arg) {
		if (arg[0] == '/') {
			target = pdbg_target_from_path(NULL, arg);
		} else {
			target = from_cronus_target(arg);
		}
		if (!target) {
			fprintf(stderr, "Failed to translate %s\n", arg);
			return 2;
		}

		atdb_blob_iterate(binfo, target, false, do_dump_func1, do_dump_func2, NULL);
	} else {
		atdb_blob_iterate(binfo, pdbg_target_root(), true, do_dump_func1, do_dump_func2, NULL);
	}

	atdb_blob_close(binfo);

	return 0;
}

struct do_export_state {
	struct attr_info *ainfo;
	struct dtree_context *dtree;
	struct target *target;
};

void do_export_data_type(struct attr *attr)
{
	switch (attr->type) {
	case ATTR_TYPE_UINT8:
		printf("u8");
		break;

	case ATTR_TYPE_UINT16:
		printf("u16");
		break;

	case ATTR_TYPE_UINT32:
		printf("u32");
		break;

	case ATTR_TYPE_UINT64:
		printf("u64");
		break;

	case ATTR_TYPE_INT8:
		printf("s8");
		break;

	case ATTR_TYPE_INT16:
		printf("s16");
		break;

	case ATTR_TYPE_INT32:
		printf("s32");
		break;

	case ATTR_TYPE_INT64:
		printf("s64");
		break;

	default:
		printf("UNKNWON");
		break;
	}

	if (attr->enum_count > 0) {
		printf("e");
	}
}

int do_export_value_string(struct attr *attr, uint8_t *value)
{
	uint64_t val = 0;
	int used = 0;

	if (attr->type == ATTR_TYPE_UINT8 || attr->type == ATTR_TYPE_INT8) {
		val = *(uint8_t *)value;
		used = 1;
	} else if (attr->type == ATTR_TYPE_UINT16 || attr->type == ATTR_TYPE_INT16) {
		val = *(uint16_t *)value;
		used = 2;
	} else if (attr->type == ATTR_TYPE_UINT32 || attr->type == ATTR_TYPE_INT32) {
		val = *(uint32_t *)value;
		used = 4;
	} else if (attr->type == ATTR_TYPE_UINT64 || attr->type == ATTR_TYPE_INT64) {
		val = *(uint64_t *)value;
		used = 8;
	}

	if (attr->enum_count > 0) {
		int i;

		for (i=0; i<attr->enum_count; i++) {
			if (attr->aenum[i].value == val) {
				printf("%s", attr->aenum[i].key);
				return used;
			}
		}
		printf("UNKNOWN_ENUM");
		return used;
	}

	if (used == 1) {
		printf("0x%02x", (uint8_t)val);
	} else if (used == 2) {
		printf("0x%04x", (uint16_t)val);
	} else if (used == 4) {
		printf("0x%08x", (uint32_t)val);
	} else if (used == 8) {
		printf("0x%016llx", (unsigned long long)val);
	} else {
		printf("ERROR%d", used);
	}

	return used;
}

void do_export_value_scalar(struct attr *attr, uint8_t *value, uint32_t len)
{
	int used;

	printf("%s", attr->name);
	printf("    ");
	do_export_data_type(attr);
	printf("    ");
	used = do_export_value_string(attr, value);
	assert(used == len);
	printf("\n");
}

void do_export_value_array1(struct attr *attr, uint8_t *value, uint32_t len)
{
	int offset = 0, used;
	int i;

	for (i=0; i<attr->dim[0]; i++) {
		printf("%s", attr->name);
		printf("[%d]", i);
		printf(" ");
		do_export_data_type(attr);
		printf("[%d]", attr->dim[0]);
		printf(" ");
		used = do_export_value_string(attr, value + offset);
		assert(offset + used <= len);
		offset += used;
		printf("\n");
	}
}

void do_export_value_array2(struct attr *attr, uint8_t *value, uint32_t len)
{
	int offset = 0, used;
	int i, j;

	for (i=0; i<attr->dim[0]; i++) {
		for (j=0; j<attr->dim[1]; j++) {
			printf("%s", attr->name);
			printf("[%d][%d]", i, j);
			printf(" ");
			do_export_data_type(attr);
			printf("[%d][%d]", attr->dim[0], attr->dim[1]);
			printf(" ");
			used = do_export_value_string(attr, value + offset);
			assert(offset + used <= len);
			offset += used;
			printf("\n");
		}
	}
}

void do_export_value_array3(struct attr *attr, uint8_t *value, uint32_t len)
{
	int offset = 0, used;
	int i, j, k;

	for (i=0; i<attr->dim[0]; i++) {
		for (j=0; j<attr->dim[1]; j++) {
			for (k=0; k<attr->dim[2]; k++) {
				printf("%s", attr->name);
				printf("[%d][%d][%d]", i, j, k);
				printf(" ");
				do_export_data_type(attr);
				printf("[%d][%d][%d]", attr->dim[0], attr->dim[1], attr->dim[2]);
				printf(" ");
				used = do_export_value_string(attr, value + offset);
				assert(offset + used <= len);
				offset += used;
				printf("\n");
			}
		}
	}
}


void do_export_value(struct attr *attr, uint8_t *value, uint32_t len)
{
	if (attr->dim_count == 0) {
		do_export_value_scalar(attr, value, len);
	} else if (attr->dim_count == 1) {
		do_export_value_array1(attr, value, len);
	} else if (attr->dim_count == 2) {
		do_export_value_array2(attr, value, len);
	} else if (attr->dim_count == 3) {
		do_export_value_array3(attr, value, len);
	}
}

static void do_export_func1(struct pdbg_target *target, void *private_data)
{
	struct do_export_state *state = (struct do_export_state *)private_data;
	struct dtree_node *node;
	char *name;

	state->target = NULL;
	for_each_dtree_node(state->dtree, node) {
		if (dtree_node_target(node) == target) {
			state->target = (struct target *)dtree_node_get_private(node);
		}
	}

	assert(state->target);

	name = to_cronus_target(target);
	if (name == NULL) {
		state->target = NULL;
		return;
	}

	printf("target = %s\n", name);
	free(name);
}

static void do_export_func2(const char *attr_name, uint8_t data_type, uint8_t *value, uint32_t len, void *private_data)
{
	struct do_export_state *state = (struct do_export_state *)private_data;
	struct attr *attr;
	int i;

	if (state->target == NULL || value == NULL)
		return;

	for (i=0; i<state->target->id_count; i++) {
		int id = state->target->id[i];

		attr = &state->ainfo->alist.attr[id];
		if (!strcmp(attr_name, attr->name))
			break;

		attr = NULL;
	}

	assert(attr);

	do_export_value(attr, value, len);
}

static int do_export(const char *blob, const char *infodb)
{
	struct attr_info ainfo;
	struct atdb_blob_info *binfo;
	struct dtree_context *dtree;
	struct do_export_state state;

	if (!attr_db_load(infodb, &ainfo))
		return 1;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 2;

	dtree = dtree_init(pdbg_default_dtb());
	if (!dtree)
		return 3;

	device_tree_set_target(&ainfo, dtree);

	state = (struct do_export_state) {
		.ainfo = &ainfo,
		.dtree = dtree,
	};

	atdb_blob_iterate(binfo, pdbg_target_root(), true, do_export_func1, do_export_func2, &state);

	dtree_free(dtree);
	atdb_blob_close(binfo);

	return 0;
}

struct do_import_state {
	struct attr_info *ainfo;
	struct dtree_context *dtree;
	struct atdb_context *atdb;
	struct dtree_node *node;

	struct attr *attr;
	void *value;
	int count;
};

static bool do_import_parse_target(struct do_import_state *state, char *line)
{
	struct pdbg_target *target;
	struct dtree_node *node;
	char *tok, *ptr, *path;

	state->node = NULL;

	tok = strtok(line, "=");
	if (!tok)
		return false;

	tok = strtok(NULL, "=");
	if (!tok)
		return false;

	ptr = tok;
	while (*ptr == ' ')
		ptr++;

	target = from_cronus_target(ptr);
	if (!target) {
		fprintf(stderr, "%s --> Failed to translate\n", ptr);
		return false;
	}

	for_each_dtree_node(state->dtree, node) {
		if (dtree_node_target(node) == target) {
			state->node = node;
		}
	}
	assert(state->node);

	path = pdbg_target_path(target);
	assert(path);

	fprintf(stderr, "%s --> %s\n", ptr, path);
	free(path);

	return true;
}

static bool do_import_parse_attr(struct do_import_state *state, char *line)
{
	struct target *attr_target;
	struct attr *attr;
	char *attr_name, *data_type, *value_str;
	char *tok, *saveptr = NULL;
	unsigned long long value;
	int idx[3] = { 0, 0, 0 };
	int dim[3] = { -1, -1, -1 };
	int i, ret, index;
	bool is_enum = false;

	if (!state->node)
		return true;

	/* attribute name */
	tok = strtok_r(line, " ", &saveptr);
	if (!tok)
		return false;

	attr_name = strtok(tok, "[");
	if (!attr_name)
		return false;

	if (strlen(attr_name) < 4 || strncmp(attr_name, "ATTR", 4))
		return true;

	for (i=0; i<3; i++) {
		tok = strtok(NULL, "[");
		if (!tok)
			break;

		assert(tok[strlen(tok)-1] == ']');
		tok[strlen(tok)-1] = '\0';
		idx[i] = atoi(tok);
	}

	/* attribute data type */
	tok = strtok_r(NULL, " ", &saveptr);
	if (!tok)
		return false;

	data_type = strtok(tok, "[");
	if (!data_type)
		return false;

	if (data_type[strlen(data_type)-1] == 'e') {
		data_type[strlen(data_type)-1] = '\0';
		is_enum = true;
	}

	for (i=0; i<3; i++) {
		tok = strtok(NULL, "[");
		if (!tok)
			break;

		assert(tok[strlen(tok)-1] == ']');
		tok[strlen(tok)-1] = '\0';
		dim[i] = atoi(tok);
	}

	/* attribute value */
	value_str = strtok_r(NULL, " ", &saveptr);
	if (!value_str)
		return false;

	attr_target = (struct target *)dtree_node_get_private(state->node);
	for (i=0; i<attr_target->id_count; i++) {
		int id = attr_target->id[i];

		attr = &state->ainfo->alist.attr[id];
		if (!strcmp(attr_name, attr->name))
			break;

		attr = NULL;
	}

	if (!attr) {
		fprintf(stderr, "  %s: attribute not found\n", attr_name);
		return false;
	}

	for (i=0; i<attr->dim_count; i++) {
		if (dim[i] != attr->dim[i]) {
			int j;

			fprintf(stderr, "  %s: dim mismatch ", attr_name);
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", dim[j]);
			fprintf(stderr, " != ");
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", attr->dim[j]);
			fprintf(stderr, "\n");
			assert(dim[i] == attr->dim[i]);
		}
	}
	for (i=0; i<attr->dim_count; i++) {
		if (idx[i] >= attr->dim[i]) {
			int j;

			fprintf(stderr, "  %s: index overflow ", attr_name);
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", idx[j]);
			fprintf(stderr, " > ");
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", attr->dim[j]);
			fprintf(stderr, "\n");
			assert(idx[i] < attr->dim[i]);
		}
	}

	if (state->attr != attr) {
		uint32_t len;

		if (state->value)
			free(state->value);

		state->attr = attr;

		len = attr->data_size * attr->size;
		state->value = malloc(len);
		assert(state->value);

		state->count = 0;
	}

	index = 0;
	if (attr->dim_count == 1) {
		index = idx[0];
	} else if (attr->dim_count == 2) {
		index = idx[0] * attr->dim[0] + idx[1];
	} else if (attr->dim_count == 3) {
		index = idx[0] * attr->dim[0] + idx[1] * attr->dim[1] + idx[2];
	}

	if (is_enum) {
		bool found = false;

		for (i=0; i<attr->enum_count; i++) {
			if (!strcmp(attr->aenum[i].key, value_str)) {
				value = attr->aenum[i].value;
				found = true;
				break;
			}
		}
		assert(found);
	} else {
		value = strtoull(value_str, NULL, 0);
	}

	if (attr->data_size == 1) {
		uint8_t *ptr = (uint8_t *)state->value;
		uint8_t val = value;
		ptr[index] =  val;

	} else if (attr->data_size == 2) {
		uint16_t *ptr = (uint16_t *)state->value;
		uint16_t val = value;
		ptr[index] = val;

	} else if (attr->data_size == 4) {
		uint32_t *ptr = (uint32_t *)state->value;
		uint32_t val = value;
		ptr[index] = val;

	} else if (attr->data_size == 8) {
		uint64_t *ptr = (uint64_t *)state->value;
		uint64_t val = value;
		ptr[index] = val;
	}

	state->count++;

	if (state->count < attr->size) {
		return true;
	}

	ret = atdb_set_attribute(state->atdb, dtree_node_target(state->node), attr_name, state->value, attr->data_size * attr->size);
	if (ret != 0) {
		fprintf(stderr, "  %s: failed to set, ret=%d\n", attr_name, ret);
		return false;
	}

	fprintf(stderr, "  %s: imported\n", attr_name);

	state->attr = NULL;
	free(state->value);
	state->value = NULL;
	state->count = 0;

	return true;
}

static bool do_import_parse(struct do_import_state *state, char *line)
{
	char str[128];

	/* Skip empty lines */
	if (strlen(line) < 6)
		return true;

	strcpy(str, line);

	if (!strncmp(line, "target", 6)) {
		if (!do_import_parse_target(state, line))
			return true;
	} else {
		if (!do_import_parse_attr(state, line))
			return true;
	}

	return true;
}

static int do_import(const char *blob, const char *infodb, const char *dump)
{
	struct attr_info ainfo;
	struct atdb_blob_info *binfo;
	struct atdb_context *atdb;
	struct dtree_context *dtree;
	struct do_import_state state;
	char line[1024], *ptr;
	FILE *fp;

	if (!attr_db_load(infodb, &ainfo))
		return 1;

	binfo = atdb_blob_open(blob, true);
	if (!binfo)
		return 2;

	atdb = atdb_blob_atdb(binfo);

	dtree = dtree_init(pdbg_default_dtb());
	if (!dtree)
		return 3;

	device_tree_set_target(&ainfo, dtree);

	state = (struct do_import_state) {
		.ainfo = &ainfo,
		.dtree = dtree,
		.atdb = atdb,
	};

	fp = fopen(dump, "r");
	if (!fp) {
		fprintf(stderr, "Failed to open cronus attr dump '%s'\n", dump);
		return 4;
	}

	while ((ptr = fgets(line, sizeof(line), fp))) {
		assert(ptr[strlen(ptr)-1] == '\n');
		ptr[strlen(ptr)-1] = '\0';

		if (!do_import_parse(&state, line))
			break;
	}
	fclose(fp);

	dtree_free(dtree);
	atdb_blob_close(binfo);

	return 0;
}

static int do_read(const char *blob, const char *arg, const char *attr_name)
{
	struct atdb_blob_info *binfo;
	struct atdb_context *atdb;
	struct atdb_attr_index_entry entry;
	struct pdbg_target *target;
	uint8_t *data;
	uint32_t offset, len;
	int ret, i;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 2;

	atdb = atdb_blob_atdb(binfo);

	if (arg[0] == '/') {
		target = pdbg_target_from_path(NULL, arg);
	} else {
		target = from_cronus_target(arg);
	}
	if (!target) {
		fprintf(stderr, "Failed to translate %s\n", arg);
		return 3;
	}

	if (!atdb_attr_index_find(atdb, attr_name, &entry, &offset)) {
		fprintf(stderr, "No such attribute %s\n", attr_name);
		return 4;
	}

	len = 0;
	ret = atdb_get_attribute(atdb, target, attr_name, NULL, &len);
	assert(ret == 2);

	data = (uint8_t *)malloc(len);
	assert(data);

	ret = atdb_get_attribute(atdb, target, attr_name, data, &len);
	if (ret == 5) {
		fprintf(stderr, "Attribute %s is UNDEFINED\n", attr_name);
		free(data);
		return 5;
	}
	assert(ret == 0);

	printf("%s: ", attr_name);
	for (i=0; i<entry.attr_dim; i++) {
		if (entry.attr_data_len == 1) {
			uint8_t value = *data;
			printf("%02x ", value);
		} else if (entry.attr_data_len == 2) {
			uint16_t value = *(uint16_t *)data;
			printf("%04x ", value);
		} else if (entry.attr_data_len == 4) {
			uint32_t value = *(uint32_t *)data;
			printf("%08x ", value);
		} else if (entry.attr_data_len == 8) {
			uint64_t value = *(uint64_t *)data;
			printf("%016" PRIx64 " ", value);
		}

		data += entry.attr_data_len;
	}
	printf("\n");

	atdb_blob_close(binfo);

	return 0;
}

static int do_translate(const char *blob, const char *arg)
{
	struct atdb_blob_info *binfo;
	struct pdbg_target *target;
	char *path;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 2;

	if (arg[0] == '/') {
		target = pdbg_target_from_path(NULL, arg);
		if (!target) {
			fprintf(stderr, "No such target %s\n", arg);
			return 3;
		}

		path = to_cronus_target(target);
		if (!path) {
			fprintf(stderr, "Failed to translate %s\n", arg);
			return 3;
		}
	} else {
		target = from_cronus_target(arg);
		if (!target) {
			fprintf(stderr, "Failed to translate %s\n", arg);
			return 3;
		}

		path = pdbg_target_path(target);
		assert(path);
	}

	printf("%s ---> %s\n", arg, path);
	free(path);

	atdb_blob_close(binfo);

	return 0;
}

static void do_tree_print(struct pdbg_target *target, int level)
{
	struct pdbg_target *child;
	const char *name;
	int i;

	for (i=0; i<level; i++) {
		printf("  ");
	}
	name = pdbg_target_dn_name(target);
	printf("%s\n", name[0] ? name : "/");

	pdbg_for_each_child_target(target, child) {
		do_tree_print(child, level + 1);
	}
}

static int do_tree(const char *blob)
{
	struct atdb_blob_info *binfo;

	binfo = atdb_blob_open(blob, false);
	if (!binfo)
		return 2;

	do_tree_print(pdbg_target_root(), 0);

	atdb_blob_close(binfo);

	return 0;
}

void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s create <machine> <infodb>\n", prog);
	fprintf(stderr, "       %s info <atdb>\n", prog);
	fprintf(stderr, "       %s dump <atdb> [<target>]\n", prog);
	fprintf(stderr, "       %s export <atdb> <infodb>\n", prog);
	fprintf(stderr, "       %s import <atdb> <infodb> <attr-dump>\n", prog);
	fprintf(stderr, "       %s read <atdb> <target> <attribute>\n", prog);
	fprintf(stderr, "       %s translate <atdb> <target>\n", prog);
	fprintf(stderr, "       %s tree <atdb>\n", prog);
	exit(1);
}

int main(int argc, const char **argv)
{
	int ret;

	if (argc < 2) {
		usage(argv[0]);
	}

	if (strcmp(argv[1], "create") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_create(argv[2], argv[3]);

	} else if (strcmp(argv[1], "info") == 0) {
		if (argc != 3)
			usage(argv[0]);

		ret = do_info(argv[2]);

	} else if (strcmp(argv[1], "debug") == 0) {
		if (argc != 3)
			usage(argv[0]);

		ret = do_debug(argv[2]);

	} else if (strcmp(argv[1], "dump") == 0) {
		if (argc != 3 && argc != 4)
			usage(argv[0]);

		if (argc == 3)
			ret = do_dump(argv[2], NULL);
		else if (argc == 4)
			ret = do_dump(argv[2], argv[3]);

	} else if (strcmp(argv[1], "export") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_export(argv[2], argv[3]);

	} else if (strcmp(argv[1], "import") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_import(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "read") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_read(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "translate") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_translate(argv[2], argv[3]);

	} else if (strcmp(argv[1], "tree") == 0) {
		if (argc != 3)
			usage(argv[0]);

		ret = do_tree(argv[2]);

	} else {
		usage(argv[0]);
	}

	exit(ret);
}
