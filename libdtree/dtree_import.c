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
#include <assert.h>

#include "libdtm/dtm.h"
#include "dtree.h"
#include "dtree_attr.h"
#include "dtree_attr_list.h"
#include "dtree_infodb.h"


struct dtree_import_state {
	struct dtm_file *dfile;
	struct dtree_infodb *infodb;
	struct dtm_node *root;
	struct dtm_node *node;
	struct dtree_attr *value;
	int count;
};

int dtree_import(const char *dtb_path,
		 const char *infodb_path,
		 dtree_import_parse_fn parse_fn,
		 void *priv)
{
	struct dtree_import_state state;
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct dtree_infodb infodb;
	int ret;

	dfile = dtm_file_open(dtb_path, true);
	if (!dfile)
		return -1;

	root = dtm_file_read(dfile);
	if (!root)
		return -2;

	if (!dtree_infodb_load(infodb_path, &infodb))
		return -3;

	state = (struct dtree_import_state) {
		.dfile = dfile,
		.infodb = &infodb,
		.root = root,
	};

	ret = parse_fn(&state, priv);
	dtm_file_close(dfile);
	return ret;
}

void *dtree_import_root(void *ctx)
{
	struct dtree_import_state *state = (struct dtree_import_state *)ctx;

	return state->root;
}

struct dtm_node *dtree_import_node(void *ctx)
{
	struct dtree_import_state *state = (struct dtree_import_state *)ctx;

	return state->node;
}

static void dtree_import_free_value(struct dtree_import_state *state)
{
	if (state->value) {
		dtree_attr_free(state->value);
		free(state->value);
		state->value = NULL;
	}
}

int dtree_import_set_node(struct dtm_node *node, void *ctx)
{
	struct dtree_import_state *state = (struct dtree_import_state *)ctx;

	state->node = node;
	dtree_import_free_value(state);

	return 0;
}

int dtree_import_attr(const char *attr_name, void *ctx, struct dtree_attr **value)
{
	struct dtree_import_state *state = (struct dtree_import_state *)ctx;
	struct dtree_attr *attr;
	struct dtm_property *prop;
	const uint8_t *cbuf;
	int len = 0;

	if (!state->node)
		return -1;

	if (state->value) {
		if (strcmp(attr_name, state->value->name) == 0) 
			goto done;

		dtree_import_free_value(state);
	}

	attr = dtree_infodb_attr(state->infodb, attr_name);
	if (!attr)
		return -1;

	prop = dtm_node_get_property(state->node, attr_name);
	if (!prop)
		return -1;

	state->value = malloc(sizeof(struct dtree_attr));
	if (!state->value)
		return -1;

	dtree_attr_copy(attr, state->value);

	cbuf = dtm_prop_value(prop, &len);
	dtree_attr_decode(state->value, cbuf, len);

	state->count = 0;

done:
	*value = state->value;
	return 0;
}

int dtree_import_attr_update(void *ctx)
{
	struct dtree_import_state *state = (struct dtree_import_state *)ctx;
	struct dtm_property *prop;
	uint8_t *buf;
	int len = 0;
	bool ok;

	if (!state->node)
		return -1;

	if (!state->value)
		return -1;

	state->count += 1;
	if (state->count < state->value->count)
		return 0;

	prop = dtm_node_get_property(state->node, state->value->name);
	if (!prop)
		return -1;

	dtree_attr_encode(state->value, &buf, &len);
	dtm_prop_set_value(prop, buf, len);
	free(buf);

	ok = dtm_file_update_node(state->dfile, state->node, state->value->name);
	if (!ok)
		return -1;

	return 0;
}
