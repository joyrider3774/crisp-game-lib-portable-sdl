#include "cglp.h"

static char *title = "SMILY ANGRY";
static char *description = "[Tap] Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " llll ",
        "l ll l",
        " llll ",
        "l ll l",
        "ll  ll",
        " llll ",
    },
    {
        " llll ",
        "l ll l",
        "llllll",
        "ll  ll",
        "l ll l",
        " llll ",
    },
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
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector targetPos;
  float fireInterval;
  float fireSpeed;
  float fireTicks;
  bool isSmile;
  bool isRed;
} Smile;
static Smile smiles[2];

typedef struct {
  Vector pos;
  Vector vel;
  bool isRed;
  bool isBonus;
  bool isAlive;
} Bullet;
#define MAX_BULLET_COUNT 64
static Bullet bullets[MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  int vx;
  float speed;
} Player;
static Player player;

static int multiplier;

static void fire(float x, float y, float speed, bool isSmile, bool isRed) {
  if (isSmile) {
    play(LASER);
  } else {
    play(HIT);
  }
  Vector vel;
  if (isRed) {
    float t1 = distanceTo(&player.pos, x, y) / speed;
    Vector p2;
    vectorSet(&p2, player.pos.x + t1 * player.vx * player.speed, player.pos.y);
    float t2 = distanceTo(&p2, x, y) / speed;
    Vector p3;
    vectorSet(&p3, player.pos.x + t2 * player.vx * player.speed, player.pos.y);
    Vector src;
    vectorSet(&src, x, y);
    float a = angleTo(&src, p3.x, p3.y);
    vectorSet(&vel, 0, 0);
    addWithAngle(&vel, a, speed);
  } else {
    Vector src2;
    vectorSet(&src2, x, y);
    float a2 = angleTo(&src2, player.pos.x, player.pos.y);
    vectorSet(&vel, 0, 0);
    addWithAngle(&vel, a2, speed);
  }
  ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
  vectorSet(&nb->pos, x, y);
  nb->vel = vel;
  nb->isRed = isRed;
  nb->isBonus = isSmile;
  nb->isAlive = true;
  bulletIndex = wrap(bulletIndex + 1, 0, MAX_BULLET_COUNT);
}

static void update() {
  if (!ticks) {
    vectorSet(&smiles[0].pos, 30, 20);
    vectorSet(&smiles[0].targetPos, 30, 20);
    smiles[0].fireInterval = 30;
    smiles[0].fireSpeed = 1;
    smiles[0].fireTicks = 20;
    smiles[0].isSmile = true;
    smiles[0].isRed = true;
    vectorSet(&smiles[1].pos, 70, 20);
    vectorSet(&smiles[1].targetPos, 70, 20);
    smiles[1].fireInterval = 30;
    smiles[1].fireSpeed = 1;
    smiles[1].fireTicks = 50;
    smiles[1].isSmile = false;
    smiles[1].isRed = false;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    vectorSet(&player.pos, 50, 90);
    player.vx = 1;
    player.speed = 1;
    multiplier = 1;
  }
  color = LIGHT_BLACK;
  rect(0, 93, 100, 7);
  TIMES(2, si) {
    Smile *s = &smiles[si];
    s->pos.x += (s->targetPos.x - s->pos.x) * 0.1;
    s->pos.y += (s->targetPos.y - s->pos.y) * 0.1;
    s->fireTicks--;
    if (s->fireTicks < 0) {
      fire(s->pos.x, s->pos.y, s->fireSpeed, s->isSmile, s->isRed);
      s->fireTicks = s->fireInterval;
    }
    float changeRatio = 0.002 * sqrt(difficulty);
    if (rnd(0, 1) < changeRatio) {
      vectorSet(&s->targetPos, rnd(10, 90), rnd(10, 40));
    }
    if (rnd(0, 1) < changeRatio) {
      s->isSmile = !s->isSmile;
    }
    if (rnd(0, 1) < changeRatio) {
      s->isRed = !s->isRed;
    }
    if (rnd(0, 1) < changeRatio) {
      s->fireInterval = rnd(30, 40) / sqrt(difficulty);
      s->fireSpeed = rnd(0.9, 1.2) * sqrt(difficulty);
    }
    if (s->isRed) {
      if (s->isSmile) {
        color = YELLOW;
      } else {
        color = RED;
      }
    } else {
      if (s->isSmile) {
        color = GREEN;
      } else {
        color = CYAN;
      }
    }
    if (s->isSmile) {
      character("a", s->pos.x, s->pos.y);
    } else {
      character("b", s->pos.x, s->pos.y);
    }
  }
  player.speed = sqrt(difficulty) * 0.5;
  if (input.isJustPressed) {
    play(SELECT);
    player.vx *= -1;
  }
  player.pos.x = wrap(player.pos.x + player.vx * player.speed, -3, 103);
  color = BLACK;
  char pc[2];
  pc[0] = 'c' + (ticks / 30) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = player.vx < 0;
  character(pc, player.pos.x, player.pos.y);
  characterOptions.isMirrorX = false;
  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    b->pos.x = wrap(b->pos.x, -3, 103);
    if (b->isRed) {
      color = RED;
    } else {
      color = CYAN;
    }
    Collision bc;
    if (b->isBonus) {
      bc = text("$", b->pos.x, b->pos.y);
    } else {
      thickness = 2;
      bc = bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel));
    }
    if (bc.isColliding.character['c'] || bc.isColliding.character['d']) {
      if (b->isBonus) {
        play(POWER_UP);
        addScore(multiplier, player.pos.x, player.pos.y);
        multiplier++;
        b->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (bc.isColliding.rect[LIGHT_BLACK]) {
      if (b->isBonus && multiplier > 1) {
        multiplier--;
      }
      b->isAlive = false;
      continue;
    }
  }
}

void addGameSmilyAngry() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
