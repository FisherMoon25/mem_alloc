/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

unsigned long magic_number(void *ptr, MemKind k) {
    unsigned long magic = knuth_mmix_one_round((unsigned long) ptr);
    return (magic & ~(0b11UL)) | k;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    unsigned long magic = magic_number(ptr, k);
    size_t s = sizeof(unsigned long);

    unsigned long *tmp = (unsigned long *) ptr;
    *tmp = size;
    *(tmp+1) = magic;

    tmp = (unsigned long *) (ptr + size - 2*s);
    *tmp = magic;
    *(tmp+1) = size;


    return (ptr + 2*s);
}

Alloc
mark_check_and_get_alloc(void *ptr)
{

    // Positionner pointeur sur taille
    size_t s = sizeof(unsigned long);

    unsigned long *tmp = (unsigned long *) ptr;

    unsigned long size_init = *(tmp-2);

    unsigned long magic_init = *(tmp-1);

    tmp = (unsigned long *) (ptr + size_init -2*s);

    unsigned long size_end = *(tmp-1);
    unsigned long  magic_end = *(tmp-2);

    MemKind memK = (MemKind) (magic_init & 0b11UL);

    assert(magic_init == magic_end);
    assert(size_init == size_end);
    
    Alloc a = {
            (void *) ptr -2*s,
            memK,
            size_init
    };
    return a;

}





unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
