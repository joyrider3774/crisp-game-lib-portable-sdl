#include "cglp.h"

static char *title = "JUMP ON";
static char *description = "[Tap] Jump on";

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
    {
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
            "      ",
},
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "      ",
        "  ll  ",
        " l ll ",
        " llll ",
        "  ll  ",
    },
    {
        "      ",
        "      ",
        "llllll",
            "      ",
        "      ",
        "      ",
},
    {
        "      ",
        "      ",
        "ll  ll",
        "  ll  ",
            "      ",
        "      ",
},
    {
        "      ",
        "      ",
        "l    l",
        " l  l ",
        "  ll  ",
            "      ",
},
    {
        "      ",
        "  ll  ",
        "ll  ll",
            "      ",
        "      ",
        "      ",
},
    {
        "  ll  ",
        " l  l ",
        "l    l",
            "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 12;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

static int holeAnimPattern[7] = {1, 2, 1, 0, 3, 4, 3};

#define JUMPON_STATE_WALK 0
#define JUMPON_STATE_JUMP_TO 1
#define JUMPON_STATE_DOWN 2
#define JUMPON_STATE_UP 3
#define JUMPON_STATE_JUMP_FROM 4

typedef struct {
  Vector pos;
  int holeIndex;
  float nextDotsDist;
  int state;
  bool isAlive;
} JumponEnemy;
#define JUMPON_MAX_ENEMY_COUNT 32
static JumponEnemy enemies[JUMPON_MAX_ENEMY_COUNT];
static int enemyIndex;
static int groupEnemyCount;
static float nextGroupTicks;
static float nextEnemyTicks;

typedef struct {
  float x;
  float animTicks;
  bool isAlive;
} JumponHole;
#define JUMPON_MAX_HOLE_COUNT 32
static JumponHole holes[JUMPON_MAX_HOLE_COUNT];
static int holeIndex;
static float nextHoleDist;

typedef struct {
  Vector pos;
  int holeIndex;
  int state;
} JumponPlayer;
static JumponPlayer player;

typedef struct {
  Vector pos;
  bool isAlive;
} JumponDot;
#define JUMPON_MAX_DOT_COUNT 64
static JumponDot dots[JUMPON_MAX_DOT_COUNT];
static int dotIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    groupEnemyCount = 0;
    nextGroupTicks = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(holes);
    holeIndex = 0;
    ASSIGN_ARRAY_ITEM(holes, holeIndex, JumponHole, h0);
    h0->x = 30;
    h0->animTicks = 99;
    h0->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    ASSIGN_ARRAY_ITEM(holes, holeIndex, JumponHole, h1);
    h1->x = 60;
    h1->animTicks = 99;
    h1->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    nextHoleDist = 0;
    vectorSet(&player.pos, 5, 87);
    player.holeIndex = -1;
    player.state = JUMPON_STATE_WALK;
    INIT_UNALIVED_ARRAY(dots);
    dotIndex = 0;
    multiplier = 1;
  }
  color = LIGHT_YELLOW;
  rect(0, 90, 100, 10);
  TIMES(5, i) { rect(0, 90 - (i + 1) * 12, 100, 3); }
  color = LIGHT_PURPLE;
  rect(0, 10, 100, 10);
  float scr = difficulty * 0.03;
  if (player.pos.x > 15) {
    scr += (player.pos.x - 15) * 0.1;
  }
  nextHoleDist -= scr;
  if (nextHoleDist < 0) {
    ASSIGN_ARRAY_ITEM(holes, holeIndex, JumponHole, nh);
    nh->x = 104;
    nh->animTicks = 99;
    nh->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, JUMPON_MAX_HOLE_COUNT);
    nextHoleDist += rnd(30, 45);
  }
  FOR_EACH(holes, i) {
    ASSIGN_ARRAY_ITEM(holes, i, JumponHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    color = WHITE;
    box(h->x, 60, 8, 80);
    color = BLACK;
    h->animTicks += difficulty;
    int ai;
    if (h->animTicks < 21) {
      ai = holeAnimPattern[(int)floor(h->animTicks / 3)];
    } else {
      ai = 0;
    }
    char hc[2];
    hc[0] = 'h' + ai;
    hc[1] = 0;
    character(hc, h->x, 90);
    if (h->x < -4) {
      h->isAlive = false;
      continue;
    }
  }
  nextGroupTicks--;
  if (nextGroupTicks < 0) {
    nextEnemyTicks = 0;
    groupEnemyCount = rndi(1, 1 + (int)round(sqrt(difficulty) * 2));
    nextGroupTicks = (groupEnemyCount * 8 + rnd(100, 110)) / difficulty;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, JumponEnemy, ne);
    vectorSet(&ne->pos, 103, 87 - rndi(0, 6) * 12);
    ne->holeIndex = -1;
    ne->state = JUMPON_STATE_WALK;
    ne->nextDotsDist = rnd(0, 6);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, JUMPON_MAX_ENEMY_COUNT);
    groupEnemyCount--;
    if (groupEnemyCount == 0) {
      nextEnemyTicks = 9999;
    } else {
      nextEnemyTicks = rnd(8, 10) / difficulty;
    }
  }
  color = RED;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, JumponEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->state == JUMPON_STATE_WALK) {
      e->pos.x += -difficulty - scr;
      FOR_EACH(holes, hi) {
        ASSIGN_ARRAY_ITEM(holes, hi, JumponHole, h);
        SKIP_IS_NOT_ALIVE(h);
        float o = e->pos.x - h->x;
        if (o < 8 && o > 0) {
          play(LASER);
          e->state = JUMPON_STATE_JUMP_TO;
          e->holeIndex = hi;
        }
      }
      characterOptions.isMirrorX = true;
      character("e", VEC_XY(e->pos));
      characterOptions.isMirrorX = false;
    } else if (e->state == JUMPON_STATE_JUMP_TO) {
      e->pos.x += -difficulty - scr;
      float o = e->pos.x - holes[e->holeIndex].x;
      if (o < 4) {
        e->pos.y += 1;
      } else {
        e->pos.y -= 1;
      }
      if (o <= 0) {
        e->state = JUMPON_STATE_DOWN;
      }
      characterOptions.isMirrorX = true;
      character("d", VEC_XY(e->pos));
      characterOptions.isMirrorX = false;
    } else if (e->state == JUMPON_STATE_DOWN) {
      e->pos.x = holes[e->holeIndex].x;
      e->pos.y += difficulty;
      if (e->pos.y > 90) {
        play(HIT);
        e->state = JUMPON_STATE_UP;
        holes[e->holeIndex].animTicks = 0;
      }
      character("f", VEC_XY(e->pos));
    } else if (e->state == JUMPON_STATE_UP) {
      e->pos.x = holes[e->holeIndex].x;
      e->pos.y -= difficulty;
      if (e->pos.y < 23) {
        e->state = JUMPON_STATE_DOWN;
      }
      float o = e->pos.y - player.pos.y;
      if (o < 1 && o > -9) {
        e->state = JUMPON_STATE_JUMP_FROM;
        e->pos.y = ceil((e->pos.y - 90) / 12) * 12 + 87;
      }
      character("f", VEC_XY(e->pos));
    } else if (e->state == JUMPON_STATE_JUMP_FROM) {
      e->pos.x += -difficulty - scr;
      float o = holes[e->holeIndex].x - e->pos.x;
      if (o < 4) {
        e->pos.y -= 1;
      } else {
        e->pos.y += 1;
      }
      if (o >= 8) {
        play(LASER);
        e->state = JUMPON_STATE_WALK;
        e->pos.y = round((e->pos.y - 90) / 12) * 12 + 87;
        multiplier = 1;
      }
      characterOptions.isMirrorX = true;
      character("d", VEC_XY(e->pos));
      characterOptions.isMirrorX = false;
    }
    if (e->state == JUMPON_STATE_WALK) {
      e->nextDotsDist -= difficulty;
      if (e->nextDotsDist < 0) {
        ASSIGN_ARRAY_ITEM(dots, dotIndex, JumponDot, nd);
        nd->pos = e->pos;
        nd->isAlive = true;
        dotIndex = wrap(dotIndex + 1, 0, JUMPON_MAX_DOT_COUNT);
        e->nextDotsDist += 6;
      }
    }
    if (e->pos.x < -3) {
      e->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (player.state == JUMPON_STATE_WALK) {
    player.pos.x += difficulty - scr;
    FOR_EACH(holes, hi) {
      ASSIGN_ARRAY_ITEM(holes, hi, JumponHole, h);
      SKIP_IS_NOT_ALIVE(h);
      float o = h->x - player.pos.x;
      if (o < 8 && o > 0) {
        play(JUMP);
        player.state = JUMPON_STATE_JUMP_TO;
        player.holeIndex = hi;
      }
    }
    char pc[2];
    pc[0] = 'a' + (int)floor(ticks / 10) % 2;
    pc[1] = 0;
    Collision pcColl = character(pc, VEC_XY(player.pos));
    if (pcColl.isColliding.character['e']) {
      play(EXPLOSION);
      gameOver();
    }
  } else if (player.state == JUMPON_STATE_JUMP_TO) {
    player.pos.x += difficulty - scr;
    float o = holes[player.holeIndex].x - player.pos.x;
    if (o < 4) {
      player.pos.y += 1;
    } else {
      player.pos.y -= 1;
    }
    if (o <= 0) {
      player.state = JUMPON_STATE_DOWN;
    }
    character("a", VEC_XY(player.pos));
  } else if (player.state == JUMPON_STATE_DOWN) {
    player.pos.x = holes[player.holeIndex].x;
    player.pos.y += difficulty;
    if (player.pos.y > 90) {
      play(POWER_UP);
      player.state = JUMPON_STATE_UP;
      holes[player.holeIndex].animTicks = 0;
    }
    character("c", VEC_XY(player.pos));
  } else if (player.state == JUMPON_STATE_UP) {
    player.pos.x = holes[player.holeIndex].x;
    player.pos.y -= difficulty;
    if (player.pos.y < 23) {
      player.state = JUMPON_STATE_DOWN;
    }
    if (input.isPressed) {
      play(JUMP);
      player.state = JUMPON_STATE_JUMP_FROM;
      player.pos.y = ceil((player.pos.y - 90) / 12) * 12 + 87;
    }
    character("c", VEC_XY(player.pos));
  } else if (player.state == JUMPON_STATE_JUMP_FROM) {
    player.pos.x += difficulty - scr;
    float o = player.pos.x - holes[player.holeIndex].x;
    if (o < 4) {
      player.pos.y -= 1;
    } else {
      player.pos.y += 1;
    }
    if (o >= 8) {
      player.state = JUMPON_STATE_WALK;
      player.pos.y = round((player.pos.y - 90) / 12) * 12 + 87;
    }
    character("a", VEC_XY(player.pos));
  }
  if (player.pos.x < 0) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(dots, i) {
    ASSIGN_ARRAY_ITEM(dots, i, JumponDot, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.x -= scr;
    Collision dc = character("g", VEC_XY(d->pos));
    if (dc.isColliding.character['a'] || dc.isColliding.character['b']) {
      play(COIN);
      addScore(multiplier, VEC_XY(d->pos));
      multiplier++;
      d->isAlive = false;
      continue;
    }
    if (d->pos.x < -3) {
      d->isAlive = false;
      continue;
    }
  }
}

void addGameJumpOn() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
