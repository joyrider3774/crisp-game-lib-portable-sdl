#include "cglp.h"

static char *title = "D LASER";
static char *description = "[Tap]  Turn\n[Hold] Stop";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " ll   ",
    "ll    ",
    " l    ",
    " ll   ",
    "l     ",
    "      ",
}, {
    " ll   ",
    "ll    ",
    " l    ",
    "ll    ",
    "  l   ",
    "      ",
}, {
    "l l l ",
    " lll  ",
    "  l   ",
    " l l  ",
    "l   l ",
    "      ",
}};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  Vector pos;
  bool isHit;
  bool isAlive;
} Laser;

typedef struct {
  Vector pos;
  float tx;
  bool isAlive;
} Shield;

typedef struct {
  Vector pos;
  int vx;
  float ticks;
  float dist;
} Player;

#define DLASER_MAX_LASER_COUNT 10
#define DLASER_MAX_SHIELD_COUNT 32

static Laser lasers[DLASER_MAX_LASER_COUNT];
static float laserCount;
static float lcv;
static float lcVv;
static Shield shields[DLASER_MAX_SHIELD_COUNT];
static int shieldIndex;
static float nextShieldDist;
static Player player;
static float scrY;
// Using hard-coded value 10 directly where needed

static void resetLaser() {
  INIT_UNALIVED_ARRAY(lasers);
  laserCount = 0;
  lcv *= rnd(0, 1);
  lcVv *= rnd(0, 1);
  player.dist = 0;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(lasers);
    laserCount = 0;
    lcv = 0;
    lcVv = 0;
    INIT_UNALIVED_ARRAY(shields);
    shieldIndex = 0;
    nextShieldDist = 0;
    scrY = 80;
    vectorSet(&player.pos, 50, 90);
    player.vx = 1;
    player.ticks = 0;
    player.dist = 0;
  }
  
  float scr = sqrt(difficulty) * 0.05;
  if (player.pos.y < scrY) {
    scr += (scrY - player.pos.y) * 0.1;
  }
  scrY += (80 - scrY) * 0.001;
  
  nextShieldDist -= scr;
  if (nextShieldDist < 0) {
    play(HIT);
    float tx = rndi(0, 10) * 10 + 5;
    ASSIGN_ARRAY_ITEM(shields, shieldIndex, Shield, s);
    vectorSet(&s->pos, tx < 50 ? 0 : 99, 15);
    s->tx = tx;
    s->isAlive = true;
    shieldIndex = wrap(shieldIndex + 1, 0, DLASER_MAX_SHIELD_COUNT);
    nextShieldDist += rnd(9, 36);
  }
  
  FOR_EACH(shields, i) {
    ASSIGN_ARRAY_ITEM(shields, i, Shield, s);
    SKIP_IS_NOT_ALIVE(s);
    
    s->pos.y += scr;
    
    if (fabs(s->pos.x - s->tx) < 1) {
      s->pos.x = s->tx;
      color = BLUE;
    } else {
      s->pos.x += (s->tx - s->pos.x) * 0.2;
      color = LIGHT_BLUE;
    }
    
    box(VEC_XY(s->pos), 8, 4);
    color = BLACK;
    character("c", s->pos.x, s->pos.y + 4);
    
    s->isAlive = s->pos.y <= 99;
  }
  
  COUNT_IS_ALIVE(lasers, activeLasers);
  if (laserCount > 10) {
    laserCount += sqrt(difficulty) * 9;
    
    if (activeLasers == 0) {
      play(POWER_UP);
      for (int i = 0; i < DLASER_MAX_LASER_COUNT; i++) {
        lasers[i].pos.x = i * 10;
        lasers[i].pos.y = 10;
        lasers[i].isHit = false;
        lasers[i].isAlive = true;
      }
    }
    
    play(LASER);
    
    if (laserCount > 199) {
      play(COIN);
      addScore(
        ceil(player.dist * sqrt(player.dist) * sqrt(difficulty) * 0.1),
        VEC_XY(player.pos)
      );
      resetLaser();
    }
  } else {
    lcVv += rnd(-sqrt(difficulty), sqrt(difficulty)) * 0.0001;
    lcv += lcVv;
    
    if ((lcv < 0 && lcVv < 0) || (lcv > sqrt(difficulty) * 0.2 && lcVv > 0)) {
      lcVv = -lcVv;
      lcv *= rnd(0.2, 1.5);
    }
    
    laserCount += lcv;
  }
  
  color = LIGHT_RED;
  rect(0, 9, 100, 1);
  color = RED;
  rect(0, 0, 100, laserCount < 10 ? laserCount : 10);
  
  FOR_EACH(lasers, i) {
    ASSIGN_ARRAY_ITEM(lasers, i, Laser, l);
    SKIP_IS_NOT_ALIVE(l);
    
    if (l->isHit) {
      l->pos.y += scr;
    } else {
      l->pos.y += sqrt(difficulty) * 9;
      
      // Check collision with shields
      color = TRANSPARENT;
      Collision collision = rect(l->pos.x, 10, 10, l->pos.y - 10);
      
      if (collision.isColliding.rect[BLUE]) {
        l->isHit = true;
        // Back up laser to just before collision
        for (int j = 0; j < 99; j++) {
          l->pos.y--;
          color = TRANSPARENT;
          collision = rect(l->pos.x, 10, 10, l->pos.y - 10);
          if (!collision.isColliding.rect[BLUE]) {
            break;
          }
        }
      }
    }
    
    color = RED;
    rect(l->pos.x, 10, 10, l->pos.y - 10);
  }
  
  if (input.isJustPressed) {
    play(SELECT);
    player.vx *= -1;
  }
  
  player.pos.y += scr;
  
  if (!input.isPressed) {
    player.pos.x += player.vx * sqrt(difficulty);
    
    if ((player.pos.x < 0 && player.vx < 0) || (player.pos.x > 99 && player.vx > 0)) {
      player.vx *= -1;
    }
    
    player.pos.x = clamp(player.pos.x, 0, 99);
    player.pos.y -= sqrt(difficulty) * 0.5;
    player.dist += sqrt(difficulty) * 0.5;
    player.ticks += sqrt(difficulty);
  }
  
  if (player.pos.y > 99) {
    play(RANDOM);
    gameOver();
  }
  
  color = BLACK;
  characterOptions.isMirrorX = player.vx < 0;
  char pc[2] = {'a' + (int)(player.ticks / 20) % 2, '\0'};
  
  if (character(pc, VEC_XY(player.pos)).isColliding.rect[RED]) {
    particle(VEC_XY(player.pos), 30, 3, 0, M_PI * 2);
    scrY += 7;
    player.pos.y += 7;
    play(EXPLOSION);
    resetLaser();
  }
  characterOptions.isMirrorX = false;
}

void addGameDLaser() {
  addGame(title, description, characters, charactersCount, options, false, update);
}