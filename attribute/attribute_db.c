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

static bool attr_db_read_attr_data(struct attr *attr, char *data)
{
	uint8_t *ptr;
	char *tok;
	int defined, i;

	tok = strtok(data, " ");
	if (!tok)
		return false;

	attr->type = attr_type_from_string(tok);
	if (attr->type == ATTR_TYPE_UNKNOWN)
		return false;

	if (attr->type == ATTR_TYPE_STRING) {
		tok = strtok(NULL, " ");
		if (!tok)
			return false;

		attr->data_size = atoi(tok);
	} else {
		attr->data_size = attr_type_size(attr->type);
	}
	assert(attr->data_size > 0);

	tok = strtok(NULL, " ");
	if (!tok)
		return false;

	attr->dim_count = atoi(tok);
	assert(attr->dim_count >= 0 && attr->dim_count <= 3);

	attr->size = 1;
	if (attr->dim_count > 0) {
		attr->dim = (int *)malloc(attr->dim_count * sizeof(int));
		assert(attr->dim);

		for (i=0; i<attr->dim_count; i++) {
			tok = strtok(NULL, " ");
			if (!tok)
				return false;

			attr->dim[i] = atoi(tok);
			assert(attr->dim[i] > 0);

			attr->size *= attr->dim[i];
		}
	}

	if (attr->type != ATTR_TYPE_STRING) {
		tok = strtok(NULL, " ");
		if (!tok)
			return false;

		attr->enum_count = atoi(tok);
		if (attr->enum_count > 0) {
			attr->aenum = (struct attr_enum *)malloc(attr->enum_count * sizeof(struct attr_enum));
			assert(attr->aenum);

			for (i=0; i<attr->enum_count; i++) {
				tok = strtok(NULL, " ");
				if (!tok)
					return false;

				attr->aenum[i].key = strdup(tok);
				assert(attr->aenum[i].key);

				tok = strtok(NULL, " ");
				if (!tok)
					return false;

				attr->aenum[i].value = strtoull(tok, NULL, 0);
			}
		}
	}

	tok = strtok(NULL, " ");
	if (!tok)
		return false;

	defined = atoi(tok);
	if (defined != 0 && defined != 1)
		return false;

	attr->value = (uint8_t *)calloc(attr->size, attr->data_size);
	assert(attr->value);

	if (defined == 0)
		return true;

	ptr = attr->value;
	for (i=0; i<attr->size; i++) {
		if (attr->type == ATTR_TYPE_STRING) {
			tok = strtok(NULL, " ");
			if (!tok)
				return false;

			attr_set_string_value(attr, ptr, tok);
			ptr += attr->data_size;
		} else {
			tok = strtok(NULL, " ");
			if (!tok)
				return false;

			if (!attr_set_enum_value(attr, ptr, tok))
				attr_set_value(attr, ptr, tok);
			ptr += attr->data_size;
		}
	}

	return true;
}

static bool attr_db_read_attr(FILE *fp, struct attr_info *info)
{
	char *data;
	int i;
	bool ok;

	for (i=0; i<info->alist.count; i++) {
		struct attr *attr = &info->alist.attr[i];

		data = attr_db_get_value(fp, attr->name);
		if (!data)
			return false;

		ok = attr_db_read_attr_data(attr, data);

		free(data);

		if (!ok) {
			fprintf(stderr, "Failed to read %s\n", attr->name);
			return false;
		}
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
