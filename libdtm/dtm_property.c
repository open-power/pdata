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

#include <libfdt.h>

#include "dtm_internal.h"
#include "dtm.h"

struct dtm_property *dtm_prop_new(const char *name, void *value, int len)
{
	struct dtm_property *prop;

	prop = calloc(1, sizeof(struct dtm_property));
	if (!prop)
		return NULL;

	prop->name = strdup(name);
	if (!prop->name) {
		dtm_prop_free(prop);
		return NULL;
	}

	prop->value = malloc(len);
	if (!prop->value) {
		dtm_prop_free(prop);
		return NULL;
	}

	memcpy(prop->value, value, len);
	prop->len = len;

	return prop;
}

void dtm_prop_free(struct dtm_property *prop)
{
	if (prop->name)
		free(prop->name);
	if (prop->value)
		free(prop->value);
	free(prop);
}

struct dtm_property *dtm_prop_copy(struct dtm_property *prop)
{
	return dtm_prop_new(prop->name, prop->value, prop->len);
}

const char *dtm_prop_name(struct dtm_property *prop)
{
	return prop->name;
}

const void *dtm_prop_value(struct dtm_property *prop, int *value_len)
{
	if (value_len)
		*value_len = prop->len;

	return prop->value;
}

int dtm_prop_set_value(struct dtm_property *prop, uint8_t *value, int value_len)
{
	if (prop->len != value_len)
		return -1;

	memcpy(prop->value, value, value_len);
	return 0;
}

uint32_t dtm_prop_value_u32(struct dtm_property *prop)
{
	uint32_t data;

	assert(prop->len == sizeof(uint32_t));

	memcpy(&data, prop->value, sizeof(uint32_t));
	return fdt32_to_cpu(data);
}
