#ifndef GUI_H
#define GUI_H

#define Kilobytes(x) ((x) * 1024LL)
#define Megabytes(x) (Kilobytes(x) * (1024LL))

#define u8 unsigned char
#define u32 unsigned int

#include <raylib.h>
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

typedef struct {
  Vector2 start;
  Vector2 end;
} Segment;

typedef struct {
  Vector2 vertices[10000];
  int num_vertices;

  Vector2 temp_vertices[10000];
  int temp_vertex_count;
} Cell;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Vector2 centroid;
  Color color;
} Vertex;

struct app_state {
  Vertex vertices[1000];
  Cell cells[1000];
  int num_vertices;
};

#endif
