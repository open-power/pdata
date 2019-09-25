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

#include <ccan/list/list.h>

#include "dtm_internal.h"
#include "dtm.h"

struct dtm_node *dtm_tree_new(void)
{
	struct dtm_node *root;

	root = dtm_node_new("");
	if (!root)
		return NULL;

	return root;
}

void dtm_tree_free(struct dtm_node *node)
{
	struct dtm_node *parent, *child = NULL, *next;

	list_for_each_safe(&node->children, child, next, list) {
		dtm_tree_free(child);
	}

	parent = node->parent;
	if (parent) {
		list_del_from(&parent->children, &node->list);
	}

	dtm_node_free(node);
}

void dtm_tree_add_node(struct dtm_node *parent, struct dtm_node *child)
{
	child->parent = parent;
	list_add_tail(&parent->children, &child->list);
}

static bool dtm_tree_level_copy(struct dtm_node *node, struct dtm_node *node_copy)
{
	struct dtm_node *child = NULL;

	list_for_each(&node->children, child, list) {
		struct dtm_node *child_copy;

		child_copy = dtm_node_copy(child);
		if (!child_copy) {
			return false;
		}

		dtm_tree_add_node(node_copy, child_copy);

		if (!dtm_tree_level_copy(child, child_copy))
			return false;
	}

	return true;
}

struct dtm_node *dtm_tree_copy(struct dtm_node *root)
{
	struct dtm_node *root_copy;

	root_copy = dtm_node_copy(root);
	if (!root_copy)
		return NULL;

	if (!dtm_tree_level_copy(root, root_copy)) {
		dtm_tree_free(root_copy);
		return NULL;
	}

	return root_copy;
}

/*
 * keep track of old --> new mapping
 * keep track of all the old nodes added
 * keep check against same-as node
 */
struct dtm_node *dtm_tree_rearrange(struct dtm_node *root,
				    struct dtm_nodelist *nlist)
{
	struct dtm_nodelist *map = NULL;
	struct dtm_nodelist *plist = NULL;
	struct dtm_nodelist *clist = NULL;
	struct dtm_node *root_copy = NULL;
	int i;

	map = dtm_nodelist_new(20);
	if (!map)
		goto fail;

	plist = dtm_nodelist_new(10);
	if (!plist)
		goto fail;

	root_copy = dtm_node_copy(root);
	if (!root_copy)
		goto fail;

	for (i=0; i<nlist->count; i++) {
		struct dtm_node *node = nlist->node[i];
		struct dtm_node *node_copy;
		struct dtm_property *prop;
		int index;

		index = dtm_nodelist_find(map, node);
		if (index != -1)
			continue;

		prop = dtm_node_get_property(node, "same-as");
		if (prop) {
			char *path;
			struct dtm_node *node2;

			path = (char *)prop->value;
			node2 = dtm_find_node_by_path(root, path);

			index = dtm_nodelist_find(map, node2);
			if (index != -1)
				continue;
		}

		node_copy = dtm_node_copy(node);
		if (!node_copy)
			goto fail;

		dtm_tree_add_node(root_copy, node_copy);

		if (!dtm_nodelist_add(map, node))
			goto fail;
		if (!dtm_nodelist_add(map, node_copy))
			goto fail;

		if (!dtm_nodelist_add(plist, node))
			goto fail;
	}

	while (plist->count > 0) {
		clist = dtm_nodelist_new(10);
		if (!clist)
			goto fail;

		for (i=0; i<plist->count; i++) {
			struct dtm_node *node = plist->node[i];
			struct dtm_node *node_copy;
			struct dtm_node *child, *child_copy;
			int index;

			index = dtm_nodelist_find(map, node);
			assert(index != -1);
			node_copy = dtm_nodelist_get(map, index+1);

			dtm_node_for_each_child(node, child) {
				struct dtm_property *prop;
				int index2;

				index2 = dtm_nodelist_find(map, child);
				if (index2 != -1)
					continue;

				prop = dtm_node_get_property(child, "same-as");
				if (prop) {
					char *path;
					struct dtm_node *child2;

					path = (char *)prop->value;
					child2 = dtm_find_node_by_path(root, path);
					assert(child2);

					index2 = dtm_nodelist_find(map, child2);
					if (index2 != -1)
						continue;
				}

				child_copy = dtm_node_copy(child);
				if (!child_copy)
					goto fail;

				dtm_tree_add_node(node_copy, child_copy);

				if (!dtm_nodelist_add(map, child))
					goto fail;
				if (!dtm_nodelist_add(map, child_copy))
					goto fail;

				if (!dtm_nodelist_add(clist, child))
					goto fail;
			}
		}

		dtm_nodelist_free(plist);
		plist = clist;
		clist = NULL;
	}

	dtm_nodelist_free(plist);
	dtm_nodelist_free(map);
	return root_copy;

fail:
	if (map)
		dtm_nodelist_free(map);
	if (plist)
		dtm_nodelist_free(plist);
	if (clist)
		dtm_nodelist_free(clist);
	if (root_copy)
		dtm_tree_free(root_copy);
	return NULL;
}
