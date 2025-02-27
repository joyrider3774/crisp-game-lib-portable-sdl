#include "cglp.h"

static char *title = "DARK CAVE";
static char *description = "[Slide] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = true};

typedef struct {
  Vector pos;
  char type; // 's' for spike, 'c' for coin
  bool isAlive;
} Object;

#define DARKCAVE_MAX_OBJ_COUNT 64
static Object objs[DARKCAVE_MAX_OBJ_COUNT];
static int objIndex;
static float nextSpikeDist;
static float nextCoinDist;

typedef struct {
  Vector pos;
  float angle;
  float range;
} Player;

static Player player;
static int pressedTicks;
static const float angleWidth = M_PI / 6;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    nextSpikeDist = 0;
    nextCoinDist = 10;
    pressedTicks = 0;
    player.pos.x = 50;
    player.pos.y = 90;
    player.angle = 0;
    player.range = 40;
  }
  
  float scr = difficulty * (0.1 + player.range * 0.01);
  
  // Update spike generation
  nextSpikeDist -= scr;
  if (nextSpikeDist < 0) {
    ASSIGN_ARRAY_ITEM(objs, objIndex, Object, o);
    o->pos.x = rnd(5, 95);
    o->pos.y = 0;
    o->type = 's'; // spike
    o->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, DARKCAVE_MAX_OBJ_COUNT);
    nextSpikeDist = rnd(20, 30);
  }
  
  // Update coin generation
  nextCoinDist -= scr;
  if (nextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(objs, objIndex, Object, o);
    o->pos.x = rnd(10, 90);
    o->pos.y = 0;
    o->type = 'c'; // coin
    o->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, DARKCAVE_MAX_OBJ_COUNT);
    nextCoinDist = rnd(30, 40);
  }
  
  // Update player movement
  const float pp = player.pos.x;
  player.pos.x = clamp(input.pos.x, 3, 97);
  const float vx = clamp(player.pos.x - pp, -0.1, 0.1);
  player.angle = clamp(player.angle + vx * sqrtf(difficulty), -M_PI / 8, M_PI / 8);
  player.range *= 1 - 0.001 * sqrtf(difficulty);
  
  // Draw player
  color = WHITE;
  box(VEC_XY(player.pos), 7, 7);
  
  // Update and draw objects
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, Object, o);
    SKIP_IS_NOT_ALIVE(o);
    
    o->pos.y += scr;
    const float d = distanceTo(&player.pos, VEC_XY(o->pos));
    const float a = angleTo(&player.pos, VEC_XY(o->pos)) + M_PI / 2;
    
    if (fabsf(a - player.angle) < angleWidth) {
      color = (o->type == 'c') ? YELLOW : PURPLE;
      if (d < player.range) {
        text((o->type == 'c') ? "$" : "*", VEC_XY(o->pos));
      } else {
        particle(VEC_XY(o->pos), 0.05, 1, 0, M_PI * 2);
      }
    }
    
    color = TRANSPARENT;
    if (text((o->type == 'c') ? "$" : "*", VEC_XY(o->pos)).isColliding.rect[WHITE]) {
      if (o->type == 'c') {
        play(POWER_UP);
        addScore(floorf(player.range), VEC_XY(o->pos));
        player.range = clamp(player.range + 9, 9, 99);
      } else {
        play(EXPLOSION);
        color = RED;
        text("*", VEC_XY(o->pos));
        particle(VEC_XY(o->pos), 5, 1, 0, M_PI * 2);
        player.range /= 3;
      }
      o->isAlive = false;
    }
  }
  
  // Draw player's vision cone
  color = (player.range < 9) ? RED : BLACK;
  thickness = 2;
  
  const float a1 = player.angle - angleWidth - M_PI / 2;
  const float a2 = a1 + angleWidth * 2;
  
  Vector p1, p2;
  vectorSet(&p1, VEC_XY(player.pos));
  vectorSet(&p2, VEC_XY(player.pos));
  addWithAngle(&p1, a1, player.range);
  addWithAngle(&p2, a2, player.range);
  
  line(VEC_XY(player.pos), VEC_XY(p1));
  line(VEC_XY(player.pos), VEC_XY(p2));
  arc(VEC_XY(player.pos), player.range, a1, a2);
  
  // Check game over condition
  if (player.range < 9) {
    play(RANDOM);
    gameOver();
  }
}

void addGameDarkCave() {
  addGame(title, description, characters, charactersCount, options, true, update);
}