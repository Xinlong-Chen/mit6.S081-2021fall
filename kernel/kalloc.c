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
void steal(int to);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

#define STEAL_NUM 32

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  if (NCPU <= 0) {
    panic("NCPU must larger than 0");
  }

  char* lock = "kmem";
  int n = NCPU, lock_name_size = strlen(lock) + 1;
  while (n /= 10) lock_name_size++;
  char lock_name[lock_name_size];

  for (int i = 0; i < NCPU; ++i) {
    snprintf(lock_name, lock_name_size, "%s%d",lock, i);
    initlock(&kmem[i].lock, lock_name);
    kmem[i].freelist = 0;
  }

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
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  push_off();

  int cid = cpuid();
  r = (struct run*)pa;
  acquire(&kmem[cid].lock);
  r->next = kmem[cid].freelist;
  kmem[cid].freelist = r;
  release(&kmem[cid].lock);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  
  int cid = cpuid();
  acquire(&kmem[cid].lock);
  if(kmem[cid].freelist == 0) steal(cid);
  
  r = kmem[cid].freelist; 
  if(r) {
    kmem[cid].freelist = r->next;
  }
  release(&kmem[cid].lock);

  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void steal(int to)
{
  struct run *r;
  int num = STEAL_NUM;
  for (int i = 0; i < NCPU; ++i) {
    if (i == to) continue;
    acquire(&kmem[i].lock);

    r = kmem[i].freelist;
    while (r && num--) {
      kmem[i].freelist = r->next;
      r->next = kmem[to].freelist;
      kmem[to].freelist = r;
      r = kmem[i].freelist;
    }
    if (num <= 0) {
      release(&kmem[i].lock);
      return;
    }
    release(&kmem[i].lock);
  }
  // printf("cannot steal enough page!\n");
}