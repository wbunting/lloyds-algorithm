#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>
#include <raymath.h>

#include "gui.h"

#define APP_PLUG

static float Magnitude(Vector2 v) {
  float result = sqrt(v.x * v.x + v.y * v.y);
  return result;
}

void initBeachlineItem(BeachlineItem *item) {
  if (item) {
    item->parent = NULL;
    item->left = NULL;
    item->right = NULL;
  }
}

// void SetLeft(BeachlineItem *item, BeachlineItem *newLeft) {
//   assert(item && item->type == Edge);
//   assert(newLeft);
//   item->left = newLeft;
//   newLeft->parent = item;
// }

void SetLeft(BeachlineItem *this, BeachlineItem *child) {
  assert(this->type == Edge);
  // assert(this != NULL && child != NULL);
  if (this == NULL || child == NULL) {
    return;
  }
  this->left = child;
  child->parent = this;
}

// void SetRight(BeachlineItem *item, BeachlineItem *newRight) {
//   assert(item && item->type == Edge);
//   assert(newRight);
//   item->right = newRight;
//   newRight->parent = item;
// }

void SetRight(BeachlineItem *this, BeachlineItem *child) {
  assert(this->type == Edge);
  // assert(this != NULL && child != NULL);
  if (this == NULL || child == NULL) {
    return;
  }
  this->right = child;
  child->parent = this;
}

// void SetParentFromItem(BeachlineItem *item, BeachlineItem *newItem) {
//   assert(item);
//   if (!item->parent) {
//     newItem->parent = NULL;
//     return;
//   }
//   if (item->parent->left == item) {
//     SetLeft(item->parent, newItem);
//   } else {
//     assert(item->parent->right == item);
//     SetRight(item->parent, newItem);
//   }
// }

void SetParentFromItem(BeachlineItem *this, BeachlineItem *item) {
  assert(item != NULL);

  if (item->parent == NULL) {
    this->parent = NULL; // If item's parent is NULL, set this's parent to NULL
    return;
  }

  if (item->parent->left == item) {
    SetLeft(item->parent, this); // Sets this as the left child of item's parent
  } else {
    assert(item->parent->right == item);
    SetRight(item->parent,
             this); // Sets this as the right child of item's parent
  }
}

typedef struct {
  SweepEvent *events[MAX_EVENTS];
  int size;
} EventQueue;

void initEventQueue(EventQueue *queue) { queue->size = 0; }

void pushEvent(EventQueue *queue, SweepEvent *event) {
  // Simplified push: should include sorting to maintain priority queue behavior
  assert(queue->size < MAX_EVENTS);
  // queue->events[queue->size++] = event;

  // Find the position where the new event should be inserted to keep the queue
  // sorted
  int i = 0;
  for (; i < queue->size; i++) {
    if (queue->events[i]->yCoord > event->yCoord) {
      break; // Found the insertion point
    }
  }

  // Shift all elements from the insertion point one position to the right
  for (int j = queue->size; j > i; j--) {
    queue->events[j] = queue->events[j - 1];
  }

  // Insert the new event at the found position
  queue->events[i] = event;
  queue->size++; // Increase the size of the queue
}

int isEventQueueEmpty(const EventQueue *queue) { return queue->size == 0; }

SweepEvent *popEvent(EventQueue *queue) {
  // Simplified pop: always pops the last item, which is not correct for a
  // priority queue
  assert(queue->size > 0);
  return queue->events[--queue->size];
}

SweepEvent *peekEvent(const EventQueue *queue) {
  assert(queue->size > 0);
  return queue->events[queue->size - 1];
}

void initializeBeachlineItems(BeachlineItem items[], int size) {
  for (int i = 0; i < size; i++) {
    items[i].parent = NULL;
    items[i].left = NULL;
    items[i].right = NULL;
  }
}

void PrintQueue(EventQueue *queue) {
  for (int i = 0; i < queue->size; i++) {
    SweepEvent *evt = queue->events[i];
    printf("Event at y=%f\n", evt->yCoord);
  }
}

BeachlineItem *allocateBeachlineItem(FortuneState *state) {
  if (state->beachlineItemCount < MAX_BEACHLINE_ITEMS) {
    BeachlineItem *item = &state->beachlineItems[state->beachlineItemCount++];
    item->parent = NULL;
    item->left = NULL;
    item->right = NULL;
    return item;
  }
  return NULL; // No more items available
}

BeachlineItem *createArc(Vector2 focus, FortuneState *state) {
  if (state->beachlineItemCount < MAX_BEACHLINE_ITEMS) {
    BeachlineItem *item = &state->beachlineItems[state->beachlineItemCount++];
    item->type = Arc;
    item->arc.focus = focus;
    item->arc.squeezeEvent = NULL;
    item->parent = NULL;
    item->left = NULL;
    item->right = NULL;
    return item;
  }
  return NULL; // No more items available
}

// Function to find the first parent on the left
BeachlineItem *GetFirstParentOnTheLeft(BeachlineItem *item) {
  BeachlineItem *current = item;
  while (current->parent != NULL && current->parent->left == current) {
    current = current->parent;
  }
  assert(current->parent == NULL || current->parent->type == Edge);
  return current->parent;
}

// Function to find the first parent on the right
BeachlineItem *GetFirstParentOnTheRight(BeachlineItem *item) {
  BeachlineItem *current = item;
  while (current->parent != NULL && current->parent->right == current) {
    current = current->parent;
  }
  assert(current->parent == NULL || current->parent->type == Edge);
  return current->parent;
}

// Function to find the first leaf node on the left
BeachlineItem *GetFirstLeafOnTheLeft(BeachlineItem *item) {
  if (item->left == NULL) {
    return NULL;
  }
  BeachlineItem *current = item->left;
  while (current->right != NULL) {
    current = current->right;
  }
  assert(current->type == Arc);
  return current;
}

// Function to find the first leaf node on the right
BeachlineItem *GetFirstLeafOnTheRight(BeachlineItem *item) {
  if (item->right == NULL) {
    return NULL;
  }
  BeachlineItem *current = item->right;
  while (current->left != NULL) {
    current = current->left;
  }
  assert(current->type == Arc);
  return current;
}

float GetArcYForXCoord(ArcStruct *arc, float x, float directrixY) {
  // NOTE: In the interest of keeping the formula simple when moving away from
  // the origin,
  //       we'll use the substitution from (x,y) -> (w,y) = (x-focusX,y).
  //       In particular this substitution means that the formula always has the
  //       form: y = aw^2 + c, the linear term's coefficient is always 0.
  float a = 1.0f / (2.0f * (arc->focus.y - directrixY));
  float c = (arc->focus.y + directrixY) * 0.5f;

  float w = x - arc->focus.x;
  return a * w * w + c;
}

bool GetEdgeArcIntersectionPoint(EdgeStruct *edge, ArcStruct *arc,
                                 float directrixY, Vector2 *intersectionPt) {
  // Special case 1: Edge is a vertical line.
  if (edge->direction.x == 0.0f) {
    if (directrixY == arc->focus.y) {
      if (edge->start.x == arc->focus.x) {
        intersectionPt->x = arc->focus.x;
        intersectionPt->y =
            arc->focus
                .y; // Determine the correct y-value based on your use case
        return true;
      } else {
        return false;
      }
    }
    float arcY = GetArcYForXCoord(arc, edge->start.x, directrixY);
    intersectionPt->x = edge->start.x;
    intersectionPt->y = arcY;
    return true;
  }

  // Line equation: y = px + q
  float p = edge->direction.y / edge->direction.x;
  float q = edge->start.y - p * edge->start.x;

  if (arc->focus.y == directrixY) {
    float intersectionXOffset = arc->focus.x - edge->start.x;
    if (intersectionXOffset * edge->direction.x < 0) {
      return false;
    }

    intersectionPt->x = arc->focus.x;
    intersectionPt->y = p * arc->focus.x + q;
    return true;
  }

  // Parabola equation: y = a_0 + a_1x + a_2x^2
  float a2 = 1.0f / (2.0f * (arc->focus.y - directrixY));
  float a1 = -p - 2.0f * a2 * arc->focus.x;
  float a0 =
      a2 * arc->focus.x * arc->focus.x + (arc->focus.y + directrixY) * 0.5f - q;

  float discriminant = a1 * a1 - 4.0f * a2 * a0;
  if (discriminant < 0) {
    return false;
  }

  float rootDisc = sqrtf(discriminant);
  float x1 = (-a1 + rootDisc) / (2.0f * a2);
  float x2 = (-a1 - rootDisc) / (2.0f * a2);

  float x1Offset = x1 - edge->start.x;
  float x2Offset = x2 - edge->start.x;
  float x1Dot = x1Offset * edge->direction.x;
  float x2Dot = x2Offset * edge->direction.x;

  float x;
  if ((x1Dot >= 0.0f) && (x2Dot < 0.0f))
    x = x1;
  else if ((x1Dot < 0.0f) && (x2Dot >= 0.0f))
    x = x2;
  else if ((x1Dot >= 0.0f) && (x2Dot >= 0.0f)) {
    if (x1Dot < x2Dot)
      x = x1;
    else
      x = x2;
  } else // (x1Dot < 0.0f) && (x2Dot < 0.0f)
  {
    if (x1Dot < x2Dot)
      x = x2;
    else
      x = x1;
  }

  float y = GetArcYForXCoord(arc, x, directrixY);
  intersectionPt->x = x;
  intersectionPt->y = y;

  return true;
}

BeachlineItem *GetActiveArcForXCoord(BeachlineItem *root, float x,
                                     float directrixY) {
  BeachlineItem *currentItem = root;
  while (currentItem && currentItem->type != Arc) {
    assert(currentItem->type == Edge);
    BeachlineItem *left = GetFirstLeafOnTheLeft(currentItem);
    BeachlineItem *right = GetFirstLeafOnTheRight(currentItem);

    assert(left && left->type == Arc);
    assert(right && right->type == Arc);

    BeachlineItem *fromLeft = GetFirstParentOnTheRight(left);
    BeachlineItem *fromRight = GetFirstParentOnTheLeft(right);

    assert(fromLeft && fromLeft == fromRight && fromLeft->type == Edge);

    Vector2 leftIntersect, rightIntersect;
    int didLeftIntersect = GetEdgeArcIntersectionPoint(
        &fromLeft->edge, &left->arc, directrixY, &leftIntersect);
    int didRightIntersect = GetEdgeArcIntersectionPoint(
        &fromLeft->edge, &right->arc, directrixY, &rightIntersect);

    float intersectionX =
        (didLeftIntersect ? leftIntersect.x : rightIntersect.x);

    if (x < intersectionX) {
      currentItem = currentItem->left;
    } else {
      currentItem = currentItem->right;
    }
  }

  assert(currentItem && currentItem->type == Arc);
  return currentItem;
}

BeachlineItem *createEdge(Vector2 start, Vector2 dir, FortuneState *state) {
  if (state->beachlineItemCount < MAX_BEACHLINE_ITEMS) {
    BeachlineItem *item = &state->beachlineItems[state->beachlineItemCount++];
    item->type = Edge;
    item->edge.start = start;
    item->edge.direction = dir;
    item->edge.extendsUpwardsForever = 0; // False in C
    item->parent = NULL;
    item->left = NULL;
    item->right = NULL;
    return item;
  }
  return NULL; // No more items available
}

float vector2Length(Vector2 v) { return sqrt(v.x * v.x + v.y * v.y); }

Vector2 normalize(Vector2 v) {
  Vector2 result = {0, 0};
  float length = vector2Length(v);
  if (length == 0) {
    return result;
  }
  result.x = v.x / length;
  result.y = v.y / length;
  return result;
}

bool TryGetEdgeIntersectionPoint(EdgeStruct *e1, EdgeStruct *e2,
                                 Vector2 *intersectionPt) {
  float dx = e2->start.x - e1->start.x;
  float dy = e2->start.y - e1->start.y;
  float det =
      e2->direction.x * e1->direction.y - e2->direction.y * e1->direction.x;
  float u = (dy * e2->direction.x - dx * e2->direction.y) / det;
  float v = (dy * e1->direction.x - dx * e1->direction.y) / det;

  if ((u < 0.0f) && !e1->extendsUpwardsForever)
    return false;
  if ((v < 0.0f) && !e2->extendsUpwardsForever)
    return false;
  if ((u == 0.0f) && (v == 0.0f) && !e1->extendsUpwardsForever &&
      !e2->extendsUpwardsForever)
    return false;

  intersectionPt->x = e1->start.x + e1->direction.x * u;
  intersectionPt->y = e1->start.y + e1->direction.y * u;

  return true;
}

void AddArcSqueezeEvent(FortuneState *state, EventQueue *eventQueue,
                        BeachlineItem *arc) {
  if (arc->type != Arc) {
    return;
  }

  BeachlineItem *leftEdge = GetFirstParentOnTheLeft(arc);
  BeachlineItem *rightEdge = GetFirstParentOnTheRight(arc);

  if (!leftEdge || !rightEdge) {
    return;
  }

  Vector2 circleEventPoint;
  if (!TryGetEdgeIntersectionPoint(&leftEdge->edge, &rightEdge->edge,
                                   &circleEventPoint)) {
    return;
  }

  Vector2 circleCentreOffset = {arc->arc.focus.x - circleEventPoint.x,
                                arc->arc.focus.y - circleEventPoint.y};
  float circleRadius = Magnitude(circleCentreOffset);
  float circleEventY = circleEventPoint.y - circleRadius;

  if (arc->arc.squeezeEvent && arc->arc.squeezeEvent->yCoord >= circleEventY) {
    return;
  }

  // NOTE: If we already have an intersection event that we'll encounter
  // sooner than this one, then
  //       just don't add this one (because otherwise it'll reference a
  //       deleted arc when it gets processed)
  if (arc->arc.squeezeEvent != NULL) {
    if (arc->arc.squeezeEvent->yCoord >= circleEventY) {
      return;
    } else {
      assert(arc->arc.squeezeEvent->type == EdgeIntersection);
      arc->arc.squeezeEvent->edgeIntersect.isValid = false;
    }
  }

  if (state->eventsSize < MAX_EVENTS) {
    SweepEvent *newEvt = &state->events[state->eventsSize++];
    newEvt->type = EdgeIntersection;
    newEvt->yCoord = circleEventY;
    newEvt->edgeIntersect.squeezedArc = arc;
    newEvt->edgeIntersect.intersectionPoint = circleEventPoint;
    newEvt->edgeIntersect.isValid = true;

    arc->arc.squeezeEvent = newEvt;
    assert(arc->arc.squeezeEvent == newEvt);
    pushEvent(eventQueue, newEvt);
    // verify that the correct event was added by peeking it
    // SweepEvent *peeked = peekEvent(eventQueue);
    // assert(peeked->type == EdgeIntersection);
  }
}

static void VerifyThatThereAreNoReferencesToItem(BeachlineItem *root,
                                                 BeachlineItem *item) {
  if (root == NULL)
    return;
  if (root->type == Arc)
    return;

  assert(root->parent != item);
  assert(root->left != item);
  assert(root->right != item);

  VerifyThatThereAreNoReferencesToItem(root->left, item);
  VerifyThatThereAreNoReferencesToItem(root->right, item);
}

BeachlineItem *AddArcToBeachline(EventQueue *eventQueue, FortuneState *state,
                                 BeachlineItem *root, SweepEvent *evt,
                                 float sweepLineY) {
  Vector2 newPoint = evt->newPoint.point;
  BeachlineItem *replacedArc =
      GetActiveArcForXCoord(root, newPoint.x, sweepLineY);
  assert(replacedArc != NULL && replacedArc->type == Arc);

  BeachlineItem *splitArcLeft = createArc(replacedArc->arc.focus, state);
  BeachlineItem *splitArcRight = createArc(replacedArc->arc.focus, state);
  BeachlineItem *newArc = createArc(newPoint, state);

  float intersectionY =
      GetArcYForXCoord(&replacedArc->arc, newPoint.x, sweepLineY);
  assert(isfinite(intersectionY));
  Vector2 edgeStart = {newPoint.x, intersectionY};
  Vector2 focusOffset = {newArc->arc.focus.x - replacedArc->arc.focus.x,
                         newArc->arc.focus.y - replacedArc->arc.focus.y};
  Vector2 edgeDir = normalize((Vector2){focusOffset.y, -focusOffset.x});
  BeachlineItem *edgeLeft = createEdge(edgeStart, edgeDir, state);
  BeachlineItem *edgeRight =
      createEdge(edgeStart, (Vector2){-edgeDir.x, -edgeDir.y}, state);

  // Assuming all pointers are initially NULL
  assert(replacedArc->left == NULL);
  assert(replacedArc->right == NULL);
  SetParentFromItem(edgeLeft, replacedArc);
  SetLeft(edgeLeft, splitArcLeft);
  SetRight(edgeLeft, edgeRight);
  SetLeft(edgeRight, newArc);
  SetRight(edgeRight, splitArcRight);

  BeachlineItem *newRoot = (root == replacedArc) ? edgeLeft : root;

  if (replacedArc->arc.squeezeEvent != NULL) {
    assert(replacedArc->arc.squeezeEvent->type == EdgeIntersection);
    assert(replacedArc->arc.squeezeEvent->edgeIntersect.isValid);
    replacedArc->arc.squeezeEvent->edgeIntersect.isValid = false;
  }

  VerifyThatThereAreNoReferencesToItem(newRoot, replacedArc);
  assert((replacedArc->arc.squeezeEvent == NULL) ||
         (replacedArc->arc.squeezeEvent->edgeIntersect.isValid == false));

  AddArcSqueezeEvent(state, eventQueue, splitArcLeft);
  AddArcSqueezeEvent(state, eventQueue, splitArcRight);

  return newRoot;
}

void FinishEdge(BeachlineItem *item, CompleteEdge *edges, int *edgeCount,
                int maxEdges) {
  if (item == NULL) {
    return;
  }

  if (item->type == Edge) {
    float length = 10000.0;
    Vector2 edgeEnd = {item->edge.start.x + length * item->edge.direction.x,
                       item->edge.start.y + length * item->edge.direction.y};

    if (*edgeCount < maxEdges) {
      CompleteEdge *edge = &edges[(*edgeCount)++];
      edge->endpointA = item->edge.start;
      edge->endpointB = edgeEnd;
    }

    FinishEdge(item->left, edges, edgeCount, maxEdges);
    FinishEdge(item->right, edges, edgeCount, maxEdges);
  }
}

void PrintBeachlineItem(BeachlineItem *item) {
  if (item == NULL) {
    return;
  }

  if (item->type == Arc) {
    printf("Arc: %f, %f\n", item->arc.focus.x, item->arc.focus.y);
  } else {
    printf("Edge: %f, %f\n", item->edge.start.x, item->edge.start.y);
  }

  PrintBeachlineItem(item->left);
  PrintBeachlineItem(item->right);
}

const char *GetTypeName(BeachlineItemType type) {
  switch (type) {
  case Arc:
    return "Arc";
  case Edge:
    return "Edge";
  default:
    return "Unknown";
  }
}

void PrintTree(BeachlineItem *node, int level) {
  if (node == NULL) {
    return;
  }

  // Print the current node with indentation based on its level in the tree
  for (int i = 0; i < level; i++) {
    printf("    "); // 4 spaces per level of depth
  }

  // Print details about the node
  printf("%s [Address: %p, Parent: %p, Left: %p, Right: %p]\n",
         GetTypeName(node->type), (void *)node, (void *)node->parent,
         (void *)node->left, (void *)node->right);

  // Recursively print the left and right children
  PrintTree(node->left, level + 1);
  PrintTree(node->right, level + 1);
}

int CountTreeNodes(BeachlineItem *node) {
  if (node == NULL) {
    return 0;
  }

  return 1 + CountTreeNodes(node->left) + CountTreeNodes(node->right);
}

BeachlineItem *RemoveArcFromBeachline(EventQueue *eventQueue,
                                      FortuneState *state, BeachlineItem *root,
                                      SweepEvent *evt) {

  BeachlineItem *squeezedArc = evt->edgeIntersect.squeezedArc;
  assert(evt->type == EdgeIntersection);
  assert(evt->edgeIntersect.isValid);
  assert(squeezedArc->arc.squeezeEvent == evt);

  BeachlineItem *leftEdge = GetFirstParentOnTheLeft(squeezedArc);
  BeachlineItem *rightEdge = GetFirstParentOnTheRight(squeezedArc);
  assert(leftEdge && rightEdge);

  BeachlineItem *leftArc = GetFirstLeafOnTheLeft(leftEdge);
  BeachlineItem *rightArc = GetFirstLeafOnTheRight(rightEdge);
  assert(leftArc && rightArc && leftArc != rightArc);

  Vector2 circleCentre = evt->edgeIntersect.intersectionPoint;
  if (state->edgesSize < MAX_EDGES - 1) {
    CompleteEdge *edgeA = &state->edges[state->edgesSize++];
    edgeA->endpointA = leftEdge->edge.start;
    edgeA->endpointB = circleCentre;

    CompleteEdge *edgeB = &state->edges[state->edgesSize++];
    edgeB->endpointA = circleCentre;
    edgeB->endpointB = rightEdge->edge.start;

    if (leftEdge->edge.extendsUpwardsForever) {
      edgeA->endpointA.y = FLT_MAX;
    }
    if (rightEdge->edge.extendsUpwardsForever) {
      edgeB->endpointA.y = FLT_MAX;
    }
  }

  Vector2 adjacentArcOffset = {};
  adjacentArcOffset.x = rightArc->arc.focus.x - leftArc->arc.focus.x;
  adjacentArcOffset.y = rightArc->arc.focus.y - leftArc->arc.focus.y;
  Vector2 newEdgeDirection = {adjacentArcOffset.y, -adjacentArcOffset.x};
  newEdgeDirection = normalize(newEdgeDirection);
  BeachlineItem *newItem = createEdge(circleCentre, newEdgeDirection, state);

  BeachlineItem *higherEdge = NULL;
  BeachlineItem *tempItem = squeezedArc;
  while (tempItem->parent != NULL) {
    tempItem = tempItem->parent;
    if (tempItem == leftEdge) {
      higherEdge = leftEdge;
    }
    if (tempItem == rightEdge) {
      higherEdge = rightEdge;
    }
  }
  assert((higherEdge != NULL) && (higherEdge->type == Edge));

  SetParentFromItem(newItem, higherEdge);
  SetLeft(newItem, higherEdge->left);
  SetRight(newItem, higherEdge->right);

  assert((squeezedArc->parent == NULL) || (squeezedArc->parent->type == Edge));
  BeachlineItem *remainingItem = NULL;
  BeachlineItem *parent = squeezedArc->parent;
  if (parent->left == squeezedArc) {
    remainingItem = parent->right;
  } else {
    assert(parent->right == squeezedArc);
    remainingItem = parent->left;
  }
  assert((parent == leftEdge) || (parent == rightEdge));
  assert(parent != higherEdge);

  SetParentFromItem(remainingItem, parent);

  BeachlineItem *newRoot = root;
  if ((root == leftEdge) || (root == rightEdge)) {
    newRoot = newItem;
  }
  VerifyThatThereAreNoReferencesToItem(newRoot, leftEdge);
  VerifyThatThereAreNoReferencesToItem(newRoot, squeezedArc);
  VerifyThatThereAreNoReferencesToItem(newRoot, rightEdge);
  assert(squeezedArc->type == Arc);
  if (squeezedArc->arc.squeezeEvent != NULL) {
    assert(squeezedArc->arc.squeezeEvent->type == EdgeIntersection);
    assert(squeezedArc->arc.squeezeEvent->edgeIntersect.isValid);
    squeezedArc->arc.squeezeEvent->edgeIntersect.isValid = false;
  }
  // delete leftEdge;
  // delete squeezedArc;
  // delete rightEdge;

  AddArcSqueezeEvent(state, eventQueue, leftArc);
  AddArcSqueezeEvent(state, eventQueue, rightArc);

  return newRoot;
}

FortuneState FortunesAlgorithm(struct app_state *AppState, float cutoffY) {
  FortuneState result = AppState->fortuneState;
  result.eventsSize = 0;
  result.edgesSize = 0;
  result.beachlineItemCount = 0;
  EventQueue eventQueue;
  initEventQueue(&eventQueue);
  initializeBeachlineItems(result.beachlineItems, MAX_BEACHLINE_ITEMS);

  for (int i = 0; i < AppState->num_vertices; i++) {
    // TODO maybe these have to go into the event state where we allocated
    // memory?
    if (result.eventsSize < MAX_EVENTS) {
      SweepEvent *evt = &result.events[result.eventsSize++];
      evt->type = NewPoint;
      evt->newPoint.point = AppState->vertices[i].position;
      evt->yCoord = AppState->vertices[i].position.y;
      pushEvent(&eventQueue, evt);
    } else {
      printf("Event queue full\n");
    }
  }

  if (isEventQueueEmpty(&eventQueue)) {
    // there were not initial events or points .. assert zero
    assert(0);
  }

  SweepEvent *firstEvent = peekEvent(&eventQueue);
  if (firstEvent->yCoord < cutoffY) {
    result.sweepY = cutoffY;
    while (!isEventQueueEmpty(&eventQueue)) {
      SweepEvent *event = popEvent(&eventQueue);
      result.unencounteredEvents[result.unencounteredEventsSize++] = event;
    }
    printf("returning after only the first event\n");
    // return result;
  }
  popEvent(&eventQueue);

  BeachlineItem *firstArc = allocateBeachlineItem(&result);

  if (firstArc) {
    firstArc->type = Arc;
    firstArc->arc.focus = firstEvent->newPoint.point;
    firstArc->arc.squeezeEvent = NULL;
    // delete the first event?

    // This firstArc now becomes the root of our beachline structure
    BeachlineItem *root = firstArc; // Root is assigned to firstArc
    float startupSpecialCaseEndY = firstArc->arc.focus.y - 1.0f;
    while (!isEventQueueEmpty(&eventQueue) &&
           peekEvent(&eventQueue)->yCoord > startupSpecialCaseEndY) {
      SweepEvent *evt = peekEvent(&eventQueue);
      if (evt->yCoord < cutoffY)
        break;
      popEvent(&eventQueue);

      assert(evt->type == NewPoint);
      Vector2 newFocus = evt->newPoint.point;
      BeachlineItem *newArc = createArc(newFocus, &result);

      BeachlineItem *activeArc =
          GetActiveArcForXCoord(root, newFocus.x, newFocus.y);
      assert(activeArc && activeArc->type == Arc);

      Vector2 edgeStart = {(newFocus.x + activeArc->arc.focus.x) / 2.0f,
                           newFocus.y + 100.0f};
      Vector2 edgeDir = {0.0f, -1.0f};
      BeachlineItem *newEdge = createEdge(edgeStart, edgeDir, &result);
      newEdge->edge.extendsUpwardsForever = true;

      if (activeArc->parent) {
        if (activeArc == activeArc->parent->left) {
          SetLeft(activeArc->parent, newEdge);
        } else {
          assert(activeArc == activeArc->parent->right);
          SetRight(activeArc->parent, newEdge);
        }
      } else {
        root = newEdge;
      }
      if (newFocus.x < activeArc->arc.focus.x) {
        SetLeft(newEdge, newArc);
        SetRight(newEdge, activeArc);
      } else {
        SetLeft(newEdge, activeArc);
        SetRight(newEdge, newArc);
      }
    }

    while (!isEventQueueEmpty(&eventQueue)) {
      SweepEvent *nextEvent = peekEvent(&eventQueue);
      if (nextEvent->yCoord < cutoffY) {
        break;
      }
      popEvent(&eventQueue);

      float sweepY = nextEvent->yCoord;
      if (nextEvent->type == NewPoint) {
        root = AddArcToBeachline(&eventQueue, &result, root, nextEvent, sweepY);
      } else if (nextEvent->type == EdgeIntersection) {
        if (nextEvent->edgeIntersect.isValid) {
          root = RemoveArcFromBeachline(&eventQueue, &result, root, nextEvent);
        }
      } else {
        printf("Unrecognized queue item type: %d\n", nextEvent->type);
      }
    }

    if (isEventQueueEmpty(&eventQueue) || (cutoffY < -200.0f)) {
      FinishEdge(root, result.edges, &result.edgesSize, MAX_EDGES);
      root = NULL;
    }
  }
  // reset event queue size
  result.eventsSize = 0;
  eventQueue.size = 0;
  result.beachlineItemCount = 0;

  return result;
}

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

void ComputeVoronoi(Vertex *vertices, int num_vertices, Rectangle box,
                    Cell *cells) {
  for (int i = 0; i < num_vertices; i++) {
    cells[i].num_vertices = 4;
    cells[i].vertices[0] = (Vector2){box.x, box.y};
    cells[i].vertices[1] = (Vector2){box.x + box.width, box.y};
    cells[i].vertices[2] = (Vector2){box.x + box.width, box.y + box.height};
    cells[i].vertices[3] = (Vector2){box.x, box.y + box.height};

    for (int j = 0; j < num_vertices; j++) {
      if (i == j)
        continue;

      Line bisector =
          perpendicularBisector(vertices[i].position, vertices[j].position);

      // check for intersection of the bisector with the cell walls
      int num_intersections = 0;
      Vector2 first_intersection = {0};
      Vector2 second_intersection = {0};
      int first_intersection_index = -1;
      int second_intersection_index = -1;
      for (int k = 0; k < cells[i].num_vertices; k++) {
        int intersect;
        Segment edge = {cells[i].vertices[k],
                        cells[i].vertices[(k + 1) % cells[i].num_vertices]};
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
        cells[i].temp_vertices[0] = first_intersection;
        int m = 1;
        for (int k = first_intersection_index; k < second_intersection_index;
             k++) {
          cells[i].temp_vertices[m] = cells[i].vertices[k];
          m++;
        }
        cells[i].temp_vertices[m] = second_intersection;
        cells[i].temp_vertex_count = m + 1;

        int collide = CheckCollisionPointPoly(vertices[i].position,
                                              cells[i].temp_vertices,
                                              cells[i].temp_vertex_count);

        // check that point is inside the cell
        if (!collide) {
          // reverse the temp cell
          cells[i].temp_vertices[0] = second_intersection;
          int m = 1;
          int k = (second_intersection_index) % cells[i].num_vertices;
          while (k != (first_intersection_index) % cells[i].num_vertices) {
            cells[i].temp_vertices[m] = cells[i].vertices[k];
            k = (k + 1) % cells[i].num_vertices;
            m++;
          }

          cells[i].temp_vertices[m] = first_intersection;
          cells[i].temp_vertex_count = m + 1;
        }

        // set the cell equal to the temp cell
        cells[i].num_vertices = cells[i].temp_vertex_count;
        for (int k = 0; k < cells[i].num_vertices; k++) {
          cells[i].vertices[k] = cells[i].temp_vertices[k];
        }

        cells[i].temp_vertex_count = 0;
        cells[i].temp_vertices[0] = (Vector2){0};
      }
    }
  }
}

float PointLineDistance(Vector2 point, Vector2 lineStart, Vector2 lineEnd) {
  float num = fabs((lineEnd.y - lineStart.y) * point.x -
                   (lineEnd.x - lineStart.x) * point.y +
                   lineEnd.x * lineStart.y - lineEnd.y * lineStart.x);
  float den = sqrt((lineEnd.y - lineStart.y) * (lineEnd.y - lineStart.y) +
                   (lineEnd.x - lineStart.x) * (lineEnd.x - lineStart.x));
  return num / den;
}

Vector2 Midpoint(Vector2 a, Vector2 b) {
  return (Vector2){.x = (a.x + b.x) / 2.0, .y = (a.y + b.y) / 2.0};
}

void AssociateEdgesWithVertices(CompleteEdge edges[], int numEdges,
                                Vertex vertices[], int numVertices) {
  for (int i = 0; i < numEdges; i++) {

    Vector2 midpoint = Midpoint(edges[i].endpointA, edges[i].endpointB);
    int closestVertices[2] = {-1, -1};
    float minDistances[2] = {FLT_MAX, FLT_MAX};

    for (int j = 0; j < numVertices; j++) {
      float dx = midpoint.x - vertices[j].position.x;
      float dy = midpoint.y - vertices[j].position.y;
      float distance = sqrt(dx * dx + dy * dy);

      // Check if this vertex is one of the two closest
      if (distance < minDistances[0]) {
        // Move the first closest to second closest
        minDistances[1] = minDistances[0];
        closestVertices[1] = closestVertices[0];

        // Update the closest vertex
        minDistances[0] = distance;
        closestVertices[0] = j;
      } else if (distance < minDistances[1]) {
        minDistances[1] = distance;
        closestVertices[1] = j;
      }
    }

    // Store the indices of the two closest vertices
    edges[i].vertices[0] = closestVertices[0];
    edges[i].vertices[1] = closestVertices[1];
  }
}

int VectorExistsInPolygon(Vector2 *polygon, int polySize, Vector2 point) {
  for (int i = 0; i < polySize; i++) {
    if (Vector2Distance(polygon[i], point) <
        0.001) { // Consider a small threshold for floating point comparison
      return 1;
    }
  }
  return 0;
}

void AddPointToPolygon(Vector2 *polygon, int *polySize, Vector2 point) {
  for (int i = 0; i < *polySize; i++) {
    if (Vector2Distance(polygon[i], point) <
        0.001) { // Consider a small threshold for floating point comparison
      return;
    }
  }
  polygon[(*polySize)++] = point;
}

int FindBoundaryIntersections(Vector2 start, Vector2 end,
                              Vector2 *intersections, int screenWidth,
                              int screenHeight) {
  int count = 0;
  if (start.y != end.y) { // Avoid division by zero
    float slope = (end.x - start.x) / (end.y - start.y);
    float intersectX;

    // Top boundary (y = 0)
    intersectX = start.x + slope * (0 - start.y);
    if (intersectX >= 0 && intersectX <= screenWidth) {
      intersections[count++] = (Vector2){intersectX, 0};
    }

    // Bottom boundary (y = screenHeight)
    intersectX = start.x + slope * (screenHeight - start.y);
    if (intersectX >= 0 && intersectX <= screenWidth) {
      intersections[count++] = (Vector2){intersectX, screenHeight};
    }
  }

  if (start.x != end.x) { // Avoid division by zero
    float slope = (end.y - start.y) / (end.x - start.x);
    float intersectY;

    // Left boundary (x = 0)
    intersectY = start.y + slope * (0 - start.x);
    if (intersectY >= 0 && intersectY <= screenHeight) {
      intersections[count++] = (Vector2){0, intersectY};
    }

    // Right boundary (x = screenWidth)
    intersectY = start.y + slope * (screenWidth - start.x);
    if (intersectY >= 0 && intersectY <= screenHeight) {
      intersections[count++] = (Vector2){screenWidth, intersectY};
    }
  }
  return count;
}

void ClosePolygonOnBoundary(Vector2 *polygon, int *polySize, int screenWidth,
                            int screenHeight) {
  Vector2 newPolygon[MAX_EDGES];
  int newSize = 0;

  for (int i = 0; i < *polySize; i++) {
    Vector2 current = polygon[i];
    Vector2 next = polygon[(i + 1) % *polySize];

    current.x = CLAMP(current.x, 0, screenWidth);
    current.y = CLAMP(current.y, 0, screenHeight);

    AddPointToPolygon(newPolygon, &newSize, current);

    Vector2 intersections[4];
    int numIntersections = FindBoundaryIntersections(
        current, next, intersections, screenWidth, screenHeight);
    for (int j = 0; j < numIntersections; j++) {
      AddPointToPolygon(newPolygon, &newSize, intersections[j]);
    }
  }

  if (newSize > 0 &&
      Vector2Distance(newPolygon[newSize - 1], newPolygon[0]) > 0.001) {
    AddPointToPolygon(newPolygon, &newSize, newPolygon[0]);
  }

  memcpy(polygon, newPolygon, newSize * sizeof(Vector2));
  *polySize = newSize;
}

bool IsWithinBoundary(Vector2 point, float screenWidth, float screenHeight) {
  return (point.x >= 0 && point.x <= screenWidth && point.y >= 0 &&
          point.y <= screenHeight);
}

#define INSIDE 0 // 0000
#define LEFT 1   // 0001
#define RIGHT 2  // 0010
#define BOTTOM 4 // 0100
#define TOP 8    // 1000

// Function to compute the outcode of a point (x, y)
int ComputeOutCode(double x, double y, double screenWidth,
                   double screenHeight) {
  int code = INSIDE;

  if (x < 0) {
    code |= LEFT;
  } else if (x > screenWidth) {
    code |= RIGHT;
  }
  if (y < 0) {
    code |= BOTTOM;
  } else if (y > screenHeight) {
    code |= TOP;
  }

  return code;
}

double clamp(double val, double min, double max) {
  return val < min ? min : (val > max ? max : val);
}

typedef struct {
  Vector2 points[2];
  int count;
} IntersectionResult;

IntersectionResult IntersectBoundary(Vector2 endpointA, Vector2 endpointB,
                                     double screenWidth, double screenHeight) {

  IntersectionResult result = {{}, 0};
  double x0 = endpointA.x, y0 = endpointA.y;
  double x1 = endpointB.x, y1 = endpointB.y;
  int done = 0;

  int outcode0 = ComputeOutCode(x0, y0, screenWidth, screenHeight);
  int outcode1 = ComputeOutCode(x1, y1, screenWidth, screenHeight);

  while (!done) {
    if (!(outcode0 | outcode1)) {
      assert(IsWithinBoundary((Vector2){x0, y0}, screenWidth, screenHeight));
      assert(IsWithinBoundary((Vector2){x1, y1}, screenWidth, screenHeight));
      result.points[result.count++] = (Vector2){x0, y0};
      result.points[result.count++] = (Vector2){x1, y1};
      done = 1;
    } else if (outcode0 & outcode1) {
      // Both points share an outside zone (bitwise AND is not zero)
      done = 1;
    } else {
      // At least one endpoint is outside the viewing area
      double x, y;
      int outcodeOut = outcode0 ? outcode0 : outcode1;

      if (outcodeOut & TOP) {
        x = x0 + (x1 - x0) * (screenHeight - y0) / (y1 - y0);
        y = screenHeight;
      } else if (outcodeOut & BOTTOM) {
        x = x0 + (x1 - x0) * (-y0) / (y1 - y0);
        y = 0;
      } else if (outcodeOut & RIGHT) {
        y = y0 + (y1 - y0) * (screenWidth - x0) / (x1 - x0);
        x = screenWidth;
      } else if (outcodeOut & LEFT) {
        y = y0 + (y1 - y0) * (-x0) / (x1 - x0);
        x = 0;
      }

      x = clamp(x, 0, screenWidth);
      y = clamp(y, 0, screenHeight);

      // Replace the outside endpoint with intersection point
      if (outcodeOut == outcode0) {
        x0 = x;
        y0 = y;
        outcode0 = ComputeOutCode(x0, y0, screenWidth, screenHeight);
      } else {
        x1 = x;
        y1 = y;
        outcode1 = ComputeOutCode(x1, y1, screenWidth, screenHeight);
      }

      // if (result.count < 2) {
      //   Vector2 intersection = {x, y};
      //   result.points[result.count].x = x;
      //   result.points[result.count].y = y;
      //   assert(IsWithinBoundary(intersection, screenWidth, screenHeight));
      //   result.count++;
      // }
    }
  }
  return result;
}

Vector2 centroidGlobal;
int compare(const void *a, const void *b) {
  const Vector2 *p1 = (const Vector2 *)a;
  const Vector2 *p2 = (const Vector2 *)b;

  double a1 = atan2(p1->y - centroidGlobal.y, p1->x - centroidGlobal.x);
  double a2 = atan2(p2->y - centroidGlobal.y, p2->x - centroidGlobal.x);

  if (a1 < a2)
    return -1;
  else if (a1 > a2)
    return 1;
  else
    return 0;
}

Vector2 ComputeInitialCentroid(Vector2 *points, int n) {
  Vector2 centroid = {0, 0};
  for (int i = 0; i < n; i++) {
    centroid.x += points[i].x;
    centroid.y += points[i].y;
  }
  centroid.x /= n;
  centroid.y /= n;
  return centroid;
}

Vector2 ComputeTrueCentroid(Vector2 *polygon, int n) {
  Vector2 centroid = {0, 0};
  double signedArea = 0.0;

  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    double xi = polygon[i].x, yi = polygon[i].y;
    double xj = polygon[j].x, yj = polygon[j].y;
    double factor = xi * yj - xj * yi;
    signedArea += factor;
    double cx_term = (xi + xj) * factor;
    double cy_term = (yi + yj) * factor;
    centroid.x += cx_term;
    centroid.y += cy_term;
  }
  signedArea *= 0.5;

  if (fabs(signedArea) > 0.01) {
    centroid.x /= (6.0 * signedArea);
    centroid.y /= (6.0 * signedArea);
  } else {
    // Handle the zero area case more gracefully
    centroid.x = centroid.y = 0;
    for (int i = 0; i < n; i++) {
      centroid.x += polygon[i].x;
      centroid.y += polygon[i].y;
    }
    centroid.x /= n;
    centroid.y /= n;
  }

  return centroid;
}

void LloydRelaxationFortune(struct app_state *AppState) {
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  for (int i = 0; i < AppState->num_vertices; i++) {
    float x0, y0, x1, y1;
    Vector2 polygon[100];
    int polySize = 0;

    for (int j = 0; j < AppState->fortuneState.edgesSize; j++) {
      CompleteEdge edge = AppState->fortuneState.edges[j];
      if (edge.vertices[0] == i || edge.vertices[1] == i) {
        if (IsWithinBoundary(edge.endpointA, screenWidth, screenHeight)) {
          if (!VectorExistsInPolygon(polygon, polySize, edge.endpointA)) {
            polygon[polySize++] = edge.endpointA;
          }
        }

        if (IsWithinBoundary(edge.endpointB, screenWidth, screenHeight)) {
          if (!VectorExistsInPolygon(polygon, polySize, edge.endpointB)) {
            polygon[polySize++] = edge.endpointB;
          }
        }

        if (!IsWithinBoundary(edge.endpointA, screenWidth, screenHeight) ||
            !IsWithinBoundary(edge.endpointB, screenWidth, screenHeight)) {
          IntersectionResult intersections = IntersectBoundary(
              edge.endpointA, edge.endpointB, screenWidth, screenHeight);
          for (int k = 0; k < intersections.count; k++) {
            Vector2 intersection = intersections.points[k];
            if (intersection.x != -1 && intersection.y != -1 &&
                !VectorExistsInPolygon(polygon, polySize, intersection)) {
              polygon[polySize++] = intersection;
            }
          }
        }
      }
    }

    if (polySize == 0) {
      continue;
    }

    centroidGlobal = ComputeInitialCentroid(polygon, polySize);
    qsort(polygon, polySize, sizeof(Vector2), compare);
    Vector2 centroid = ComputeTrueCentroid(polygon, polySize);
    assert(IsWithinBoundary(centroid, screenWidth, screenHeight));
    // assert(CheckCollisionPointPoly(centroid, polygon, polySize));

    // Update vertex position towards centroid
    Vector2 direction =
        Vector2Subtract(centroid, AppState->vertices[i].position);
    direction = Vector2Scale(Vector2Normalize(direction),
                             1.0f); // Assuming a step size of 1.0f
    AppState->vertices[i].position =
        Vector2Add(AppState->vertices[i].position, direction);

    AppState->vertices[i].centroid = centroid;
  }
}

void LloydRelaxation(struct app_state *AppState) {
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  Rectangle box = {
      0,
      0,
      screenWidth,
      screenHeight,
  };

  // Calculate Voronoi Diagram
  // ComputeVoronoi(AppState->vertices, AppState->num_vertices, box,
  //                AppState->cells);
  // AppState->fortuneState = FortunesAlgorithm(AppState, -screenHeight);

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
}

float GetArcYForXCoordddd(Vector2 focus, float x, float directrixY) {
  // NOTE: In the interest of keeping the formula simple when moving away from
  // the origin,
  //       we'll use the substitution from (x,y) -> (w,y) = (x-focusX,y).
  //       In particular this substitution means that the formula always has the
  //       form: y = aw^2 + c, the linear term's coefficient is always 0.
  float a = 1.0f / (4.0f * (focus.y - directrixY));
  float c = (focus.y + directrixY) * 0.5f;

  float w = x - focus.x;
  return a * w * w + c;
}

void DrawParabola(Vector2 focus, float directrixY, Color color, int screenWidth,
                  int screenHeight, Vector2 *curvePts) {
  float x = 0.0f;
  for (int i = 0; i < screenWidth; i++) {
    // Change of variables from (x,y) to (w,y) for simplicity of expression
    curvePts[i].x = x;
    curvePts[i].y = GetArcYForXCoordddd(focus, x, directrixY);

    // Y increases downwards in screencoords, so flip each point around the
    // x-axis. This is just so that it is closer to what we usually get/expect
    // in mathematics.
    curvePts[i].y = screenHeight - curvePts[i].y;
    x += 1.0f;
  }

  for (int i = 1; i < screenWidth; i++) {
    DrawLineV(curvePts[i - 1], curvePts[i], color);
  }
}

APP_PLUG int plug_update(struct app_memory *Memory) {
  ASSERT(sizeof(struct app_state) <= Memory->PermanentStorageSize);

  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  struct app_state *AppState = (struct app_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized) {
    printf("starting app state initilization\n");
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;

    AppState->num_vertices = 25 + 1;

    // float scatterW = screenWidth / 2.0f - 10;
    // float scatterH = screenHeight / 2.0f - 10;
    float scatterW = 100;
    float scatterH = 100;

    Vector2 initialPoints[11] = {{275, 99},  {715, 118}, {88, 196},  {497, 177},
                                 {261, 278}, {673, 299}, {458, 358}, {211, 419},
                                 {624, 463}, {155, 552}, {405, 552}};

    // Vector2 initialPoints[13] = {{158.0, 100.0}, {350.0, 514.0},
    // {272.0, 79.0},
    //                              {488.0, 206.0}, {443.0, 495.0}, {398.0,
    //                              198.0}, {236.0, 96.0},  {576.0, 339.0},
    //                              {210.0, 441.0}, {643.0, 85.0},  {369.0,
    //                              502.0}, {516.0, 237.0}, {400, 1500}};

    for (int i = 0; i < AppState->num_vertices; i++) {
      AppState->vertices[i].position =
          (Vector2){centerX + GetRandomValue(-scatterW, scatterW),
                    centerY + GetRandomValue(-scatterH, scatterH)};
      // AppState->vertices[i].velocity =
      //     (Vector2){GetRandomValue(-1, 1), GetRandomValue(-1, 1)};
      // AppState->vertices[i].position = initialPoints[i];
      AppState->vertices[i].color =
          (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                  GetRandomValue(0, 255), 255};
      AppState->vertices[i].centroid = (Vector2){0};
    }

    AppState->vertices[AppState->num_vertices].position.x = 400;
    AppState->vertices[AppState->num_vertices].position.y = 1500;
    AppState->vertices[AppState->num_vertices].color =
        (Color){GetRandomValue(0, 255), GetRandomValue(0, 255),
                GetRandomValue(0, 255), 255};
    AppState->vertices[AppState->num_vertices].centroid = (Vector2){0};

    AppState->fortuneState.edgesSize = 0;
    AppState->fortuneState = FortunesAlgorithm(AppState, -screenHeight);
    AssociateEdgesWithVertices(AppState->fortuneState.edges,
                               AppState->fortuneState.edgesSize,
                               AppState->vertices, AppState->num_vertices);
    LloydRelaxationFortune(AppState);

    AppState->glowShader = LoadShader(0, "shaders/glow.fs");
    SetShaderValue(AppState->glowShader,
                   GetShaderLocation(AppState->glowShader, "resolution"),
                   (float[2]){screenWidth, screenHeight}, SHADER_UNIFORM_VEC2);
    Memory->IsInitialized = 1;

    printf("Initialized app state\n");
  }

  BeginDrawing();
  ClearBackground(BLACK);

  int numLines = AppState->fortuneState.edgesSize;
  Vector2 lineA[1000];
  Vector2 lineB[1000];

  for (int i = 0; i < AppState->fortuneState.edgesSize; i++) {
    // lineA[i] = AppState->fortuneState.edges[i].endpointA;
    // lineB[i] = AppState->fortuneState.edges[i].endpointB;
    lineA[i].x = AppState->fortuneState.edges[i].endpointA.x;
    lineA[i].y = screenHeight - AppState->fortuneState.edges[i].endpointA.y;
    lineB[i].x = AppState->fortuneState.edges[i].endpointB.x;
    lineB[i].y = screenHeight - AppState->fortuneState.edges[i].endpointB.y;
    // int indexVertex = AppState->fortuneState.edges[i].vertices[0];
    // Vector2 pointA = AppState->fortuneState.edges[i].endpointA;
    // Vector2 pointB = AppState->fortuneState.edges[i].endpointB;

    // SetShaderValue(AppState->glowShader,
    //                GetShaderLocation(AppState->glowShader, "lineA"),
    //                (float[2]){pointA.x, pointA.y}, SHADER_UNIFORM_VEC2);
    // SetShaderValue(AppState->glowShader,
    //                GetShaderLocation(AppState->glowShader, "lineB"),
    //                (float[2]){pointB.x, pointB.y}, SHADER_UNIFORM_VEC2);
  }
  // BeginShaderMode(AppState->glowShader);

  // DrawLineV(pointA, pointB, BLACK);
  // EndShaderMode();
  //
  SetShaderValueV(AppState->glowShader,
                  GetShaderLocation(AppState->glowShader, "lineA"), lineA,
                  SHADER_UNIFORM_VEC2, numLines);
  SetShaderValueV(AppState->glowShader,
                  GetShaderLocation(AppState->glowShader, "lineB"), lineB,
                  SHADER_UNIFORM_VEC2, numLines);
  SetShaderValue(AppState->glowShader,
                 GetShaderLocation(AppState->glowShader, "numLines"), &numLines,
                 SHADER_UNIFORM_INT);

  float timeValue = GetTime();
  SetShaderValue(AppState->glowShader,
                 GetShaderLocation(AppState->glowShader, "time"), &timeValue,
                 SHADER_UNIFORM_FLOAT);

  BeginShaderMode(AppState->glowShader);

  // Draw a fullscreen rectangle to apply the shadeAppState->glowShaderr
  DrawRectangle(0, 0, screenWidth, screenHeight, BLANK);

  // End using the shader
  EndShaderMode();

  for (int i = 0; i < AppState->num_vertices; i++) {
    // DrawCircleV(AppState->vertices[i].position, 5, WHITE);
    // DrawCircleV(AppState->vertices[i].centroid, 5,
    // AppState->vertices[i].color);
  }

  DrawFPS(10, 10);

  EndDrawing();

  AppState->fortuneState = FortunesAlgorithm(AppState, -screenHeight);
  AssociateEdgesWithVertices(AppState->fortuneState.edges,
                             AppState->fortuneState.edgesSize,
                             AppState->vertices, AppState->num_vertices);
  LloydRelaxationFortune(AppState);

  return 1;
}
