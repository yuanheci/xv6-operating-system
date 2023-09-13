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

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct{
    struct spinlock lock;
    uint page_cnt[PHYSTOP / PGSIZE];  //该页面的用户页表数的“引用计数”
}kcow;


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kcow.lock, "kcow");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kcow.page_cnt[(uint64)p / PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kcow.lock);
  if(--kcow.page_cnt[(uint64)pa / PGSIZE] > 0) {
    release(&kcow.lock);
    return;  //不用真正释放
  }
  release(&kcow.lock);

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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&kcow.lock);
    kcow.page_cnt[(uint64)r / PGSIZE] = 1;
    release(&kcow.lock);
  }

  return (void*)r;
}

void opref(uint64 pa, int num){
    if(pa >= PHYSTOP){
        panic("opref: pa too big");
    }
    acquire(&kcow.lock);
    kcow.page_cnt[pa / PGSIZE] += num;
    release(&kcow.lock);
}



