#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#include "gui.h"

#define APP_PLUG

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

bool IsCounterclockwise(Vector2 p1, Vector2 p2, Vector2 p3) {
  float area = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
  return (area > 0);
}

float calculateArea(const Cell *cell) {
  float area = 0.0f;
  for (int i = 0, j = cell->num_vertices - 1; i < cell->num_vertices; j = i++) {
    area += (cell->vertices[j].x + cell->vertices[i].x) *
            (cell->vertices[j].y - cell->vertices[i].y);
  }
  return area / 2.0f;
}

Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
  Vector2 result = {v1.x + v2.x, v1.y + v2.y};

  return result;
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
  Vector2 result = {v1.x - v2.x, v1.y - v2.y};

  return result;
}

Vector2 Vector2Scale(Vector2 v, float scale) {
  Vector2 result = {v.x * scale, v.y * scale};

  return result;
}

Vector2 Vector2Normalize(Vector2 v) {
  Vector2 result = {0};
  float length = sqrtf((v.x * v.x) + (v.y * v.y));

  if (length > 0) {
    float ilength = 1.0f / length;
    result.x = v.x * ilength;
    result.y = v.y * ilength;
  }

  return result;
}

Vector2 calculateCentroid(const Cell *cell) {
  Vector2 centroid = {0.0f, 0.0f};
  float A = calculateArea(cell);
  float factor = 0.0f;

  for (int i = 0, j = cell->num_vertices - 1; i < cell->num_vertices; j = i++) {
    factor = (cell->vertices[i].x * cell->vertices[j].y -
              cell->vertices[j].x * cell->vertices[i].y);
    centroid.x += (cell->vertices[i].x + cell->vertices[j].x) * factor;
    centroid.y += (cell->vertices[i].y + cell->vertices[j].y) * factor;
  }

  float sixArea = 6.0f * A;
  centroid.x /= sixArea;
  centroid.y /= sixArea;

  // Correcting for polygons in CW order
  // if (A < 0) {
  //   centroid.x = -centroid.x;
  //   centroid.y = -centroid.y;
  // }

  return centroid;
}

Vector2 midPoint(Vector2 a, Vector2 b) {
  Vector2 mid = {(a.x + b.x) / 2, (a.y + b.y) / 2};
  return mid;
}

typedef struct {
  Vector2 point;
  Vector2 direction;
} Line;

Line perpendicularBisector(Vector2 a, Vector2 b) {
  Vector2 mid = midPoint(a, b);
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  Line bisector = {mid, {dy, -dx}};
  return bisector;
}

Vector2 intersection(Segment seg, Line bisector, int *intersect) {
  Vector2 result = {0, 0};

  double x1 = seg.start.x, y1 = seg.start.y;
  double x2 = seg.end.x, y2 = seg.end.y;
  double px = bisector.point.x, py = bisector.point.y;
  double dx = bisector.direction.x, dy = bisector.direction.y;

  double seg_dx = x2 - x1;
  double seg_dy = y2 - y1;

  double determinant = -seg_dx * dy + seg_dy * dx;

  if (fabs(determinant) < 1e-10) {
    *intersect = 0;
    return result;
  }

  double t = -1 * (dx * (y1 - py) - dy * (x1 - px)) / determinant;

  if (t >= 0 && t <= 1) {
    result.x = x1 + t * seg_dx;
    result.y = y1 + t * seg_dy;
    if (result.x == x2 && result.y == y2) {
      *intersect = 0;
    } else {
      *intersect = 1;
    }
  } else {
    *intersect = 0;
  }

  return result;
}

APP_PLUG int plug_update(struct app_memory *Memory) {
  ASSERT(sizeof(struct app_state) <= Memory->PermanentStorageSize);

  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  struct app_state *AppState = (struct app_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;

    AppState->num_vertices = 100;

    for (int i = 0; i < AppState->num_vertices; i++) {
      AppState->vertices[i].position = (Vector2){
          centerX + GetRandomValue(-50, 50), centerY + GetRandomValue(-50, 50)};
      AppState->vertices[i].color =
          (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                  GetRandomValue(0, 255), 255};
      AppState->vertices[i].centroid = (Vector2){0};
    }

    Memory->IsInitialized = 1;

    printf("Initialized app state\n");
  }

  Rectangle box = {
      0,
      0,
      screenWidth,
      screenHeight,
  };

  // Calculate Voronoi Diagram
  for (int i = 0; i < AppState->num_vertices; i++) {
    AppState->cells[i].num_vertices = 4;
    AppState->cells[i].vertices[0] = (Vector2){box.x, box.y};
    AppState->cells[i].vertices[1] = (Vector2){box.x + box.width, box.y};
    AppState->cells[i].vertices[2] =
        (Vector2){box.x + box.width, box.y + box.height};
    AppState->cells[i].vertices[3] = (Vector2){box.x, box.y + box.height};

    for (int j = 0; j < AppState->num_vertices; j++) {
      if (i == j)
        continue;

      Line bisector = perpendicularBisector(AppState->vertices[i].position,
                                            AppState->vertices[j].position);

      // check for intersection of the bisector with the cell walls
      int num_intersections = 0;
      Vector2 first_intersection = {0};
      Vector2 second_intersection = {0};
      int first_intersection_index = -1;
      int second_intersection_index = -1;
      for (int k = 0; k < AppState->cells[i].num_vertices; k++) {
        int intersect;
        Segment edge = {
            AppState->cells[i].vertices[k],
            AppState->cells[i]
                .vertices[(k + 1) % AppState->cells[i].num_vertices]};
        Vector2 p = intersection(edge, bisector, &intersect);
        if (intersect) {
          if (num_intersections == 0) {
            first_intersection = p;
            first_intersection_index = k + 1;
          } else {
            second_intersection = p;
            second_intersection_index = k + 1;
          }
          num_intersections++;
        }
      }

      if (num_intersections == 2) {
        AppState->cells[i].temp_vertices[0] = first_intersection;
        int m = 1;
        for (int k = first_intersection_index; k < second_intersection_index;
             k++) {
          AppState->cells[i].temp_vertices[m] = AppState->cells[i].vertices[k];
          m++;
        }
        AppState->cells[i].temp_vertices[m] = second_intersection;
        AppState->cells[i].temp_vertex_count = m + 1;

        int collide = CheckCollisionPointPoly(
            AppState->vertices[i].position, AppState->cells[i].temp_vertices,
            AppState->cells[i].temp_vertex_count);

        // check that point is inside the cell
        if (!collide) {
          // reverse the temp cell
          AppState->cells[i].temp_vertices[0] = second_intersection;
          int m = 1;
          int k = (second_intersection_index) % AppState->cells[i].num_vertices;
          while (k !=
                 (first_intersection_index) % AppState->cells[i].num_vertices) {
            AppState->cells[i].temp_vertices[m] =
                AppState->cells[i].vertices[k];
            k = (k + 1) % AppState->cells[i].num_vertices;
            m++;
          }

          AppState->cells[i].temp_vertices[m] = first_intersection;
          AppState->cells[i].temp_vertex_count = m + 1;
        }

        // set the cell equal to the temp cell
        AppState->cells[i].num_vertices = AppState->cells[i].temp_vertex_count;
        for (int k = 0; k < AppState->cells[i].num_vertices; k++) {
          AppState->cells[i].vertices[k] = AppState->cells[i].temp_vertices[k];
        }

        AppState->cells[i].temp_vertex_count = 0;
        AppState->cells[i].temp_vertices[0] = (Vector2){0};
      }
    }
  }

  BeginDrawing();
  ClearBackground(LIGHTGRAY);

  for (int i = 0; i < AppState->num_vertices; i++) {
    float area = (-1.0f * 10 * calculateArea(&AppState->cells[i])) /
                 (screenWidth * screenHeight);

    float intensity = CLAMP(area, 0.1, 0.7);

    Color color = (Color){255, 0, 0, (unsigned char)(intensity * 255)};

    for (int j = 0; j < AppState->cells[i].num_vertices; j++) {
      Vector2 p1 = AppState->vertices[i].position;
      Vector2 p2 = AppState->cells[i].vertices[j];
      Vector2 p3 = AppState->cells[i]
                       .vertices[(j + 1) % AppState->cells[i].num_vertices];

      if (!IsCounterclockwise(p1, p2, p3)) {
        DrawTriangle(p1, p2, p3, color);
      } else {
        DrawTriangle(p1, p3, p2, color);
      }
      DrawLineV(AppState->cells[i].vertices[j],
                AppState->cells[i]
                    .vertices[(j + 1) % AppState->cells[i].num_vertices],
                WHITE);
    }
  }

  // UPDATE (LLoyd)
  for (int i = 0; i < AppState->num_vertices; i++) {
    Vector2 centroid = calculateCentroid(&AppState->cells[i]);
    // DrawCircleV(AppState->vertices[i].position, 2, WHITE);
    // DrawCircleV(centroid, 2, BLACK);

    Vector2 direction =
        Vector2Subtract(centroid, AppState->vertices[i].position);
    // Normalize the direction vector and scale it by the step factor
    direction = Vector2Scale(Vector2Normalize(direction), 1.0f);
    // Update the vertex position
    AppState->vertices[i].position =
        Vector2Add(AppState->vertices[i].position, direction);
  }

  DrawFPS(10, 10);

  EndDrawing();

  return 1;
}
