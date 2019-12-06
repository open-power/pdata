/* Copyright 2019 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "attribute.h"
#include "attribute_db.h"
#include "attribute_util.h"

static char *attr_db_get_value(FILE *fp, char *keystr)
{
	char *line = NULL, *tok, *value;
	size_t len = 0;
	ssize_t n;

	n = getline(&line, &len, fp);
	if (n == -1) {
		fprintf(stderr, "Failed to read key '%s'\n", keystr);
		return NULL;
	}

	tok = strtok(line, " ");
	assert(tok);

	if (strcmp(tok, keystr)) {
		fprintf(stderr, "Expected %s, got %s\n", keystr, tok);
		free(line);
		return NULL;
	}

	len = strlen(tok);
	tok += (len + 1);

	len = strlen(tok);
	value = malloc(len);
	if (!value) {
		free(line);
		return NULL;
	}

	strncpy(value, tok, len-1);
	value[len-1] = '\0';

	free(line);
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


static bool attr_db_read_all(FILE *fp, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int count, i;

	data = attr_db_get_value(fp, "all");
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
		uint16_t value = htobe16(data & 0xffff);
		memcpy(ptr, &value, 2);
	} else if (type == ATTR_TYPE_UINT32 || type == ATTR_TYPE_INT32) {
		uint32_t value = htobe32(data & 0xffffffff);
		memcpy(ptr, &value, 4);
	} else if (type == ATTR_TYPE_UINT64 || type == ATTR_TYPE_INT64) {
		uint64_t value = htobe64(data);
		memcpy(ptr, &value, 8);
	}
}

static bool attr_db_read_attr(FILE *fp, struct attr_info *info)
{
	char *data, *tok;
	int i, j, count;

	for (i=0; i<info->alist.count; i++) {
		struct attr *attr = &info->alist.attr[i];
		int defined;

		data = attr_db_get_value(fp, attr->name);
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

		/* Special case of instance attribute ATTR_PG */
		if (0 && !strcmp(attr->name, "ATTR_PG"))
			count = NUM_CHIPLETS;
		else
			count = attr->size;

		attr->value = (uint8_t *)calloc(count, attr->data_size);
		if (!attr->value)
			return false;

		if (defined == 1) {
			uint8_t *ptr = attr->value;

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

static bool attr_db_read_targets(FILE *fp, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int count, i;

	data = attr_db_get_value(fp, "targets");
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

static bool attr_db_read_target(FILE *fp, struct attr_info *info)
{
	char *data, *tmp, *tok;
	int i, j;

	for (i=0; i<info->tlist.count; i++) {
		struct target *target = &info->tlist.target[i];

		data = attr_db_get_value(fp, target->name);
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
	FILE *fp;
	bool rc;

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Failed to open db: %s\n", filename);
		return false;
	}

	rc = attr_db_read_all(fp, info);
	if (!rc) {
		fprintf(stderr, "Failed to read all\n");
		goto done;
	}

	rc = attr_db_read_attr(fp, info);
	if (!rc) {
		fprintf(stderr, "Failed to read attributes\n");
		goto done;
	}

	rc = attr_db_read_targets(fp, info);
	if (!rc) {
		fprintf(stderr, "Failed to read targets\n");
		goto done;
	}

	rc = attr_db_read_target(fp, info);
	if (!rc) {
		fprintf(stderr, "Failed to read per target attributes\n");
		goto done;
	}

done:
	fclose(fp);
	return rc;
}

struct attr *attr_db_attr(struct attr_info *info, const char *name)
{
	int i;

	for (i=0; i<info->alist.count; i++) {
		struct attr *attr = &info->alist.attr[i];
		if (strcmp(attr->name, name) == 0)
			return attr;
	}

	return NULL;
}
