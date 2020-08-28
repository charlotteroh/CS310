#include <stdio.h>  // needed for size_t etc.
#include <unistd.h> // needed for sbrk etc.
#include <sys/mman.h> // needed for mmap
#include <assert.h> // needed for asserts
#include "dmm.h"

/*
 * The lab handout and code guide you to a solution with a single free list containing all free
 * blocks (and only the blocks that are free) sorted by starting address.  Every block (allocated
 * or free) has a header (type metadata_t) with list pointers, but no footers are defined.
 * That solution is "simple" but inefficient.  You can improve it using the concepts from the
 * reading.
 */

/*
 *size_t is the return type of the sizeof operator.   size_t type is large enough to represent
 * the size of the largest possible object (equivalently, the maximum virtual address).
 */

typedef struct metadata {
  size_t size;
  struct metadata* next;
  struct metadata* prev;
} metadata_t;

/* Head of the freelist: pointer to the header of the first free block. */
static metadata_t* freelist = NULL;

void* dmalloc(size_t numbytes) {

  if(freelist == NULL) {
    if(!dmalloc_init())
      return NULL;
  }

  assert(numbytes > 0);

  /* your code here */
	metadata_t* temp = freelist;
  void* allocate;

	// loop through heap to find big space to fit memory
  // numbytes is big enough & is empty
	while (ALIGN(numbytes) > temp->size - METADATA_T_ALIGNED || temp->size & 1) {
    temp = temp->next;
    // edge case
		if (temp == NULL) {
      return NULL;
    }
  }

  // split if find appropriate location
  if (temp->size >= ALIGN(numbytes) + METADATA_T_ALIGNED * 2) {
    metadata_t *block = temp;
    allocate = (void *) temp;

    // next block starts at:
    allocate = allocate + ALIGN(numbytes) + METADATA_T_ALIGNED;
    temp = (metadata_t *) allocate;
    temp->size = block->size - (ALIGN(numbytes) + METADATA_T_ALIGNED);
    temp->next = block->next;

    // edge case
    if (temp->next != NULL) {
      temp->next->prev = temp;
    }

    // update pointers
    temp->prev = block;
    block->next = temp;
    block->size = ALIGN(numbytes) + METADATA_T_ALIGNED;

    // return allocation
    block->size += 1;
    allocate = (char *) block;
    allocate += METADATA_T_ALIGNED;
  }
  // leave as it is
  else {
    // update pointers
    temp->size += 1;
    allocate = (void *) temp;
    allocate += METADATA_T_ALIGNED;
  }
	return allocate;
}

void merge(metadata_t* lefty) {
  metadata_t *righty = lefty->next;
  lefty->size = lefty->size + righty->size;
  lefty->next = righty->next;
  if (lefty->next != NULL) {
    lefty->next->prev = lefty;
  }
}

void coalesce(void* ptr) {
  metadata_t *cur = ptr;
  metadata_t *prev = cur->prev;
  metadata_t *next = cur->next;

  bool shouldMergeLeft = prev != NULL && !(prev->size & 1);
  bool shouldMergeRight = next != NULL && !(next->size & 1);

  if(shouldMergeLeft && !shouldMergeRight) {
    merge(prev);
  }

  if(shouldMergeLeft && shouldMergeRight) {
    merge(prev);
    merge(prev);
  }

  if(!shouldMergeLeft && shouldMergeRight) {
    merge(cur);
  }
}

void dfree(void* ptr) {
  /* your code here */
  assert(ptr > 0);

  // move pointer
  ptr -= METADATA_T_ALIGNED;
  metadata_t* temp = (metadata_t*) ptr;

  // is not empty, free block & coalesce
  if (temp->size & 1) {
    temp->size -= 1;
    //printf("temp->size:%p\n", temp);
    // coalesce
    coalesce(ptr);
  }
}

/*
 * Allocate heap_region slab with a suitable syscall. Treat it as one large free block on freelist.
 */
bool dmalloc_init() {

  size_t max_bytes = ALIGN(MAX_HEAP_SIZE);

  //  freelist = (metadata_t*) sbrk(max_bytes);
  freelist = (metadata_t*)
     mmap(NULL, max_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (freelist == (void *)-1)
    return false;
  freelist->next = NULL;
  freelist->prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}


/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p Used:%u\t",
	  freelist_head->size,
	  freelist_head,
	  freelist_head->prev,
	  freelist_head->next,
    freelist_head->size&1);
    freelist_head = freelist_head->next;
  }
  DEBUG("\n");
}
