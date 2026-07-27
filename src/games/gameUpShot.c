#include "cglp.h"

static char *title = "UP SHOT";
static char *description = "[Hold]\n Stop & Shoot\n[Release]\n Run";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 50, .isDarkColor = false};

typedef struct {
  float x;
  float size;
  float speed;
  float interval;
  float intervalVariation;
  float ticks;
  bool isAlive;
} RockSpawn;
#define UPSHOT_MAX_ROCK_SPAWN_COUNT 16
static RockSpawn rockSpawns[UPSHOT_MAX_ROCK_SPAWN_COUNT];
static int rockSpawnIndex;
static float nextRockSpawnDist;

typedef struct {
  Vector pos;
  float vy;
  float size;
  float speed;
  bool isAlive;
} Rock;
// Rock spawn interval shrinks as difficulty^-1.25 while fall lifetime only shrinks as difficulty^-0.25, so concurrent rocks grow ~linearly with difficulty (minutes elapsed) - 64 overflowed after ~10-12 min.
#define UPSHOT_MAX_ROCK_COUNT 512
static Rock rocks[UPSHOT_MAX_ROCK_COUNT];
static int rockIndex;

typedef struct {
  Vector pos;
  float vx;
  float shotTicks;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} Shot;
#define UPSHOT_MAX_SHOT_COUNT 32
static Shot shots[UPSHOT_MAX_SHOT_COUNT];
static int shotIndex;

static float lx;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(rockSpawns);
    rockSpawnIndex = 0;
    nextRockSpawnDist = 0;
    INIT_UNALIVED_ARRAY(rocks);
    rockIndex = 0;
    vectorSet(&player.pos, 10, 87);
    player.vx = 0;
    player.shotTicks = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    lx = 50;
    multiplier = 1;
  }
  float scr = difficulty * 0.1;
  if (player.pos.x > 30) {
    scr += (player.pos.x - 30) * 0.05;
  }
  lx = wrap(lx - scr, 0, 99);
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10);
  color = WHITE;
  rect(lx, 90, 1, 10);
  player.shotTicks--;
  if (input.isPressed) {
    if (input.isJustPressed) {
      multiplier = 1;
      player.vx = 0;
    }
    if (player.shotTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, s);
      vectorSet(&s->pos, player.pos.x, player.pos.y);
      s->vy = -2 * difficulty;
      s->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, UPSHOT_MAX_SHOT_COUNT);
      player.shotTicks = 10 / difficulty;
    }
  } else if (input.isJustReleased) {
    play(SELECT);
    player.vx = difficulty * 1.2;
  }
  player.pos.x += player.vx - scr;
  color = BLACK;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scr;
    s->pos.y += s->vy;
    box(s->pos.x, s->pos.y, 5, 9);
    s->isAlive = s->pos.y >= 0;
  }
  nextRockSpawnDist -= scr;
  if (nextRockSpawnDist < 0) {
    float size = rnd(5, 15);
    float interval = rnd(10, 50) / difficulty;
    float speed = (rnd(5, 10) / sqrt(size)) * sqrt(difficulty);
    interval /= sqrt(speed) / sqrt(size);
    ASSIGN_ARRAY_ITEM(rockSpawns, rockSpawnIndex, RockSpawn, rs);
    rs->x = 200;
    rs->size = size;
    rs->speed = speed;
    rs->interval = interval;
    rs->intervalVariation = rnd(0.3, 0.9);
    rs->ticks = rnd(0, interval);
    rs->isAlive = true;
    rockSpawnIndex = wrap(rockSpawnIndex + 1, 0, UPSHOT_MAX_ROCK_SPAWN_COUNT);
    nextRockSpawnDist += rnd(50, 60);
  }
  FOR_EACH(rockSpawns, i) {
    ASSIGN_ARRAY_ITEM(rockSpawns, i, RockSpawn, rs);
    SKIP_IS_NOT_ALIVE(rs);
    rs->x -= scr;
    rs->ticks--;
    if (rs->ticks < 0) {
      ASSIGN_ARRAY_ITEM(rocks, rockIndex, Rock, r);
      vectorSet(&r->pos, rs->x, -rs->size / 2);
      r->vy = 0;
      r->size = rs->size;
      r->speed = rs->speed;
      r->isAlive = true;
      rockIndex = wrap(rockIndex + 1, 0, UPSHOT_MAX_ROCK_COUNT);
      rs->ticks = rs->interval * (1 + rnd(0, rs->intervalVariation) * RNDPM());
    }
    rs->isAlive = rs->x >= 0;
  }
  color = RED;
  FOR_EACH(rocks, i) {
    ASSIGN_ARRAY_ITEM(rocks, i, Rock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->vy += r->speed * 0.01;
    r->pos.x -= scr;
    r->pos.y += r->vy;
    Collision rc = box(r->pos.x, r->pos.y, r->size, r->size);
    if (rc.isColliding.rect[BLACK]) {
      r->size *= 0.7;
      color = BLACK;
      particle(r->pos.x, r->pos.y, 5, 3, M_PI_2, 0.5);
      color = RED;
      if (r->size < 5) {
        play(POWER_UP);
        addScore(multiplier * 10, r->pos.x, clamp(r->pos.y, 20, 99));
        particle(r->pos.x, r->pos.y, 19, 3, 0, M_PI * 2);
        r->isAlive = false;
        continue;
      } else {
        play(HIT);
        addScore(multiplier, r->pos.x, clamp(r->pos.y, 20, 99));
        multiplier++;
      }
    }
    if (r->pos.y > 90 - r->size / 2) {
      particle(r->pos.x, r->pos.y, r->size * 0.3, sqrt(r->size) * 0.3, 0, M_PI * 2);
      r->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char pc[2];
  if (input.isPressed) {
    pc[0] = 'b';
  } else {
    pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  }
  pc[1] = 0;
  Collision cc = character(pc, player.pos.x, player.pos.y);
  if (cc.isColliding.rect[RED] || player.pos.x < -2) {
    play(EXPLOSION);
    gameOver();
  }
  color = TRANSPARENT;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    Collision sc = box(s->pos.x, s->pos.y, 5, 9);
    if (sc.isColliding.rect[RED]) {
      s->isAlive = false;
    }
  }
}

void addGameUpShot() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
