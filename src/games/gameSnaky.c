#include "cglp.h"

static char *title = "SNAKY";
static char *description = "[Hold] Up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "ll lll",
    "l llll",
    "llllll",
    "llllll",
    " llll ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

typedef struct {
  float angle;
  float va;
} Node;
#define MAX_NODE_COUNT 10
static Node nodes[MAX_NODE_COUNT];
static int nodeCount;

typedef struct {
  Vector pos;
  float vx;
  bool isRed;
  bool isAlive;
} Item;
#define MAX_ITEM_COUNT 32
static Item items[MAX_ITEM_COUNT];
static int itemIndex;
static float nextItemTicks;
static int nextRedItemCount;
static float nextItemY;

static void addNode() {
  if (nodeCount > 9) {
    return;
  }
  nodes[nodeCount].angle = 0;
  nodes[nodeCount].va = 0;
  nodeCount++;
}

static void removeNode() {
  nodeCount--;
}

static void update() {
  if (!ticks) {
    nodeCount = 1;
    nodes[0].angle = 0;
    nodes[0].va = 0;
    addNode();
    INIT_UNALIVED_ARRAY(items);
    itemIndex = 0;
    nextItemTicks = 0;
    nextRedItemCount = 2;
    nextItemY = 50;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  float nl = (50 + nodeCount * 3) / (float)nodeCount;
  float na = M_PI / (sqrt(nodeCount - 1) + 3);
  Vector p;
  vectorSet(&p, 0, 50);
  TIMES(nodeCount, i) {
    Node *n = &nodes[i];
    if (i == 0) {
      float pa = n->angle;
      float dir;
      if (input.isPressed) {
        dir = -1;
      } else {
        dir = 1;
      }
      n->angle = clamp(n->angle + dir * 0.03 * sqrt(difficulty), -na, na);
      n->va = n->angle - pa;
    } else {
      Node *prevN = &nodes[i - 1];
      n->angle += n->va;
      if ((n->angle > prevN->angle + na && n->va > 0) ||
          (n->angle < prevN->angle - na && n->va < 0)) {
        n->va *= -0.2;
      }
    }
    n->va *= 0.95;
    if (i < nodeCount - 1) {
      Node *nextN = &nodes[i + 1];
      nextN->va += n->va * 0.07 * sqrt(sqrt(difficulty));
    }
    if (i > 0) {
      Node *prevN2 = &nodes[i - 1];
      prevN2->va += n->va * 0.01 * sqrt(sqrt(difficulty));
    }
    if (i == nodeCount - 1) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(p.x, p.y, nl, n->angle);
    addWithAngle(&p, n->angle, nl);
  }
  nextItemTicks--;
  if (nextItemTicks < 0) {
    bool isRed = false;
    nextRedItemCount--;
    if (nextRedItemCount < 0) {
      isRed = true;
      nextRedItemCount = 7;
    }
    ASSIGN_ARRAY_ITEM(items, itemIndex, Item, it);
    vectorSet(&it->pos, 103, nextItemY);
    it->vx = -rnd(1, difficulty) * 0.1;
    it->isRed = isRed;
    it->isAlive = true;
    itemIndex = wrap(itemIndex + 1, 0, MAX_ITEM_COUNT);
    float ny = rnd(20, 80);
    nextItemTicks = (fabs(nextItemY - ny + 10) * 4) / sqrt(difficulty);
    nextItemY = ny;
  }
  FOR_EACH(items, idx) {
    ASSIGN_ARRAY_ITEM(items, idx, Item, it);
    SKIP_IS_NOT_ALIVE(it);
    it->vx *= 1 + 0.005 * sqrt(difficulty);
    it->pos.x += it->vx;
    if (it->isRed) {
      color = RED;
    } else {
      color = BLACK;
    }
    Collision cc = character("a", it->pos.x, it->pos.y);
    float sc = 0;
    if (cc.isColliding.rect[RED]) {
      if (it->isRed) {
        play(POWER_UP);
        sc = nodeCount * nodeCount;
        addNode();
      } else {
        play(COIN);
        sc = nodeCount;
      }
    } else if (cc.isColliding.rect[BLACK]) {
      if (it->isRed) {
        play(HIT);
        sc = -nodeCount;
        removeNode();
      } else {
        play(LASER);
        sc = 1;
      }
    }
    if (sc != 0) {
      addScore(sc, it->pos.x, it->pos.y);
      it->isAlive = false;
      continue;
    }
    if (it->pos.x < 0) {
      play(EXPLOSION);
      color = RED;
      text("X", 3, it->pos.y);
      gameOver();
    }
  }
}

void addGameSnaky() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
