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

#include "dtm_internal.h"
#include "dtm.h"

struct dtm_nodelist *dtm_nodelist_new(int increment)
{
	struct dtm_nodelist *list;

	assert(increment > 0);

	list = calloc(1, sizeof(struct dtm_nodelist));
	if (!list)
		return NULL;

	list->increment = increment;

	return list;
}

bool dtm_nodelist_extend(struct dtm_nodelist *list)
{
	struct dtm_node **node;

	node = realloc(list->node,
		       sizeof(struct dtm_node *) * (list->allocated + list->increment));
	if (!node)
		return false;

	list->node = node;
	list->allocated += list->increment;
	return true;
}

bool dtm_nodelist_add(struct dtm_nodelist *list, struct dtm_node *node)
{
	if (list->count == list->allocated) {
		if (!dtm_nodelist_extend(list))
			return false;
	}

	list->node[list->count] = node;
	list->count += 1;
	return true;
}

void dtm_nodelist_free(struct dtm_nodelist *list)
{
	if (list->node)
		free(list->node);
	free(list);
}

int dtm_nodelist_find(struct dtm_nodelist *list, struct dtm_node *node)
{
	int i;

	for (i=0; i<list->count; i++) {
		if (list->node[i] == node)
			return i;
	}

	return -1;
}

struct dtm_node *dtm_nodelist_get(struct dtm_nodelist *list, int index)
{
	if (index < 0 || index >= list->count)
		return NULL;

	return list->node[index];
}
