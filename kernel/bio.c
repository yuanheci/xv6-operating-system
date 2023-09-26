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

#define BUCKETS 13
extern uint ticks;

struct bucket{
    struct spinlock lock;
    struct buf head;
};

struct {
  struct buf buf[NBUF];
  struct bucket bkt[BUCKETS];
} bcache;

//头插法建立双向环形链表-tql
//其实用双向链表就可以，不用环形~
void
binit(void)
{
  struct buf *b;
  char bname[16] = {0};

  for(int i = 0; i < BUCKETS; i++) {
    snprintf(bname, 10, "bcache_%d", i);
    initlock(&bcache.bkt[i].lock, bname);
    bcache.bkt[i].head.prev = &bcache.bkt[i].head;
    bcache.bkt[i].head.next = &bcache.bkt[i].head;
  }

    // Create linked list of buffers，先把所有buffer放到bkt[0]里面
    for(b = bcache.buf; b < bcache.buf + NBUF; b++){
        b->next = bcache.bkt[0].head.next;
        b->prev = &bcache.bkt[0].head;
        initsleeplock(&b->lock, "buffer");
        bcache.bkt[0].head.next->prev = b;
        bcache.bkt[0].head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

//   acquire(&bcache.lock);
  int bid = blockno % BUCKETS;
  acquire(&bcache.bkt[bid].lock);

  // Is the block already cached?
  //先在当前hash表中查
  for(b = bcache.bkt[bid].head.next; b != &bcache.bkt[bid].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.bkt[bid].lock);

      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //从邻居hash表中去偷取buf，然后建立缓存
  int nextbid, flag = 0;
  struct buf *tmp;
  b = 0;
  for(int i = 0; i < BUCKETS; i++){  //从当前hash表开始查
    nextbid = (bid + i) % BUCKETS;
    if(nextbid != bid){   
        if(!holding(&bcache.bkt[nextbid].lock))  //防止死锁
            acquire(&bcache.bkt[nextbid].lock);  //获取邻居的锁
        else continue;   
    }
    for(tmp = bcache.bkt[nextbid].head.next; tmp != &bcache.bkt[nextbid].head; tmp = tmp->next){
        if(tmp->refcnt == 0){
            flag = 1;
            break;
        }
    }
    if(flag) break;
    //发现邻居没有可用buf后，得把邻居的锁释放掉！（卡了我一万年）
    if(nextbid != bid) release(&bcache.bkt[nextbid].lock);  
  }

  if(flag){  //可以偷到
    struct buf* tmp;
    //LRU(时间戳)找到这个hash表中最近最少使用的块
    for(tmp = bcache.bkt[nextbid].head.next; tmp != &bcache.bkt[nextbid].head; tmp = tmp->next){
        if(tmp->refcnt == 0 && (b == 0 || tmp->timestamp < b->timestamp)){
            b = tmp;
        }
    }
    //是从邻居那里偷来的
    if(nextbid != bid){
        //在nextbid这个hash表中删除
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.bkt[nextbid].lock);   //释放掉邻居的锁

        //在bid这个hash表中加入(头插法)
        b->next = bcache.bkt[bid].head.next;
        b->prev = &bcache.bkt[bid].head;
        bcache.bkt[bid].head.next->prev = b;
        bcache.bkt[bid].head.next = b;
    }

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;

    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);

    release(&bcache.bkt[bid].lock);
    acquiresleep(&b->lock);

    return b;
  }
  else{
    if(nextbid != bid) release(&bcache.bkt[nextbid].lock);
    release(&bcache.bkt[bid].lock);
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

  int bid = b->blockno % BUCKETS;

  releasesleep(&b->lock);

  acquire(&bcache.bkt[bid].lock);
  b->refcnt--;
  
  //更新时间戳
  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  
  release(&bcache.bkt[bid].lock);
}

void
bpin(struct buf *b) {
//   acquire(&bcache.lock);
  int bid = b->blockno % BUCKETS;
  acquire(&bcache.bkt[bid].lock);
  b->refcnt++;
  release(&bcache.bkt[bid].lock);
//   release(&bcache.lock);
}

void
bunpin(struct buf *b) {
//   acquire(&bcache.lock);
  int bid = b->blockno % BUCKETS;
  acquire(&bcache.bkt[bid].lock);
  b->refcnt--;
  release(&bcache.bkt[bid].lock);
//   release(&bcache.lock);
}


