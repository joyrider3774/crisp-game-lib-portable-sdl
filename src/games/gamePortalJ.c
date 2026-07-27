#include "cglp.h"
#include "string.h"

static char *title = "PORTAL J";
static char *description = "[Tap] Portal jump";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 50, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  float x;
  float vx;
  float width;
  bool isAlive;
} Floor;
#define MAX_FLOOR_COUNT 32
static Floor floors[MAX_FLOOR_COUNT];
static int floorIndex;

typedef struct {
  float x;
  bool isAlive;
} Gold;
#define MAX_GOLD_COUNT 32
static Gold golds[MAX_GOLD_COUNT];
static int goldIndex;

static float playerX;
static float portalTicks;
static float fallTicks;
static float portalX;
static float backX;
static float nextFloorDist;
static float nextGoldDist;
static int multiplier;
static float nextScore;

static void update() {
  if (!ticks) {
    playerX = 30;
    portalTicks = 0;
    fallTicks = 0;
    portalX = 36;
    backX = 0;
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
    ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, f1);
    f1->x = 0;
    f1->vx = 0;
    f1->width = 100;
    f1->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MAX_FLOOR_COUNT);
    ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, f2);
    f2->x = 110;
    f2->vx = 0;
    f2->width = 80;
    f2->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MAX_FLOOR_COUNT);
    nextFloorDist = 0;
    INIT_UNALIVED_ARRAY(golds);
    goldIndex = 0;
    nextGoldDist = 30;
    multiplier = 1;
    nextScore = 0;
  }
  float scrX;
  if (playerX > 30) {
    scrX = (playerX - 30) * 0.1;
  } else {
    scrX = 0;
  }
  scrX += difficulty * 0.1;
  color = LIGHT_BLACK;
  backX = wrap(backX - scrX, 0, 200);
  TIMES(2, i) { rect(wrap(backX + i * 50, 0, 200), 0, 1, 50); }
  nextFloorDist -= scrX;
  if (nextFloorDist < 0) {
    float itv = rnd(50, 99);
    float fr = rnd(0.4, 0.7);
    float vx;
    if (rnd(0, 1) > 1 / difficulty) {
      vx = rnd(1, difficulty) * RNDPM() * 0.25;
    } else {
      vx = 0;
    }
    ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, nf);
    nf->x = 200 + nextFloorDist + itv * rnd(0, 1 - fr);
    nf->vx = vx;
    nf->width = itv * fr;
    nf->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MAX_FLOOR_COUNT);
    nextFloorDist += itv;
  }
  color = GREEN;
  FOR_EACH(floors, i) {
    ASSIGN_ARRAY_ITEM(floors, i, Floor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->x += f->vx - scrX;
    FOR_EACH(floors, j) {
      ASSIGN_ARRAY_ITEM(floors, j, Floor, af);
      SKIP_IS_NOT_ALIVE(af);
      if (af->x >= f->x) {
        continue;
      }
      if (af->x + af->width >= f->x) {
        f->vx *= -1;
        af->vx *= -1;
        f->x += f->vx * 2;
      }
    }
    rect(f->x, 40, f->width, 8);
    f->isAlive = f->x + f->width >= 0;
  }
  playerX -= scrX;
  portalX -= scrX;
  float py = 39;
  if (fallTicks > 0) {
    fallTicks += difficulty * 0.2;
    py += fallTicks;
  }
  if (portalTicks > 0) {
    portalTicks += difficulty;
    if (portalTicks < 5) {
      py += portalTicks;
    } else {
      if (playerX < portalX) {
        play(LASER);
        nextScore = portalX - playerX;
        nextScore = ceil(nextScore * sqrt(nextScore) * 0.1 * multiplier);
        playerX = portalX;
      }
      py = 42 - (portalTicks - 5);
      if (py < 39) {
        play(POWER_UP);
        addScore(nextScore, playerX, 36);
        py = 39;
        portalTicks = 0;
      }
    }
  } else {
    portalX += difficulty * 2;
    if (portalX > 200) {
      portalX = playerX + 6;
    }
    if (input.isJustPressed) {
      play(HIT);
      portalTicks = 1;
    }
  }
  color = RED;
  Collision c = box(playerX, py, 5, 5);
  color = PURPLE;
  box(portalX, 43, 7, 5);
  if (portalTicks > 0 && portalTicks < 5) {
    box(playerX, 43 + fallTicks, 7, 5);
  }
  if (portalTicks == 0) {
    if (c.isColliding.rect[GREEN]) {
      fallTicks = 0;
    } else if (fallTicks == 0) {
      fallTicks = 1;
    }
  }
  if (py > 52 || playerX < 0) {
    play(EXPLOSION);
    gameOver();
  }
  nextGoldDist -= scrX;
  if (nextGoldDist < 0) {
    ASSIGN_ARRAY_ITEM(golds, goldIndex, Gold, ng);
    ng->x = 203;
    ng->isAlive = true;
    goldIndex = wrap(goldIndex + 1, 0, MAX_GOLD_COUNT);
    nextGoldDist += rnd(99, 120);
  }
  color = YELLOW;
  FOR_EACH(golds, i) {
    ASSIGN_ARRAY_ITEM(golds, i, Gold, g);
    SKIP_IS_NOT_ALIVE(g);
    g->x -= scrX;
    Collision gc = text("$", g->x, 36);
    if (gc.isColliding.rect[RED]) {
      play(COIN);
      multiplier++;
      g->isAlive = false;
      continue;
    }
    g->isAlive = g->x >= -3;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGamePortalJ() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
