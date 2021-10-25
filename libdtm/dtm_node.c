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

#include "dtm_internal.h"
#include "dtm.h"

struct dtm_node *dtm_node_new(const char *name)
{
	struct dtm_node *node;

	node = calloc(1, sizeof(struct dtm_node));
	if (!node)
		return NULL;

	node->name = strdup(name);
	if (!node->name) {
		free(node);
		return NULL;
	}

	list_head_init(&node->properties);
	list_head_init(&node->children);

	node->enabled = false;

	return node;
}

void dtm_node_free(struct dtm_node *node)
{
	struct dtm_property *prop = NULL, *next;

	list_for_each_safe(&node->properties, prop, next, list) {
		list_del_from(&node->properties, &prop->list);
		dtm_prop_free(prop);
	}

	free(node->name);
	free(node);
}

const char *dtm_node_name(const struct dtm_node *node)
{
	return node->name;
}

struct dtm_node *dtm_node_copy(const struct dtm_node *node)
{
	struct dtm_node *node_copy;
	struct dtm_property *prop = NULL, *prop_copy;

	node_copy = dtm_node_new(node->name);
	if (!node_copy)
		return NULL;

	list_for_each(&node->properties, prop, list) {
		prop_copy = dtm_prop_copy(prop);
		if (!prop_copy) {
			dtm_node_free(node_copy);
			return NULL;
		}

		list_add_tail(&node_copy->properties, &prop_copy->list);
	}

	node_copy->enabled = node->enabled;

	return node_copy;
}

int dtm_node_add_property(struct dtm_node *node, const char *name, void *value, int valuelen)
{
	struct dtm_property *prop;

	prop = dtm_prop_new(name, value, valuelen);
	if (!prop)
		return -1;

	list_add_tail(&node->properties, &prop->list);
	return 0;
}

struct dtm_node *dtm_node_next_child(struct dtm_node *node, struct dtm_node *prev)
{
	struct dtm_node *next;

	if (list_empty(&node->children))
		return NULL;

	if (!prev) {
		next = list_top(&node->children, struct dtm_node, list);
	} else if (prev == list_tail(&node->children, struct dtm_node, list)) {
		next = NULL;
	} else {
		next = list_entry(prev->list.next, struct dtm_node, list);
	}

	return next;
}

struct dtm_property *dtm_node_next_property(struct dtm_node *node, struct dtm_property *prev)
{
	struct dtm_property *next;

	if (list_empty(&node->properties))
		return NULL;

	if (!prev) {
		next = list_top(&node->properties, struct dtm_property, list);
	} else if (prev == list_tail(&node->properties, struct dtm_property, list)) {
		next = NULL;
	} else {
		next = list_entry(prev->list.next, struct dtm_property, list);
	}

	return next;
}

struct dtm_property *dtm_node_get_property(const struct dtm_node *node, const char *name)
{
	struct dtm_property *prop = NULL;

	list_for_each(&node->properties, prop, list) {
		if (strcmp(prop->name, name) == 0)
			return prop;
	}

	return NULL;
}

static char *_dtm_node_path(const struct dtm_node *node, int len)
{
	char *p;

	if (node->parent) {
		len += strlen(node->name) + 1;

		p = _dtm_node_path(node->parent, len);
		if (!p)
			return NULL;

		strcat(p, "/");
		strcat(p, node->name);
	} else {
		if (len == 1)
			p = strdup("/");
		else
			p = calloc(1, len);

		if (!p)
			return NULL;
	}

	return p;
}

char *dtm_node_path(const struct dtm_node *node)
{
	return _dtm_node_path(node, 1);
}

int dtm_node_index(const struct dtm_node *node)
{
	struct dtm_property *prop;

	if (!node->parent)
		return -1;

	prop = dtm_node_get_property(node, "index");
	if (!prop)
		return dtm_node_index(node->parent);

	return dtm_prop_value_u32(prop);
}

struct dtm_node *dtm_node_parent(const struct dtm_node *node)
{
	return node->parent;
}
