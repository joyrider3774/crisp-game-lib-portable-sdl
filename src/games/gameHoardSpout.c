#include "cglp.h"

static char *title = "HOARD SPOUT";
static char *description = "Gather same color sparks\n[Hold] Thrust";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = true};

#define HOARDSPOUT_FIELD_WIDTH 100
#define HOARDSPOUT_FIELD_HEIGHT 100

typedef struct {
  Vector pos;
  float angle;
  float speed;
  int color;
  float size;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  int color;
  float size;
  bool isAlive;
} Spark;
#define HOARDSPOUT_MAX_SPARK_COUNT 64
static Spark sparks[HOARDSPOUT_MAX_SPARK_COUNT];
static int sparkIndex;
static float nextSparkTicks;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    player.angle = rnd(0, 2 * M_PI);
    player.speed = 1;
    player.color = RED;
    player.size = 3;
    INIT_UNALIVED_ARRAY(sparks);
    sparkIndex = 0;
    nextSparkTicks = 0;
  }
  addWithAngle(&player.pos, player.angle, player.speed);
  if (input.isJustReleased) {
    play(LASER);
    if (player.color == RED) {
      player.color = BLUE;
    } else {
      player.color = RED;
    }
  }
  if (input.isPressed) {
    if (player.speed < 2) {
      player.speed += 0.01 * difficulty;
    }
    player.angle += 0.01 * difficulty;
    nextSparkTicks -= sqrt(difficulty);
  } else {
    player.speed += (difficulty - player.speed) * 0.1;
  }
  nextSparkTicks -= 0.1 * sqrt(difficulty);
  if (nextSparkTicks < 0) {
    Vector velocity;
    vectorSet(&velocity, 1, 0);
    rotate(&velocity, player.angle + M_PI + rnd(0, 0.2) * RNDPM());
    ASSIGN_ARRAY_ITEM(sparks, sparkIndex, Spark, ns);
    ns->pos = player.pos;
    ns->vel = velocity;
    if (player.color == RED) {
      ns->color = BLUE;
    } else {
      ns->color = RED;
    }
    ns->size = 1;
    ns->isAlive = true;
    sparkIndex = wrap(sparkIndex + 1, 0, HOARDSPOUT_MAX_SPARK_COUNT);
    nextSparkTicks = 9;
  }
  if (player.pos.x < 0) {
    player.angle = M_PI - player.angle;
    player.pos.x = 0;
  }
  if (player.pos.x > HOARDSPOUT_FIELD_WIDTH) {
    player.angle = -M_PI - player.angle;
    player.pos.x = HOARDSPOUT_FIELD_WIDTH;
  }
  if (player.pos.y < 0) {
    player.angle = -player.angle;
    player.pos.y = 0;
  }
  if (player.pos.y > HOARDSPOUT_FIELD_HEIGHT) {
    player.angle = -player.angle;
    player.pos.y = HOARDSPOUT_FIELD_HEIGHT;
  }
  player.size -= 0.002 * difficulty;
  if (player.size < 1) {
    play(EXPLOSION);
    gameOver();
  } else if (player.size > 9) {
    player.size = 9;
  }
  color = player.color;
  thickness = player.size * 1.5;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(player.pos), player.size * 2.5, player.angle);
  color = BLACK;
  thickness = player.size;
  bar(VEC_XY(player.pos), player.size + 3, player.angle);
  FOR_EACH(sparks, i) {
    ASSIGN_ARRAY_ITEM(sparks, i, Spark, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    if (s->size < 5) {
      if (s->color == RED) {
        color = LIGHT_RED;
      } else {
        color = LIGHT_BLUE;
      }
    } else {
      color = s->color;
    }
    if (s->size < 5) {
      s->size += 0.2 * difficulty;
    } else {
      s->size += 0.01 * difficulty;
    }
    if (s->size > 9) {
      particle(s->pos.x, s->pos.y, 9, vectorLength(&s->vel), vectorAngle(&s->vel), 0.4);
      s->isAlive = false;
      continue;
    }
    Collision sc = box(VEC_XY(s->pos), s->size, s->size);
    if (sc.isColliding.rect[BLACK] && s->size >= 5) {
      if (s->color == player.color) {
        play(POWER_UP);
        addScore(floor(player.size), VEC_XY(s->pos));
        player.size += 0.2 * difficulty;
        s->isAlive = false;
        continue;
      } else {
        play(HIT);
        particle(s->pos.x, s->pos.y, 9, 2, 0, M_PI * 2);
        player.size -= 0.4 * difficulty;
        s->isAlive = false;
        continue;
      }
    }
    if (s->pos.x < 0) {
      s->vel.x *= -1;
      s->pos.x = 0;
    }
    if (s->pos.x > HOARDSPOUT_FIELD_WIDTH) {
      s->vel.x *= -1;
      s->pos.x = HOARDSPOUT_FIELD_WIDTH;
    }
    if (s->pos.y < 0) {
      s->vel.y *= -1;
      s->pos.y = 0;
    }
    if (s->pos.y > HOARDSPOUT_FIELD_HEIGHT) {
      s->vel.y *= -1;
      s->pos.y = HOARDSPOUT_FIELD_HEIGHT;
    }
  }
}

void addGameHoardSpout() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
