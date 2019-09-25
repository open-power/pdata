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

int dtm_traverse(struct dtm_node *root,
		 bool do_all,
		 dtm_traverse_node_fn node_fn,
		 dtm_traverse_prop_fn prop_fn,
		 void *priv)
{
	struct dtm_node *child;
	int ret;

	ret = node_fn(root, priv);
	if (ret)
		return ret;

	if (prop_fn) {
		struct dtm_property *prop;

		dtm_node_for_each_property(root, prop) {
			ret = prop_fn(root, prop, priv);
			if (ret)
				return ret;
		}
	}

	dtm_node_for_each_child(root, child) {
		if (!do_all && !child->enabled)
			continue;

		ret = dtm_traverse(child, do_all, node_fn, prop_fn, priv);
		if (ret)
			return ret;
	}

	return 0;
}

static struct dtm_nodelist *dtm_traverse_bfs_children(struct dtm_nodelist *plist, bool do_all)
{
	struct dtm_nodelist *clist;
	int i;

	clist = dtm_nodelist_new(10);
	if (!clist)
		return NULL;

	for (i=0; i<plist->count; i++) {
		struct dtm_node *parent = plist->node[i];
		struct dtm_node *child;

		dtm_node_for_each_child(parent, child) {
			if (!do_all && !child->enabled)
				continue;

			if (!dtm_nodelist_add(clist, child)) {
				dtm_nodelist_free(clist);
				return NULL;
			}
		}
	}

	return clist;
}

int dtm_traverse_bfs(struct dtm_node *root,
		     bool do_all,
		     dtm_traverse_node_fn node_fn,
		     dtm_traverse_prop_fn prop_fn,
		     void *priv)
{
	struct dtm_nodelist *plist, *clist;
	int ret = 0;

	plist = dtm_nodelist_new(1);
	if (!plist)
		return -1;

	if (!dtm_nodelist_add(plist, root)) {
		dtm_nodelist_free(plist);
		return -1;
	}

	while (plist->count > 0) {
		int i;

		for (i=0; i<plist->count; i++) {
			struct dtm_node *node = plist->node[i];
			struct dtm_property *prop;

			ret = node_fn(node, priv);
			if (ret)
				goto done;

			if (prop_fn) {
				dtm_node_for_each_property(node, prop) {
					ret = prop_fn(node, prop, priv);
					if (ret)
						goto done;
				}
			}
		}

		clist = dtm_traverse_bfs_children(plist, do_all);
		dtm_nodelist_free(plist);
		if (!clist) {
			return -1;
		}

		plist = clist;
	}

done:
	dtm_nodelist_free(plist);
	return ret;
}
