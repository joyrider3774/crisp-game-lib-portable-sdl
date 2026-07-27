#include "cglp.h"

static char *title = "GRAVITY BUBBLE";
static char *description = "[Hold & Release] \nPlace \n anti-gravity bubbles";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 101, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  float size;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float size;
  int life;
  bool isAlive;
} Bubble;
// Each bubble lives a fixed 180 ticks (3s); a press+release cycle can be as
// short as ~5-7 ticks under realistic rapid tapping, so concurrent count can
// reach ~25-36 - raised above the old 32 cap, which sat right at that edge.
#define GRAVITYBUBBLE_MAX_BUBBLE_COUNT 64
static Bubble bubbles[GRAVITYBUBBLE_MAX_BUBBLE_COUNT];
static int bubbleIndex;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  bool inBubble;
  bool isAlive;
} Enemy;
// Sized generously above the ~2-3 concurrent estimate, extra headroom for bubble-slowed enemies.
#define GRAVITYBUBBLE_MAX_ENEMY_COUNT 64
static Enemy enemies[GRAVITYBUBBLE_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  float width;
  float height;
  bool isAlive;
} Wall;
#define GRAVITYBUBBLE_MAX_WALL_COUNT 32
static Wall walls[GRAVITYBUBBLE_MAX_WALL_COUNT];
static int wallIndex;

static float gameHeight;
static float nextWallHeight;
static float nextEnemySpawn;
static float lastHeightScore;
static bool showingPreview;
static int holdTime;
static Vector savedPreviewPos;
static float savedPreviewSize;
static bool playerInBubble;

static void update() {
  // Never reads a Collision result - all hits are decided by distance/overlap math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    vectorSet(&player.vel, 1, 0);
    player.size = 6;
    INIT_UNALIVED_ARRAY(bubbles);
    bubbleIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    gameHeight = 0;
    nextWallHeight = 150;
    nextEnemySpawn = 60;
    lastHeightScore = 0;
    showingPreview = false;
    holdTime = 0;
    vectorSet(&savedPreviewPos, 0, 0);
    savedPreviewSize = 8;
    playerInBubble = false;
  }

  // === Player Physics ===
  player.vel.y += 0.025;
  player.vel.y *= 0.995;

  player.pos.x += player.vel.x;
  if (player.pos.x > 95 || player.pos.x < 5) {
    player.vel.x *= -1;
  }

  player.pos.y += player.vel.y;

  // === Scroll Processing ===
  if (player.pos.y < 60) {
    float scrollOffset = (60 - player.pos.y) * 0.5;

    player.pos.y += scrollOffset;

    FOR_EACH(bubbles, sbi) {
      ASSIGN_ARRAY_ITEM(bubbles, sbi, Bubble, b);
      SKIP_IS_NOT_ALIVE(b);
      b->pos.y += scrollOffset;
    }
    FOR_EACH(enemies, sei) {
      ASSIGN_ARRAY_ITEM(enemies, sei, Enemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->pos.y += scrollOffset;
    }
    FOR_EACH(walls, swi) {
      ASSIGN_ARRAY_ITEM(walls, swi, Wall, w);
      SKIP_IS_NOT_ALIVE(w);
      w->pos.y += scrollOffset;
    }

    gameHeight += scrollOffset;
  }

  // === Bubble System ===
  if (input.isPressed) {
    showingPreview = true;
    holdTime++;

    int previewFrames = holdTime;

    float vpx = player.pos.x;
    float vpy = player.pos.y;
    float vvx = player.vel.x;
    float vvy = player.vel.y;

    TIMES(previewFrames, simI) {
      vpx += vvx;
      vpy += vvy;
      if (vpx > 95 || vpx < 5) {
        vvx *= -1;
      }
    }

    vectorSet(&savedPreviewPos, vpx, vpy);

    savedPreviewSize = 8 + fmin(25, holdTime * 0.2);

    color = CYAN;
    thickness = 2;
    arc(savedPreviewPos.x, savedPreviewPos.y, savedPreviewSize,
        0, M_PI * 2);
  }

  if (input.isJustReleased && showingPreview) {
    ASSIGN_ARRAY_ITEM(bubbles, bubbleIndex, Bubble, nb);
    vectorSet(&nb->pos, savedPreviewPos.x, savedPreviewPos.y);
    nb->size = savedPreviewSize;
    nb->life = 180;
    nb->isAlive = true;
    bubbleIndex = wrap(bubbleIndex + 1, 0, GRAVITYBUBBLE_MAX_BUBBLE_COUNT);

    play(POWER_UP);
    particle(savedPreviewPos.x, savedPreviewPos.y,
             floor(savedPreviewSize / 3), 1.5, 0, M_PI * 2);
  }

  if (!input.isPressed) {
    showingPreview = false;
    holdTime = 0;
  }

  // === Altitude Calculation ===
  float currentHeight = gameHeight + fmax(0, 100 - player.pos.y);

  // === Enemy Generation System ===
  int difficultyLevel = (int)floor(currentHeight / 100) + 1;

  if (ticks >= nextEnemySpawn) {
    float enemySpeed = rnd(0.2, 0.25 + sqrt(difficultyLevel) * 0.08);
    int levelHalf = (int)floor((float)difficultyLevel / 2.0);
    int sizeExtra = (int)fmin(3, levelHalf);
    float enemySize = 7 + sizeExtra;

    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, rnd(15, 85), -5);
    vectorSet(&ne->vel, rnd(-0.6, 0.6), enemySpeed);
    ne->size = enemySize;
    ne->inBubble = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, GRAVITYBUBBLE_MAX_ENEMY_COUNT);

    int baseInterval = (int)fmax(45, 120 - difficultyLevel * 10);
    float variation = baseInterval * 0.3;
    nextEnemySpawn = ticks + baseInterval + rnd(-variation, variation);
  }

  // === Wall Generation System ===
  if (difficultyLevel > 2 && currentHeight >= nextWallHeight) {
    float wallLength = rnd(25, 50);
    float wallX = rnd(0, 100 - wallLength);

    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, nw);
    vectorSet(&nw->pos, wallX + wallLength / 2, -5);
    nw->width = wallLength;
    nw->height = 4;
    nw->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, GRAVITYBUBBLE_MAX_WALL_COUNT);

    nextWallHeight = currentHeight + rnd(70, 150);
  }

  // === Bubble Processing ===
  bool wasInBubble = playerInBubble;
  playerInBubble = false;

  FOR_EACH(bubbles, bi) {
    ASSIGN_ARRAY_ITEM(bubbles, bi, Bubble, b);
    SKIP_IS_NOT_ALIVE(b);
    b->life--;

    float shrinkPhase = fmax(0, 150 - b->life);
    float currentBubbleSize = b->size * (1 - shrinkPhase / 150);

    float distanceToPlayer = distanceTo(&player.pos, b->pos.x, b->pos.y);
    if (distanceToPlayer < currentBubbleSize + 9) {
      player.vel.y -= 0.05;
      playerInBubble = true;
    }

    FOR_EACH(enemies, bei) {
      ASSIGN_ARRAY_ITEM(enemies, bei, Enemy, e);
      SKIP_IS_NOT_ALIVE(e);
      float distanceToEnemy = distanceTo(&e->pos, b->pos.x, b->pos.y);
      if (distanceToEnemy < currentBubbleSize + e->size / 2) {
        e->vel.y -= 0.08;
        e->inBubble = true;
      } else {
        e->inBubble = false;
      }
    }

    color = CYAN;
    thickness = 1;
    arc(b->pos.x, b->pos.y, currentBubbleSize, 0, M_PI * 2);

    if (b->life <= 0) {
      b->isAlive = false;
      continue;
    }
  }

  if (!wasInBubble && playerInBubble) {
    particle(player.pos.x, player.pos.y, 5, 1, 0, M_PI * 2);
  } else if (wasInBubble && !playerInBubble) {
    play(SELECT);
  }

  // === Enemy Processing and Drawing ===
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!e->inBubble) {
      e->vel.y += 0.02;
    }

    vectorAdd(&e->pos, e->vel.x, e->vel.y);

    if (e->pos.x > 95 || e->pos.x < 5) {
      e->vel.x *= -1;
    }

    FOR_EACH(walls, ewi) {
      ASSIGN_ARRAY_ITEM(walls, ewi, Wall, w);
      SKIP_IS_NOT_ALIVE(w);
      float enemyLeft = e->pos.x - e->size / 2;
      float enemyRight = e->pos.x + e->size / 2;
      float wallLeft = w->pos.x - w->width / 2;
      float wallRight = w->pos.x + w->width / 2;

      bool xOverlap = enemyRight > wallLeft && enemyLeft < wallRight;
      bool yOverlap = fabs(e->pos.y - w->pos.y) < (e->size + w->height) / 2;

      if (xOverlap && yOverlap) {
        e->vel.y *= -0.6;
        if (e->pos.y < w->pos.y) {
          e->pos.y = w->pos.y - (w->height + e->size) / 2 - 1;
        } else {
          e->pos.y = w->pos.y + (w->height + e->size) / 2 + 1;
        }
      }
    }

    if (e->inBubble) {
      color = YELLOW;
    } else {
      color = RED;
    }
    box(e->pos.x, e->pos.y, e->size, e->size);

    float distanceToPlayerE = distanceTo(&player.pos, e->pos.x, e->pos.y);
    if (distanceToPlayerE < (player.size + e->size) / 2 + 2) {
      float impulse = e->vel.y * 1.5;
      player.vel.y += impulse;

      play(HIT);
      particle(e->pos.x, e->pos.y, 25, 2, 0, M_PI * 2);
      addScore(100, VEC_XY(e->pos));

      e->isAlive = false;
      continue;
    }

    if (e->pos.y > 110) {
      e->isAlive = false;
      continue;
    }
  }

  // === Wall Processing and Drawing ===
  FOR_EACH(walls, wdi) {
    ASSIGN_ARRAY_ITEM(walls, wdi, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    color = GREEN;
    box(w->pos.x, w->pos.y, w->width, w->height);

    float playerLeft = player.pos.x - player.size / 2;
    float playerRight = player.pos.x + player.size / 2;
    float wallLeft = w->pos.x - w->width / 2;
    float wallRight = w->pos.x + w->width / 2;

    bool xOverlap = playerRight > wallLeft && playerLeft < wallRight;
    bool yOverlap = fabs(player.pos.y - w->pos.y) < (player.size + w->height) / 2;

    if (xOverlap && yOverlap) {
      player.vel.y *= -0.8;
      play(HIT);
      particle(w->pos.x, w->pos.y, 5, 1.5, 0, M_PI * 2);

      if (player.pos.y < w->pos.y) {
        player.pos.y = w->pos.y - (w->height + player.size) / 2 - 1;
      } else {
        player.pos.y = w->pos.y + (w->height + player.size) / 2 + 1;
      }
    }

    if (w->pos.y > 110) {
      w->isAlive = false;
      continue;
    }
  }

  // === Player Drawing ===
  if (playerInBubble) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  box(player.pos.x, player.pos.y, player.size, player.size);

  // === Score Update ===
  float currentHeightScore = floor(currentHeight);
  if (currentHeightScore > lastHeightScore) {
    addScore(currentHeightScore - lastHeightScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    lastHeightScore = currentHeightScore;
  }

  if (player.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameGravityBubble() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
