/* Copyright 2020 IBM Corp.
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

#include "fdt/fdt_prop.h"

#include "dtm_internal.h"
#include "dtm.h"

bool dtm_file_update_node(struct dtm_file *dfile, struct dtm_node *node, const char *name)
{
	struct dtm_property *prop;
	char *path;
	int ret;

	if (!dfile->ptr || dfile->do_create || !dfile->do_write)
		return false;

	path = dtm_node_path(node);
	if (!path)
		return false;

	list_for_each(&node->properties, prop, list) {
		if (name && strcmp(prop->name, name) != 0)
			continue;

		ret = fdt_prop_write(dfile->ptr, path, name, prop->value, prop->len);
		if (ret != 0)
			goto fail;

		if (name)
			break;
	}

	free(path);
	return true;

fail:
	free(path);
	return false;
}
