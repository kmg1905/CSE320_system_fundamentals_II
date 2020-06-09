#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"

#define MIN_BLOCK_SIZE (32)

void assert_free_block_count(size_t size, int count);
void assert_free_list_block_count(size_t size, int count);

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
    sf_block *bp = sf_free_list_heads[i].body.links.next;
    while(bp != &sf_free_list_heads[i]) {
        if(size == 0 || size == (bp->header & BLOCK_SIZE_MASK))
        cnt++;
        bp = bp->body.links.next;
    }
   }
    cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
         size, count, cnt);
}

Test(sf_memsuite_student, malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    int *x = sf_malloc(sizeof(int));

    cr_assert_not_null(x, "x is NULL!");

    *x = 4;

    cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

    assert_free_block_count(0, 1);
    assert_free_block_count(4016, 1);

    cr_assert(sf_errno == 0, "sf_errno is not zero!");
    cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, malloc_three_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    void *x = sf_malloc(3 * PAGE_SZ - 2 * sizeof(sf_block));

    cr_assert_not_null(x, "x is NULL!");
    assert_free_block_count(0, 0);
    cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sf_memsuite_student, malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    void *x = sf_malloc(PAGE_SZ << 2);

    cr_assert_null(x, "x is not NULL!");
    assert_free_block_count(0, 1);
    assert_free_block_count(16336, 1);
    cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_quick, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    /* void *x = */ sf_malloc(8);
    void *y = sf_malloc(32);
    /* void *z = */ sf_malloc(1);

    sf_free(y);

    assert_free_block_count(0, 2);
    assert_free_block_count(3936, 1);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    /* void *x = */ sf_malloc(8);
    void *y = sf_malloc(200);
    /* void *z = */ sf_malloc(1);

    sf_free(y);

    assert_free_block_count(0, 2);
    assert_free_block_count(224, 1);
    assert_free_block_count(3760, 1);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    /* void *w = */ sf_malloc(8);
    void *x = sf_malloc(200);
    void *y = sf_malloc(300);
    /* void *z = */ sf_malloc(4);

    sf_free(y);
    sf_free(x);

    assert_free_block_count(0, 2);
    assert_free_block_count(544, 1);
    assert_free_block_count(3440, 1);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *u = sf_malloc(200);
    /* void *v = */ sf_malloc(300);
    void *w = sf_malloc(200);
    /* void *x = */ sf_malloc(500);
    void *y = sf_malloc(200);
    /* void *z = */ sf_malloc(700);

    sf_free(u);
    sf_free(w);
    sf_free(y);

    assert_free_block_count(0, 4);
    assert_free_block_count(224, 3);
    assert_free_block_count(1808, 1);

    // First block in list should be the most recently freed block.
    int i = 3;
    sf_block *bp = sf_free_list_heads[i].body.links.next;
    cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
             "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 2*sizeof(sf_header));
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(int));
    /* void *y = */ sf_malloc(10);
    x = sf_realloc(x, sizeof(int) * 10);

    cr_assert_not_null(x, "x is NULL!");
    sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
    cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
    cr_assert((bp->header & BLOCK_SIZE_MASK) == 64, "Realloc'ed block size not what was expected!");

    assert_free_block_count(0, 2);
    assert_free_block_count(3920, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(int) * 8);
    void *y = sf_realloc(x, sizeof(char));

    cr_assert_not_null(y, "y is NULL!");
    cr_assert(x == y, "Payload addresses are different!");

    sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
    cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
    cr_assert((bp->header & BLOCK_SIZE_MASK) == 48, "Block size not what was expected!");

    // There should be only one free block of size 4000.
    assert_free_block_count(0, 1);
    assert_free_block_count(4000, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(double) * 8);
    void *y = sf_realloc(x, sizeof(int));

    cr_assert_not_null(y, "y is NULL!");

    sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
    cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
    cr_assert((bp->header & BLOCK_SIZE_MASK) == 32, "Realloc'ed block size not what was expected!");

    // After realloc'ing x, we can return a block of size 48 to the freelist.
    // This block will go into the main freelist and be coalesced.
    assert_free_block_count(0, 1);
    assert_free_block_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

//This will test whether the program will be able to handle malloc request for complete free heap
//such that the next block after allocated block is epilogue.
Test(sf_memsuite_student, total_heap_alloc, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *u = sf_malloc(4012);

    sf_free(u);

    assert_free_block_count(0, 1);
    assert_free_block_count(4048, 1);

}


//This test case will check whether the pointe returned about relloc block is whether same / different
Test(sf_memsuite_student, relloc_change, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *uptr = sf_malloc(200);

    void *yptr = sf_realloc(uptr, 1000);

    cr_assert(uptr != yptr, "Realloc pointer didn't change");

    sf_free(yptr);

    assert_free_block_count(0, 1);
    assert_free_block_count(4048, 1);

}

//This test block will check whether the allocate bit of previous block is initialized or not
Test(sf_memsuite_student, realloc_smaller_block_header, .init = sf_mem_init, .fini = sf_mem_fini){
    double *dptr = sf_malloc(sizeof(double));
    *dptr = 20.56;
    void *ptr = sf_malloc(100);
    void *realloced = sf_realloc(ptr, sizeof(char));

    sf_block* test_block = (sf_block *)(realloced -2*sizeof(sf_header));
    cr_assert(((test_block->header & PREV_BLOCK_ALLOCATED) !=0),
        "Previous block's allocated bit is not properly initialized");
}

//This test case will check for freeing a given block twice error
Test(sf_memsuite_student, freeing_already_freed_block, .init = sf_mem_init,
 .fini = sf_mem_fini, .signal = SIGABRT) {
    void *ptr = sf_malloc(500);

    sf_free(ptr);
    sf_free(ptr);

    assert_free_block_count(0, 1);
    assert_free_block_count(4048, 1);
}



//This test case checks whether realloc block  returns null for a size of more than four pages
Test(sf_memsuite_student, realloc_more_than_four_pages, .init = sf_mem_init,
 .fini = sf_mem_fini) {
    void *ptr = sf_malloc(500);

    void *realloc = sf_realloc(ptr, PAGE_SZ * 5);

    cr_assert_null(realloc,"realloced block didn't return null");
}


//This test case will check for allcating multiple blocks over three pages and frees them at once.
Test(sf_memsuite_student, Malloc_three_pages_across_multiple_mallocs_then_frees_all, .init = sf_mem_init, .fini = sf_mem_fini)
{
    void *first = sf_malloc(2400);
    void *second =  sf_malloc(2600);
    void *third = sf_malloc(1000);
    void *fourth =  sf_malloc(500);
    void *fifth = sf_malloc(2000);


    //free operations
    sf_free(first);
    sf_free(second);
    sf_free(third);
    sf_free(fourth);
    sf_free(fifth);
}
