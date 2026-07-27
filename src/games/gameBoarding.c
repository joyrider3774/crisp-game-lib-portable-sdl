#include "cglp.h"

static char *title = "BOARDING";
static char *description = "[Hold] Boarding";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " lll  ",
        "l l ll",
        " llll ",
        " l  l ",
        "ll  ll",
        "      ",
    },
    {
        " lll  ",
        "l l ll",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
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
    {
        "     l",
        "    ll",
        "   ll ",
        "  ll  ",
        " ll   ",
        "ll    ",
    },
    {
        "l     ",
        "ll    ",
        " ll   ",
        "  ll  ",
        "   ll ",
        "    ll",
    },
    {
        "  l   ",
        " l    ",
        "lllll ",
        " l    ",
        "  l   ",
            "      ",
},
    {
        "  l   ",
        "   l  ",
        "lllll ",
        "   l  ",
        "  l   ",
            "      ",
},
};
static int charactersCount = 8;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  Vector pos;
  int angle;
  bool isAlive;
} Board;
#define BOARDING_MAX_BOARD_COUNT 64
static Board boards[BOARDING_MAX_BOARD_COUNT];
static int boardIndex;

typedef struct {
  Vector pos;
  Vector vel;
  Vector prevPos;
  int type;
  int bc;
  bool isAlive;
} Passenger;
#define BOARDING_MAX_PASSENGER_COUNT 32
static Passenger passengers[BOARDING_MAX_PASSENGER_COUNT];
static int passengerIndex;

static float px;
static int pt;
static float pi;
static float pvx;
static float gy;
static float tgy;
static float bi;
static float scr;
static bool isFirstPressing;

static void addBoard() {
  Vector pos;
  vectorSet(&pos, rnd(9, 90), gy - 3);
  bool tooClose = false;
  FOR_EACH(boards, i) {
    ASSIGN_ARRAY_ITEM(boards, i, Board, b);
    SKIP_IS_NOT_ALIVE(b);
    if (distanceTo(&b->pos, pos.x, pos.y) < 16) {
      tooClose = true;
      break;
    }
  }
  if (tooClose) {
    return;
  }
  ASSIGN_ARRAY_ITEM(boards, boardIndex, Board, nb);
  nb->pos = pos;
  nb->angle = rndi(0, 2);
  nb->isAlive = true;
  boardIndex = wrap(boardIndex + 1, 0, BOARDING_MAX_BOARD_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(boards);
    boardIndex = 0;
    INIT_UNALIVED_ARRAY(passengers);
    passengerIndex = 0;
    px = 50;
    pt = -1;
    pi = 0;
    pvx = -1;
    gy = 50;
    bi = 0;
    scr = 0.1;
    isFirstPressing = true;
    while (gy < 91) {
      addBoard();
      gy += 2;
    }
    gy = tgy = 91;
  }
  color = BLACK;
  FOR_EACH(boards, i) {
    ASSIGN_ARRAY_ITEM(boards, i, Board, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= scr * difficulty;
    char boardChar[2];
    boardChar[0] = 'e' + b->angle;
    boardChar[1] = 0;
    character(boardChar, b->pos.x, b->pos.y);
    if (!(b->pos.y > 15 && b->pos.y < gy)) {
      b->isAlive = false;
    }
  }
  tgy -= scr * difficulty;
  if (tgy < 11) {
    tgy = 11;
  }
  gy += (tgy - gy) * 0.1;
  color = RED;
  rect(0, gy, 50, 8);
  color = BLUE;
  rect(50, gy, 50, 8);
  color = WHITE;
  character("a", 5, gy + 4);
  character("g", 12, gy + 4);
  text("GATE1", 20, gy + 4);
  character("c", 95, gy + 4);
  character("h", 88, gy + 4);
  text("GATE2", 55, gy + 4);
  if (bi < 0) {
    addBoard();
    bi += 4;
  }
  if (pt < 0) {
    color = RED;
  } else {
    color = BLUE;
  }
  char playerChar[2];
  playerChar[0] = 'a' + pt + 1 + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  character(playerChar, px, 9);
  if (gy <= 12) {
    play(EXPLOSION);
    gameOver();
  }
  pi--;
  float speed = 1;
  if (input.isPressed && !isFirstPressing) {
    if (pi < 0) {
      ASSIGN_ARRAY_ITEM(passengers, passengerIndex, Passenger, p);
      vectorSet(&p->pos, px, 9);
      vectorSet(&p->vel, 0, 0);
      p->type = pt;
      p->prevPos = p->pos;
      p->bc = 0;
      p->isAlive = true;
      passengerIndex = wrap(passengerIndex + 1, 0, BOARDING_MAX_PASSENGER_COUNT);
      pi = 9;
    }
    speed = 0.1;
  }
  px += pvx * difficulty * speed;
  if ((px < 10 && pvx < 0) || (px > 90 && pvx > 0)) {
    pvx *= -1;
  }
  if (input.isJustReleased) {
    if (isFirstPressing) {
      isFirstPressing = false;
    } else {
      pt *= -1;
    }
  }
  FOR_EACH(passengers, i) {
    ASSIGN_ARRAY_ITEM(passengers, i, Passenger, p);
    SKIP_IS_NOT_ALIVE(p);
    p->vel.y += 0.2;
    vectorMul(&p->vel, 0.9);
    p->prevPos = p->pos;
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    if (p->type < 0) {
      color = RED;
    } else {
      color = BLUE;
    }
    char pChar[2];
    pChar[0] = 'a' + p->type + 1 + ((int)(ticks / 30) % 2);
    pChar[1] = 0;
    Collision c = character(pChar, p->pos.x, p->pos.y);
    bool ce = c.isColliding.character['e'];
    bool cf = c.isColliding.character['f'];
    if (ce || cf) {
      float a = wrap(angleTo(&p->pos, p->prevPos.x, p->prevPos.y), -M_PI, M_PI);
      float ra;
      if (ce) {
        if (a < -M_PI / 4 || a > M_PI / 4 * 3) {
          ra = -M_PI / 4 * 3;
        } else {
          ra = M_PI / 4;
        }
      } else {
        if (a < -M_PI / 4 * 3 || a > M_PI / 4) {
          ra = M_PI / 4 * 3;
        } else {
          ra = -M_PI / 4;
        }
      }
      Vector v;
      vectorSet(&v, vectorLength(&p->vel) * 2, 0);
      rotate(&v, ra);
      vectorAdd(&p->vel, v.x, v.y);
      vectorAdd(&p->pos, p->vel.x, p->vel.y);
      vectorAdd(&p->pos, p->vel.x, p->vel.y);
      p->bc++;
    }
    if (p->pos.y < 0 && p->vel.y < 0) {
      p->vel.y *= -0.5;
    }
    if ((p->pos.x < 0 && p->vel.x < 0) || (p->pos.x > 99 && p->vel.x > 0)) {
      p->vel.x *= -1;
    }
    if (p->pos.y > gy) {
      bool isOk = (p->pos.x - 50) * p->type > 0;
      if (isOk) {
        if (p->bc > 0) {
          play(POWER_UP);
          addScore(p->bc, p->pos.x, p->pos.y);
          float oy = p->bc * 2 * difficulty;
          tgy += oy;
          bi -= oy;
          if (tgy > 91) {
            tgy = 91;
          }
        } else {
          addScore(0, p->pos.x, p->pos.y);
        }
        p->isAlive = false;
        continue;
      } else {
        play(HIT);
        addScore(-1 - p->bc, p->pos.x, p->pos.y);
        float oy2 = sqrt(1 + p->bc) * difficulty;
        if (oy2 > 20) {
          oy2 = 20;
        }
        tgy -= oy2;
        p->isAlive = false;
        continue;
      }
    }
  }
}

void addGameBoarding() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
