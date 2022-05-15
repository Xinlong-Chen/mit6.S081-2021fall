// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock[NBUCKET];
  struct buf hash_table[NBUCKET][NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

uint hash(uint dev, uint blockno) {
  return (dev + blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache");
  }

  for (int i = 0; i < NBUCKET; i++) {
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
    for(b = bcache.hash_table[i]; b < bcache.hash_table[i]+NBUF; b++){
      b->next = bcache.head[i].next;
      b->prev = &bcache.head[i];
      initsleeplock(&b->lock, "buffer");
      bcache.head[i].next->prev = b;
      bcache.head[i].next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint hash_index = hash(dev, blockno);
  acquire(&bcache.lock[hash_index]);

  // Is the block already cached?
  for(b = bcache.head[hash_index].next; b != &bcache.head[hash_index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash_index]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head[hash_index].prev; b != &bcache.head[hash_index]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[hash_index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint hash_index = hash(b->dev, b->blockno);
  acquire(&bcache.lock[hash_index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[hash_index].next;
    b->prev = &bcache.head[hash_index];
    bcache.head[hash_index].next->prev = b;
    bcache.head[hash_index].next = b;
  }
  
  release(&bcache.lock[hash_index]);
}

void
bpin(struct buf *b) {
  uint hash_index = hash(b->dev, b->blockno);
  acquire(&bcache.lock[hash_index]);
  b->refcnt++;
  release(&bcache.lock[hash_index]);
}

void
bunpin(struct buf *b) {
  uint hash_index = hash(b->dev, b->blockno);
  acquire(&bcache.lock[hash_index]);
  b->refcnt--;
  release(&bcache.lock[hash_index]);
}


