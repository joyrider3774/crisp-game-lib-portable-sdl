#include "cglp.h"
#include "string.h"

static char *title = "GEYSER HOP";
static char *description = "[Hold] Rise\nStomp geysers!";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  float x;
  float y;
  float vy;
} Player;
static Player player;

typedef struct {
  float x;
  float height;
  float vx;
  bool stomped;
  // Vircon32 port note: upstream identifies the tutorial-hint geyser by
  // array index 0 (the very first one ever pushed); the ring buffer here
  // recycles slots, so track that same geyser explicitly instead.
  bool isFirst;
  bool isAlive;
} Geyser;
// Sized generously above the ~1.4 concurrent geysers estimated (spawn rate and speed cancel out).
#define GEYSERHOP_MAX_GEYSER_COUNT 16
static Geyser geysers[GEYSERHOP_MAX_GEYSER_COUNT];
static int geyserIndex;

typedef struct {
  float x;
  float y;
  int life;
  bool isAlive;
} Trail;
// One trail spawned per tick while moving fast, each lives 8 ticks - so at
// most ~8 concurrent; doubled for headroom.
#define GEYSERHOP_MAX_TRAIL_COUNT 16
static Trail trails[GEYSERHOP_MAX_TRAIL_COUNT];
static int trailIndex;

static float spawnTicks;
static float ceilingY;
static float groundY;
static int stompTimer;
static int multiplier;
static float prevVy;

static void update() {
  if (!ticks) {
    groundY = 92;
    ceilingY = 3;
    player.x = 40;
    player.y = 40;
    player.vy = 0;
    INIT_UNALIVED_ARRAY(geysers);
    geyserIndex = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    stompTimer = 0;
    ASSIGN_ARRAY_ITEM(geysers, geyserIndex, Geyser, initG);
    initG->x = 95;
    initG->height = 40;
    initG->vx = -1;
    initG->stomped = false;
    initG->isFirst = true;
    initG->isAlive = true;
    geyserIndex = wrap(geyserIndex + 1, 0, GEYSERHOP_MAX_GEYSER_COUNT);
    spawnTicks = 60;
    multiplier = 1;
    prevVy = 0;
  }

  float baseSpeed = sqrt(difficulty);

  spawnTicks--;
  if (spawnTicks < 0) {
    float h = 30 + floor(rnd(10, 30));
    ASSIGN_ARRAY_ITEM(geysers, geyserIndex, Geyser, ng);
    ng->x = 110;
    ng->height = h;
    ng->vx = -baseSpeed;
    ng->stomped = false;
    ng->isFirst = false;
    ng->isAlive = true;
    geyserIndex = wrap(geyserIndex + 1, 0, GEYSERHOP_MAX_GEYSER_COUNT);
    spawnTicks = 90 / sqrt(difficulty);
  }

  stompTimer++;
  if (stompTimer > 200) {
    ceilingY += 0.1;
  }

  if (input.isPressed) {
    player.vy -= 0.16;
  } else {
    player.vy += 0.13;
  }
  player.vy = clamp(player.vy, -2.2, 3);

  if (prevVy < 0 && player.vy > 0) {
    color = CYAN;
    particle(player.x, player.y - 3, 5, 1, -M_PI / 2, M_PI / 4);
  }
  prevVy = player.vy;

  player.y += player.vy * sqrt(difficulty);

  if (fabs(player.vy) > 1) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    nt->x = player.x;
    nt->y = player.y;
    nt->life = 8;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, GEYSERHOP_MAX_TRAIL_COUNT);
  }

  if (player.y < ceilingY || player.y > groundY) {
    play(EXPLOSION);
    gameOver();
  }

  color = RED;
  rect(0, 0, 100, ceilingY);
  if (stompTimer > 150) {
    color = YELLOW;
    text("!", 50, ceilingY + 7);
  }

  color = GREEN;
  rect(0, groundY, 100, 8);

  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
    color = LIGHT_CYAN;
    float tsize = 4 * ((float)t->life / 8);
    box(t->x, t->y, tsize, tsize);
  }

  float stretch = clamp(-player.vy * 0.4, -1.5, 1.5);
  float pw = 6 - stretch;
  float ph = 6 + stretch;
  color = CYAN;
  box(player.x, player.y, pw, ph);

  float eyeOffsetY;
  if (player.vy > 0) {
    eyeOffsetY = 0.8;
  } else if (player.vy < -0.5) {
    eyeOffsetY = -0.8;
  } else {
    eyeOffsetY = 0;
  }
  color = WHITE;
  box(player.x - 1.5, player.y - 0.5, 2.5, 3);
  box(player.x + 1.5, player.y - 0.5, 2.5, 3);
  color = BLACK;
  rect(player.x - 2, player.y - 0.5 + eyeOffsetY, 1, 1.5);
  rect(player.x + 1, player.y - 0.5 + eyeOffsetY, 1, 1.5);

  FOR_EACH(geysers, gi) {
    ASSIGN_ARRAY_ITEM(geysers, gi, Geyser, g);
    SKIP_IS_NOT_ALIVE(g);
    g->x += g->vx;

    if (g->x < -15) {
      if (!g->stomped) {
        multiplier = (int)fmax(multiplier - 1, 1);
      }
      g->isAlive = false;
      continue;
    }

    float gTop = groundY - g->height;

    color = BLUE;
    Collision bodyCol = rect(g->x - 5, gTop + 4, 10, g->height - 4);

    if (g->stomped) {
      color = PURPLE;
    } else {
      color = YELLOW;
    }
    Collision topCol = rect(g->x - 6, gTop, 12, 6);

    if (!g->stomped) {
      float lookX = clamp((player.x - g->x) * 0.05, -1, 1);
      float lookY = clamp((player.y - gTop) * 0.03, -1, 1);
      color = WHITE;
      box(g->x - 2.5, gTop + 3, 3, 4);
      box(g->x + 2.5, gTop + 3, 3, 4);
      color = BLACK;
      rect(g->x - 3 + lookX, gTop + 2.5 + lookY, 1, 2);
      rect(g->x + 2 + lookX, gTop + 2.5 + lookY, 1, 2);
    }

    color = LIGHT_BLUE;
    particle(g->x, gTop, 1, 0.7, -M_PI / 2, M_PI / 5);

    if (g->isFirst && !g->stomped && score == 0) {
      color = BLACK;
      text("v STOMP!", g->x, gTop - 12);
    }

    if (!g->stomped && topCol.isColliding.rect[CYAN]) {
      if (player.vy > 0) {
        play(POWER_UP);
        addScore(multiplier, g->x, gTop);
        multiplier = (int)fmin(multiplier + 1, 16);
        color = YELLOW;
        particle(g->x, gTop, 30, 3, 0, M_PI);
        player.vy = -2.8;
        g->stomped = true;
        g->height = 8;
        stompTimer = 0;
        ceilingY = fmax(3, ceilingY - 8);
      } else {
        play(HIT);
        gameOver();
      }
    }

    if (!g->stomped && bodyCol.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
  }

  player.x += (45 - player.x) * 0.01;

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameGeyserHop() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
