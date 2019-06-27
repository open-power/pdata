#ifndef __DTREE_H__
#define __DTREE_H__

struct dtree_node;
struct dtree_context;

struct dtree_context *dtree_init(void *fdt);
void dtree_free(struct dtree_context *dtree);

int dtree_num_nodes(struct dtree_context *dtree);

struct pdbg_target *dtree_node_target(struct dtree_node *node);
uint32_t dtree_node_hash(struct dtree_node *node);

void dtree_node_set_private(struct dtree_node *node, void *private_data);
void *dtree_node_get_private(struct dtree_node *node);

struct dtree_node *dtree_node_next(struct dtree_context *dtree,
				   struct dtree_node *node);

#define for_each_dtree_node(dtree, node)		\
	for (node = dtree_node_next(dtree, NULL);	\
	     node;					\
	     node = dtree_node_next(dtree, node))

#endif /* __DTREE_H__ */
