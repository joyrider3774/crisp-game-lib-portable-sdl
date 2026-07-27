#include "cglp.h"
#include "string.h"

static char *title = "I SLASH";
static char *description = "[Hold] Slash";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        " ll   ",
        "llll  ",
        " l  l ",
        "l l   ",
            "      ",
},
    {
        "   ll ",
        " ll   ",
        "  l   ",
        " l ll ",
        "l     ",
            "      ",
},
    {
        "   ll ",
        " ll   ",
        "  l   ",
        " l l  ",
        "  l l ",
            "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 50, .soundSeed = 7, .isDarkColor = false};

#define ISLASH_STATE_WAIT 0
#define ISLASH_STATE_SLASH 1

typedef struct {
  float x;
  float ticks;
} Enemy;
// Enemies are only ever removed by slashing (index 0); a player that never
// slashes lets them pile up indefinitely (same latent behavior as upstream
// JS's unbounded array), so this is sized generously rather than tightly.
#define ISLASH_MAX_ENEMY_COUNT 256
static Enemy enemies[ISLASH_MAX_ENEMY_COUNT];
static int enemyCount;
static float nextEnemyDist;

static void removeEnemy(int index) {
  memcpy(&enemies[index], &enemies[index + 1],
         (enemyCount - 1 - index) * sizeof(enemies[0]));
  enemyCount--;
}

typedef struct {
  float x;
  float vx;
  float slashDist;
  float ticks;
  float hitX;
  int state;
} Player;
static Player player;

static bool isSlashReady;
static float floorX;
static int multiplier;
#define ISLASH_PLAYER_Y 37

static void hitEnemy(int ei) {
  play(SELECT);
  player.vx = (player.x - enemies[ei].x + 9) * 0.25;
  player.x = enemies[ei].x - 6;
  player.hitX = enemies[ei].x;
  color = BLACK;
  particle(player.hitX, ISLASH_PLAYER_Y, 9, player.vx, M_PI, 0.1);
  player.state = ISLASH_STATE_WAIT;
  if (multiplier > 1) {
    multiplier--;
  }
}

static void startSlash() {
  player.x += player.slashDist;
  bool isHitting = false;
  if (enemyCount > 1 && enemies[1].x < player.x) {
    hitEnemy(1);
    isHitting = true;
  } else {
    player.state = ISLASH_STATE_SLASH;
    player.ticks = 9 / sqrt(difficulty);
  }
  if (enemies[0].x < player.x) {
    color = RED;
    particle(enemies[0].x, ISLASH_PLAYER_Y, 16, 1, 0, M_PI * 2);
    if (!isHitting) {
      play(EXPLOSION);
      addScore(multiplier, enemies[0].x, ISLASH_PLAYER_Y);
      multiplier++;
    }
    removeEnemy(0);
  } else {
    player.state = ISLASH_STATE_WAIT;
  }
}

static void update() {
  // Never reads a Collision result anywhere in this file - enemy hits and
  // slashes are direct position comparisons (see hitEnemy()/
  // startSlash() above), so the engine's own O(n^2) hitbox scan (see
  // checkHitBox() in cglp.c) is pure waste here. Restored automatically
  // when the next real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    enemyCount = 0;
    nextEnemyDist = 0;
    player.x = 10;
    player.vx = 0;
    player.slashDist = 0;
    player.ticks = 0;
    player.hitX = 0;
    player.state = ISLASH_STATE_WAIT;
    isSlashReady = false;
    floorX = 100;
    multiplier = 1;
  }
  float scr = (player.x - 30) * 0.05;
  color = LIGHT_BLACK;
  rect(0, 40, 200, 10);
  floorX = wrap(floorX - scr, 0, 200);
  color = WHITE;
  rect(floorX, 40, 1, 10);
  float waitBonus;
  if (player.state == ISLASH_STATE_WAIT) {
    waitBonus = sqrt(difficulty);
  } else {
    waitBonus = 0;
  }
  nextEnemyDist -= scr + waitBonus;
  if (enemyCount == 0) {
    nextEnemyDist = 0;
  }
  if (nextEnemyDist <= 0) {
    float d = rnd(15, 30);
    int c = rndi(3, 10);
    float x = 203;
    TIMES(c, i) {
      if (enemyCount < ISLASH_MAX_ENEMY_COUNT) {
        enemies[enemyCount].x = x;
        enemies[enemyCount].ticks = 0;
        enemyCount++;
      }
      x += d;
    }
    nextEnemyDist = d * c + rnd(0, 30);
  }
  if (input.isJustReleased) {
    isSlashReady = true;
  }
  bool isSlashing = isSlashReady && input.isPressed;
  player.x -= scr + player.vx;
  if (player.x < 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  player.vx *= 0.9;
  if (isSlashing && player.vx < 1 && player.state == ISLASH_STATE_WAIT) {
    player.slashDist = (enemies[0].x - player.x) * 2;
    startSlash();
  }
  if (player.state == ISLASH_STATE_WAIT) {
    if (enemies[0].x < player.x) {
      hitEnemy(0);
    }
  } else {
    color = RED;
    rect(player.x, ISLASH_PLAYER_Y, -player.slashDist, 1);
    player.ticks--;
    if (player.ticks < 0) {
      if (isSlashing) {
        startSlash();
      } else {
        player.state = ISLASH_STATE_WAIT;
      }
    }
  }
  color = BLACK;
  if (player.vx > 1) {
    player.hitX -= scr;
    rect(player.x, ISLASH_PLAYER_Y, player.hitX - player.x, 1);
  }
  character("a", player.x, ISLASH_PLAYER_Y);
  for (int i = 0; i < enemyCount; i++) {
    Enemy *e = &enemies[i];
    e->x -= scr;
    if (player.state == ISLASH_STATE_WAIT) {
      e->x -= sqrt(difficulty);
      e->ticks += sqrt(difficulty);
    }
    char ec[2];
    ec[0] = 'b' + (int)floor(e->ticks / 15) % 2;
    ec[1] = 0;
    character(ec, e->x, ISLASH_PLAYER_Y);
  }
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameISlash() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
