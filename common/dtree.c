#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ccan/hash/hash.h"
#include "libpdbg.h"

#include "dtree.h"

struct dtree_node {
	int index;
	struct pdbg_target *target;
	uint32_t hash;
	void *private_data;
};

struct dtree_node_list {
	int count;
	struct dtree_node *node;
};

struct dtree_context {
	void *fdt;
	struct dtree_node_list nlist;
};

static bool dtree_setup(struct dtree_context *dtree);

struct dtree_context *dtree_init(void *fdt)
{
	struct dtree_context *dtree;

	dtree = (struct dtree_context *)malloc(sizeof(struct dtree_context));
	if (!dtree) {
		fprintf(stderr, "Failed to allocate memory for dtree\n");
		return NULL;
	}

	*dtree = (struct dtree_context) {
		.fdt = NULL,
		.nlist = (struct dtree_node_list) {
			.count = 0,
			.node = NULL,
		},
	};

	dtree->fdt = fdt;

	if (!pdbg_target_root())
		pdbg_targets_init(dtree->fdt);

	if (!dtree_setup(dtree)) {
		dtree_free(dtree);
		return NULL;
	}

	return dtree;
}

void dtree_free(struct dtree_context *dtree)
{
	assert(dtree);

	if (dtree->nlist.count > 0)
		free(dtree->nlist.node);

	free(dtree);
}

static int dtree_count(struct pdbg_target *target)
{
	struct pdbg_target *child;
	int count = 1;

	pdbg_for_each_child_target(target, child) {
		count += dtree_count(child);
	}

	return count;
}

static void dtree_assign(struct pdbg_target *target,
			 struct dtree_node_list *nlist,
			 int *index)
{
	struct pdbg_target *child;
	struct dtree_node *node;
	char *path;

	assert(*index < nlist->count);

	node = &nlist->node[*index];

	node->index = *index;
	node->target = target;
	node->private_data = NULL;

	path = pdbg_target_path(target);
	assert(path);
	node->hash = hash_stable_8(path, strlen(path), 0);
	free(path);

	*index = *index + 1;

	pdbg_for_each_child_target(target, child) {
		dtree_assign(child, nlist, index);
	}
}

static bool dtree_setup(struct dtree_context *dtree)
{
	struct pdbg_target *root;
	int count, index;

	assert(dtree);
	root = pdbg_target_root();

	count = dtree_count(root);
	assert(count > 0);

	dtree->nlist.node = malloc(count * sizeof(struct dtree_node));
	if (!dtree->nlist.node) {
		fprintf(stderr, "Failed to allocate memory for nodes\n");
		return false;
	}
	dtree->nlist.count  = count;

	index = 0;
	dtree_assign(root, &dtree->nlist, &index);

	return true;
}

int dtree_num_nodes(struct dtree_context *dtree)
{
	return dtree->nlist.count;
}

struct pdbg_target *dtree_node_target(struct dtree_node *node)
{
	return node->target;
}

uint32_t dtree_node_hash(struct dtree_node *node)
{
	return node->hash;
}

void dtree_node_set_private(struct dtree_node *node, void *private_data)
{
	node->private_data = private_data;
}

void *dtree_node_get_private(struct dtree_node *node)
{
	return node->private_data;
}

struct dtree_node *dtree_node_next(struct dtree_context *dtree,
				   struct dtree_node *node)
{
	int index;

	assert(dtree);

	if (!node)
		return &dtree->nlist.node[0];

	index = node->index + 1;
	if (index >= dtree->nlist.count)
		return NULL;

	return &dtree->nlist.node[index];
}
