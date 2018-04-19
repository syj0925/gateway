/********************************************************************************
**
** 文件名:     mem.c
** 版权所有:   
** 文件描述:   实现定时器驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#include "sys_typedef.h"
#include <string.h>
#include "mem.h"

#define MEM_STATS   0

/* lwIP replacement for your libc malloc() */

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_STRUCT_MEM, which automatically alignes.
 */
struct mem {
  /** index (-> ram[next]) of the next struct */
  mem_size_t next;
  /** index (-> ram[next]) of the next struct */
  mem_size_t prev;
  /** 1: this area is used; 0: this area is unused */
  INT8U used;
};




/** All allocated blocks will be MIN_SIZE bytes big, at least!
 * MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef MIN_SIZE
#define MIN_SIZE             12
#endif /* MIN_SIZE */
/* some alignment macros: we define them here for better source code layout */
#define MIN_SIZE_ALIGNED     OS_MEM_ALIGN_SIZE(MIN_SIZE)
#define SIZEOF_STRUCT_MEM    OS_MEM_ALIGN_SIZE(sizeof(struct mem))
#define MEM_SIZE_ALIGNED     OS_MEM_ALIGN_SIZE(MEM_SIZE)

/** the heap. we need one struct mem at the end and some room for alignment */
static INT8U ram_heap[MEM_SIZE_ALIGNED + (2*SIZEOF_STRUCT_MEM) + MEM_ALIGNMENT];
/** pointer to the heap (ram_heap): for alignment, ram is now a pointer instead of an array */
static INT8U *ram;
/** the last entry, always unused! */
static struct mem *ram_end;
/** pointer to the lowest free block, this is used for faster search */
static struct mem *lfree;
/** concurrent access protection */
//static sys_sem_t mem_sem;

/*******************************************************************
** 函数名:     mem_malloc
** 函数描述:   分配动态内存
** 参数:       [in] size:   分配尺寸     
** 返回:       动态内存首地址
** 注意:       
********************************************************************/
static void plug_holes(struct mem *mem)
{
  struct mem *nmem;
  struct mem *pmem;

  OS_ASSERT("plug_holes: mem >= ram", (INT8U *)mem >= ram);
  OS_ASSERT("plug_holes: mem < ram_end", (INT8U *)mem < (INT8U *)ram_end);
  OS_ASSERT("plug_holes: mem->used == 0", mem->used == 0);

  /* plug hole forward */
  OS_ASSERT("plug_holes: mem->next <= MEM_SIZE_ALIGNED", mem->next <= MEM_SIZE_ALIGNED);

  nmem = (struct mem *)&ram[mem->next];
  if (mem != nmem && nmem->used == 0 && (INT8U *)nmem != (INT8U *)ram_end) {
    /* if mem->next is unused and not end of ram, combine mem and mem->next */
    if (lfree == nmem) {
      lfree = mem;
    }
    mem->next = nmem->next;
    ((struct mem *)&ram[nmem->next])->prev = (INT8U *)mem - ram;
  }

  /* plug hole backward */
  pmem = (struct mem *)&ram[mem->prev];
  if (pmem != mem && pmem->used == 0) {
    /* if mem->prev is unused, combine mem and mem->prev */
    if (lfree == mem) {
      lfree = pmem;
    }
    pmem->next = mem->next;
    ((struct mem *)&ram[mem->next])->prev = (INT8U *)pmem - ram;
  }
}

/*******************************************************************
** 函数名:     mem_init
** 函数描述:   初始化动态内存
** 参数:       无     
** 返回:       无
** 注意:       
********************************************************************/
void mem_init(void)
{
  struct mem *mem;

  OS_ASSERT("Sanity check alignment",
    (SIZEOF_STRUCT_MEM & (MEM_ALIGNMENT-1)) == 0);

  /* align the heap */
  ram = OS_MEM_ALIGN(ram_heap);
  /* initialize the start of the heap */
  mem = (struct mem *)ram;
  mem->next = MEM_SIZE_ALIGNED;
  mem->prev = 0;
  mem->used = 0;
  /* initialize the end of the heap */
  ram_end = (struct mem *)&ram[MEM_SIZE_ALIGNED];
  ram_end->used = 1;
  ram_end->next = MEM_SIZE_ALIGNED;
  ram_end->prev = MEM_SIZE_ALIGNED;

  //mem_sem = sys_sem_new(1);

  /* initialize the lowest-free pointer to the start of the heap */
  lfree = (struct mem *)ram;

#if MEM_STATS
  lwip_stats.mem.avail = MEM_SIZE_ALIGNED;
#endif /* MEM_STATS */
}

/*******************************************************************
** 函数名:     mem_malloc
** 函数描述:   分配动态内存
** 参数:       [in] size:   分配尺寸     
** 返回:       动态内存首地址
** 注意:       
********************************************************************/
void mem_free(void *rmem)
{
  struct mem *mem;

  if (rmem == NULL) {
    //OS_DEBUGF(MEM_DEBUG | OS_DBG_TRACE | 2, ("mem_free(p == NULL) was called.\n"));
    return;
  }
  OS_ASSERT("mem_free: sanity check alignment", (((mem_ptr_t)rmem) & (MEM_ALIGNMENT-1)) == 0);

  /* protect the heap from concurrent access */
  //sys_arch_sem_wait(mem_sem, 0);

  OS_ASSERT("mem_free: legal memory", (INT8U *)rmem >= (INT8U *)ram &&
    (INT8U *)rmem < (INT8U *)ram_end);

  if ((INT8U *)rmem < (INT8U *)ram || (INT8U *)rmem >= (INT8U *)ram_end) {
    //OS_DEBUGF(MEM_DEBUG | 3, ("mem_free: illegal memory\n"));
#if MEM_STATS
    ++OS_stats.mem.err;
#endif /* MEM_STATS */
    //sys_sem_signal(mem_sem);
    return;
  }
  /* Get the corresponding struct mem ... */
  mem = (struct mem *)((INT8U *)rmem - SIZEOF_STRUCT_MEM);
  /* ... which has to be in a used state ... */
  OS_ASSERT("mem_free: mem->used", mem->used);
  /* ... and is now unused. */
  mem->used = 0;

  if (mem < lfree) {
    /* the newly freed struct is now the lowest */
    lfree = mem;
  }

#if MEM_STATS
  OS_stats.mem.used -= mem->next - ((INT8U *)mem - ram);
#endif /* MEM_STATS */

  /* finally, see if prev or next are free also */
  plug_holes(mem);
  //sys_sem_signal(mem_sem);
}

#if 1
/**
 * In contrast to its name, mem_realloc can only shrink memory, not expand it.
 * Since the only use (for now) is in pbuf_realloc (which also can only shrink),
 * this shouldn't be a problem!
 *
 * @param rmem pointer to memory allocated by mem_malloc the is to be shrinked
 * @param newsize required size after shrinking (needs to be smaller than or
 *                equal to the previous size)
 * @return for compatibility reasons: is always == rmem, at the moment
 */
void *mem_realloc(void *rmem, mem_size_t newsize)
{
  mem_size_t size;
  mem_size_t ptr, ptr2;
  struct mem *mem, *mem2;

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
  newsize = OS_MEM_ALIGN_SIZE(newsize);

  if(newsize < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    newsize = MIN_SIZE_ALIGNED;
  }

  if (newsize > MEM_SIZE_ALIGNED) {
    return NULL;
  }

  OS_ASSERT("mem_realloc: legal memory", (INT8U *)rmem >= (INT8U *)ram &&
   (INT8U *)rmem < (INT8U *)ram_end);

  if ((INT8U *)rmem < (INT8U *)ram || (INT8U *)rmem >= (INT8U *)ram_end) {
    //OS_DEBUGF(MEM_DEBUG | 3, ("mem_realloc: illegal memory\n"));
    return rmem;
  }
  /* Get the corresponding struct mem ... */
  mem = (struct mem *)((INT8U *)rmem - SIZEOF_STRUCT_MEM);
  /* ... and its offset pointer */
  ptr = (INT8U *)mem - ram;

  size = mem->next - ptr - SIZEOF_STRUCT_MEM;
  OS_ASSERT("mem_realloc can only shrink memory", newsize <= size);
  if (newsize > size) {
    /* not supported */
    return NULL;
  }
  if (newsize == size) {
    /* No change in size, simply return */
    return rmem;
  }

  /* protect the heap from concurrent access */
  //sys_arch_sem_wait(mem_sem, 0);

#if MEM_STATS
  lwip_stats.mem.used -= (size - newsize);
#endif /* MEM_STATS */

  mem2 = (struct mem *)&ram[mem->next];
  if(mem2->used == 0) {
    /* The next struct is unused, we can simply move it at little */
    mem_size_t next;
    /* remember the old next pointer */
    next = mem2->next;
    /* create new struct mem which is moved directly after the shrinked mem */
    ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
    if (lfree == mem2) {
      lfree = (struct mem *)&ram[ptr2];
    }
    mem2 = (struct mem *)&ram[ptr2];
    mem2->used = 0;
    /* restore the next pointer */
    mem2->next = next;
    /* link it back to mem */
    mem2->prev = ptr;
    /* link mem to it */
    mem->next = ptr2;
    /* last thing to restore linked list: as we have moved mem2,
     * let 'mem2->next->prev' point to mem2 again. but only if mem2->next is not
     * the end of the heap */
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)&ram[mem2->next])->prev = ptr2;
    }
    /* no need to plug holes, we've already done that */
  } else if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED <= size) {
    /* Next struct is used but there's room for another struct mem with
     * at least MIN_SIZE_ALIGNED of data.
     * Old size ('size') must be big enough to contain at least 'newsize' plus a struct mem
     * ('SIZEOF_STRUCT_MEM') with some data ('MIN_SIZE_ALIGNED').
     * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
     *       region that couldn't hold data, but when mem->next gets freed,
     *       the 2 regions would be combined, resulting in more free memory */
    ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
    mem2 = (struct mem *)&ram[ptr2];
    if (mem2 < lfree) {
      lfree = mem2;
    }
    mem2->used = 0;
    mem2->next = mem->next;
    mem2->prev = ptr;
    mem->next = ptr2;
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)&ram[mem2->next])->prev = ptr2;
    }
    /* the original mem->next is used, so no need to plug holes! */
  }
  /* else {
    next struct mem is used but size between mem and mem2 is not big enough
    to create another struct mem
    -> don't do anyhting. 
    -> the remaining space stays unused since it is too small
  } */
  //sys_sem_signal(mem_sem);
  return rmem;
}
#endif
/*******************************************************************
** 函数名:     mem_malloc
** 函数描述:   分配动态内存
** 参数:       [in] size:   分配尺寸     
** 返回:       动态内存首地址
** 注意:       
********************************************************************/
void *mem_malloc(mem_size_t size)
{
  mem_size_t ptr, ptr2;
  struct mem *mem, *mem2;

  if (size == 0) {
    return NULL;
  }

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
  size = OS_MEM_ALIGN_SIZE(size);

  if(size < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    size = MIN_SIZE_ALIGNED;
  }

  if (size > MEM_SIZE_ALIGNED) {
    return NULL;
  }

  /* protect the heap from concurrent access */
  //sys_arch_sem_wait(mem_sem, 0);


  /* Scan through the heap searching for a free block that is big enough,
   * beginning with the lowest free block.
   */
  for (ptr = (INT8U *)lfree - ram; ptr < MEM_SIZE_ALIGNED - size;
       ptr = ((struct mem *)&ram[ptr])->next) {
    mem = (struct mem *)&ram[ptr];

    if ((!mem->used) &&
        (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size) {
      /* mem is not used and at least perfect fit is possible:
       * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

      if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED)) {
        /* (in addition to the above, we test if another struct mem (SIZEOF_STRUCT_MEM) containing
         * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
         * -> split large block, create empty remainder,
         * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
         * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
         * struct mem would fit in but no data between mem2 and mem2->next
         * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
         *       region that couldn't hold data, but when mem->next gets freed,
         *       the 2 regions would be combined, resulting in more free memory
         */
        ptr2 = ptr + SIZEOF_STRUCT_MEM + size;
        /* create mem2 struct */
        mem2 = (struct mem *)&ram[ptr2];
        mem2->used = 0;
        mem2->next = mem->next;
        mem2->prev = ptr;
        /* and insert it between mem and mem->next */
        mem->next = ptr2;
        mem->used = 1;

        if (mem2->next != MEM_SIZE_ALIGNED) {
          ((struct mem *)&ram[mem2->next])->prev = ptr2;
        }
#if MEM_STATS
        lwip_stats.mem.used += (size + SIZEOF_STRUCT_MEM);
        if (lwip_stats.mem.max < lwip_stats.mem.used) {
          lwip_stats.mem.max = lwip_stats.mem.used;
        }
#endif /* MEM_STATS */
      } else {
        /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
         * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
         * take care of this).
         * -> near fit or excact fit: do not split, no mem2 creation
         * also can't move mem->next directly behind mem, since mem->next
         * will always be used at this point!
         */
        mem->used = 1;
#if MEM_STATS
        lwip_stats.mem.used += mem->next - ((u8_t *)mem - ram);
        if (lwip_stats.mem.max < lwip_stats.mem.used) {
          lwip_stats.mem.max = lwip_stats.mem.used;
        }
#endif /* MEM_STATS */
      }

      if (mem == lfree) {
        /* Find next free block after mem and update lowest free pointer */
        while (lfree->used && lfree != ram_end) {
          lfree = (struct mem *)&ram[lfree->next];
        }
        OS_ASSERT("mem_malloc: !lfree->used", ((lfree == ram_end) || (!lfree->used)));
      }
      //sys_sem_signal(mem_sem);
      OS_ASSERT("mem_malloc: allocated memory not above ram_end.",
       (mem_ptr_t)mem + SIZEOF_STRUCT_MEM + size <= (mem_ptr_t)ram_end);
      OS_ASSERT("mem_malloc: allocated memory properly aligned.",
       (unsigned long)((u8_t *)mem + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT == 0);
      OS_ASSERT("mem_malloc: sanity check alignment",
        (((mem_ptr_t)mem) & (MEM_ALIGNMENT-1)) == 0);

      return (INT8U *)mem + SIZEOF_STRUCT_MEM;
    }
  }
  //OS_DEBUGF(MEM_DEBUG | 2, ("mem_malloc: could not allocate %"S16_F" bytes\n", (s16_t)size));
#if MEM_STATS
  ++lwip_stats.mem.err;
#endif /* MEM_STATS */
  //sys_sem_signal(mem_sem);
  return NULL;
}

/**
 * Contiguously allocates enough space for count objects that are size bytes
 * of memory each and returns a pointer to the allocated memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *mem_calloc(mem_size_t count, mem_size_t size)
{
  void *p;

  /* allocate 'count' objects of size 'size' */
  p = mem_malloc(count * size);
  if (p) {
    /* zero the memory */
    memset(p, 0, count * size);
  }
  return p;
}

char *mem_strdup(const char *s)
{
    char *p;
    mem_size_t size;
        
    if (s == NULL) {
        return NULL;
    }

    size = strlen(s);
    p = mem_malloc(size);

    if (p == NULL) {
        return NULL;
    }
    memcpy(p, s, size);
    return p;
}

#if 0
/*******************************************************************
** 函数名:     pbuf_alloc
** 函数描述:   分配包缓存动态内存
** 参数:       [in] size:       目前要用的大小 
** 参数:       [in] freesize:   预留部分的大小
** 返回:       动态包内存首地址
** 注意:       
********************************************************************/
PACKET_BUF_T *pbuf_alloc(mem_size_t size, mem_size_t freesize)
{
    void *p;
    PACKET_BUF_T *pbuf;

    p = mem_malloc(size + freesize + sizeof(PACKET_BUF_T));/*PACKET_BUF_T占16byte*/
    if (p == NULL) {
        RS232_Printf("包缓存分配失败!!!\r\n");
        return NULL;
    }
    pbuf = (PACKET_BUF_T*)p;
    
    pbuf->curptr = (INT8U*)p + freesize + sizeof(PACKET_BUF_T);
    pbuf->free_len = freesize;
    pbuf->used_len = size;
    pbuf->tot_len = size + freesize;
    pbuf->next = NULL;
    
    return pbuf;
}

/*******************************************************************
** 函数名:     pbuf_move
** 函数描述:   移动动态内存包的数据指针
** 参数:       [in] pbuf:   要操作的动态内存包的指针 
** 参数:       [in] size:   移动的长度
** 参数:       [in] dir:    移动的方向
** 返回:       true or false
** 注意:       
********************************************************************/
BOOLEAN pbuf_move(PACKET_BUF_T *pbuf, mem_size_t size, BOOLEAN dir)
{

    if (pbuf == NULL) {
        return false;
    }
    if (dir == MEM_MOVE_UP) {
        if (pbuf->free_len < size) {
            return false;
        } else {
            pbuf->free_len -= size;
            pbuf->used_len += size;
            pbuf->curptr = (INT8U*)(pbuf->curptr) - size;
        }
    } else if (dir == MEM_MOVE_DOWN) {
        if (pbuf->used_len < size) {
            return false;
        } else {
            pbuf->free_len += size;
            pbuf->used_len -= size;
            pbuf->curptr = (INT8U*)(pbuf->curptr) + size;
        }
    }
    return true;
}
#endif /* if 0. 2014-10-18 10:55:00 suyoujiang */

