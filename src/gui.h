#ifndef GUI_H
#define GUI_H

#define Kilobytes(x) ((x) * 1024LL)
#define Megabytes(x) (Kilobytes(x) * (1024LL))

#define u8 unsigned char
#define u32 unsigned int

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#if SLOW == 1
#define ASSERT(X)                                                              \
  if (!(X)) {                                                                  \
    *(int *)0 = 0;                                                             \
  }
#else
#define ASSERT(X)
#endif

#define MAX_NODES 1000
#define MAX_LINKS 100000

struct Node {
  int32 id;
  float x;
  float y;
  float vx;
  float vy;
};

struct Link {
  int32 id;
  int32 from;
  int32 to;
};

struct app_memory {
  bool32 IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage;

  uint64 TransientStorageSize;
  void *TransientStorage;
};

#define LIST_OF_PLUGS PLUG(plug_update, void *, struct app_memory *Memory)

#define PLUG(name, ret, ...) typedef ret(name##_t)(__VA_ARGS__);
LIST_OF_PLUGS
#undef PLUG

struct app_state {
  struct Node nodes[MAX_NODES];
  int nodes_count;

  struct Link links[MAX_LINKS];
  int links_count;
};

#endif
