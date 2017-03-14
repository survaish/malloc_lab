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

/* book macros*/
#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc)) 

#define GET(p)       (*(unsigned int *)(p)) 
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) 

#define GET_SIZE(p)  (GET(p) & ~0x7) 
#define GET_ALLOC(p) (GET(p) & 0x1) 

#define HDRP(ptr)       ((char *)(ptr) - WSIZE) 
#define FTRP(ptr)       ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE) 

#define NEXT_BLKP(ptr)  ((char *)(ptr) + GET_SIZE(((char *)(ptr) - WSIZE))) 
#define PREV_BLKP(ptr)  ((char *)(ptr) - GET_SIZE(((char *)(ptr) - DSIZE)))

//Other Macros//

// Puts pointers in the next and previous elements of free list
#define SET_NEXT_PTR(bp, p) (GET_NEXT_PTR(bp) = p)
#define SET_PREV_PTR(bp, p) (GET_PREV_PTR(bp) = p)

// Given a pointer in a free list, get next and previous pointer
#define GET_NEXT_PTR(bp)  (*(char **)(bp + WSIZE))
#define GET_PREV_PTR(bp)  (*(char **)(bp))



//HELPER FUNCTIONS - WOO//

static void *extend_heap(size_t words){
        char *bp;
        size_t size;
        size = (words % 2) ? (words+1)*WSIZE : words * WSIZE; /*Allocate even # of words for alignment*/
        if (size < 16)
        {
            size = 16;
        }

        if ((long)(bp = mem_sbrk(size)) == -1)
                return NULL;
        PUT(HDRP(bp), PACK(size,0)); /*Initialize free block header*/
        PUT(FTRP(bp), PACK(size,0));/*Initialize free block footer*/
        PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); /*new epilogue header*/
        return coalesce(bp); /*coalesce if previous block was free*/
}

static void *coalesce(void *bp){
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))) || PREV_BLK(blockp) == blockp;
        size_t size = GET_SIZE(HDRP(bp));
        if (prev_alloc && next_alloc){ /* if both allocated, do nothing */
                return bp;}
        else if (prev_alloc && !next_alloc){ /* next block free condition*/
                size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
                freelistremove(NEXT_BLK(blockp));
                PUT(HDRP(bp), PACK(size, 0));
                PUT(FTRP(bp), PACK(size,0));}
        else if (!prev_alloc && next_alloc){ /*previous block free condition */
                size += GET_SIZE(HDRP(PREV_BLKP(bp)));
                PUT(FTRP(bp), PACK(size, 0));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                bp = PREV_BLKP(bp);
                freelistremove(blockp);}
        else if (!prev_alloc && !next_alloc)
                {
                    size += GET_SIZE( HDRP(PREV_BLK(blockp))) + GET_SIZE(HDRP(NEXT_BLK(blockp)));
                    freelistremove(PREV_BLK(blockp));
                    freelistremove(NEXT_BLK(blockp));
                    blockp = PREV_BLK(blockp);
                    PUT(HDRP(blockp), PACK(size, 0));
                    PUT(FTRP(blockp), PACK(size, 0));
                }
        return bp;
}


static void *find_fit(size_t asize){
    void *bp;
    static int prevsize = 0;
    static int counter = 0;
    if(prevsize == (int)asize)
    {
      if(counter>30)
      {  
        int extendsize = MAX(asize, 2*DSIZE);
        bp = extend_heap(extendsize / 4);
        return bp;
      }
      else
        counter++;
  }
  else
    counter = 0;
  for (bp = freelistbegin; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT_PTR(bp))
  {
    if (asize <= (size_t)GET_SIZE(HDRP(bp)))
    {
      prevsize = asize;
      return bp;
    }
  }
  return NULL;
}


static void place(void *bp, size_t asize){
  size_t size2 = GET_SIZE(HDRP(bp));

  if ((size2 - asize) >= 2 * DSIZE)
  {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    freelistremove(bp);
    bp = NEXT_BLK(bp);
    PUT(HDRP(bp), PACK(size2-asize, 0));
    PUT(FTRP(bp), PACK(size2-asize, 0));
    coalesce(bp);
  }
  else
  {
    PUT(HDRP(bp), PACK(size2, 1));
    PUT(FTRP(bp), PACK(size2, 1));
    freelistremove(bp);
  }
}


// adds a free block pointer to the free list
static void freelistadd(void *bp){
  SET_NEXT_PTR(bp, freelistbegin);
  SET_PREV_PTR(freelistbegin, bp);
  SET_PREV_PTR(bp, NULL);
  freelistbegin = bp;
}


// removes a free block pointer from the free list
static void freelistremove(void *bp){
  if (GET_PREV_PTR(bp))
  {
    SET_NEXT_PTR(GET_PREV_PTR(bp), GET_NEXT_PTR(bp));
  }
  else
    freelistbegin = GET_NEXT_PTR(bp);
  SET_PREV_PTR(GET_NEXT_PTR(bp), GET_PREV_PTR(bp));
}

static char *heaplist = 0; 
static char *freelistbegin = 0;

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
    if (bp == NULL)
    {
        return;
    }
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp); /*coalesces immediately*/
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    if((int)size < 0)
    {
        return NULL;
    }
    else if((int)size == 0)
    {
        mm_free(bp);
        return NULL;
    }
    else if(size > 0)
    {
      size_t oldsize = GET_SIZE(HDRP(bp));
      size_t newsize = (size + DSIZE);
      if(newsize <= oldsize)
      {
          return bp;
      }
      else {
          size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
          size_t csize;
          if(!next_alloc && ((csize = oldsize + GET_SIZE(HDRP(NEXT_BLK(bp))))) >= newsize)
          {
            freelistremove(NEXT_BLK(bp));
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
            return bp;
          }
          else {
            void *new_ptr = mm_malloc(newsize);
            place(new_ptr, newsize);
            memcpy(new_ptr, bp, newsize);
            mm_free(bp);
            return new_ptr;
          }
      }
  }
  else
    return NULL;
}
