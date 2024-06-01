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

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define FLT_MAX __FLT_MAX__
#define MAX_EDGES 1000
#define MAX_EVENTS 1000
#define MAX_BEACHLINE_ITEMS 10000

typedef enum BeachlineItemType { NoneBeachline, Arc, Edge } BeachlineItemType;

typedef struct EdgeStruct {
  Vector2 start;
  Vector2 direction;
  int extendsUpwardsForever;
} EdgeStruct;

typedef struct ArcStruct {
  Vector2 focus;
  struct SweepEvent *squeezeEvent;
} ArcStruct;

typedef struct BeachlineItem {
  BeachlineItemType type;
  union {
    ArcStruct arc;
    EdgeStruct edge;
  };
  struct BeachlineItem *parent;
  struct BeachlineItem *left;
  struct BeachlineItem *right;
} BeachlineItem;

typedef enum SweepEventType {
  NoneSweep,
  NewPoint,
  EdgeIntersection
} SweepEventType;

typedef struct NewPointEvent {
  Vector2 point;
} NewPointEvent;

typedef struct EdgeIntersectionEvent {
  Vector2 intersectionPoint;
  BeachlineItem *squeezedArc;
  int isValid;
} EdgeIntersectionEvent;

typedef struct SweepEvent {
  float yCoord;
  SweepEventType type;
  union {
    NewPointEvent newPoint;
    EdgeIntersectionEvent edgeIntersect;
  };
} SweepEvent;

typedef struct CompleteEdge {
  Vector2 endpointA;
  Vector2 endpointB;
  int vertices[2];
} CompleteEdge;

typedef struct {
  float sweepY;
  CompleteEdge edges[MAX_EDGES];
  int edgesSize;
  SweepEvent *unencounteredEvents[MAX_EVENTS];
  int unencounteredEventsSize;
  SweepEvent events[MAX_EVENTS];
  int eventsSize; // Current number of events
  BeachlineItem beachlineItems[MAX_BEACHLINE_ITEMS];
  int beachlineItemCount;
} FortuneState;

struct app_state {
  Vertex vertices[1000];
  int num_vertices;
  FortuneState fortuneState;
  Shader glowShader;

  int mouse_x;
  int mouse_y;

  Vector2 curvePts[1920];
  Cell cells[1000];
};

#endif
