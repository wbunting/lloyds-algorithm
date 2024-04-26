#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#include "gui.h"

#define APP_PLUG

// APP_PLUG int plug_handle_enter(struct app_memory *Memory) {
//   ASSERT(sizeof(struct app_state) <= Memory->PermanentStorageSize);
//   struct app_state *AppState = (struct app_state *)Memory->PermanentStorage;

//   return 1;
// }

APP_PLUG int plug_update(struct app_memory *Memory) {
  ASSERT(sizeof(struct app_state) <= Memory->PermanentStorageSize);

  struct app_state *AppState = (struct app_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    AppState->nodes_count = 3;
    for (int i = 0; i < AppState->nodes_count; i++) {
      AppState->nodes[i].id = i;
      AppState->nodes[i].x = GetRandomValue(0, GetScreenWidth());
      AppState->nodes[i].y = GetRandomValue(0, GetScreenHeight());
      AppState->nodes[i].vx = GetRandomValue(-5, 5);
      AppState->nodes[i].vy = GetRandomValue(-5, 5);
    }

    AppState->links[0].id = 0;
    AppState->links[0].from = 0;
    AppState->links[0].to = 1;

    AppState->links[1].id = 1;
    AppState->links[1].from = 1;
    AppState->links[1].to = 2;

    AppState->links_count = 2;

    Memory->IsInitialized = 1;
    printf("Initialized app state\n");
  }

  // // UPDATE
  // for (int i = 0; i < AppState->nodes_count; i++) {
  //   // loop through all the connected nodes
  //   for (int j = 0; j < AppState->links_count; j++) {
  //     if (AppState->links[j].from == i) {
  //       int to = AppState->links[j].to;
  //       float dx = AppState->nodes[to].x - AppState->nodes[i].x;
  //       float dy = AppState->nodes[to].y - AppState->nodes[i].y;
  //       float d = sqrt(dx * dx + dy * dy);
  //       float f = 0.1 * (d - 100);
  //       float fx = f * dx / (d * d);
  //       float fy = f * dy / (d * d);
  //       AppState->nodes[i].vx += fx / 100;
  //       AppState->nodes[i].vy += fy / 100;
  //     }
  //   }

  //   float provisional_x = AppState->nodes[i].x + AppState->nodes[i].vx;
  //   float provisional_y = AppState->nodes[i].y + AppState->nodes[i].vy;

  //   if (provisional_x < 0 || provisional_x > GetScreenWidth()) {
  //     AppState->nodes[i].vx = -AppState->nodes[i].vx;
  //   } else {
  //     AppState->nodes[i].x = provisional_x;
  //   }
  //   if (provisional_y < 0 || provisional_y > GetScreenHeight()) {
  //     AppState->nodes[i].vy = -AppState->nodes[i].vy;
  //   } else {
  //     AppState->nodes[i].y = provisional_y;
  //   }
  // }

  BeginDrawing();
  printf("drawing\n");
  ClearBackground(DARKGRAY);
  DrawText("Network!", GetScreenWidth() / 2 - 140, 20, 20, LIGHTGRAY);

  // for (int i = 0; i < AppState->nodes_count; i++) {
  //   DrawCircle(AppState->nodes[i].x, AppState->nodes[i].y, 10, RED);
  // }

  // for (int i = 0; i < AppState->links_count; i++) {
  //   DrawLine(AppState->nodes[AppState->links[i].from].x,
  //            AppState->nodes[AppState->links[i].from].y,
  //            AppState->nodes[AppState->links[i].to].x,
  //            AppState->nodes[AppState->links[i].to].y, BLUE);
  // }
  EndDrawing();

  return 1;
}
