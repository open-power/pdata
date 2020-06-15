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
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "fdt/fdt_traverse.h"
#include "dtm_internal.h"
#include "dtm.h"

static void dtm_file_free(struct dtm_file *dfile)
{
	if (!dfile)
		return;

	if (dfile->do_create) {
		if (dfile->ptr)
			free(dfile->ptr);
	} else {
		if (dfile->ptr != MAP_FAILED)
			munmap(dfile->ptr, dfile->len);
	}

	if (dfile->fd != -1)
		close(dfile->fd);

	free(dfile);
}

static struct dtm_file *_dtm_file_open(const char *filename, bool do_create, bool do_write)
{
	struct dtm_file *dfile;
	struct stat statbuf;
	int flags, prot, ret;

	dfile = malloc(sizeof(struct dtm_file));
	if (!dfile)
		return NULL;

	/* Create always means write */
	if (do_create)
		do_write = true;

	*dfile = (struct dtm_file) {
		.filename = filename,
		.fd = -1,
		.ptr = MAP_FAILED,
		.do_create = do_create,
		.do_write = do_write,
	};

	flags = do_write ? O_RDWR : O_RDONLY;
	if (do_create)
		flags |= O_CREAT;

	dfile->fd = open(filename, flags, 0644);
	if (dfile->fd == -1) {
		fprintf(stderr, "open() failed for %s\n", filename);
		goto fail;
	}

	if (do_create)
		return dfile;

	ret = fstat(dfile->fd, &statbuf);
	if (ret != 0) {
		fprintf(stderr, "fstat() failed for %s\n", filename);
		goto fail;
	}

	if (do_write) {
		prot = PROT_WRITE;
		flags = MAP_SHARED;
	} else {
		prot = PROT_READ;
		flags = MAP_PRIVATE;
	}

	dfile->len = statbuf.st_size;
	dfile->ptr = mmap(NULL, dfile->len, prot, flags, dfile->fd, 0);
	if (dfile->ptr == MAP_FAILED) {
		fprintf(stderr, "mmap() failed for %s\n", filename);
		goto fail;
	}

	return dfile;

fail:
	dtm_file_free(dfile);
	return NULL;
}

struct dtm_file *dtm_file_open(const char *filename, bool do_write)
{
	return _dtm_file_open(filename, false, do_write);
}

struct dtm_file *dtm_file_create(const char *filename)
{
	return _dtm_file_open(filename, true, true);
}

static int dtm_file_store(struct dtm_file *dfile)
{
	size_t nwritten = 0;
	ssize_t n;

	while (nwritten < dfile->len) {
		n = write(dfile->fd, dfile->ptr + nwritten, dfile->len - nwritten);
		if (n == -1)
			return errno;

		nwritten += n;
	}

	return 0;
}

int dtm_file_close(struct dtm_file *dfile)
{
	int ret;

	if (!dfile->do_create) {
		dtm_file_free(dfile);
		return 0;
	}

	ret = dtm_file_store(dfile);
	dtm_file_free(dfile);

	return ret;
}

static void *dtm_file_read_node(const char *name, void *_parent, void *priv)
{
	struct dtm_node *parent = (struct dtm_node *)_parent;
	struct dtm_node *child;

	child = dtm_node_new(name);
	if (!child)
		return NULL;

	dtm_tree_add_node(parent, child);
	return child;
}

static int dtm_file_read_prop(void *_node, const char *name, void *value, int valuelen, void *priv)
{
	struct dtm_node *node = (struct dtm_node *)_node;

	return  dtm_node_add_property(node, name, value, valuelen);
}

struct dtm_node *dtm_file_read(struct dtm_file *dfile)
{
	struct dtm_node *root;

	/* Parse only for files opened for read */
	if (dfile->do_create)
		return NULL;

	root = dtm_tree_new();
	if (!root)
		return NULL;

	if (!fdt_traverse_read(dfile->ptr, root, dtm_file_read_node, dtm_file_read_prop, NULL)) {
		dtm_tree_free(root);
		return NULL;
	}

	return root;
}

static const char *dtm_file_write_node(void *_parent, void **_child)
{
	struct dtm_node *parent = (struct dtm_node *)_parent;
	struct dtm_node **child = (struct dtm_node **)_child;
	struct dtm_node *next;

	next = dtm_node_next_child(parent, *child);
	if (next == NULL)
		return NULL;

	*child = next;

	return next->name;
}

static const void *dtm_file_write_prop(void *_node,
				       void **_prop,
				       const char **name,
				       int *value_len)
{
	struct dtm_node *node = (struct dtm_node *)_node;
	struct dtm_property **prop = (struct dtm_property **)_prop;
	struct dtm_property *next;

	next = dtm_node_next_property(node, *prop);
	if (next == NULL)
		return NULL;

	*prop = next;

	*name = next->name;
	*value_len = next->len;
	return next->value;
}

bool dtm_file_write(struct dtm_file *dfile, struct dtm_node *root)
{
	if (!dfile->do_create)
		return false;

	dfile->ptr = fdt_traverse_write(root, dtm_file_write_node, dtm_file_write_prop, &dfile->len);
	if (!dfile->ptr)
		return false;

	return true;
}
