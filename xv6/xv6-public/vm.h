#include "mmu.h"
#include "types.h"

int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
static pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);
