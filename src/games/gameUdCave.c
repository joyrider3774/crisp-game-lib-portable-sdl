#include "cglp.h"
#include "string.h"

static char *title = "UD CAVE";
static char *description = "[Hold] Go right";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " l    ",
    "lll   ",
    " l    ",
    "l l   ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = true};

#define UDCAVE_WALL_HEIGHT 10

typedef struct {
  float x;
  float vx;
  float w;
  float vw;
} Cave;
#define UDCAVE_CAVE_COUNT 3
static Cave caves[UDCAVE_CAVE_COUNT];

typedef struct {
  Vector pos;
  float width;
  float vy;
  bool isAlive;
} Wall;
#define UDCAVE_MAX_WALL_COUNT 64
static Wall walls[UDCAVE_MAX_WALL_COUNT];
static int wallIndex;
static float nextWallDist;

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} Gold;
#define UDCAVE_MAX_GOLD_COUNT 32
static Gold golds[UDCAVE_MAX_GOLD_COUNT];
static int goldIndex;
static float nextGoldDist;

static float playerX;
static int multiplier;

static void update() {
  if (!ticks) {
    playerX = 0;
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    nextWallDist = -50;
    TIMES(UDCAVE_CAVE_COUNT, i) {
      caves[i].x = 0;
      caves[i].vx = 0;
      if (i > 0) {
        caves[i].w = 15;
      } else {
        caves[i].w = 20;
      }
      caves[i].vw = 0;
    }
    INIT_UNALIVED_ARRAY(golds);
    goldIndex = 0;
    nextGoldDist = 5;
    multiplier = 1;
  }
  float vy = difficulty;
  nextWallDist -= vy;
  if (nextWallDist < 0) {
    addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    TIMES(UDCAVE_CAVE_COUNT, i) {
      Cave *c = &caves[i];
      c->vx += rnd(0, 2) * RNDPM() * sqrt(difficulty);
      c->vw += rnd(0, 1) * RNDPM() * sqrt(difficulty);
      c->x += c->vx;
      c->w += c->vw;
      float minX, maxX;
      if (i == 0) {
        minX = -(17 - 7 / sqrt(difficulty));
        maxX = 17 - 7 / sqrt(difficulty);
      } else {
        minX = caves[0].x - caves[0].w;
        maxX = caves[0].x + caves[0].w;
      }
      if ((c->x - c->w < minX && c->vx < 0) || (c->x + c->w > maxX && c->vx > 0)) {
        c->vx *= -0.5;
        c->x += c->vx;
      }
      float minW, maxW;
      if (i == 0) {
        minW = 5 + 5 / sqrt(difficulty);
        maxW = 7 + 7 / sqrt(difficulty);
      } else {
        minW = caves[0].w;
        maxW = 9 + 9 / sqrt(difficulty);
      }
      if ((c->w < minW && c->vw < 0) || (c->w > maxW && c->vw > 0)) {
        c->vw *= -0.5;
        c->w += c->vw;
      }
    }
    Cave *c1 = &caves[1];
    float x11 = c1->x - c1->w + 25;
    float x12 = c1->x + c1->w + 25;
    if (x11 > 0) {
      ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w1);
      vectorSet(&w1->pos, x11, -nextWallDist);
      w1->width = -x11;
      w1->vy = 1;
      w1->isAlive = true;
      wallIndex = wrap(wallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    if (x12 < 50) {
      ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w2);
      vectorSet(&w2->pos, x12, -nextWallDist);
      w2->width = 50 - x12;
      w2->vy = 1;
      w2->isAlive = true;
      wallIndex = wrap(wallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    Cave *c2 = &caves[2];
    float x21 = 75 - c2->x - c2->w;
    float x22 = 75 - c2->x + c2->w;
    if (x21 > 50) {
      ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w3);
      vectorSet(&w3->pos, x21, 100 + nextWallDist);
      w3->width = 50 - x21;
      w3->vy = -1;
      w3->isAlive = true;
      wallIndex = wrap(wallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    if (x22 < 100) {
      ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w4);
      vectorSet(&w4->pos, x22, 100 + nextWallDist);
      w4->width = 100 - x22;
      w4->vy = -1;
      w4->isAlive = true;
      wallIndex = wrap(wallIndex + 1, 0, UDCAVE_MAX_WALL_COUNT);
    }
    nextGoldDist--;
    if (nextGoldDist < 0) {
      if (rnd(0, 1) < 0.5) {
        ASSIGN_ARRAY_ITEM(golds, goldIndex, Gold, ng);
        vectorSet(&ng->pos, caves[1].x + rnd(0, caves[1].w * 0.8) * RNDPM() + 25,
                  -nextWallDist - UDCAVE_WALL_HEIGHT / 2);
        ng->vy = 1;
        ng->isAlive = true;
        goldIndex = wrap(goldIndex + 1, 0, UDCAVE_MAX_GOLD_COUNT);
      } else {
        ASSIGN_ARRAY_ITEM(golds, goldIndex, Gold, ng2);
        vectorSet(&ng2->pos, 75 - caves[2].x + rnd(0, caves[2].w * 0.8) * RNDPM(),
                  100 + nextWallDist + UDCAVE_WALL_HEIGHT / 2);
        ng2->vy = -1;
        ng2->isAlive = true;
        goldIndex = wrap(goldIndex + 1, 0, UDCAVE_MAX_GOLD_COUNT);
      }
      nextGoldDist = rnd(3, 9);
    }
    nextWallDist += UDCAVE_WALL_HEIGHT;
  }
  color = RED;
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y += w->vy * vy;
    rect(w->pos.x, w->pos.y, w->width, (UDCAVE_WALL_HEIGHT - 1) * -w->vy);
    bool removed;
    if (w->vy > 0) {
      removed = w->pos.y > 100 + UDCAVE_WALL_HEIGHT;
    } else {
      removed = w->pos.y < -UDCAVE_WALL_HEIGHT;
    }
    w->isAlive = !removed;
  }
  float pdir;
  if (input.isPressed) {
    pdir = 1;
  } else {
    pdir = -1;
  }
  playerX = clamp(playerX + pdir * difficulty * 0.5, -25, 25);
  if (input.isJustPressed) {
    play(SELECT);
  } else if (input.isJustReleased) {
    play(LASER);
  }
  color = BLACK;
  Collision c1coll = character("a", playerX + 25, 90);
  Collision c2coll = character("a", 75 - playerX, 10);
  if (c1coll.isColliding.rect[RED] || c2coll.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(golds, i) {
    ASSIGN_ARRAY_ITEM(golds, i, Gold, g);
    SKIP_IS_NOT_ALIVE(g);
    g->pos.y += g->vy * vy;
    Collision gc = text("$", g->pos.x, g->pos.y);
    if (gc.isColliding.rect[RED]) {
      g->isAlive = false;
      continue;
    }
    if (gc.isColliding.character['a']) {
      play(POWER_UP);
      multiplier++;
      g->isAlive = false;
      continue;
    }
    bool removed;
    if (g->vy > 0) {
      removed = g->pos.y > 103;
    } else {
      removed = g->pos.y < -3;
    }
    g->isAlive = !removed;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameUdCave() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
