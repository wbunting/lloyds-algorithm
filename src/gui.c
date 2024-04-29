#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#include "gui.h"

#define APP_PLUG

bool IsCounterclockwise(Vector2 p1, Vector2 p2, Vector2 p3) {
  float area = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
  return (area > 0); // Positive area means counterclockwise
}

Vector2 midPoint(Vector2 a, Vector2 b) {
  Vector2 mid = {(a.x + b.x) / 2, (a.y + b.y) / 2};
  return mid;
}

typedef struct {
  Vector2 point;     // A point on the line
  Vector2 direction; // Direction vector of the line
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

  // Extracting coordinates and direction components
  double x1 = seg.start.x, y1 = seg.start.y;
  double x2 = seg.end.x, y2 = seg.end.y;
  double px = bisector.point.x, py = bisector.point.y;
  double dx = bisector.direction.x, dy = bisector.direction.y;

  // Calculating the parameters for the line equation of the segment
  double seg_dx = x2 - x1;
  double seg_dy = y2 - y1;

  // Line equation for segment: (x - x1) / seg_dx = (y - y1) / seg_dy
  // Line equation for bisector: x = px + t * dx, y = py + t * dy

  // Determinant to solve for intersection
  double determinant = -seg_dx * dy + seg_dy * dx;

  // If determinant is zero, lines are parallel
  if (fabs(determinant) < 1e-10) {
    *intersect = 0; // No intersection
    return result;
  }

  // Solve linear system:
  // (x1 + t * seg_dx) = px + s * dx
  // (y1 + t * seg_dy) = py + s * dy
  double t = -1 * (dx * (y1 - py) - dy * (x1 - px)) / determinant;
  // double s = (seg_dx * (py - y1) + seg_dy * (px - x1)) / determinant;

  // Check if the intersection point t is within the bounds of the segment
  if (t >= 0 && t <= 1) {
    result.x = x1 + t * seg_dx;
    result.y = y1 + t * seg_dy;
    // the intersection cannot be x2 y2 exactly
    if (result.x == x2 && result.y == y2) {
      *intersect = 0;
    } else {
      *intersect = 1; // Intersection occurs
    }
  } else {
    *intersect = 0; // Intersection point is not within the segment
  }

  return result;
}

APP_PLUG int plug_update(struct app_memory *Memory) {
  ASSERT(sizeof(struct app_state) <= Memory->PermanentStorageSize);

  struct app_state *AppState = (struct app_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    float centerX = GetScreenWidth() / 2.0f;
    float centerY = GetScreenHeight() / 2.0f;

    AppState->num_vertices = 4;

    AppState->vertices[0].position = (Vector2){100.000000, 300.000000};
    AppState->vertices[0].color =
        (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                GetRandomValue(0, 255), 255};
    AppState->vertices[1].position = (Vector2){200.000000, 200.000000};
    AppState->vertices[1].color =
        (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                GetRandomValue(0, 255), 255};
    AppState->vertices[2].position = (Vector2){300.000000, 400.000000};
    AppState->vertices[2].color =
        (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                GetRandomValue(0, 255), 255};
    AppState->vertices[3].position = (Vector2){200.000000, 100.000000};
    AppState->vertices[3].color =
        (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                GetRandomValue(0, 255), 255};

    for (int i = 0; i < AppState->num_vertices; i++) {
      // AppState->vertices[i].position =
      //     (Vector2){centerX + (GetRandomValue(-centerX, centerX)),
      //               centerY + (GetRandomValue(-centerY, centerY))};
      AppState->vertices[i].velocity =
          (Vector2){GetRandomValue(-1, 1), GetRandomValue(-1, 1)};
      AppState->vertices[i].centroid = (Vector2){0};

      // print the seed vertices
      printf("Vertex %d: (%f, %f)\n", i, AppState->vertices[i].position.x,
             AppState->vertices[i].position.y);
    }

    Memory->IsInitialized = 1;

    printf("Initialized app state\n");
  }

  // get the min x coordinate among vertices
  float min_x = AppState->vertices[0].position.x;
  for (int i = 1; i < AppState->num_vertices; i++) {
    if (AppState->vertices[i].position.x < min_x) {
      min_x = AppState->vertices[i].position.x;
    }
  }

  // get the max x coordinate among vertices
  float max_x = AppState->vertices[0].position.x;
  for (int i = 1; i < AppState->num_vertices; i++) {
    if (AppState->vertices[i].position.x > max_x) {
      max_x = AppState->vertices[i].position.x;
    }
  }

  // get the min y coordinate among vertices
  float min_y = AppState->vertices[0].position.y;
  for (int i = 1; i < AppState->num_vertices; i++) {
    if (AppState->vertices[i].position.y < min_y) {
      min_y = AppState->vertices[i].position.y;
    }
  }

  // get the max y coordinate among vertices
  float max_y = AppState->vertices[0].position.y;
  for (int i = 1; i < AppState->num_vertices; i++) {
    if (AppState->vertices[i].position.y > max_y) {
      max_y = AppState->vertices[i].position.y;
    }
  }

  // padding is 1px for now. Box with min / max and padding
  // Rectangle box = {
  //     min_x - 50,
  //     min_y - 50,
  //     max_x - min_x + 100,
  //     max_y - min_y + 100,
  // };
  //
  Rectangle box = {
      0,
      0,
      GetScreenWidth(),
      GetScreenHeight(),
  };

  // Draw the box
  DrawRectangleLinesEx(box, 1, RED);

  // Calculate Voronoi Diagram
  for (int i = 0; i < AppState->num_vertices; i++) {
    AppState->cells[i].num_vertices = 4;
    // set the initial cell to be the bounding box including padding
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

      // Draw the bisector extended to the screen edges
      Vector2 p1 = bisector.point;
      Vector2 p2 = (Vector2){p1.x + bisector.direction.x * 1000,
                             p1.y + bisector.direction.y * 1000};
      DrawLineV(p1, p2, GREEN);

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

        // reset the temp cell
        AppState->cells[i].temp_vertex_count = 0;
        AppState->cells[i].temp_vertices[0] = (Vector2){0};
      }
    }
  }

  BeginDrawing();
  ClearBackground(DARKGRAY);

  for (int i = 0; i < AppState->num_vertices; i++) {
    for (int j = 0; j < AppState->cells[i].num_vertices; j++) {
      Vector2 p1 = AppState->vertices[i].position;
      Vector2 p2 = AppState->cells[i].vertices[j];
      Vector2 p3 = AppState->cells[i]
                       .vertices[(j + 1) % AppState->cells[i].num_vertices];

      if (!IsCounterclockwise(p1, p2, p3)) {
        DrawTriangle(p1, p2, p3, AppState->vertices[i].color);
      } else {
        DrawTriangle(p1, p3, p2, AppState->vertices[i].color);
      }
      // DrawTriangleLines(p1, p2, p3, BLACK);
    }
  }

  for (int i = 0; i < AppState->num_vertices; i++) {
    DrawCircleV(AppState->vertices[i].position, 2, WHITE);
    // DrawText(TextFormat("%d", i), AppState->vertices[i].position.x + 4,
    //          AppState->vertices[i].position.y + 4, 10, WHITE);
  }

  // update the vertex positions
  for (int i = 0; i < AppState->num_vertices; i++) {
    AppState->vertices[i].position.x += AppState->vertices[i].velocity.x;
    AppState->vertices[i].position.y += AppState->vertices[i].velocity.y;

    // check for collision with the screen edges
    if (AppState->vertices[i].position.x <= 0 ||
        AppState->vertices[i].position.x >= GetScreenWidth()) {
      AppState->vertices[i].velocity.x *= -1;
    }
    if (AppState->vertices[i].position.y <= 0 ||
        AppState->vertices[i].position.y >= GetScreenHeight()) {
      AppState->vertices[i].velocity.y *= -1;
    }
  }

  DrawFPS(10, 10);

  EndDrawing();

  return 1;
}
