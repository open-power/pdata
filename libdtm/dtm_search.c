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
#include <string.h>

#include <libfdt.h>

#include "dtm_internal.h"
#include "dtm.h"

struct match_by_name {
	const char *name;
	struct dtm_node *match;
};

static int match_node_by_name(struct dtm_node *node, void *priv)
{
	struct match_by_name *state = (struct match_by_name *)priv;

	if (strcmp(node->name, state->name) == 0) {
		state->match = node;
		return 1;
	}

	return 0;
}

struct dtm_node *dtm_find_node_by_name(struct dtm_node *root, const char *name)
{
	struct match_by_name state = {
		.name = name,
	};
	int ret;

	ret = dtm_traverse(root, true, match_node_by_name, NULL, &state);
	if (!ret)
		return NULL;

	return state.match;
}

struct match_by_compatible {
	const char *compatible;
	struct dtm_node *match;
};

static int match_node_by_compatible(struct dtm_node *node, void *priv)
{
	struct match_by_compatible *state = (struct match_by_compatible *)priv;
	struct dtm_property *prop;
	int ret;

	prop = dtm_node_get_property(node, "compatible");
	if (!prop)
		return 0;

	ret = fdt_stringlist_contains((char *)prop->value, prop->len, state->compatible);
	if (ret == 1) {
		state->match = node;
		return 1;
	}

	return 0;
}

struct dtm_node *dtm_find_node_by_compatible(struct dtm_node *root, const char *compatible)
{
	struct match_by_compatible state = {
		.compatible = compatible,
	};
	int ret;

	ret = dtm_traverse(root, true, match_node_by_compatible, NULL, &state);
	if (!ret)
		return NULL;

	return state.match;
}

struct match_by_path {
	const char *path;
	struct dtm_node *match;
};

static int match_node_by_path(struct dtm_node *node, void *priv)
{
	struct match_by_path *state = (struct match_by_path *)priv;
	char *path;

	path = dtm_node_path(node);
	if (!path)
		return 0;

	if (strcmp(path, state->path) == 0) {
		state->match = node;
		free(path);
		return 1;
	}

	free(path);
	return 0;
}

struct dtm_node *dtm_find_node_by_path(struct dtm_node *root, const char *path)
{
	struct match_by_path state = {
		.path = path,
	};
	int ret;

	ret = dtm_traverse(root, true, match_node_by_path, NULL, &state);
	if (!ret)
		return NULL;

	return state.match;
}
