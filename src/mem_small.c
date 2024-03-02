/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    /* ecrire votre code ici */
    if (arena.chunkpool == NULL|| *((void **) arena.chunkpool) == NULL) {
        unsigned long size = mem_realloc_small();
        for (unsigned long i =0 ; i< size-CHUNKSIZE;i+=CHUNKSIZE) {
            void **curr_address =  arena.chunkpool;
            *curr_address = (arena.chunkpool + CHUNKSIZE);
            arena.chunkpool += CHUNKSIZE;
        }
        *((void**) arena.chunkpool) = NULL;
        arena.chunkpool -= size - CHUNKSIZE;
    }

    void *first_element = arena.chunkpool;
    arena.chunkpool = *((void **) arena.chunkpool);
    return mark_memarea_and_get_user_ptr(first_element,CHUNKSIZE,SMALL_KIND);

}

void efree_small(Alloc a) {
    /* ecrire votre code ici */

    *((void**) a.ptr) = arena.chunkpool;
    arena.chunkpool = a.ptr;

}
