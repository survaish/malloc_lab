/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /*Team name*/
    "ATeam",
    /* First member's full name */
    "AAA AAA",
    /* First member's NetID */
    "aaa123",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's NetID (leave blank if none)*/
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* book macros - MY CODE*/
#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc)) 

#define GET(p)       (*(unsigned int *)(p)) /
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) 

#define GET_SIZE(p)  (GET(p) & ~0x7) 
#define GET_ALLOC(p) (GET(p) & 0x1) 

#define HDRP(ptr)       ((char *)(ptr) - WSIZE) 
#define FTRP(ptr)       ((char *)(ptr) + GET_SIZE(HEADER(ptr)) - DSIZE) 

#define NEXT_BLKP(ptr)  ((char *)(ptr) + GET_SIZE(((char *)(ptr) - WSIZE))) 
#define PREV_BLKP(ptr)  ((char *)(ptr) - GET_SIZE(((char *)(ptr) - DSIZE))) 


//HELPER FUNCTIONS - WOO//

static void *extend_heap(size_t words){
        char *bp;
        size_t size;
        size = (words % 2) ? (words+1)*WSIZE : words * WSIZE; /*Allocate even # of words for alignment*/
        if ((long)(bp = mem_sbrk(size)) == -1)
                return NULL;
        PUT(HDRP(bp), PACK(size,0)); /*Initialize free block header*/
        PUT(FTRP(bp), PACK(size,0));/*Initialize free block footer*/
        PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); /*new epilogue header*/
        return coalesce(bp); /*coalesce if previous block was free*/
}

static void *coalesce(void *bp){
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));
        if (prev_alloc && next_alloc){ /* if both allocated, do nothing */
                return bp;}
        else if (prev_alloc && !next_alloc){ /* next block free condition*/
                size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(HDRP(bp), PACK(size, 0));
                PUT(FTRP(bp), PACK(size,0));}
        else if (!prev_alloc && next_alloc){ /*previous block free condition */
                size += GET_SIZE(HDRP(PREV_BLKP(bp)));
                PUT(FTRP(bp), PACK(size, 0));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                bp = PREV_BLKP(bp);}
        else{ /*both blocks free condition */
                size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
                bp = PREV_BLKP(bp);}
        return bp;
}


static void *find_fit(size_t asize){
        void *bp;
        for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){ /*first fit*/
                if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
                        return bp;}}
        return NULL; /*no fit*/
}

static void place(void *bp, size_t asize){
        size_t csize = GET_SIZE(HDRP(bp));
        if ((csize - asize) >= (2*DSIZE)){ /* if remaining block after splitting would be at or above min block size, split*/
                PUT(HDRP(bp), PACK(asize, 1));
                PUT(FTRP(bp), PACK(asize, 1));
                bp = NEXT_BLKP(bp);
                PUT(HDRP(bp), PACK(csize-asize, 0));
                PUT(FTRP(bp), PACK(csize-asize, 0));}
        else{/*if less, don't split */
                PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));}
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
                return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE) , PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE) , PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE) , PACK(0, 1));
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
        size_t extendsize;
        char *bp;
        if (size == 0)/* if empty call made, ignore */
                return NULL;
        if (size <= DSIZE) /*if block less than or equal to double word, allocate enough for header/footer*/
                asize = 2*DSIZE;
        else /* allocate more if bigger*/
                asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
        if ((bp = find_fit(asize)) != NULL){ /*search free list for fit*/
                place(bp, asize);
                return bp;}
        extendsize = MAX(asize, CHUNKSIZE); /* if no fit found, extend list*/
        if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
                return NULL;
        place (bp, asize);
        return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        coalesce(bp); /*coalesces immediately*/
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
