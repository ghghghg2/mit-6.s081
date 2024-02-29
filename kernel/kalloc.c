// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

struct page_info {
  uint8 refCnt;
};

// Declare a map stores information of each page.
static struct page_info mem_map[(PHYSTOP - KERNBASE) >> PGSHIFT]; 

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

int incr_pageRefCnt(uint64 pa)
{
  int tmp;
  if ((pa >= PHYSTOP) || (pa < (uint64)end)) {
    return -1;
  }
  acquire(&kmem.lock);
  mem_map[IDX_MEM_MAP(pa)].refCnt++;
  tmp = mem_map[IDX_MEM_MAP(pa)].refCnt;
  release(&kmem.lock);
  return tmp;
}

int decr_pageRefCnt(uint64 pa)
{
  int tmp;
  if ((pa >= PHYSTOP) || (pa < (uint64)end)) {
    return -1;
  }
  acquire(&kmem.lock);
  tmp = mem_map[IDX_MEM_MAP(pa)].refCnt;
  if (tmp > 0) {
    mem_map[IDX_MEM_MAP(pa)].refCnt--;
    tmp--;
  }
  release(&kmem.lock);
  return tmp;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  int tmp;
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  if ((tmp = decr_pageRefCnt((uint64)pa)) > 0) {
    // There is other process still ref to this page, 
    // so do not free it.
    return;
  } else if (tmp < 0){
    panic("kfree: get_pageRefCnt");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    mem_map[IDX_MEM_MAP((uint64)r)].refCnt = 1;
  }
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
    
  return (void*)r;
}
