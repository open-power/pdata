/* Copyright 2021 IBM Corp.
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
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "dtree_attr_list.h"

static void name_list_init(struct name_list *nlist)
{
	*nlist = (struct name_list) {
		.name = NULL,
		.count = 0,
		.size = 0,
	};
}

static bool name_list_add(struct name_list *nlist, char *name)
{
	if (nlist->count == nlist->size) {
		char **list;

		list = reallocarray(nlist->name, nlist->size + 10, sizeof(char *));
		if (!list)
			return false;

		nlist->name = list;
		nlist->size += 10;
	}

	nlist->name[nlist->count] = strdup(name);
	if (!nlist->name[nlist->count])
		return false;

	nlist->count += 1;
	return true;
}

static int name_compare(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

static void name_list_sort(struct name_list *nlist)
{
	qsort(nlist->name, nlist->count, sizeof(char *), name_compare);
}

static bool name_list_find(const struct name_list *nlist, const char *name)
{
	void *r;

	r = bsearch(&name, nlist->name, nlist->count, sizeof(char *), name_compare);
	if (r)
		return true;

	return false;
}

bool dtree_attr_list_parse(const char *attrdb, struct name_list *alist)
{
	FILE *fp;
	char line[128];

	name_list_init(alist);

	if (!attrdb)
		return true;

	fp = fopen(attrdb, "r");
	if (!fp)
		return false;

	while (!feof(fp)) {
		char *ptr;

		if (!fgets(line, sizeof(line), fp))
			break;

		ptr = strtok(line, "\n");
		if (!ptr)
			continue;

		if (!name_list_add(alist, ptr)) {
			fclose(fp);
			return false;
		}
	}

	fclose(fp);

	name_list_sort(alist);
	return true;
}

bool dtree_attr_list_exists(const struct name_list *alist, const char *attr)
{
	/* If attribute list is empty, do not skip attributes */
	if (alist->count == 0)
		return true;

	return name_list_find(alist, attr);
}
