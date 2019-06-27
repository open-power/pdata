#ifndef __TRANSLATE_H
#define __TRANSLATE_H

#include <libpdbg.h>

struct pdbg_target *from_cronus_target(const char *name);
char *to_cronus_target(struct pdbg_target *target);

#endif /* __TRANSLATE_H */
