#ifndef INCLUDED_PRMEM_H
#define INCLUDED_PRMEM_H

#include <stdlib.h>

#define PR_Malloc(size) malloc(size)
#define PR_Free(size) free(size)

#define PR_FREEIF(ptr) if (ptr) { free(ptr); (ptr) = 0; }

#endif /* INCLUDED_PRMEM_H */
