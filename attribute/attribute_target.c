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
#include <assert.h>
#include <ctype.h>

#include "libdtm/dtm.h"

#include "config.h"
#include "attribute.h"
#include "attribute_target.h"
#include "attribute_util.h"

struct {
	const char *fapi;
	const char *dtree;
	const char *cronus;
} class_map[] = {
	{ "TARGET_TYPE_ABUS", "abus", "abus" },
	{ "TARGET_TYPE_CAPP", "capp", "capp" },
	{ "TARGET_TYPE_CORE", "core", "c" },
	{ "TARGET_TYPE_DIMM", "dimm", "dimm" },
	{ "TARGET_TYPE_DMI", "dmi", "dmi" },
	{ "TARGET_TYPE_EQ", "eq", "eq" },
	{ "TARGET_TYPE_EX", "ex", "ex" },
	{ "TARGET_TYPE_FC", "fc", "fc" },
	{ "TARGET_TYPE_IOHS", "iohs", "iohs" },
	{ "TARGET_TYPE_L4", "l4", "l4" },
	{ "TARGET_TYPE_MBA", "mba", "mba" },
	{ "TARGET_TYPE_MC", "mc", "mc" },
	{ "TARGET_TYPE_MCA", "mca", "mca" },
	{ "TARGET_TYPE_MCBIST", "mcbist", "mcbist" },
	{ "TARGET_TYPE_MCC", "mcc", "mcc" },
	{ "TARGET_TYPE_MCS", "mcs", "mcs" },
	{ "TARGET_TYPE_MEMBUF_CHIP", "membuf_chip", "membuf_chip" },
	{ "TARGET_TYPE_MEM_PORT", "mem_port", "mem_port" },
	{ "TARGET_TYPE_MI", "mi", "mi" },
	{ "TARGET_TYPE_NMMU", "nmmu", "nmmu" },
	{ "TARGET_TYPE_OBUS", "obus", "obus" },
	{ "TARGET_TYPE_OBUS_BRICK", "obus_brick", "obus_brick" },
	{ "TARGET_TYPE_OCMB_CHIP", "ocmb_chip", "obmc_chip" },
	{ "TARGET_TYPE_OMI", "omi", "omi" },
	{ "TARGET_TYPE_OMIC", "omic", "omic" },
        { "TARGET_TYPE_PAU", "pau", "pau" },
        { "TARGET_TYPE_PAUC", "pauc", "pauc" },
	{ "TARGET_TYPE_PEC", "pec", "pec" },
	{ "TARGET_TYPE_PERV", "chiplet", "perv" },
	{ "TARGET_TYPE_PERV", "perv", "perv" },
	{ "TARGET_TYPE_PHB", "phb", "phb" },
	{ "TARGET_TYPE_PMIC", "pmic", "pmic" },
	{ "TARGET_TYPE_PPE", "ppe", "ppe" },
	{ "TARGET_TYPE_PROC_CHIP", "proc", "proc_chip" },
	{ "TARGET_TYPE_SBE", "sbe", "sbe" },
	{ "TARGET_TYPE_SYSTEM", "root", "system" },
	{ "TARGET_TYPE_XBUS", "xbus", "xbus" },

	{ "TARGET_TYPE_NX", "nx", "nx" },
	{ "TARGET_TYPE_OCC", "occ", "occ" },
	{ NULL, NULL, NULL },
};

const char *dtree_to_fapi_class(const char *dtree_class)
{
	int i;

	assert(dtree_class);

	for (i=0; class_map[i].fapi; i++) {
		if (strcmp(dtree_class, class_map[i].dtree) == 0)
			return class_map[i].fapi;
	}

	return NULL;
}

const char *cronus_to_dtree_class(const char *cronus_class)
{
	int i;

	assert(cronus_class);

	for (i=0; class_map[i].fapi; i++) {
		if (strcmp(cronus_class, class_map[i].cronus) == 0)
			return class_map[i].dtree;
	}

	return NULL;
}

const char *dtree_to_cronus_class(const char *dtree_class)
{
	int i;

	assert(dtree_class);

	for (i=0; class_map[i].fapi; i++) {
		if (strcmp(dtree_class, class_map[i].dtree) == 0)
			return class_map[i].cronus;
	}

	return NULL;
}

static char *dtree_name_to_class(const char *name)
{
	char *tmp, *tok, *class_name;
	size_t i, n;

	if (name[0] == '\0')
		return strdup("root");

	tmp = strdup(name);
	assert(tmp);

	tok = strtok(tmp, "@");
	assert(tok);

	n = strlen(tok);
	for (i = n-1; i >= 0; i--) {
		if (isdigit(tok[i]))
			tok[i] = '\0';
		else
			break;
	}

	class_name = strdup(tok);
	assert(class_name);

	free(tmp);

	return class_name;
}

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

	assert(ct->class_name);
	assert(ct->chip_unit != -1);

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

struct dtm_node *from_cronus_target(struct dtm_node *root, const char *name)
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

	/* Proc with index */
	assert(ct.chip_position != -1);

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

char *to_cronus_target(struct dtm_node *root, struct dtm_node *node)
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

		if (proc == NULL)
			return NULL;

		assert(ct.chip_position != -1);

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
