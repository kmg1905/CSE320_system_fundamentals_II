#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

/* @Brief: This method can be used to add free memory blocks
 * to the pool of available free memory
 * /

void add_free_memory(sf_block *block) {

        size_t size = (block->header & BLOCK_SIZE_MASK);

        int m =  32;

        if (size < m)
            return;

        for (int i = 0; i < NUM_FREE_LISTS; i++, m = m*2) {
            if( size <= m || i == 8) {
                sf_block *temp = sf_free_list_heads[i].body.links.next;
                sf_free_list_heads[i].body.links.next = block;
                block->body.links.prev = &sf_free_list_heads[i];
                block->body.links.next = temp;
                temp->body.links.prev = block;
                break;
            }

        }
}


/* @ Brief: This method can be used to delete a free memory 
 * block from the pool of available free memory
 */

void delete_element(sf_block *temp) {

    sf_block *prev, *next;
    prev = (temp->body.links.prev);
    next = (temp->body.links.next);
    next->body.links.prev = prev;
    prev->body.links.next = next;
}

/* @Brief: This method is used to allocate requested memory
 *  from the pool of available memory
 */ 
void *big_block(size_t block_size) {
    size_t size = 32;

    for(int i = 0; i < NUM_FREE_LISTS; i++, size = size * 2) {

        if (((block_size <= size || (i == 8)) && block_size >= 32) &&
            (sf_free_list_heads[i].body.links.next != &sf_free_list_heads[i])) {

            sf_block *list_next = sf_free_list_heads[i].body.links.next;
            while(list_next != &sf_free_list_heads[i]) {
                sf_header header_size= list_next->header & BLOCK_SIZE_MASK;
                if (header_size < block_size) {
                   list_next = list_next->body.links.next;
                   continue;
                 }

                if ((header_size - block_size) >= 32) {
                    sf_block *new_block = ((sf_block*)(((void*)list_next)+block_size));
                    new_block->header = header_size - block_size;
                    new_block->header = new_block->header | PREV_BLOCK_ALLOCATED;
                    sf_block *temp2 =((sf_block*)(((void*)new_block)+(header_size - block_size)));
                    temp2->prev_footer = (new_block->header) ^ sf_magic();

                    add_free_memory(new_block);

                    list_next->header = block_size | THIS_BLOCK_ALLOCATED;
                    list_next->header = list_next->header | PREV_BLOCK_ALLOCATED;
                    new_block->prev_footer = list_next->header ^ sf_magic();

                    delete_element(list_next);

                    return (void*)list_next->body.payload;

                }

                else {

                    list_next->header = header_size | THIS_BLOCK_ALLOCATED;
                    list_next->header = list_next->header | PREV_BLOCK_ALLOCATED;
                    size_t size = list_next->header & BLOCK_SIZE_MASK;
                    sf_block *temp = ((sf_block*)(((void*)list_next)+size));

                    temp->prev_footer = list_next->header ^ sf_magic();

                    size = temp->header & BLOCK_SIZE_MASK;

                    delete_element(list_next);


                    if ((temp->header & BLOCK_SIZE_MASK) != 0) {
                        temp->header = size | PREV_BLOCK_ALLOCATED;
                        temp->header = temp->header | THIS_BLOCK_ALLOCATED;
                        //Moving to the next block
                        sf_block *temp2 = ((sf_block*)(((void*)temp)+size));
                        temp2->prev_footer = (temp->header) ^ sf_magic();
                    }
                     else {
                        sf_epilogue *epilogue = sf_mem_end()-8;
                        epilogue->header = PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED;
                    }
                    test_block(list_next);
                    return (void*)list_next->body.payload;
                }

                list_next = list_next->body.links.next;

            }

        }

    }
    return NULL;

}

/* @Brief: This method is used to search for a freeblock 
 * from the pool of freeblocks. If freeblocks are not available, 
 * then the size of the free memory is called by adding PAGE
 * size memory to the available freeblocks. 
 */ 
void *search_block(size_t block_size) {

    int flag = 0;
    size_t size = 32;

    for(int i = 0; i < NUM_FREE_LISTS; i++, size = size * 2) {

        if ((block_size <= size || i == 8) &&
            (sf_free_list_heads[i].body.links.next != &sf_free_list_heads[i])) {


            sf_block *list_next = sf_free_list_heads[i].body.links.next;
            while(list_next != &sf_free_list_heads[i]) {

                sf_header header_size= list_next->header & BLOCK_SIZE_MASK;

                if (header_size < block_size) {
                list_next = list_next->body.links.next;
                     continue;
                 }

                 flag = 1;

                if ((header_size - block_size) >= 32) {
                    sf_block *new_block = ((sf_block*)(((void*)list_next)+block_size));
                    new_block->header = header_size - block_size;
                    new_block->header = new_block->header | PREV_BLOCK_ALLOCATED;
                    sf_block *temp2 =((sf_block*)(((void*)new_block)+(header_size - block_size)));
                    temp2->prev_footer = (new_block->header) ^ sf_magic();

                    add_free_memory(new_block);

                    list_next->header = block_size | THIS_BLOCK_ALLOCATED;
                    list_next->header = list_next->header | PREV_BLOCK_ALLOCATED;
                    new_block->prev_footer = list_next->header ^ sf_magic();

                    delete_element(list_next);

                    return (void*)list_next->body.payload;

                }

                else {

                    list_next->header = header_size | THIS_BLOCK_ALLOCATED;
                    list_next->header = list_next->header | PREV_BLOCK_ALLOCATED;
                    size_t size = list_next->header & BLOCK_SIZE_MASK;
                    sf_block *temp = ((sf_block*)(((void*)list_next)+size));

                    temp->prev_footer = list_next->header ^ sf_magic();

                    size = temp->header & BLOCK_SIZE_MASK;

                    delete_element(list_next);


                    if ((temp->header & BLOCK_SIZE_MASK) != 0) {
                        temp->header = size | PREV_BLOCK_ALLOCATED;
                        temp->header = temp->header | THIS_BLOCK_ALLOCATED;
                        //Moving to the next block
                        sf_block *temp2 = ((sf_block*)(((void*)temp)+size));
                        temp2->prev_footer = (temp->header) ^ sf_magic();
                    }
                    else {
                        sf_epilogue *epilogue = sf_mem_end()-8;
                        epilogue->header = PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED;
                    }
                    return (void*)list_next->body.payload;
                }

                list_next = list_next->body.links.next;

            }

        }

    }

    if (flag == 0) {
        int status = 0;
        sf_block *bptr = NULL;
        sf_block *temp = NULL;
        size_t tsize = 0;
        page:
        status = 0;
        if ((bptr = sf_mem_grow()) == NULL) {
            sf_errno = ENOMEM;
            return NULL;
        }

        else {
            bptr = (void*)bptr - 16;
            bptr->header = PAGE_SZ ;

            if ((bptr->prev_footer ^ sf_magic()) & THIS_BLOCK_ALLOCATED) {
                bptr->header = bptr->header | PREV_BLOCK_ALLOCATED;

                status = 1;

            }

            else {
                size_t size = (bptr->prev_footer ^ sf_magic()) & BLOCK_SIZE_MASK;

                temp = (((void*)bptr) - size);

                if ((temp->prev_footer ^ sf_magic()) & PREV_BLOCK_ALLOCATED) {
                    temp->header = (bptr->header & BLOCK_SIZE_MASK) + size;
                    temp->header = temp->header | PREV_BLOCK_ALLOCATED;
                }
                else {
                    temp->header = (bptr->header & BLOCK_SIZE_MASK) + size;


                }
                delete_element(temp);
            }

            //setting up epilogue
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end() - 8);
            epilogue->header = 0;
            epilogue->header= epilogue->header | THIS_BLOCK_ALLOCATED;

            //adding footer before epilogue block
            sf_footer *footer = (sf_footer*)(sf_mem_end() - 16);

           if (status == 1) {
            *footer = bptr->header ^ sf_magic();
            tsize = bptr->header & BLOCK_SIZE_MASK;
            add_free_memory(bptr);

           }
           else {
            *footer = temp->header ^ sf_magic();
            tsize = temp->header & BLOCK_SIZE_MASK;
            add_free_memory(temp);
        }
        }

        if (tsize < block_size)
            goto page;
        else {
            return big_block(block_size);
        }

    }
    return NULL;
}



void *sf_malloc(size_t size) {

    if (size <= 0)
        return NULL;

    //A page of memory is initialised with prologuw and epilogue
    if (sf_mem_start() == sf_mem_end()) {

        if (sf_mem_grow() == NULL) {
            sf_errno = EINVAL;
            abort();
        }

        //initializing prologue block in allocated free block
        sf_prologue *prologue = (sf_prologue*)sf_mem_start();
        prologue->header = 32;
        prologue->header = prologue->header | PREV_BLOCK_ALLOCATED;
        prologue->header = prologue->header | THIS_BLOCK_ALLOCATED;

        prologue->footer = prologue->header ^ sf_magic();

        //initializing epilogue block in allocated free block
        sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end() - 8);
        epilogue->header = 0;
        epilogue->header= epilogue->header | THIS_BLOCK_ALLOCATED;

        //making entire unallocated block as a free block
        sf_block *block = (sf_block*)&(prologue->footer);
        block->header = PAGE_SZ - 40 - 8;
        block->header = block->header | PREV_BLOCK_ALLOCATED;
        //block->body.links.next =

        //adding footer before epilogue block
        sf_footer *footer = (sf_footer*)(sf_mem_end() - 16);
        *footer = block->header ^ sf_magic();

        //initializing sf_free_list_heads block
        for (int i =0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &(sf_free_list_heads[i]);
            sf_free_list_heads[i].body.links.prev = &(sf_free_list_heads[i]);
        }

        add_free_memory(block);

    }

    size_t block_size = size + 16;

    if ((block_size % 16) != 0) {
        size_t i = block_size/16;
        block_size = (i+1) * 16;
    }

    return search_block(block_size);
}


void sf_free(void *pp) {

    if (pp == NULL)
        abort();

    pp = pp - 16;

    sf_block *free_ptr = (sf_block*)(pp);

    if (!(free_ptr->header & THIS_BLOCK_ALLOCATED))
        abort();

    size_t test_size = free_ptr->header & BLOCK_SIZE_MASK;

    sf_block *footer = (sf_block*)(((void*)free_ptr)+ test_size);

    if (free_ptr->header != (footer->prev_footer ^ sf_magic()))
        abort();


    if ((free_ptr->header & BLOCK_SIZE_MASK) < 32)
        abort();

    //
    if ((free_ptr->header & PREV_BLOCK_ALLOCATED) ==0) {
        size_t size = (free_ptr->prev_footer^ sf_magic()) & BLOCK_SIZE_MASK;
        sf_block *temp3 = (sf_block*)(((void*)free_ptr) - size);
        if (((temp3->header) & THIS_BLOCK_ALLOCATED) != 0) {
            abort();
        }
    }
    size_t size = (free_ptr->header & BLOCK_SIZE_MASK);

    if (!((free_ptr->header) & THIS_BLOCK_ALLOCATED) || (size < 32)) {
        abort();
    }

    if ((free_ptr->header & PREV_BLOCK_ALLOCATED)) {

         if (!(free_ptr->header & THIS_BLOCK_ALLOCATED)) {
             abort();
         }
     }

    sf_block *test = ((sf_block*)(((void*)free_ptr)+size));

    if ((void*)&(free_ptr->header) < (sf_mem_start()+40) ||
            (void*)&(test->prev_footer) >= (sf_mem_end() - 8))
        abort();

    sf_block *temp  = NULL;

    if (!(free_ptr->header & PREV_BLOCK_ALLOCATED)) {
        size = (((free_ptr->prev_footer) ^ sf_magic()) & BLOCK_SIZE_MASK);
        temp = free_ptr;
        free_ptr = (sf_block*)(((void*)free_ptr) - size);

        free_ptr->header = ((temp->header & BLOCK_SIZE_MASK) + size);
        free_ptr->header = free_ptr->header | PREV_BLOCK_ALLOCATED;

        delete_element(free_ptr);


        size = ((free_ptr->header) & BLOCK_SIZE_MASK);

        temp = (sf_block*)(((void*)free_ptr) + size);


        if (!(temp->header & THIS_BLOCK_ALLOCATED)) {
            delete_element(temp);
            size = (temp->header) & BLOCK_SIZE_MASK;

            free_ptr->header = ((free_ptr->header & BLOCK_SIZE_MASK) + size);
            free_ptr->header = free_ptr->header | PREV_BLOCK_ALLOCATED;

            sf_block *temp2 = (sf_block*)(((void*)temp) + size );

            temp2->prev_footer = (free_ptr->header) ^ sf_magic();

            size = (temp2->header) & BLOCK_SIZE_MASK;
            if ((temp2->header & BLOCK_SIZE_MASK) != 0) {
                temp2->header = size | THIS_BLOCK_ALLOCATED;
                        //Moving to the next block
                sf_block *temp3 = ((sf_block*)(((void*)temp)+size));
                temp3->prev_footer = (temp2->header) ^ sf_magic();
            }
            add_free_memory(free_ptr);
        }
        else {
            temp->prev_footer = (free_ptr->header) ^ sf_magic();
            size = ((temp->header) & BLOCK_SIZE_MASK);
           if ((temp->header & BLOCK_SIZE_MASK) != 0) {
                temp->header = size | THIS_BLOCK_ALLOCATED;
                        //Moving to the next block
                sf_block *temp3 = ((sf_block*)(((void*)temp)+size));
                temp3->prev_footer = (temp->header) ^ sf_magic();
            }
            add_free_memory(free_ptr);
        }
    }

    else {

    temp = (sf_block*)(((void*)free_ptr) + size);
    size = ((temp->header) & BLOCK_SIZE_MASK);

    if (!(temp->header & THIS_BLOCK_ALLOCATED)) {



            free_ptr->header = ((free_ptr->header & BLOCK_SIZE_MASK) + size);
            free_ptr->header = free_ptr->header | PREV_BLOCK_ALLOCATED;


            sf_block *temp2;
            temp2 = (sf_block*)(((void*)temp) + size);
            size = temp2->header & BLOCK_SIZE_MASK;

            temp2->prev_footer = (free_ptr->header) ^ sf_magic();
            if (((temp2->header & BLOCK_SIZE_MASK)) != 0) {
                temp2->header = size | THIS_BLOCK_ALLOCATED;
                        //Moving to the next block
                sf_block *temp3 = ((sf_block*)(((void*)temp)+size));
                temp3->prev_footer = (temp2->header) ^ sf_magic();
            }
        delete_element(temp);

        add_free_memory(free_ptr);
    }

    else {
        size = ((free_ptr->header) & BLOCK_SIZE_MASK);
        temp = (sf_block*)(((void*)free_ptr) + size);
        free_ptr->header = size | PREV_BLOCK_ALLOCATED;


        temp->prev_footer = (free_ptr->header ^ sf_magic());
        size = ((temp->header) & BLOCK_SIZE_MASK);
        if ((temp->header & BLOCK_SIZE_MASK) != 0) {
        temp->header = size | THIS_BLOCK_ALLOCATED;

        sf_block *temp2 = (sf_block*)(((void*)temp) + size);
        temp2->prev_footer = ((temp->header) ^ sf_magic());
        }
        add_free_memory(free_ptr);
        }
    }

}



void *sf_realloc(void *pp, size_t rsize) {

    if (pp == NULL) {
        sf_errno = EINVAL;
        return NULL;

    }

    if (rsize == 0) {
        sf_free(pp);
        return NULL;

    }

    size_t temp = rsize;
    rsize += 16;
    if ((rsize % 16) != 0) {
        size_t i = rsize/16;
        rsize = (i+1) * 16;
    }
    pp = pp - 16;

    sf_block *init_block = (sf_block*)pp;

    if ((init_block->header & BLOCK_SIZE_MASK) < 32) {
        sf_errno = EINVAL;
        return NULL;
    }

    if (!(init_block->header & THIS_BLOCK_ALLOCATED)) {
        sf_errno = EINVAL;
        return NULL;
    }

    size_t isize = (init_block->header & BLOCK_SIZE_MASK);

    //sf_block *test2 = ((sf_block*)(((void*)init_block)));

    if ((void*)(&(init_block->header))< (sf_mem_start()+ 40) ||
            (void*)(&(init_block->header)) >= (sf_mem_end() - 8)) {
        sf_errno = EINVAL;
        return NULL;
    }

    if ((init_block->header & PREV_BLOCK_ALLOCATED) ==0) {
        size_t size = (init_block->prev_footer^ sf_magic()) & BLOCK_SIZE_MASK;
        sf_block *temp3 = (sf_block*)(((void*)init_block) - size);
        if (((temp3->header) & THIS_BLOCK_ALLOCATED) != 0) {
           sf_errno = EINVAL;
           return NULL;
        }
    }

    sf_block *test = ((sf_block*)(((void*)init_block)+isize));

    if (init_block->header != (test->prev_footer ^ sf_magic())) {
        sf_errno = EINVAL;
        return NULL;
    }

     test_block(init_block);

     if ((void*)init_block < (sf_mem_start()+32) ||
            (void*)(test) > (sf_mem_end() - 8)) {
                sf_errno = EINVAL;
                return NULL;
            }

    if (rsize >= isize) {
        void *test_block =(sf_malloc(temp));

        if ((test_block)  == NULL)
            return NULL;

        sf_block *new_block = ((sf_block*)(test_block - 16));

        memcpy(new_block->body.payload, init_block->body.payload, isize);

        init_block->header = isize | PREV_BLOCK_ALLOCATED;
        init_block->header = init_block->header | THIS_BLOCK_ALLOCATED;

        sf_block *temp2 = ((sf_block*)(((void*)init_block)+isize));

        temp2->prev_footer = init_block->header ^ sf_magic();

        sf_free(init_block->body.payload);
        return (void*)new_block->body.payload;
    }

    else if (rsize < isize) {
        if ((isize - rsize) >= 32) {
            sf_block *temp = ((sf_block*)(((void*)init_block)+rsize));
            size_t size = rsize | THIS_BLOCK_ALLOCATED;
            if (init_block->header & PREV_BLOCK_ALLOCATED)
                size = size | PREV_BLOCK_ALLOCATED;

            init_block->header = size;

            temp->prev_footer = init_block->header ^ sf_magic();

            temp->header = (isize - rsize) | PREV_BLOCK_ALLOCATED | THIS_BLOCK_ALLOCATED;

            sf_block *temp2 = (sf_block*)(((void*)temp) + (isize - rsize));
            temp2->prev_footer = ((temp->header) ^ sf_magic());

            if ((temp2->header & BLOCK_SIZE_MASK) != 0) {

                size = temp2->header & BLOCK_SIZE_MASK;
                sf_block *temp3 = (sf_block*)(((void*)temp) + size);
                temp3->prev_footer = ((temp2->header) ^ sf_magic());

                }

            sf_free(temp->body.payload);
        }

        return (void*)init_block->body.payload;
    }

    return NULL;
}
