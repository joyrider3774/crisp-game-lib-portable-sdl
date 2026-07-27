#include "cglp.h"
#include "string.h"

static char *title = "QUANTUM LEAPER";
static char *description = "[Tap] Change lane";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define NUM_UNIVERSES 5
#define WAVE_AMPLITUDE 10
#define WAVE_FREQUENCY 0.1

typedef struct {
  Vector pos;
  float waveOffset;
  int currentUniverse;
  float nextAddingScoreOffset;
} Player;
static Player player;

typedef struct {
  Vector pos;
  int universeIdx;
  bool isAlive;
} AntiParticle;
#define MAX_ANTIPARTICLE_COUNT 16
static AntiParticle antiParticles[MAX_ANTIPARTICLE_COUNT];
static int antiParticleIndex;

static float universeLanesY[NUM_UNIVERSES];

static int goldLaneIndex;
static float changingGoldLaneTicks;
static int multiplier;
static float scrollingSpeed;

static void initializeGame() {
  vectorSet(&player.pos, 20, 50);
  player.waveOffset = 0;
  player.currentUniverse = 2;
  player.nextAddingScoreOffset = 0;
  INIT_UNALIVED_ARRAY(antiParticles);
  antiParticleIndex = 0;
  TIMES(NUM_UNIVERSES, i) {
    universeLanesY[i] = 20 + i * 70.0 / (NUM_UNIVERSES - 1);
  }
  goldLaneIndex = 2;
  changingGoldLaneTicks = 300;
  scrollingSpeed = 1;
  multiplier = 1;
}

static void updateScrollingSpeed() { scrollingSpeed = difficulty; }

static void updatePlayerPosition() {
  player.waveOffset += WAVE_FREQUENCY;
  if (player.waveOffset >= player.nextAddingScoreOffset) {
    if (player.currentUniverse == goldLaneIndex) {
      play(COIN);
      addScore(multiplier, VEC_XY(player.pos));
      multiplier = (int)clamp(multiplier + 1, 1, 99);
    } else {
      play(HIT);
      multiplier = (int)clamp(multiplier - 1, 1, 99);
    }
    player.nextAddingScoreOffset += M_PI;
  }
  player.pos.y = universeLanesY[player.currentUniverse] +
                 sin(player.waveOffset) * WAVE_AMPLITUDE;
}

static void handlePlayerInput() {
  if (input.isJustPressed) {
    float dir;
    if (sin(player.waveOffset) > 0) {
      dir = 1;
    } else {
      dir = -1;
    }
    player.currentUniverse = (int)wrap(player.currentUniverse + dir, 0, NUM_UNIVERSES);
  }
}

static void updateAndDrawUniverseLanes() {
  changingGoldLaneTicks--;
  if (changingGoldLaneTicks < 0) {
    goldLaneIndex = rndi(0, NUM_UNIVERSES);
    changingGoldLaneTicks += 300;
  }
  TIMES(NUM_UNIVERSES, i) {
    if (i == goldLaneIndex) {
      color = YELLOW;
    } else {
      color = LIGHT_BLUE;
    }
    thickness = 3;
    line(0, universeLanesY[i], 100, universeLanesY[i]);
  }
}

static void updateAndDrawAntiParticles() {
  color = PURPLE;
  FOR_EACH(antiParticles, i) {
    ASSIGN_ARRAY_ITEM(antiParticles, i, AntiParticle, ap);
    SKIP_IS_NOT_ALIVE(ap);
    ap->pos.x -= scrollingSpeed;
    if (box(VEC_XY(ap->pos), 3, 3).isColliding.rect[CYAN] || ap->pos.x < 0) {
      ap->isAlive = false;
      continue;
    }
  }
}

static void drawPlayer() {
  color = CYAN;
  thickness = 3;
  arc(VEC_XY(player.pos), 2, 0, M_PI * 2);
}

static void checkCollisions() {
  if (box(VEC_XY(player.pos), 4, 4).isColliding.rect[PURPLE]) {
    play(EXPLOSION);
    gameOver();
  }
}

static void spawnAntiParticles() {
  if (rnd(0, 1) < 0.02 * difficulty) {
    play(CLICK);
    int universeIdx = rndi(0, NUM_UNIVERSES);
    ASSIGN_ARRAY_ITEM(antiParticles, antiParticleIndex, AntiParticle, nap);
    vectorSet(&nap->pos, 103, universeLanesY[universeIdx]);
    nap->universeIdx = universeIdx;
    nap->isAlive = true;
    antiParticleIndex = wrap(antiParticleIndex + 1, 0, MAX_ANTIPARTICLE_COUNT);
  }
}

static void update() {
  if (!ticks) {
    initializeGame();
  }
  updateScrollingSpeed();
  updatePlayerPosition();
  handlePlayerInput();
  updateAndDrawUniverseLanes();
  updateAndDrawAntiParticles();
  drawPlayer();
  checkCollisions();
  spawnAntiParticles();
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 2, 9);
}

void addGameQuantumLeaper() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
