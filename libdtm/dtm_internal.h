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

#ifndef __DTM_INTERNAL_H__
#define __DTM_INTERNAL_H__

#include <ccan/list/list.h>

struct dtm_file {
	const char *filename;
	int fd;
	void *ptr;
	int len;
	bool do_create;
	bool do_write;
};

struct dtm_property {
	struct list_node list;
	char *name;
	int len;
	void *value;
};

struct dtm_node {
	struct list_node list;
	char *name;
	struct dtm_node *parent;
	struct list_head properties;
	struct list_head children;
	bool enabled;
};

struct dtm_nodelist {
	struct dtm_node **node;
	int increment, count, allocated;
};

struct dtm_property *dtm_prop_new(const char *name, void *value, int len);
void dtm_prop_free(struct dtm_property *prop);
struct dtm_property *dtm_prop_copy(struct dtm_property *prop);

struct dtm_node *dtm_node_new(const char *name);
void dtm_node_free(struct dtm_node *node);
struct dtm_node *dtm_node_copy(struct dtm_node *node);
struct dtm_node *dtm_node_next_child(struct dtm_node *node, struct dtm_node *prev);
struct dtm_property *dtm_node_next_property(struct dtm_node *node, struct dtm_property *prev);

#define dtm_node_for_each_child(parent, child)          \
	for (child = dtm_node_next_child(parent, NULL); \
	     child;                                     \
	     child = dtm_node_next_child(parent, child))

#define dtm_node_for_each_property(node, prop)          \
	for (prop = dtm_node_next_property(node, NULL); \
	     prop;                                      \
	     prop = dtm_node_next_property(node, prop))

struct dtm_nodelist *dtm_nodelist_new(int increment);
bool dtm_nodelist_extend(struct dtm_nodelist *list);
bool dtm_nodelist_add(struct dtm_nodelist *list, struct dtm_node *node);
void dtm_nodelist_free(struct dtm_nodelist *list);
int dtm_nodelist_find(struct dtm_nodelist *list, struct dtm_node *node);
struct dtm_node *dtm_nodelist_get(struct dtm_nodelist *list, int index);

void dtm_tree_add_node(struct dtm_node *parent, struct dtm_node *child);

#endif /* __DTM_INTERNAL_H__ */
