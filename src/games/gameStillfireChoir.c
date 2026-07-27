#include "cglp.h"

static char *title = "STILLFIRE CHOIR";
static char *description = "[Hold] Fix choir\nAuto-ray on pulse";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vx;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float vx;
  int spawnTicks;
  bool isAlive;
} Ally;
// Allies never expire (unbounded growth): item spawn rate integrates to
// ~220 items by ~5min and ~570 by ~10min of play (interval ~150/sqrt(difficulty),
// difficulty = ticks/3600+1), and a chunk of those get caught since allies are
// the score multiplier - 128 can wrap during a single good run, raised to 512.
#define STILLFIRECHOIR_MAX_ALLY_COUNT 512
static Ally allies[STILLFIRECHOIR_MAX_ALLY_COUNT];
static int allyIndex;

typedef struct {
  Vector pos;
  float speed;
  float size;
  bool isAlive;
} Enemy;
#define STILLFIRECHOIR_MAX_ENEMY_COUNT 32
static Enemy enemies[STILLFIRECHOIR_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  float speed;
  bool isAlive;
} Item;
#define STILLFIRECHOIR_MAX_ITEM_COUNT 16
static Item items[STILLFIRECHOIR_MAX_ITEM_COUNT];
static int itemIndex;

static float nextEnemyTicks;
static float nextItemTicks;
static float nextVolleyTicks;
static float volleySpan;
static float beamTicks;
static float beamAfterglowTicks;
static float lockPulseTicks;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 88);
    player.vx = 0.72;
    INIT_UNALIVED_ARRAY(allies);
    allyIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(items);
    itemIndex = 0;
    nextEnemyTicks = 25;
    nextItemTicks = 75;
    volleySpan = 62;
    nextVolleyTicks = volleySpan;
    beamTicks = 0;
    beamAfterglowTicks = 0;
    lockPulseTicks = 0;
  }

  float pressure = sqrt(difficulty);

  if (input.isJustPressed) {
    lockPulseTicks = 5;
    play(SELECT);
  }

  if (!input.isPressed) {
    player.pos.x += player.vx * pressure;
    if (player.pos.x < 5 || player.pos.x > 95) {
      player.pos.x = clamp(player.pos.x, 5, 95);
      player.vx *= -1;
      color = LIGHT_BLUE;
      float bounceAngle = 0;
      if (player.vx <= 0) {
        bounceAngle = M_PI;
      }
      particle(player.pos.x, player.pos.y, 3, 0.8, bounceAngle, M_PI / 3);
      play(CLICK);
    }
    FOR_EACH(allies, ai) {
      ASSIGN_ARRAY_ITEM(allies, ai, Ally, ally);
      SKIP_IS_NOT_ALIVE(ally);
      ally->pos.x += ally->vx * pressure;
      if (ally->pos.x < 4 || ally->pos.x > 96) {
        ally->pos.x = clamp(ally->pos.x, 4, 96);
        ally->vx *= -1;
        color = LIGHT_BLUE;
        float allyBounceAngle = 0;
        if (ally->vx <= 0) {
          allyBounceAngle = M_PI;
        }
        particle(ally->pos.x, ally->pos.y, 2, 0.7, allyBounceAngle, M_PI / 3);
      }
    }
  }

  nextVolleyTicks--;
  if (nextVolleyTicks <= 0) {
    beamTicks = 1;
    // Four includes the live beam frame, leaving three visual-only frames.
    beamAfterglowTicks = 4;
    volleySpan = fmax(36, 62 / pressure);
    nextVolleyTicks = volleySpan;
    play(LASER);
  }

  nextEnemyTicks--;
  if (nextEnemyTicks <= 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, rnd(5, 95), -4);
    ne->speed = rnd(0.45, 0.78);
    ne->size = rnd(5, 9);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, STILLFIRECHOIR_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(31, 49) / pressure;
  }

  nextItemTicks--;
  if (nextItemTicks <= 0) {
    ASSIGN_ARRAY_ITEM(items, itemIndex, Item, ni);
    vectorSet(&ni->pos, rnd(7, 93), -3);
    ni->speed = rnd(0.36, 0.52);
    ni->isAlive = true;
    itemIndex = wrap(itemIndex + 1, 0, STILLFIRECHOIR_MAX_ITEM_COUNT);
    nextItemTicks = rnd(125, 175) / pressure;
  }

  color = BLUE;
  box(VEC_XY(player.pos), 7, 7);
  FOR_EACH(allies, ai2) {
    ASSIGN_ARRAY_ITEM(allies, ai2, Ally, ally2);
    SKIP_IS_NOT_ALIVE(ally2);
    box(VEC_XY(ally2->pos), 5, 5);
  }

  // The emitters themselves are the volley gauge: their cores fill upward
  // as the next synchronized ray approaches. Keep the blue body unchanged
  // so the visual charge never alters its collision footprint.
  float volleyCharge = clamp(1 - nextVolleyTicks / volleySpan, 0, 1);
  bool isChargeFlickering = nextVolleyTicks < 10 && (ticks / 2) % 2 == 0;
  if (isChargeFlickering) {
    color = LIGHT_BLUE;
  } else {
    color = LIGHT_CYAN;
  }
  float playerChargeHeight = 5 * volleyCharge;
  rect(player.pos.x - 1.5, player.pos.y + 2.5 - playerChargeHeight, 3, playerChargeHeight);
  FOR_EACH(allies, ai3) {
    ASSIGN_ARRAY_ITEM(allies, ai3, Ally, ally3);
    SKIP_IS_NOT_ALIVE(ally3);
    float allyChargeHeight = 3 * volleyCharge;
    rect(ally3->pos.x - 1, ally3->pos.y + 1.5 - allyChargeHeight, 2, allyChargeHeight);
  }

  // A brief ring makes the exact hold edge tactile without enlarging or
  // deforming the collision bodies.
  if (lockPulseTicks > 0) {
    color = LIGHT_CYAN;
    thickness = 1;
    arc(player.pos.x, player.pos.y, 4 + lockPulseTicks * 0.35, 0, M_PI * 2);
    FOR_EACH(allies, ai4) {
      ASSIGN_ARRAY_ITEM(allies, ai4, Ally, ally4);
      SKIP_IS_NOT_ALIVE(ally4);
      thickness = 1;
      arc(ally4->pos.x, ally4->pos.y, 3 + lockPulseTicks * 0.25, 0, M_PI * 2);
    }
  }

  // Newly collected voices settle into the formation with a short ring.
  color = LIGHT_YELLOW;
  FOR_EACH(allies, ai5) {
    ASSIGN_ARRAY_ITEM(allies, ai5, Ally, ally5);
    SKIP_IS_NOT_ALIVE(ally5);
    if (ally5->spawnTicks > 0) {
      thickness = 1;
      arc(ally5->pos.x, ally5->pos.y, 3 + ally5->spawnTicks * 0.35, 0, M_PI * 2);
      ally5->spawnTicks--;
    }
  }

  if (beamTicks > 0) {
    color = CYAN;
    rect(player.pos.x - 1, 6, 2, player.pos.y - 8);
    FOR_EACH(allies, ai6) {
      ASSIGN_ARRAY_ITEM(allies, ai6, Ally, ally6);
      SKIP_IS_NOT_ALIVE(ally6);
      rect(ally6->pos.x - 1, 6, 2, ally6->pos.y - 8);
    }
  } else if (beamAfterglowTicks > 0) {
    color = LIGHT_CYAN;
    float afterglowWidth = 0.5 + beamAfterglowTicks * 0.25;
    rect(player.pos.x - afterglowWidth / 2, 6, afterglowWidth, player.pos.y - 8);
    FOR_EACH(allies, ai7) {
      ASSIGN_ARRAY_ITEM(allies, ai7, Ally, ally7);
      SKIP_IS_NOT_ALIVE(ally7);
      rect(ally7->pos.x - afterglowWidth / 2, 6, afterglowWidth, ally7->pos.y - 8);
    }
  }

  color = YELLOW;
  FOR_EACH(items, ii) {
    ASSIGN_ARRAY_ITEM(items, ii, Item, item);
    SKIP_IS_NOT_ALIVE(item);
    item->pos.y += item->speed * pressure;
    Collision hit = box(VEC_XY(item->pos), 5, 5);
    if (hit.isColliding.rect[BLUE]) {
      int direction = 1;
      if (rndi(0, 2) == 0) {
        direction = -1;
      }
      ASSIGN_ARRAY_ITEM(allies, allyIndex, Ally, na);
      vectorSet(&na->pos, item->pos.x, item->pos.y);
      na->vx = direction * rnd(0.48, 0.9);
      na->spawnTicks = 8;
      na->isAlive = true;
      allyIndex = wrap(allyIndex + 1, 0, STILLFIRECHOIR_MAX_ALLY_COUNT);
      addScore(5, VEC_XY(item->pos));
      particle(item->pos.x, item->pos.y, 8, 1.2, -M_PI_2, M_PI * 2);
      play(POWER_UP);
      item->isAlive = false;
      continue;
    }
    item->isAlive = item->pos.y <= 105;
  }

  COUNT_IS_ALIVE(allies, allyCount);
  color = RED;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, enemy);
    SKIP_IS_NOT_ALIVE(enemy);
    enemy->pos.y += enemy->speed * pressure;
    Collision hit2 = box(enemy->pos.x, enemy->pos.y, enemy->size, enemy->size);
    if (hit2.isColliding.rect[CYAN]) {
      int points = 10 + allyCount * 2;
      addScore(points, VEC_XY(enemy->pos));
      particle(enemy->pos.x, enemy->pos.y, 9, 1.5, M_PI_2, M_PI * 2);
      play(COIN);
      enemy->isAlive = false;
      continue;
    }
    if (hit2.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    enemy->isAlive = enemy->pos.y <= 105;
  }

  beamTicks = fmax(0, beamTicks - 1);
  beamAfterglowTicks = fmax(0, beamAfterglowTicks - 1);
  lockPulseTicks = fmax(0, lockPulseTicks - 1);
}

void addGameStillfireChoir() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
