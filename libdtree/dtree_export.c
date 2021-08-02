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
#include "dtree_util.h"

struct dtree_create_state {
	struct dtree_infodb *infodb;
};

static int dtree_create_node(struct dtm_node *node, void *priv)
{
	struct dtree_create_state *state = (struct dtree_create_state *)priv;
	struct dtree_target *target = NULL;
	const char *name, *fapi_target;
	char *cname;
	int i, ret;

	name = dtm_node_name(node);
	cname = dtree_name_to_class(name);
	fapi_target = dtree_to_fapi_class(cname);
	free(cname);

	if (!fapi_target)
		return 0;

	for (i=0; i<state->infodb->tlist.count; i++) {
		struct dtree_target *t = &state->infodb->tlist.target[i];
		if (strcmp(fapi_target, t->name) == 0) {
			target = t;
			break;
		}
	}

	if (!target)
		return 0;

	for (i=0; i<target->id_count; i++) {
		struct dtree_attr *attr;
		uint8_t *buf = NULL;
		int len;
		int id = target->id[i];

		assert(id < state->infodb->alist.count);
		attr = &state->infodb->alist.attr[id];

		dtree_attr_encode(attr, &buf, &len);
		assert(buf && len > 0);

		ret = dtm_node_add_property(node, attr->name, buf, len);
		free(buf);
		if (ret)
			return ret;
	}

	return 0;
}

int dtree_create(const char *dtb_path,
		 const char *infodb_path,
		 const char *dtb_out_path)
{
	struct dtree_create_state state;
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct dtree_infodb infodb;
	int ret;

	dfile = dtm_file_open(dtb_path, false);
	if (!dfile)
		return -1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return -2;

	if (!dtree_infodb_load(infodb_path, &infodb))
		return -3;

	state = (struct dtree_create_state) {
		.infodb = &infodb,
	};

	ret = dtm_traverse(root, true, dtree_create_node, NULL, &state);
	if (ret)
		return ret;

	dfile = dtm_file_create(dtb_out_path);
	if (!dfile)
		return -4;

	if (!dtm_file_write(dfile, root))
		return -5;

	dtm_file_close(dfile);
	return 0;
}

struct dtree_export_state {
	struct dtree_infodb *infodb;
	struct dtm_node *root;
	struct name_list *alist;
	dtree_export_node_fn node_fn;
	dtree_export_attr_fn attr_fn;
	void *priv;
};

static int dtree_export_node(struct dtm_node *node, void *priv)
{
	struct dtree_export_state *state = (struct dtree_export_state *)priv;

	return state->node_fn(state->root, node, state->priv);
}

static int dtree_export_attr(struct dtm_node *node, struct dtm_property *prop, void *priv)
{
	struct dtree_export_state *state = (struct dtree_export_state *)priv;
	struct dtree_attr *attr, value;
	const char *name;
	const uint8_t *buf;
	int buflen, ret;

	name = dtm_prop_name(prop);
	if (strncmp(name, "ATTR", 4) != 0)
		return 0;

	if (!dtree_attr_list_exists(state->alist, name))
		return 0;

	attr = dtree_infodb_attr(state->infodb, name);
	if (attr) {
		dtree_attr_copy(attr, &value);
		buf = dtm_prop_value(prop, &buflen);
		dtree_attr_decode(&value, buf, buflen);
	} else {
		value = (struct dtree_attr) {
			.type = DTREE_ATTR_TYPE_UNKNOWN,
		};
		memcpy(value.name, name, strlen(name)+1);
	}

	ret = state->attr_fn(&value, state->priv);

	if (value.dim)
		free(value.dim);
	if (value.value)
		free(value.value);

	return ret;
}

int dtree_export(const char *dtb_path,
		 const char *infodb_path,
		 const char *attrdb_path,
		 dtree_export_node_fn node_fn,
		 dtree_export_attr_fn attr_fn,
		 void *priv)
{
	struct dtree_export_state state;
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct dtree_infodb infodb;
	struct name_list alist;

	dfile = dtm_file_open(dtb_path, false);
	if (!dfile)
		return -1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return -2;

	if (!dtree_infodb_load(infodb_path, &infodb))
		return -3;

	if (!dtree_attr_list_parse(attrdb_path, &alist))
		return -4;

	state = (struct dtree_export_state) {
		.infodb = &infodb,
		.root = root,
		.alist = &alist,
		.node_fn = node_fn,
		.attr_fn = attr_fn,
		.priv = priv,
	};

	return dtm_traverse(root, true, dtree_export_node, dtree_export_attr, &state);
}
