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

struct {
	const char *fapi;
	const char *dtree;
	const char *cronus;
} class_map[] = {
	{ "TARGET_TYPE_ABUS", "smpgroup", "smpgroup" },
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
	{ "TARGET_TYPE_OCMB_CHIP", "ocmb", "ocmb" },
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
	{ "TARGET_TYPE_ADC", "adc", "adc" },
	{ "TARGET_TYPE_GPIO_EXPANDER", "gpio_expander", "gpio_expander" },
	{ "TARGET_TYPE_PMIC", "pmic", "pmic" },

	{ "TARGET_TYPE_NX", "nx", "nx" },
	{ "TARGET_TYPE_OCC", "occ", "occ" },

	{ "TARGET_TYPE_TPM", "tpm", "tpm" },
	{ "TARGET_TYPE_BMC", "bmc", "bmc" },
	{ "TARGET_TYPE_OSCREFCLK", "oscrefclk", "oscrefclk" },
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

char *dtree_name_to_class(const char *name)
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
