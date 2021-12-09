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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "libdtm/dtm.h"

#include "config.h"
#include "dtree.h"
#include "dtree_cronus.h"
#include "dtree_util.h"

struct cronus_target {
	int cage;
	int node;
	int slot;
	int chip_position;
	int chip_unit;

	const char *chip_name;
	const char *class_name;
};

static void cronus_target_init(struct cronus_target *ct)
{
	*ct = (struct cronus_target) {
		.cage = -1,
		.node = -1,
		.slot = -1,
		.chip_position = -1,
		.chip_unit = -1,
	};
}

/*
 * k0
 * p9n:k0:n0:s0:p00
 * p9n.xbus:k0:n0:s0:p00:c1
 */
static bool split_cronus_target(char *name, struct cronus_target *ct)
{
	char *tok, *saveptr = NULL;

	cronus_target_init(ct);

	tok = strtok_r(name, ":", &saveptr);
	if (!tok)
		return false;

	/* chip.hwunit */
	if (tok[0] == 'p') {
		tok = strtok(tok, ".");
		if (!tok)
			return false;

		if (strcmp(tok, PCHIP) != 0)
			return false;

		ct->chip_name = strdup(tok);
		assert(ct->chip_name);

		tok = strtok(NULL, ".");
		if (tok) {
			ct->class_name = strdup(tok);
			assert(ct->class_name);
		}

		tok = strtok_r(NULL, ":", &saveptr);
		if (!tok)
			return false;
	}

	/* cage */
	if (tok[0] == 'k') {
		ct->cage = atoi(&tok[1]);

		tok = strtok_r(NULL, ":", &saveptr);
		if (!tok)
			return true;
	}

	/* node */
	if (tok[0] == 'n') {
		ct->node = atoi(&tok[1]);

		tok = strtok_r(NULL, ":", &saveptr);
		if (!tok)
			return false;
	}

	/* slot */
	if (tok[0] == 's') {
		ct->slot = atoi(&tok[1]);

		tok = strtok_r(NULL, ":", &saveptr);
		if (!tok)
			return false;
	}

	/* chip_position */
	if (tok[0] == 'p') {
		ct->chip_position = atoi(&tok[1]);

		tok = strtok_r(NULL, ":", &saveptr);
		if (!tok)
			return true;
	}

	/* chip unit */
	if (tok[0] == 'c') {
		ct->chip_unit = atoi(&tok[1]);
	}

	return true;
}

static bool construct_cronus_target(struct cronus_target *ct, char *name, size_t len)
{
	int ret;

	/* Root */
	if (!ct->chip_name) {
		assert(ct->cage != -1);
		ret = snprintf(name, len, "k%d", ct->cage);
		goto done;
	}

	/* Proc with index */
	if (!ct->class_name) {
		assert(ct->cage != -1);
		assert(ct->node != -1);
		assert(ct->slot != -1);
		assert(ct->chip_position != -1);

		ret = snprintf(name, len, "%s:k%d:n%d:s%d:p%02d", ct->chip_name, ct->cage, ct->node, ct->slot, ct->chip_position);
		goto done;
	}

	assert(ct->chip_name);
	assert(ct->class_name);
	assert(ct->cage != -1);
	assert(ct->node != -1);
	assert(ct->slot != -1);
	assert(ct->chip_unit != -1);

	/* Targets which are sitting outside proc e.g bmc, tpm */
	if(ct->chip_position == -1) {
		ret = snprintf(name, len, "%s.%s:k%d:n%d:s%d:c%d", ct->chip_name, ct->class_name, ct->cage, ct->node, ct->slot, ct->chip_unit);
		goto done;
      }

	ret = snprintf(name, len, "%s.%s:k%d:n%d:s%d:p%02d:c%d", ct->chip_name, ct->class_name, ct->cage, ct->node, ct->slot, ct->chip_position, ct->chip_unit);

done:
	if (ret >= len)
		return false;

	return true;
}

struct match_by_class {
	struct cronus_target *ct;
	struct dtm_node *match;
};

static int match_node_by_class(struct dtm_node *node, void *priv)
{
	struct match_by_class *state = (struct match_by_class *)priv;
	const char *name, *cronus_class;
	char *dtree_class;
	uint32_t index;

	name = dtm_node_name(node);
	dtree_class = dtree_name_to_class(name);
	assert(dtree_class);

	cronus_class = dtree_to_cronus_class(dtree_class);
	free(dtree_class);

	if (!cronus_class)
		return 0;

	if (strcmp(cronus_class, state->ct->class_name) != 0)
		return 0;

	index = dtm_node_index(node);
	if (index == state->ct->chip_unit) {
		state->match = node;
		return 1;
	}

	return 0;
}

static struct dtm_node *dtm_find_node_by_class(struct dtm_node *root, struct cronus_target *ct)
{
	struct match_by_class state = {
		.ct = ct,
	};
	int ret;

	ret = dtm_traverse(root, true, match_node_by_class, NULL, &state);
	if (!ret)
		return NULL;

	return state.match;
}

struct dtm_node *dtree_from_cronus_target(struct dtm_node *root, const char *name)
{
	struct dtm_node *proc, *target;
	struct cronus_target ct;
	char *copy;
	char path[128];

	copy = strdup(name);
	assert(copy);

	if (!split_cronus_target(copy, &ct)) {
		free(copy);
		return NULL;
	}

	free(copy);

	/* Root */
	if (!ct.chip_name)
		return root;

	/* Targets which are sitting outside proc e.g bmc, tpm */
	if (ct.chip_position == -1) {
		assert(ct.class_name);
		assert(ct.chip_unit != -1);

		sprintf(path, "%s%d", ct.class_name, ct.chip_unit);
		target = dtm_find_node_by_name(root, path);
		return target;
	}

	/* Proc with index */
	sprintf(path, "/proc%d", ct.chip_position);
	proc = dtm_find_node_by_path(root, path);
	assert(proc);

	if (!ct.class_name) {
		return proc;
	}

	/* Proc with index --> hwunit with index */
	assert(ct.chip_unit != -1);

	target = dtm_find_node_by_class(proc, &ct);

	return target;
}

char *dtree_to_cronus_target(const struct dtm_node *root, struct dtm_node *node)
{
	struct cronus_target ct;
	char cname[128];

	cronus_target_init(&ct);

	ct.cage = 0;
	ct.node = 0;
	ct.slot = 0;
	if (node != root) {
		struct dtm_node *proc = NULL;
		const char *name, *cronus_class;
		char *dtree_class;

		ct.chip_name = PCHIP;

		for (proc = node; proc; proc = dtm_node_parent(proc)) {
			name = dtm_node_name(proc);

			dtree_class = dtree_name_to_class(name);
			assert(dtree_class);

			if (strcmp(dtree_class, "proc") == 0) {
				free(dtree_class);
				ct.chip_position = dtm_node_index(proc);
				break;
			}

			free(dtree_class);
		}

		if (node != proc) {
			name = dtm_node_name(node);

			dtree_class = dtree_name_to_class(name);
			assert(dtree_class);

			cronus_class = dtree_to_cronus_class(dtree_class);
			free(dtree_class);

			if (!cronus_class)
				return NULL;

			ct.class_name = cronus_class;
			ct.chip_unit = dtm_node_index(node);
		}
	}

	if (!construct_cronus_target(&ct, cname, sizeof(cname)))
		return NULL;

	return strdup(cname);
}
