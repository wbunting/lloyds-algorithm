#include "raylib.h"

#include "./hotreload.h"
#include <stdio.h>

struct app_memory m;

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

void GameFrame(void) { plug_update(&m); }

typedef unsigned long size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MEMORY_SIZE (128 * 1024 * 1024)
static unsigned char memory_pool[MEMORY_SIZE];
static size_t current_offset = 0;
void *custom_alloc(size_t size) {
  if (current_offset + size > MEMORY_SIZE) {
    return NULL; // Out of memory
  }
  void *block = &memory_pool[current_offset];
  current_offset += size;
  return block;
}

void custom_free_to(size_t offset) { current_offset = offset; }

void custom_reset(void) { current_offset = 0; }

int main(void) {
  uint64_t total_size;

  m.PermanentStorageSize = Megabytes(64);
  m.TransientStorageSize = Megabytes(64);
  total_size = m.PermanentStorageSize + m.TransientStorageSize;
  m.PermanentStorage = custom_alloc(total_size);
  if (m.PermanentStorage == NULL) {
    printf("Failed to allocate memory");
  }
  m.TransientStorage = (uint8_t *)m.PermanentStorage + m.TransientStorageSize;
  m.IsInitialized = 0;

  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

  SetTargetFPS(60);

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(GameFrame, 0, 1);
#endif

  return 0;
}
