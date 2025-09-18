#include "xvr_chunk.h"
#include "xvr_memory.h"

void Xvr_initChunk(Xvr_Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

void Xvr_pushChunk(Xvr_Chunk *chunk, uint8_t byte) {
  if (chunk->count + 1 > chunk->capacity) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = XVR_GROW_CAPACITY(oldCapacity);
    chunk->code =
        XVR_GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count++] = byte;
}

void Xvr_freeChunk(Xvr_Chunk *chunk) {
  XVR_FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  Xvr_initChunk(chunk);
}
