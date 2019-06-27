#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <libpdbg.h>

#include "translate.h"

struct target_class {
	const char *shortname;
	const char *classname;
} target_class_map[] = {
	{ "c", "core" },
	{ "obus", "obus" },
	{ "xbus", "xbus" },
	{ "perv", "chiplet" },
	{ NULL, "UNKNOWN" },
};

struct target_info {
	const char *chip;
	const char *class;
	int pib_id;
	int chip_id;
};

static const char *target_class_from_shortname(char *shortname)
{
	int i;

	for (i=0; target_class_map[i].shortname; i++) {
		struct target_class *tclass = &target_class_map[i];
		if (!strcmp(tclass->shortname, shortname))
			return tclass->classname;
	}

	return NULL;
}

static const char *shortname_from_target_class(const char *classname)
{
	int i;

	for (i=0; target_class_map[i].shortname; i++) {
		struct target_class *tclass = &target_class_map[i];
		if (!strcmp(tclass->classname, classname))
			return tclass->shortname;
	}

	return NULL;
}

/*
 * k0
 * p9n:k0:n0:s0:p00
 * p9n.xbus:k0:n0:s0:p00:c1
 */
static bool split_cronus_target(char *name, struct target_info *tinfo)
{
	char *tok, *saveptr = NULL;

	memset(tinfo, 0, sizeof(struct target_info));

	tok = strtok_r(name, ":", &saveptr);
	if (!tok)
		return false;

	if (!strcmp(tok, "k0")) {
		tinfo->class = strdup("root");
		assert(tinfo->class);
		return true;
	}

	tok = strtok(tok, ".");
	if (!tok)
		return false;

	tinfo->chip = strdup(tok);
	assert(tinfo->chip);

	tok = strtok(NULL, ".");
	if (!tok)
		tinfo->class = strdup("pib");
	else
		tinfo->class = target_class_from_shortname(tok);
	if (!tinfo->class)
		return false;

	tok = strtok_r(NULL, ":", &saveptr);
	if (!tok)
		return false;
	assert(!strcmp(tok, "k0"));

	tok = strtok_r(NULL, ":", &saveptr);
	if (!tok)
		return false;
	assert(!strcmp(tok, "n0"));

	tok = strtok_r(NULL, ":", &saveptr);
	if (!tok)
		return false;
	assert(!strcmp(tok, "s0"));

	tok = strtok_r(NULL, ":", &saveptr);
	if (!tok)
		return false;
	assert(tok[0] == 'p');
	tinfo->pib_id = atoi(&tok[1]);

	tok = strtok_r(NULL, ":", &saveptr);
	if (!tok)
		tinfo->chip_id = -1;
	else
		tinfo->chip_id = atoi(&tok[1]);

	return true;
}

static bool construct_cronus_target(struct target_info *tinfo, char *name, size_t len)
{
	const char *shortname;
	int ret;

	if (!strcmp(tinfo->class, "root")) {
		ret = snprintf(name, len, "k0");
	} else if (!strcmp(tinfo->class, "pib")) {
		ret = snprintf(name, len, "%s:k0:n0:s0:p%02d", tinfo->chip, tinfo->pib_id);
	} else {
		shortname = shortname_from_target_class(tinfo->class);
		assert(shortname);

		ret = snprintf(name, len, "%s.%s:k0:n0:s0:p%02d:c%d", tinfo->chip, shortname, tinfo->pib_id, tinfo->chip_id);
	}

	if (ret >= len)
		return false;

	return true;
}

struct pdbg_target *from_cronus_target(const char *name)
{
	struct pdbg_target *pib = NULL, *target;
	struct target_info tinfo;
	char *copy;

	copy = strdup(name);
	assert(copy);

	if (!split_cronus_target(copy, &tinfo)) {
		free(copy);
		return NULL;
	}

	free(copy);

	if (!strcmp(tinfo.class, "root"))
		return pdbg_target_root();

	pdbg_for_each_class_target("pib", target) {
		if (pdbg_target_index(target) == tinfo.pib_id) {
			pib = target;
			break;
		}
	}
	assert(pib);

	if (!strcmp(tinfo.class, "pib"))
		return pib;

	pdbg_for_each_target(tinfo.class, pib, target) {
		if (pdbg_target_index(target) == tinfo.chip_id)
			break;
	}

	return target;
}

char *to_cronus_target(struct pdbg_target *target)
{
	struct target_info tinfo = { 0 };
	char name[128];

	tinfo.chip = "p9n";

	if (target == pdbg_target_root()) {
		tinfo.class = "root";
	} else {
		const char *classname = pdbg_target_class_name(target);

		if (classname == NULL) {
			return NULL;
		}

		if (!strcmp(classname, "pib")) {
			tinfo.class = "pib";
			tinfo.pib_id = pdbg_target_index(target);
		} else {
			struct pdbg_target *pib;

			pib = pdbg_target_parent("pib", target);
			assert(pib);

			tinfo.class = pdbg_target_class_name(target);
			tinfo.pib_id = pdbg_target_index(pib);
			tinfo.chip_id = pdbg_target_index(target);
		}
	}

	if (!construct_cronus_target(&tinfo, name, sizeof(name)))
		return NULL;

	return strdup(name);
}
