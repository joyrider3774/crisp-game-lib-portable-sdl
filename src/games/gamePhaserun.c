#include "cglp.h"

static char *title = "PHASERUN";
static char *description = "[Tap] Phase\n[Hold] Grow";

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
  Vector pos;
  float size;
  float targetSize;
  float scaleX;
  float scaleY;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float rot;
  bool isAlive;
} Obstacle;
#define MAX_OBSTACLE_COUNT 16
static Obstacle obstacles[MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  Vector pos;
  float rot;
  float bobPhase;
  bool isAlive;
} Coin;
#define MAX_COIN_COUNT 16
static Coin coins[MAX_COIN_COUNT];
static int coinIndex;

typedef struct {
  Vector pos;
  float size;
  int life;
  bool isAlive;
} Trail;
#define MAX_TRAIL_COUNT 32
static Trail trails[MAX_TRAIL_COUNT];
static int trailIndex;

static float spawnTimer;
static float coinTimer;
static bool isSolid;
static float phaseAnimTimer;
static float breathPhase;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 20, 50);
    player.size = 8;
    player.targetSize = 8;
    player.scaleX = 1;
    player.scaleY = 1;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    spawnTimer = 60;
    coinTimer = 0;
    isSolid = true;
    phaseAnimTimer = 0;
    breathPhase = 0;
  }

  // Breathing animation
  breathPhase += 0.08;
  float breathScale = 1 + sin(breathPhase) * 0.03;

  // Input - toggle between solid and ghost phase
  if (input.isJustPressed) {
    isSolid = !isSolid;
    play(SELECT);
    player.scaleX = 1.3;
    player.scaleY = 0.7;
    phaseAnimTimer = 10;
    if (isSolid) {
      color = BLUE;
    } else {
      color = LIGHT_BLUE;
    }
    particle(VEC_XY(player.pos), 12, 2, 0, M_PI * 2);
  }

  // Animate squash/stretch back to normal
  if (phaseAnimTimer > 0) {
    phaseAnimTimer--;
    player.scaleX += (1 - player.scaleX) * 0.3;
    player.scaleY += (1 - player.scaleY) * 0.3;
  } else {
    player.scaleX = breathScale;
    player.scaleY = 2 - breathScale;
  }

  // Spawn obstacles
  spawnTimer--;
  if (spawnTimer <= 0) {
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, 105, 50);
    no->rot = 0;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, MAX_OBSTACLE_COUNT);
    spawnTimer = floor(rnd(88, 111) / sqrt(difficulty));
  }

  // Spawn coins
  coinTimer--;
  if (coinTimer <= 0) {
    ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, ncn);
    vectorSet(&ncn->pos, 105, 50);
    ncn->rot = 0;
    ncn->bobPhase = rnd(0, M_PI * 2);
    ncn->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, MAX_COIN_COUNT);
    coinTimer = floor(rnd(11, 77) / difficulty);
  }

  float speed = 1.0 * difficulty;

  if (input.isPressed) {
    player.targetSize += 3 * sqrt(difficulty);
  } else {
    player.targetSize += (8 - player.targetSize) * 0.5;
  }
  player.size += (player.targetSize - player.size) * 0.3;

  // Add trails for obstacles
  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    color = LIGHT_RED;
    float alpha = (float)t->life / 10;
    box(t->pos.x, t->pos.y, t->size * alpha, t->size * alpha);
    t->isAlive = t->life > 0;
  }

  // Draw player FIRST for collision detection
  float drawSizeX = player.size * player.scaleX;
  float drawSizeY = player.size * player.scaleY;
  color = CYAN;
  box(VEC_XY(player.pos), drawSizeX, drawSizeY);

  // Find nearest threat for eye direction
  bool nearestFound = false;
  float nearestDist = 999;
  float nearestX = 0;
  float nearestY = 0;
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    float d = obs->pos.x - player.pos.x;
    if (d > 0 && d < nearestDist) {
      nearestDist = d;
      nearestX = obs->pos.x;
      nearestY = obs->pos.y;
      nearestFound = true;
    }
  }
  FOR_EACH(coins, ci) {
    ASSIGN_ARRAY_ITEM(coins, ci, Coin, coin);
    SKIP_IS_NOT_ALIVE(coin);
    float d2 = coin->pos.x - player.pos.x;
    if (d2 > 0 && d2 < nearestDist) {
      nearestDist = d2;
      nearestX = coin->pos.x;
      nearestY = coin->pos.y;
      nearestFound = true;
    }
  }

  // Draw coins and check collision
  color = YELLOW;
  FOR_EACH(coins, ci2) {
    ASSIGN_ARRAY_ITEM(coins, ci2, Coin, coin2);
    SKIP_IS_NOT_ALIVE(coin2);
    coin2->pos.x -= speed;
    coin2->rot += 0.1;
    coin2->bobPhase += 0.15;

    float bobY = 50 + sin(coin2->bobPhase) * 3;
    coin2->pos.y = bobY;

    float coinSize = 6;
    thickness = 2;
    bar(VEC_XY(coin2->pos), coinSize, coin2->rot);
    thickness = 2;
    bar(VEC_XY(coin2->pos), coinSize, coin2->rot + M_PI / 2);

    Collision collision = box(VEC_XY(coin2->pos), coinSize, coinSize);

    if (isSolid && collision.isColliding.rect[CYAN]) {
      addScore(ceil(player.size), VEC_XY(coin2->pos));
      play(COIN);
      color = YELLOW;
      particle(VEC_XY(coin2->pos), 8, 2, 0, M_PI * 2);
      coin2->isAlive = false;
      continue;
    }
    coin2->isAlive = coin2->pos.x >= -10;
  }

  // Draw obstacles and check collision
  color = RED;
  FOR_EACH(obstacles, oi2) {
    ASSIGN_ARRAY_ITEM(obstacles, oi2, Obstacle, obs2);
    SKIP_IS_NOT_ALIVE(obs2);
    if (ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
      vectorSet(&nt->pos, obs2->pos.x, obs2->pos.y);
      nt->size = 8;
      nt->life = 10;
      nt->isAlive = true;
      trailIndex = wrap(trailIndex + 1, 0, MAX_TRAIL_COUNT);
    }

    obs2->pos.x -= speed;
    obs2->rot -= 0.15 * speed;

    float obsSize = 10;
    thickness = 3;
    bar(VEC_XY(obs2->pos), obsSize, obs2->rot);
    thickness = 3;
    bar(VEC_XY(obs2->pos), obsSize, obs2->rot + M_PI / 2);

    Collision collision2 = box(VEC_XY(obs2->pos), obsSize, obsSize);

    if (isSolid && collision2.isColliding.rect[CYAN]) {
      play(HIT);
      color = RED;
      particle(VEC_XY(obs2->pos), 20, 3, 0, M_PI * 2);
      gameOver();
    }
    obs2->isAlive = obs2->pos.x >= -10;
  }

  // Draw phase indicator and eyes on top of player
  float eyeOffsetX = 0;
  float eyeOffsetY = 0;
  if (nearestFound) {
    Vector dir;
    vectorSet(&dir, nearestX - player.pos.x, nearestY - player.pos.y);
    float len = vectorLength(&dir);
    if (len > 0) {
      vectorMul(&dir, 1.0 / len);
    }
    eyeOffsetX = dir.x * 1.5;
    eyeOffsetY = dir.y * 1.5;
  }

  if (isSolid) {
    color = BLUE;
    box(player.pos.x, player.pos.y, 5 * player.scaleX, 5 * player.scaleY);
    // Eyes - whites
    color = WHITE;
    float eyeSpacing = 2 * player.scaleX;
    box(player.pos.x - eyeSpacing, player.pos.y - 1, 2, 3);
    box(player.pos.x + eyeSpacing, player.pos.y - 1, 2, 3);
    // Eyes - pupils (look toward threat)
    color = BLACK;
    box(player.pos.x - eyeSpacing + eyeOffsetX * 0.3, player.pos.y - 1 + eyeOffsetY * 0.3, 1, 2);
    box(player.pos.x + eyeSpacing + eyeOffsetX * 0.3, player.pos.y - 1 + eyeOffsetY * 0.3, 1, 2);
  } else {
    color = LIGHT_BLUE;
    box(player.pos.x, player.pos.y, 4 * player.scaleX, 4 * player.scaleY);
    // Ghost eyes - more ethereal
    color = WHITE;
    float eyeSpacing2 = 2 * player.scaleX;
    // Blink occasionally in ghost mode
    if ((ticks / 30) % 4 != 0) {
      box(player.pos.x - eyeSpacing2, player.pos.y - 1, 2, 2);
      box(player.pos.x + eyeSpacing2, player.pos.y - 1, 2, 2);
      color = LIGHT_PURPLE;
      box(player.pos.x - eyeSpacing2 + eyeOffsetX * 0.3, player.pos.y - 1 + eyeOffsetY * 0.3, 1, 1);
      box(player.pos.x + eyeSpacing2 + eyeOffsetX * 0.3, player.pos.y - 1 + eyeOffsetY * 0.3, 1, 1);
    }
  }

  // UI
  color = BLACK;
  if (isSolid) {
    text("SOLID", 3, 10);
  } else {
    text("GHOST", 3, 10);
  }
}

void addGamePhaserun() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
