/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"


#define MIN(a,b) (((a)<(b))?(a):(b))

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


void recursive_split(unsigned int index) {

    if (FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant == index + 1) {
        mem_realloc_medium();
    }
    else if (arena.TZL[index+1]  == NULL) {
        recursive_split(index + 1);
    }

    //split le bloc d'indice "index+1" en 2
    void *tmp = arena.TZL[index+1];
    uintptr_t tmp_addr = (uintptr_t)tmp;
    arena.TZL[index+1] = *((void **) arena.TZL[index+1]);
    arena.TZL[index] = tmp;
    *((void **) tmp) =(void *) (tmp_addr ^ (1<<index));
}
void recursive_div(void* ptr, unsigned long size_ptr, unsigned long indice_ptr, unsigned long difference) {
    if (difference == 0) {
        return;
    }
    void* bloc_libre = ptr + (size_ptr >> 1);
    *((void **) bloc_libre) = arena.TZL[indice_ptr - 1];
    arena.TZL[indice_ptr - 1] = bloc_libre;

    return recursive_div(ptr, (size_ptr >> 1), indice_ptr - 1, difference - 1);
}

void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    /* ecrire votre code ici */
    unsigned int index = puiss2(size+32);
    if (arena.TZL[index] == NULL) {
        //Splitting recursive
        //recursive_split(index);
        unsigned int indice_libre = index;
        while (indice_libre < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant
               && arena.TZL[indice_libre] == NULL) {
            indice_libre++;
        }
        if (indice_libre == FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) {
            mem_realloc_medium();
        }
        void *ptr = arena.TZL[indice_libre];
        arena.TZL[indice_libre] = *((void **) arena.TZL[indice_libre]);
        //diviser le bloc en 2
        unsigned long difference = indice_libre - index;
        recursive_div(ptr,(1<<indice_libre),indice_libre,difference);
        return mark_memarea_and_get_user_ptr(ptr,1<<index,MEDIUM_KIND);
    }
    //marquage
    void *ptr = arena.TZL[index];
    arena.TZL[index] = *((void **) ptr);

    return mark_memarea_and_get_user_ptr(ptr,1<<index,MEDIUM_KIND);

}

void efree_recursive(void *ptr, unsigned long index) {
    uintptr_t ptr_addr = (uintptr_t)ptr;
    void *buddy = (void *)(ptr_addr ^ (1<<index));

    void *current_ptr = arena.TZL[index];
    void *previous_ptr = current_ptr;
    while (current_ptr != buddy && current_ptr != NULL) {
        previous_ptr = current_ptr;
        current_ptr = *((void **) current_ptr);
    }
    if (current_ptr == NULL) {
        *((void **) ptr) = arena.TZL[index];
        arena.TZL[index] = ptr;
    }
    else if (current_ptr == buddy) {
        if (previous_ptr == NULL) {
            arena.TZL[index] = *((void **) ptr);
        }
        else {
            *((void **)  previous_ptr) = *((void **) current_ptr);
        }
        void *rec_ptr = (buddy>ptr)? ptr:buddy;
        efree_recursive(rec_ptr,index + 1);
    }
}
void efree_medium(Alloc a) {
    // calcul buddy

    void* buddy = (void *) (((unsigned long) a.ptr) ^ a.size);

    // vérifier si buddy présent
    unsigned int index = puiss2(a.size);
    void* current_ptr = arena.TZL[index];
    void* previous_ptr = NULL;
    while (current_ptr != NULL && current_ptr!=buddy) {
        previous_ptr = current_ptr;
        current_ptr = *((void **) current_ptr);
    }
    // buddy est présent
    if (current_ptr == buddy) {
        // enlève buddy
        if (previous_ptr == NULL) {
            arena.TZL[index] = *((void **) current_ptr);
        } else {
            *((void **) previous_ptr) = *((void **) current_ptr);
        }
        // appel recursif avec un nouveau bloc(le bloc fusionné)
        void* new_ptr = (buddy>a.ptr)? a.ptr:buddy;
        Alloc fused_bloc = {new_ptr, MEDIUM_KIND, (a.size << 1)};
        return efree_medium(fused_bloc);
    }
    else {
        // buddy n'est pas présent
        *((void **) a.ptr) = arena.TZL[index];
        arena.TZL[index] = a.ptr;
    }
}

void efree_medium1(Alloc a) {
    /* ecrire votre code ici */
    *((void **)a.ptr) = NULL;
    unsigned int index = puiss2(a.size);
    efree_recursive(a.ptr,index);
}


