#include "cglp.h"

static char *title = "REBIRTH";
static char *description = "[Tap] Jump / Land";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " ll   ",
        "lll l ",
        "lll ll",
        "lll ll",
        "llllll",
        " l  l ",
    },
    {
        "   ll ",
        "  l   ",
        " lll  ",
        " l    ",
        "l l   ",
        "   l  ",
    },
    {
        "  ll  ",
        "l l   ",
        " llll ",
        "  l   ",
        " l ll ",
        "l     ",
    },
    {
        "  ll  ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l ll ",
        "l     ",
    },
    {
        "  ll  ",
        "  l   ",
        " lll  ",
        "l l l ",
        " l ll ",
        "ll  l ",
    },
    {
        "  ll  ",
        " l    ",
        "ll    ",
        "l l   ",
        " l l  ",
        "  l l ",
    },
    {
        " llll ",
        "ll  ll",
        "l ll l",
        " llll ",
        "  ll  ",
            "      ",
},
};
static int charactersCount = 7;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 50, .soundSeed = 2000, .isDarkColor = true};

typedef struct {
  float x;
  float vx;
  int world;
  bool isAlive;
} Track;
#define MAX_TRACK_COUNT 64
static Track tracks[MAX_TRACK_COUNT];
static int trackIndex;
static float nextTrackTicks;
static int trackCount;

typedef struct {
  Vector pos;
  float vx;
  int world;
  bool isAlive;
} Dia;
#define MAX_DIA_COUNT 64
static Dia dias[MAX_DIA_COUNT];
static int diaIndex;
static float nextDiaTicks;
static int diaWorld;
static int diaCount;

#define STATE_RUN 0
#define STATE_JUMP 1
#define STATE_LAND 2

typedef struct {
  Vector pos;
  float ox;
  Vector vel;
  int world;
  int state;
} Player;
static Player player;

static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(tracks);
    trackIndex = 0;
    nextTrackTicks = 0;
    trackCount = 0;
    INIT_UNALIVED_ARRAY(dias);
    diaIndex = 0;
    nextDiaTicks = 60;
    diaWorld = 1;
    diaCount = 1;
    vectorSet(&player.pos, 110, 37);
    player.ox = 10;
    vectorSet(&player.vel, 0, 0);
    player.world = 1;
    player.state = STATE_RUN;
    multiplier = 1;
  }
  color = BLACK;
  rect(100, 40, 100, 10);
  color = LIGHT_BLACK;
  rect(0, 0, 100, 40);
  nextTrackTicks--;
  if (nextTrackTicks < 0) {
    int world;
    if (trackCount % 2 == 0) {
      world = 1;
    } else {
      world = -1;
    }
    ASSIGN_ARRAY_ITEM(tracks, trackIndex, Track, nt);
    if (world > 0) {
      nt->x = 203;
    } else {
      nt->x = -3;
    }
    float vxDir;
    if (world > 0) {
      vxDir = -1;
    } else {
      vxDir = 1;
    }
    nt->vx = rnd(1, sqrt(difficulty)) * vxDir;
    nt->world = world;
    nt->isAlive = true;
    trackIndex = wrap(trackIndex + 1, 0, MAX_TRACK_COUNT);
    trackCount++;
    nextTrackTicks = rnd(50, 60) / sqrt(difficulty);
  }
  FOR_EACH(tracks, ti) {
    ASSIGN_ARRAY_ITEM(tracks, ti, Track, t);
    SKIP_IS_NOT_ALIVE(t);
    t->x += t->vx;
    if (t->world > 0) {
      color = BLACK;
    } else {
      color = WHITE;
    }
    characterOptions.isMirrorX = t->world > 0;
    character("a", t->x, 37);
    characterOptions.isMirrorX = false;
    bool shouldRemove;
    if (t->world > 0) {
      shouldRemove = t->x < 103;
    } else {
      shouldRemove = t->x > 97;
    }
    if (shouldRemove) {
      t->isAlive = false;
      continue;
    }
  }
  vectorAdd(&player.pos, VEC_XY(player.vel));
  vectorMul(&player.vel, 0.99);
  player.vel.x *= 0.8;
  char pc[2];
  pc[1] = 0;
  if (player.state == STATE_RUN) {
    player.pos.x += (100 + player.world * player.ox - player.pos.x) * 0.05;
    player.ox = clamp(player.ox + sqrt(difficulty) * 0.3, 10, 80);
    if (input.isJustPressed) {
      play(JUMP);
      player.state = STATE_JUMP;
      player.vel.y = -2 * sqrt(difficulty);
    }
    pc[0] = 'b' + (ticks / 10) % 2;
  } else if (player.state == STATE_JUMP) {
    float dvy;
    if (input.isPressed) {
      dvy = 0.07;
    } else {
      dvy = 0.14;
    }
    player.vel.y += dvy * difficulty;
    if (player.pos.y > 37) {
      player.pos.y = 37;
      player.vel.y = 0;
      player.state = STATE_RUN;
    }
    if (input.isJustPressed) {
      play(LASER);
      player.state = STATE_LAND;
      player.vel.y = 4 * sqrt(difficulty);
    }
    pc[0] = 'd';
  } else if (player.state == STATE_LAND) {
    if (player.pos.y > 37) {
      player.pos.y = 37;
      player.vel.y = 0;
      player.state = STATE_RUN;
    }
    pc[0] = 'e';
  }
  if (fabs(player.vel.x) > 1) {
    pc[0] = 'f';
  }
  int pw;
  if (player.pos.x < 100) {
    pw = -1;
  } else {
    pw = 1;
  }
  if (pw > 0) {
    color = BLACK;
  } else {
    color = WHITE;
  }
  characterOptions.isMirrorX = pw < 0;
  Collision pcoll = character(pc, VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  if (pcoll.isColliding.character['a'] && fabs(player.vel.x) < 2) {
    play(HIT);
    float dir;
    if (player.world > 0) {
      dir = -1;
    } else {
      dir = 1;
    }
    player.vel.x = dir * player.ox * 0.2 * sqrt(difficulty);
    if (player.world > 0) {
      player.world = -1;
    } else {
      player.world = 1;
    }
    player.ox = 10;
    if (multiplier > 1) {
      multiplier--;
    }
  }
  nextDiaTicks--;
  if (nextDiaTicks < 0) {
    diaCount--;
    if (diaCount < 0) {
      if (diaWorld > 0) {
        diaWorld = -1;
      } else {
        diaWorld = 1;
      }
      diaCount = rndi(0, 4);
    }
    ASSIGN_ARRAY_ITEM(dias, diaIndex, Dia, nd);
    float dx;
    if (diaWorld > 0) {
      dx = 203;
    } else {
      dx = -3;
    }
    float dy;
    if (rnd(0, 1) < 0.5) {
      dy = 37;
    } else {
      dy = rnd(20, 30);
    }
    vectorSet(&nd->pos, dx, dy);
    float dvxDir;
    if (diaWorld > 0) {
      dvxDir = -1;
    } else {
      dvxDir = 1;
    }
    nd->vx = (rnd(1, sqrt(difficulty)) * dvxDir) / 3;
    nd->world = diaWorld;
    nd->isAlive = true;
    diaIndex = wrap(diaIndex + 1, 0, MAX_DIA_COUNT);
    nextDiaTicks = rnd(120, 150) / sqrt(difficulty);
  }
  FOR_EACH(dias, di) {
    ASSIGN_ARRAY_ITEM(dias, di, Dia, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.x += d->vx;
    color = YELLOW;
    characterOptions.isMirrorX = d->world > 0;
    Collision dc = character("g", VEC_XY(d->pos));
    characterOptions.isMirrorX = false;
    if (dc.isColliding.character['b'] || dc.isColliding.character['c'] ||
        dc.isColliding.character['d'] || dc.isColliding.character['e'] ||
        dc.isColliding.character['f']) {
      play(COIN);
      addScore(multiplier, VEC_XY(d->pos));
      multiplier++;
      d->isAlive = false;
      continue;
    }
    bool offscreen;
    if (d->world > 0) {
      offscreen = d->pos.x < 103;
    } else {
      offscreen = d->pos.x > 97;
    }
    if (offscreen) {
      play(EXPLOSION);
      color = RED;
      text("X", 100, d->pos.y);
      gameOver();
    }
  }
}

void addGameRebirth() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
