#include "cglp.h"

static char *title = "TECTONIC TETHER";
static char *description = "[Hold] Charge & Retract\n[Release] Extend";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 53, .isDarkColor = false};

#define TECTONICTETHER_TETHER_SPEED 0.7
#define TECTONICTETHER_BASE_ANGULAR_VELOCITY 0.01
#define TECTONICTETHER_MAX_ANGULAR_VELOCITY 0.05
#define TECTONICTETHER_MAX_CHARGE_POWER 5
#define TECTONICTETHER_MAX_ABSORPTION_POWER 30
#define TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD 10
#define TECTONICTETHER_EMPTY_ABSORPTION_THRESHOLD 0
#define TECTONICTETHER_ABSORPTION_DECAY_RATE 0.05
#define TECTONICTETHER_ABSORPTION_RECOVERY_RATE 1.5
#define TECTONICTETHER_ASTEROID_COUNT 5

typedef struct {
  float angle;
  float angularVelocity;
  float tetherLength;
  Vector pos;
} PlayerShard;
static PlayerShard playerShard;

typedef struct {
  Vector pos;
  float growthTime;
  bool isCollectible;
  Vector vel;
  bool isAlive;
} GeodeFragment;
// Geode fragments never expire upstream either (unbounded growth); sized generously but worth a second look.
#define TECTONICTETHER_MAX_GEODE_COUNT 1024
static GeodeFragment geodeFragments[TECTONICTETHER_MAX_GEODE_COUNT];
static int geodeFragmentIndex;

typedef struct {
  Vector pos;
  float size;
  Vector vel;
} Asteroid;
static Asteroid asteroids[TECTONICTETHER_ASTEROID_COUNT];

static float chargePower;
static float extensionBoost;
static float absorptionPower;

static void update() {
  if (!ticks) {
    // "sss.setQuantize(16)" is a custom sound sequencer call with no
    // equivalent in this engine - dropped.
    playerShard.angle = 0;
    playerShard.angularVelocity = TECTONICTETHER_BASE_ANGULAR_VELOCITY;
    playerShard.tetherLength = 35;
    vectorSet(&playerShard.pos, 50, 10);

    INIT_UNALIVED_ARRAY(geodeFragments);
    geodeFragmentIndex = 0;
    TIMES(7, gi) {
      ASSIGN_ARRAY_ITEM(geodeFragments, geodeFragmentIndex, GeodeFragment, ng);
      vectorSet(&ng->pos, rnd(10, 90), rnd(10, 90));
      ng->growthTime = 60;
      ng->isCollectible = true;
      vectorSet(&ng->vel, 0, 0);
      ng->isAlive = true;
      geodeFragmentIndex = wrap(geodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
    }

    TIMES(TECTONICTETHER_ASTEROID_COUNT, asi) {
      vectorSet(&asteroids[asi].pos, rnd(20, 80), rnd(20, 80));
      asteroids[asi].size = rnd(8, 12);
      vectorSet(&asteroids[asi].vel, 0, 0);
    }

    chargePower = 0;
    extensionBoost = 0;
    absorptionPower = TECTONICTETHER_MAX_ABSORPTION_POWER;
  }

  // --- Player Logic ---
  if (input.isPressed) {
    // Shorten tether
    playerShard.tetherLength -= TECTONICTETHER_TETHER_SPEED;
    playerShard.angularVelocity += 0.001;
    // Build charge only when tether is short
    if (playerShard.tetherLength <= 10) {
      chargePower = fmin(chargePower + 0.1, TECTONICTETHER_MAX_CHARGE_POWER);
    }
  } else {
    // Extend tether
    if (input.isJustReleased) {
      // Unleash the charged boost
      extensionBoost = chargePower;
      chargePower = 0;
      if (extensionBoost > 1) {
        play(LASER);
      }
    }
    playerShard.tetherLength += TECTONICTETHER_TETHER_SPEED + extensionBoost;
    playerShard.angularVelocity -= 0.0005;
    // Decay the boost
    extensionBoost *= 0.92;
  }

  playerShard.angularVelocity = clamp(playerShard.angularVelocity,
                                       TECTONICTETHER_BASE_ANGULAR_VELOCITY,
                                       TECTONICTETHER_MAX_ANGULAR_VELOCITY);

  playerShard.angle += playerShard.angularVelocity;
  vectorSet(&playerShard.pos, 50, 50);
  addWithAngle(&playerShard.pos, playerShard.angle, playerShard.tetherLength);

  // --- Absorption Power Logic ---
  absorptionPower -= TECTONICTETHER_ABSORPTION_DECAY_RATE * difficulty;

  if (absorptionPower <= TECTONICTETHER_EMPTY_ABSORPTION_THRESHOLD) {
    play(EXPLOSION);
    // This particle() call has no explicit color() of its own upstream
    // (it fires before any drawing this frame) - red (matching the
    // explosion) is used explicitly here instead.
    color = RED;
    particle(playerShard.pos.x, playerShard.pos.y, 20, 5, 0, M_PI * 2);
    gameOver();
  }

  // Wall collision for player
  if (playerShard.pos.x < 3) {
    playerShard.pos.x = 3;
  }
  if (playerShard.pos.x > 97) {
    playerShard.pos.x = 97;
  }
  if (playerShard.pos.y < 3) {
    playerShard.pos.y = 3;
  }
  if (playerShard.pos.y > 97) {
    playerShard.pos.y = 97;
  }
  playerShard.tetherLength = distanceTo(&playerShard.pos, 50, 50);
  playerShard.tetherLength = clamp(playerShard.tetherLength, 10, 999);

  // --- Drawing ---
  color = LIGHT_BLACK;
  rect(0, 0, 100, 2);
  rect(0, 98, 100, 2);
  rect(0, 0, 2, 100);
  rect(98, 0, 2, 100);

  color = BLACK;
  thickness = 1;
  line(50, 50, playerShard.pos.x, playerShard.pos.y);

  // Draw charge power as yellow line overlapping the tether
  if (chargePower > 0) {
    color = YELLOW;
    thickness = 1 + chargePower * 0.7;
    line(50, 50, playerShard.pos.x, playerShard.pos.y);
  }

  // Draw absorption power as yellow rectangle overlapping player
  color = YELLOW;
  float absorptionSize = 6 + absorptionPower * 0.25;
  rect(playerShard.pos.x - absorptionSize / 2, playerShard.pos.y - absorptionSize / 2,
       absorptionSize, absorptionSize);

  // Apply danger effects when absorption power is low
  float playerPosX = playerShard.pos.x;
  float playerPosY = playerShard.pos.y;
  if (absorptionPower <= TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD) {
    float shakeIntensity = (TECTONICTETHER_DANGER_ABSORPTION_THRESHOLD - absorptionPower) * 0.2;
    playerPosX += rnd(-shakeIntensity, shakeIntensity);
    playerPosY += rnd(-shakeIntensity, shakeIntensity);

    if (ticks % 10 < 3) {
      color = RED;
      if (ticks % 10 == 0) {
        play(CLICK);
      }
    } else {
      color = CYAN;
    }
  } else {
    color = CYAN;
  }

  box(playerPosX, playerPosY, 6, 6);

  color = GREEN;
  FOR_EACH(geodeFragments, gfi) {
    ASSIGN_ARRAY_ITEM(geodeFragments, gfi, GeodeFragment, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->growthTime < 60) {
      g->growthTime++;
      if (g->growthTime >= 60) {
        g->isCollectible = true;
        vectorSet(&g->vel, 0, 0);
      }
    }

    if (!g->isCollectible) {
      vectorAdd(&g->pos, g->vel.x, g->vel.y);
      vectorMul(&g->vel, 0.96);
    }

    float growthRatio = g->growthTime / 60;
    float currentSize = 1 + 4 * growthRatio;

    float attractionRange = 15 + absorptionPower;
    if (g->isCollectible &&
        distanceTo(&g->pos, playerShard.pos.x, playerShard.pos.y) < attractionRange) {
      float attractionStrength = 0.3 + absorptionPower * 0.05;
      float toPlayerX = playerShard.pos.x - g->pos.x;
      float toPlayerY = playerShard.pos.y - g->pos.y;
      float toPlayerLen = sqrt(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
      // Real division by zero hard-traps the CPU on Vircon32 (unlike JS) -
      // guard before normalizing toward the player.
      if (toPlayerLen > 0) {
        vectorAdd(&g->pos, toPlayerX / toPlayerLen * attractionStrength,
                  toPlayerY / toPlayerLen * attractionStrength);
      }
    }

    g->pos.x = clamp(g->pos.x, 5, 95);
    g->pos.y = clamp(g->pos.y, 5, 95);

    Collision geodeCollision = box(g->pos.x, g->pos.y, currentSize, currentSize);

    if (g->isCollectible && geodeCollision.isColliding.rect[CYAN]) {
      play(COIN);
      // Bare particle() call upstream inherits whatever color the frame
      // last set - still "green" from this loop's own color() above,
      // made explicit here for clarity.
      color = GREEN;
      particle(g->pos.x, g->pos.y, 16, 1, 0, M_PI * 2);
      absorptionPower =
          fmin(absorptionPower + TECTONICTETHER_ABSORPTION_RECOVERY_RATE,
               TECTONICTETHER_MAX_ABSORPTION_POWER);
      addScore(1, g->pos.x, g->pos.y);
      g->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  TIMES(TECTONICTETHER_ASTEROID_COUNT, ai) {
    Asteroid *a = &asteroids[ai];
    vectorAdd(&a->pos, a->vel.x, a->vel.y);
    vectorMul(&a->vel, 0.95);

    Collision asteroidCollision = box(a->pos.x, a->pos.y, a->size, a->size);

    // Player collision
    if (asteroidCollision.isColliding.rect[CYAN]) {
      float playerDist = distanceTo(&playerShard.pos, 50, 50);
      float asteroidDist = distanceTo(&a->pos, 50, 50);

      float collisionStrength =
          playerShard.angularVelocity * (playerShard.tetherLength / 20);

      play(HIT);

      if (playerDist < asteroidDist) {
        playerShard.tetherLength -= 5;
      } else {
        playerShard.tetherLength += 5;
      }

      float knockbackX = a->pos.x - playerShard.pos.x;
      float knockbackY = a->pos.y - playerShard.pos.y;
      float knockbackLen = sqrt(knockbackX * knockbackX + knockbackY * knockbackY);
      float knockbackUnitX = 0;
      float knockbackUnitY = 0;
      if (knockbackLen > 0) {
        knockbackUnitX = knockbackX / knockbackLen;
        knockbackUnitY = knockbackY / knockbackLen;
      }
      vectorAdd(&a->vel, knockbackUnitX * 0.8, knockbackUnitY * 0.8);

      // Spawn geodes based on collision strength
      int numGeodes = (int)(collisionStrength * 20 * difficulty);

      // Calculate base direction (opposite to knockback)
      float baseDirX = -knockbackUnitX;
      float baseDirY = -knockbackUnitY;
      float baseAngle = atan2(baseDirY, baseDirX);

      int kg;
      for (kg = 0; kg < numGeodes; kg++) {
        float spreadAngle = rnd(-M_PI / 3, M_PI / 3);
        float finalAngle = baseAngle + spreadAngle;

        float speed = rnd(1.5, 3.5);
        float ivx = cos(finalAngle) * speed * 0.3;
        float ivy = sin(finalAngle) * speed * 0.3;

        float spawnX = clamp(a->pos.x + rnd(-3, 3), 10, 90);
        float spawnY = clamp(a->pos.y + rnd(-3, 3), 10, 90);

        ASSIGN_ARRAY_ITEM(geodeFragments, geodeFragmentIndex, GeodeFragment, ngA);
        vectorSet(&ngA->pos, spawnX, spawnY);
        ngA->growthTime = 0;
        ngA->isCollectible = false;
        vectorSet(&ngA->vel, ivx, ivy);
        ngA->isAlive = true;
        geodeFragmentIndex = wrap(geodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
      }
    }

    // Wall collision (destruction)
    if (asteroidCollision.isColliding.rect[LIGHT_BLACK]) {
      play(EXPLOSION);
      // Bare particle(), same reasoning as the geode collect case above
      // (still "black" from this loop's own color()).
      color = BLACK;
      particle(a->pos.x, a->pos.y, 16, 1, 0, M_PI * 2);
      addScore(10, a->pos.x, a->pos.y);

      int numGeodes2 = (int)((a->size / 2 + 1) * difficulty);
      int kg2;
      for (kg2 = 0; kg2 < numGeodes2; kg2++) {
        float angle2 = rnd(0, M_PI * 2);
        float speed2 = rnd(1.0, 1.5);
        float ivx2 = cos(angle2) * speed2;
        float ivy2 = sin(angle2) * speed2;

        float spawnX2 = clamp(a->pos.x + rnd(-8, 8), 10, 90);
        float spawnY2 = clamp(a->pos.y + rnd(-8, 8), 10, 90);

        ASSIGN_ARRAY_ITEM(geodeFragments, geodeFragmentIndex, GeodeFragment, ngB);
        vectorSet(&ngB->pos, spawnX2, spawnY2);
        ngB->growthTime = 0;
        ngB->isCollectible = false;
        vectorSet(&ngB->vel, ivx2, ivy2);
        ngB->isAlive = true;
        geodeFragmentIndex = wrap(geodeFragmentIndex + 1, 0, TECTONICTETHER_MAX_GEODE_COUNT);
      }

      vectorSet(&a->pos, rnd(20, 80), rnd(20, 80));
      vectorSet(&a->vel, 0, 0);
    }

    // Asteroid vs Asteroid collision
    int aj;
    for (aj = ai + 1; aj < TECTONICTETHER_ASTEROID_COUNT; aj++) {
      Asteroid *b = &asteroids[aj];
      float dist = distanceTo(&a->pos, b->pos.x, b->pos.y);
      float minDist = (a->size + b->size) / 2;
      if (dist < minDist) {
        play(CLICK);
        float overlap = minDist - dist;
        float pushX = a->pos.x - b->pos.x;
        float pushY = a->pos.y - b->pos.y;
        float pushLen = sqrt(pushX * pushX + pushY * pushY);
        if (pushLen > 0) {
          float pushNormX = pushX / pushLen;
          float pushNormY = pushY / pushLen;
          vectorAdd(&a->pos, pushNormX * (overlap / 2), pushNormY * (overlap / 2));
          vectorAdd(&b->pos, -pushNormX * (overlap / 2), -pushNormY * (overlap / 2));
          float knockX = pushNormX * 0.4;
          float knockY = pushNormY * 0.4;
          vectorAdd(&a->vel, knockX, knockY);
          vectorAdd(&b->vel, -knockX, -knockY);
        }
      }
    }
  }
}

void addGameTectonicTether() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
