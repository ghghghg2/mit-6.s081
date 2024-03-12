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

#define NBUCKETS (13)

struct {
  struct spinlock locks[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];
} bcache;

// Find and Remove unused LRU block in bucket[idx]
// If the block is found and removed, return its handle. 
// Otherwise, 0 is returned.
// Should be called with acquired lock of bucket[idx]
static struct buf* find_and_remove_lru(uint idx) {
  struct buf *lowestpb = 0;
  uint lowestTime = 0xFFFFFFFF;

  for(struct buf *pb = bcache.head[idx].next; pb != &bcache.head[idx]; pb = pb->next) {
    if (pb->refcnt == 0) {
      if (pb->lastUsedTime <= lowestTime) {
        lowestpb = pb;
        lowestTime = pb->lastUsedTime;
      }
    }
  }
  if (lowestpb) {
    // remove from the bucket so that no other thread can access it
    lowestpb->prev->next = lowestpb->next;
    lowestpb->next->prev = lowestpb->prev;
    lowestpb->next = 0;
    lowestpb->prev = 0;
  }

  return lowestpb;
}

static uint16 blockno2Idx(uint blockno) {
  return (blockno % NBUCKETS);
}

void
binit(void)
{
  struct buf *b;

  for (uint i = 0; i < NBUCKETS; i++) {
    initlock(&bcache.locks[i], "bcache");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  uint cnt = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    // Distribute the block to buckets evenly
    b->next = bcache.head[cnt % NBUCKETS].next;
    b->prev = &bcache.head[cnt % NBUCKETS];
    bcache.head[cnt % NBUCKETS].next->prev = b;
    bcache.head[cnt % NBUCKETS].next = b;
    cnt++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
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

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


