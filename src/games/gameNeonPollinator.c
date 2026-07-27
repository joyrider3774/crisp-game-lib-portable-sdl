#include "cglp.h"

static char *title = "NEON POLLINATOR";
static char *description = "[Hold] Glow & Rise";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = true};

typedef struct {
  Vector pos;
  float energy;
  float glowRadius;
} NeonPollinatorPlayer;
static NeonPollinatorPlayer player;

typedef struct {
  Vector pos;
  float size;
  bool isPollinated;
  float glowRadius;
  bool isAlive;
} NeonPollinatorFlower;
#define NEONPOLLINATOR_MAX_FLOWER_COUNT 16
static NeonPollinatorFlower flowers[NEONPOLLINATOR_MAX_FLOWER_COUNT];
static int flowerIndex;
static float nextFlowerDist;

typedef struct {
  Vector pos;
  bool isAlive;
} NeonPollinatorOrb;
#define NEONPOLLINATOR_MAX_ORB_COUNT 16
static NeonPollinatorOrb orbs[NEONPOLLINATOR_MAX_ORB_COUNT];
static int orbIndex;
static float nextOrbDist;

static float gardenScrollSpeed;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 20, 50);
    player.energy = 100;
    player.glowRadius = 5;
    INIT_UNALIVED_ARRAY(flowers);
    flowerIndex = 0;
    nextFlowerDist = 0;
    INIT_UNALIVED_ARRAY(orbs);
    orbIndex = 0;
    nextOrbDist = 0;
    gardenScrollSpeed = 1;
    multiplier = 1;
  }
  gardenScrollSpeed = difficulty * 0.5;
  if (input.isJustPressed) {
    multiplier = 1;
    if (player.energy > 0) {
      play(LASER);
    } else {
      play(HIT);
    }
  }
  if (input.isPressed && player.energy > 0) {
    float gr = player.glowRadius + 0.25 * difficulty;
    if (gr > 25) {
      gr = 25;
    }
    player.glowRadius = gr;
    player.pos.y -= difficulty;
    player.energy -= 0.5 * difficulty;
  } else {
    float gr = player.glowRadius - 0.5 * difficulty;
    if (gr < 3) {
      gr = 3;
    }
    player.glowRadius = gr;
    player.pos.y += 0.7 * difficulty;
    player.energy -= 0.1 * difficulty;
  }
  player.energy = clamp(player.energy, 0, 100);
  if (player.glowRadius > 3) {
    color = YELLOW;
    thickness = 3;
    arc(player.pos.x, player.pos.y, player.glowRadius, 0, M_PI * 2);
  }
  color = CYAN;
  thickness = 3;
  arc(player.pos.x, player.pos.y, 3, 0, M_PI * 2);
  if (player.pos.y < 0 || player.pos.y > 100) {
    play(EXPLOSION);
    gameOver();
  }
  nextFlowerDist -= gardenScrollSpeed;
  if (nextFlowerDist < 0) {
    ASSIGN_ARRAY_ITEM(flowers, flowerIndex, NeonPollinatorFlower, nf);
    vectorSet(&nf->pos, 115, rnd(10, 90));
    nf->size = rnd(5, 15);
    nf->isPollinated = false;
    nf->glowRadius = 0;
    nf->isAlive = true;
    flowerIndex = wrap(flowerIndex + 1, 0, NEONPOLLINATOR_MAX_FLOWER_COUNT);
    nextFlowerDist += 20;
  }
  FOR_EACH(flowers, i) {
    ASSIGN_ARRAY_ITEM(flowers, i, NeonPollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= gardenScrollSpeed;
    if (f->pos.x < -10) {
      f->isAlive = false;
    }
  }
  FOR_EACH(flowers, i) {
    ASSIGN_ARRAY_ITEM(flowers, i, NeonPollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    if (f->isPollinated && f->glowRadius > 0) {
      color = YELLOW;
      thickness = 1;
      arc(f->pos.x, f->pos.y, f->glowRadius, 0, M_PI * 2);
    }
  }
  FOR_EACH(flowers, i) {
    ASSIGN_ARRAY_ITEM(flowers, i, NeonPollinatorFlower, f);
    SKIP_IS_NOT_ALIVE(f);
    if (f->isPollinated) {
      color = YELLOW;
    } else {
      color = LIGHT_PURPLE;
    }
    thickness = 3;
    Collision fCol = arc(f->pos.x, f->pos.y, f->size, 0, M_PI * 2);
    if (fCol.isColliding.rect[YELLOW] && !f->isPollinated) {
      f->isPollinated = true;
      f->glowRadius = f->size * 3;
      play(SELECT);
      addScore(multiplier, f->pos.x, f->pos.y);
      multiplier++;
    }
    if (f->isPollinated) {
      float gr2 = f->glowRadius - 0.2 * difficulty;
      if (gr2 < 0) {
        gr2 = 0;
      }
      f->glowRadius = gr2;
    }
  }
  nextOrbDist -= gardenScrollSpeed;
  if (nextOrbDist < 0) {
    ASSIGN_ARRAY_ITEM(orbs, orbIndex, NeonPollinatorOrb, no);
    vectorSet(&no->pos, 100, rnd(10, 90));
    no->isAlive = true;
    orbIndex = wrap(orbIndex + 1, 0, NEONPOLLINATOR_MAX_ORB_COUNT);
    nextOrbDist += rnd(16, 20);
  }
  FOR_EACH(orbs, i) {
    ASSIGN_ARRAY_ITEM(orbs, i, NeonPollinatorOrb, o);
    SKIP_IS_NOT_ALIVE(o);
    bool isVisible = false;
    FOR_EACH(flowers, j) {
      ASSIGN_ARRAY_ITEM(flowers, j, NeonPollinatorFlower, f);
      SKIP_IS_NOT_ALIVE(f);
      if (f->isPollinated && distanceTo(&o->pos, f->pos.x, f->pos.y) < f->glowRadius) {
        isVisible = true;
      }
    }
    if (distanceTo(&o->pos, player.pos.x, player.pos.y) < player.glowRadius) {
      isVisible = true;
    }
    if (isVisible) {
      color = CYAN;
    } else {
      color = TRANSPARENT;
    }
    Collision oCol = box(o->pos.x, o->pos.y, 5, 5);
    if (oCol.isColliding.rect[CYAN]) {
      float ne = player.energy + 20;
      if (ne > 100) {
        ne = 100;
      }
      player.energy = ne;
      color = CYAN;
      particle(o->pos.x, o->pos.y, 9, 2, 0, M_PI * 2);
      play(COIN);
      o->isAlive = false;
      continue;
    }
    o->pos.x -= gardenScrollSpeed;
    if (o->pos.x < -10) {
      o->isAlive = false;
      continue;
    }
  }
  color = LIGHT_YELLOW;
  rect(0, 97, player.energy, 3);
  int barColor;
  if (player.energy < 30) {
    if (ticks % 20 < 10) {
      barColor = RED;
    } else {
      barColor = TRANSPARENT;
    }
  } else {
    barColor = LIGHT_BLACK;
  }
  color = barColor;
  rect(player.energy, 97, 100 - player.energy, 3);
}

void addGameNeonPollinator() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
