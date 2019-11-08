#include <mmu_heap.h>
#include <mmu_frames.h>
#include <mmu_pagging.h>
#include <kernel.h>
#include <memory.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <x86.h>
#include <isr.h>

mblock_t *root_mblock;

mblock_t *find_heap_block(uint32_t size) {
  DEBUG("MMU[heap]: Heap find_heap_block %i\n\r", size);
  HEAP_WALKER(mb) {
    if (mb->free && mb->size > size) { // not >= ... little hack to not match last block in a byte 
      return mb;
    }
  }
  return NULL;
}

void debug_heap_dump() {
  HEAP_WALKER(mb) {
    DEBUG("MMU[heap]: mb: 0x%X mag:%X siz:%i free:%i\n\r", mb, mb->magic, mb->size, mb->free);
  }
}

void sbrk(uint32_t size) {
  DEBUG("MMU[heap]: Sbrk called : %i\n\r", size);
  // Find last entry and m ap to it (we are never freeing pages)
  for (uint64_t i=0; i<512; i++) {
    if (pde[i].all == 0) {
      uint64_t frame = (uint64_t)alloc_frame();
      pde[i].all = (frame * PAGE_SIZE) | 0x83;
      DEBUG("MMU[heap]: Sbrk new frame : 0x%X (0x%X)\n\r", frame, frame * PAGE_SIZE);
      break;
    }
  }
  // x86_set_cr3(TO_PHYS_U64(pml4e));
  x86_tlb_flush_all();

  //Extend last memory block

  mblock_t *mb = root_mblock;
  while(mb->next != NULL) { mb = mb->next; }
  mb->size += PAGE_SIZE;
  heap_end += PAGE_SIZE;
  DEBUG("--------------------\n\r");
  debug_heap_dump();
  DEBUG("MMU[heap]: Heap after srbk 0x%X\n\r", TO_PHYS_U64(heap_end));
}

mblock_t *split_heap_block(mblock_t *mb, uint32_t size) {
  uint32_t old_size = mb->size;
  mblock_t *old_next = mb->next;
  uint8_t *ptr = (uint8_t*)mb;

  mb->free = false;
  mb->size = size;

  mblock_t *next_mb = (mblock_t*)(ptr + sizeof(mblock_t) + mb->size);
  mb->next = next_mb;
  next_mb->magic = MBLOCK_MAGIC;
  next_mb->size = old_size - size - sizeof(mblock_t);
  next_mb->next = old_next;
  next_mb->free = true;

  DEBUG("MMU[heap]: split prev: 0x%X mag:%X siz:%i free:%i\n\r", mb, mb->magic, mb->size, mb->free);
  DEBUG("MMU[heap]: split next: 0x%X mag:%X siz:%i free:%i\n\r", next_mb, next_mb->magic, next_mb->size, next_mb->free);

  return mb;
}

void *malloc(uint32_t size) {
  DEBUG("MMU[heap]: malloc %i\n\r", size);
  mblock_t *mb = find_heap_block(size);
  if (mb == NULL) {
    DEBUG("MMU[heap]: No more empty blocks ... sbrk-ing ...\n\r");
    sbrk(size);
    return malloc(size);
  } 
  else {
    DEBUG("MMU[heap]: split block  ...\n\r");
    mb = split_heap_block(mb, size);
  }
  return ((uint8_t*)mb) + sizeof(mblock_t);
}

void free(void *ptr) {
  mblock_t *mb = (mblock_t *)(((uint8_t*)ptr) - sizeof(mblock_t));
  if (mb->magic != MBLOCK_MAGIC) {
    HALT_AND_CATCH_FIRE("HEAP: free(0x%X) - freeing bad block.\n\r");
  }
  mb->free = true;
}

void init_mmu_heap() {
  root_mblock = (mblock_t*)heap_start;
  root_mblock->free = true;
  root_mblock->magic = MBLOCK_MAGIC;
  root_mblock->size = heap_end - heap_start - sizeof(mblock_t);
  root_mblock->next = NULL;
  DEBUG("MMU[heap]: Heap init : start:0x%X end:0x%X\n\r", heap_start, heap_end);
  DEBUG("MMU[heap]: Heap first block size : %i\n\r", root_mblock->size);
}
