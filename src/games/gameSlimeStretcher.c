#include "cglp.h"
#include <string.h>

static char *title = "SLIME STRETCHER";
static char *description = "[Hold] Stretch\n[Release] Contract";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "l  lll",
    "l llll",
    "l llll",
    "llll l",
    " llll ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float width;
  float height;
  float baseWidth;
  float baseHeight;
  float maxHeight;
  float velocity;
  bool isOnGround;
} Slime;
static Slime slime;

typedef struct {
  Vector pos;
  float width;
  float height;
  bool isAlive;
} Wall;
#define MAX_WALL_COUNT 32
static Wall walls[MAX_WALL_COUNT];
static int wallIndex;

typedef struct {
  Vector pos;
  bool isAlive;
} Collectible;
#define MAX_COLLECTIBLE_COUNT 32
static Collectible collectibles[MAX_COLLECTIBLE_COUNT];
static int collectibleIndex;

static float scrollVelocity;
static float nextWallDistance;
static float nextCollectibleDistance;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&slime.pos, 50, 90);
    slime.width = 20;
    slime.height = 20;
    slime.baseWidth = 20;
    slime.baseHeight = 20;
    slime.maxHeight = 80;
    slime.velocity = 1;
    slime.isOnGround = true;
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    INIT_UNALIVED_ARRAY(collectibles);
    collectibleIndex = 0;
    scrollVelocity = 1;
    nextWallDistance = 50;
    nextCollectibleDistance = 30;
    multiplier = 1;
  }
  if (input.isPressed) {
    slime.height = fmin(slime.height + 2, slime.maxHeight);
    slime.width = fmax(slime.width - 0.5, slime.baseWidth / 2);
    slime.velocity = 0.5 * difficulty;
    slime.pos.y = fmax(slime.pos.y - 2, 10);
  } else {
    slime.height = fmax(slime.height - 4, slime.baseHeight);
    slime.width = fmin(slime.width + 1, slime.baseWidth);
    slime.velocity = 1.5 * difficulty;
  }
  slime.pos.x += slime.velocity;
  if (slime.pos.x < 50) {
    slime.pos.x++;
  }
  if (slime.pos.x < -slime.width) {
    play(EXPLOSION);
    gameOver();
  }

  nextWallDistance -= slime.velocity;
  if (nextWallDistance <= 0) {
    float wallWidth = rnd(20, 60);
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, nw);
    vectorSet(&nw->pos, 200, rnd(0, 70));
    nw->width = wallWidth;
    nw->height = rnd(20, 40);
    nw->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, MAX_WALL_COUNT);
    nextWallDistance = rndi(35, 55) + wallWidth;
  }

  nextCollectibleDistance -= slime.velocity;
  if (nextCollectibleDistance <= 0) {
    ASSIGN_ARRAY_ITEM(collectibles, collectibleIndex, Collectible, nc);
    vectorSet(&nc->pos, 203, rnd(20, 80));
    nc->isAlive = true;
    collectibleIndex = wrap(collectibleIndex + 1, 0, MAX_COLLECTIBLE_COUNT);
    nextCollectibleDistance = rndi(30, 60);
  }

  slime.isOnGround = false;
  FOR_EACH(walls, wi) {
    ASSIGN_ARRAY_ITEM(walls, wi, Wall, wall);
    SKIP_IS_NOT_ALIVE(wall);
    if (slime.pos.x + slime.width > wall->pos.x && slime.pos.x < wall->pos.x + wall->width &&
        slime.pos.y + slime.height > wall->pos.y && slime.pos.y < wall->pos.y + wall->height) {
      if (slime.pos.y + slime.height < wall->pos.y + 10) {
        slime.height--;
        slime.pos.y = wall->pos.y - slime.height;
        slime.isOnGround = true;
      } else if (slime.pos.y > wall->pos.y + wall->height - 10) {
        slime.pos.y = wall->pos.y + wall->height;
        slime.height -= 2;
      } else if (slime.pos.x + slime.width < wall->pos.x + 5) {
        slime.pos.x = wall->pos.x - slime.width;
        slime.width--;
      } else {
        slime.pos.y = wall->pos.y - slime.height;
        slime.isOnGround = true;
      }
    }
  }
  if (!slime.isOnGround) {
    slime.pos.y = fmin(slime.pos.y + 1, 90);
  }
  if (slime.pos.y + slime.height >= 90) {
    slime.pos.y = 90 - slime.height;
    slime.isOnGround = true;
  }

  color = GREEN;
  rect(slime.pos.x, slime.pos.y, slime.width, slime.height);
  color = BLACK;
  FOR_EACH(walls, wi2) {
    ASSIGN_ARRAY_ITEM(walls, wi2, Wall, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    rect(obstacle->pos.x, obstacle->pos.y, obstacle->width, obstacle->height);
  }
  rect(0, 90, 200, 10);

  scrollVelocity = slime.velocity;
  slime.pos.x -= scrollVelocity;
  FOR_EACH(walls, wi3) {
    ASSIGN_ARRAY_ITEM(walls, wi3, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.x -= scrollVelocity;
    if (w->pos.x < -w->width) {
      w->isAlive = false;
      continue;
    }
  }

  color = YELLOW;
  FOR_EACH(collectibles, ci) {
    ASSIGN_ARRAY_ITEM(collectibles, ci, Collectible, c);
    SKIP_IS_NOT_ALIVE(c);
    Collision cl = character("a", c->pos.x, c->pos.y);
    if (cl.isColliding.rect[BLACK]) {
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.rect[GREEN]) {
      play(COIN);
      addScore(multiplier, c->pos.x, c->pos.y);
      multiplier++;
      c->isAlive = false;
      continue;
    }
    c->pos.x -= scrollVelocity;
    if (c->pos.x < -3) {
      multiplier--;
      if (multiplier < 1) {
        multiplier = 1;
      }
      c->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  // Upstream draws this with an `isSmallText` option this port doesn't
  // support (no small-font variant exists here) - drawn at normal text size
  // instead (visual only).
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 2, 10);
}

void addGameSlimeStretcher() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
