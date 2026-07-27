#include "cglp.h"
#include "string.h"

static char *title = "ECHO BRIDGE";
static char *description = "[Press] Create bridge";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector size;
  bool hasBeenUsed;
  bool isAlive;
} EchobridgeIsland;
#define ECHOBRIDGE_MAX_ISLAND_COUNT 32
static EchobridgeIsland echobridgeIslands[ECHOBRIDGE_MAX_ISLAND_COUNT];
static int echobridgeIslandIndex;

// Bridges store an island index instead of a live reference, same pattern as AccelbPlayerMissile.
typedef struct {
  Vector startPos;
  Vector endPos;
  int targetIslandIdx;
  bool hasStartIsland;
  int startIslandIdx;
  bool isAlive;
} EchobridgeBridge;
#define ECHOBRIDGE_MAX_BRIDGE_COUNT 32
static EchobridgeBridge echobridgeBridges[ECHOBRIDGE_MAX_BRIDGE_COUNT];
static int echobridgeBridgeIndex;

static Vector echobridgePlayerPos;
static float echobridgePlayerSize;
static bool echobridgePlayerOnIsland;

static int echobridgeCurrentIslandIdx;
static int echobridgeActiveBridgeIdx;
static float echobridgeRotationAngle;
static float echobridgeNextIslandDistance;
static float echobridgeIslandSpawnY;
static float echobridgeLastIslandX;
static float echobridgeMaterialGauge;

static void echobridgeNormalize(Vector *v) {
  float len = vectorLength(v);
  if (len > 0) {
    vectorMul(v, 1.0 / len);
  } else {
    vectorSet(v, 0, 0);
  }
}

static void update() {
  // Never reads a Collision result - bridge/island logic is decided by distance math.
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(echobridgeIslands);
    echobridgeIslandIndex = 0;
    ASSIGN_ARRAY_ITEM(echobridgeIslands, echobridgeIslandIndex, EchobridgeIsland, firstIsland);
    vectorSet(&firstIsland->pos, 50, 0);
    vectorSet(&firstIsland->size, 15, 8);
    firstIsland->hasBeenUsed = false;
    firstIsland->isAlive = true;
    echobridgeCurrentIslandIdx = echobridgeIslandIndex;
    echobridgeIslandIndex = wrap(echobridgeIslandIndex + 1, 0, ECHOBRIDGE_MAX_ISLAND_COUNT);

    echobridgePlayerPos = firstIsland->pos;
    echobridgePlayerSize = 3;
    echobridgePlayerOnIsland = true;

    INIT_UNALIVED_ARRAY(echobridgeBridges);
    echobridgeBridgeIndex = 0;
    echobridgeRotationAngle = 0;
    echobridgeActiveBridgeIdx = -1;

    echobridgeNextIslandDistance = 10;
    echobridgeIslandSpawnY = -echobridgeNextIslandDistance;
    echobridgeLastIslandX = 50;
    echobridgeMaterialGauge = 80;
  }

  echobridgeRotationAngle += 0.05 * difficulty;

  bool canBuildBridge = false;
  if (echobridgePlayerOnIsland) {
    FOR_EACH(echobridgeIslands, cbi) {
      ASSIGN_ARRAY_ITEM(echobridgeIslands, cbi, EchobridgeIsland, island);
      SKIP_IS_NOT_ALIVE(island);
      if (cbi != echobridgeCurrentIslandIdx && !island->hasBeenUsed) {
        if (island->pos.x >= 0 && island->pos.x <= 100 && island->pos.y >= 0 && island->pos.y <= 100) {
          float bridgeLength = distanceTo(&echobridgePlayerPos, island->pos.x, island->pos.y);
          float materialCost = bridgeLength * 0.25;
          if (materialCost <= echobridgeMaterialGauge) {
            canBuildBridge = true;
          }
        }
      }
    }
  }

  float baseScrollSpeed = 0.05 * difficulty;
  float constantScrollAmount;
  if (echobridgePlayerOnIsland && !canBuildBridge) {
    constantScrollAmount = baseScrollSpeed * 3;
  } else {
    constantScrollAmount = baseScrollSpeed;
  }

  FOR_EACH(echobridgeIslands, si) {
    ASSIGN_ARRAY_ITEM(echobridgeIslands, si, EchobridgeIsland, island);
    SKIP_IS_NOT_ALIVE(island);
    island->pos.y += constantScrollAmount;
  }
  FOR_EACH(echobridgeBridges, sbi) {
    ASSIGN_ARRAY_ITEM(echobridgeBridges, sbi, EchobridgeBridge, bridge);
    SKIP_IS_NOT_ALIVE(bridge);
    bridge->startPos.y += constantScrollAmount;
    bridge->endPos.y += constantScrollAmount;
  }
  echobridgePlayerPos.y += constantScrollAmount;
  echobridgeIslandSpawnY += constantScrollAmount;

  float additionalScrollAmount = 0;
  if (echobridgePlayerPos.y < 60) {
    additionalScrollAmount = 60 - echobridgePlayerPos.y;
    echobridgePlayerPos.y = 60;
    FOR_EACH(echobridgeIslands, si2) {
      ASSIGN_ARRAY_ITEM(echobridgeIslands, si2, EchobridgeIsland, island2);
      SKIP_IS_NOT_ALIVE(island2);
      island2->pos.y += additionalScrollAmount;
    }
    FOR_EACH(echobridgeBridges, sbi2) {
      ASSIGN_ARRAY_ITEM(echobridgeBridges, sbi2, EchobridgeBridge, bridge2);
      SKIP_IS_NOT_ALIVE(bridge2);
      bridge2->startPos.y += additionalScrollAmount;
      bridge2->endPos.y += additionalScrollAmount;
    }
    echobridgeIslandSpawnY += additionalScrollAmount;
  }

  while (echobridgeIslandSpawnY > -10) {
    float newX;
    int attempts = 0;
    do {
      newX = rnd(0, 100);
      attempts++;
    } while (attempts < 10 && fabs(newX - echobridgeLastIslandX) < 25);
    if (fabs(newX - echobridgeLastIslandX) < 25) {
      if (echobridgeLastIslandX < 50) {
        newX = rnd(60, 100);
      } else {
        newX = rnd(0, 40);
      }
    }

    ASSIGN_ARRAY_ITEM(echobridgeIslands, echobridgeIslandIndex, EchobridgeIsland, newIsland);
    vectorSet(&newIsland->pos, newX, echobridgeIslandSpawnY);
    vectorSet(&newIsland->size, rnd(10, 16), rnd(5, 8));
    newIsland->hasBeenUsed = false;
    newIsland->isAlive = true;
    echobridgeIslandIndex = wrap(echobridgeIslandIndex + 1, 0, ECHOBRIDGE_MAX_ISLAND_COUNT);

    echobridgeLastIslandX = newX;
    echobridgeNextIslandDistance = rnd(5, 20);
    echobridgeIslandSpawnY -= echobridgeNextIslandDistance;
  }

  if (input.isJustPressed) {
    int targetIslandIdx = -1;
    float bestAlignment = -1;

    FOR_EACH(echobridgeIslands, ti) {
      ASSIGN_ARRAY_ITEM(echobridgeIslands, ti, EchobridgeIsland, island3);
      SKIP_IS_NOT_ALIVE(island3);
      if (ti != echobridgeCurrentIslandIdx && !island3->hasBeenUsed) {
        if (island3->pos.x >= 0 && island3->pos.x <= 100 && island3->pos.y >= 0 && island3->pos.y <= 100) {
          Vector directionToIsland;
          vectorSet(&directionToIsland, island3->pos.x - echobridgePlayerPos.x, island3->pos.y - echobridgePlayerPos.y);
          echobridgeNormalize(&directionToIsland);
          Vector rotationDirection;
          vectorSet(&rotationDirection, 0, 0);
          addWithAngle(&rotationDirection, echobridgeRotationAngle, 1);

          float alignment = directionToIsland.x * rotationDirection.x + directionToIsland.y * rotationDirection.y;

          if (alignment > 0.3 && alignment > bestAlignment) {
            float bridgeLength = distanceTo(&echobridgePlayerPos, island3->pos.x, island3->pos.y);
            float materialCost = bridgeLength * 0.25;
            if (materialCost <= echobridgeMaterialGauge) {
              bestAlignment = alignment;
              targetIslandIdx = ti;
            }
          }
        }
      }
    }

    if (targetIslandIdx >= 0) {
      ASSIGN_ARRAY_ITEM(echobridgeBridges, echobridgeBridgeIndex, EchobridgeBridge, newBridge);
      newBridge->startPos = echobridgePlayerPos;
      newBridge->endPos = echobridgeIslands[targetIslandIdx].pos;
      newBridge->targetIslandIdx = targetIslandIdx;
      newBridge->hasStartIsland = echobridgePlayerOnIsland;
      newBridge->startIslandIdx = echobridgeCurrentIslandIdx;
      newBridge->isAlive = true;
      int newBridgeIdx = echobridgeBridgeIndex;
      echobridgeBridgeIndex = wrap(echobridgeBridgeIndex + 1, 0, ECHOBRIDGE_MAX_BRIDGE_COUNT);

      echobridgeActiveBridgeIdx = newBridgeIdx;
      echobridgeIslands[targetIslandIdx].hasBeenUsed = true;

      float bridgeLength2 = distanceTo(&newBridge->startPos, newBridge->endPos.x, newBridge->endPos.y);
      float materialCost2 = bridgeLength2 * 0.25;
      echobridgeMaterialGauge = fmax(0, echobridgeMaterialGauge - materialCost2);

      play(POWER_UP);
    } else {
      echobridgeMaterialGauge = fmax(0, echobridgeMaterialGauge - 10);
      play(HIT);
    }
  }

  COUNT_IS_ALIVE(echobridgeBridges, echobridgeMultiplierNow);

  FOR_EACH(echobridgeBridges, bi) {
    ASSIGN_ARRAY_ITEM(echobridgeBridges, bi, EchobridgeBridge, bridge);
    SKIP_IS_NOT_ALIVE(bridge);

    color = LIGHT_BLUE;
    if (bi == echobridgeActiveBridgeIdx) {
      thickness = 3;
    } else {
      thickness = 2;
    }
    line(bridge->startPos.x, bridge->startPos.y, bridge->endPos.x, bridge->endPos.y);

    if (input.isPressed && bi == echobridgeActiveBridgeIdx) {
      Vector bridgeVector;
      vectorSet(&bridgeVector, bridge->endPos.x - bridge->startPos.x, bridge->endPos.y - bridge->startPos.y);
      Vector playerVector;
      vectorSet(&playerVector, echobridgePlayerPos.x - bridge->startPos.x, echobridgePlayerPos.y - bridge->startPos.y);
      float bridgeLen = vectorLength(&bridgeVector);

      if (bridgeLen > 0) {
        float projection = playerVector.x * bridgeVector.x + playerVector.y * bridgeVector.y;
        float t = projection / (bridgeLen * bridgeLen);

        if (t >= 0 && t <= 1) {
          Vector closestPoint;
          vectorSet(&closestPoint, bridge->startPos.x + bridgeVector.x * t, bridge->startPos.y + bridgeVector.y * t);
          float distanceToLine = distanceTo(&echobridgePlayerPos, closestPoint.x, closestPoint.y);

          if (distanceToLine < 8) {
            Vector direction;
            vectorSet(&direction, bridge->endPos.x - bridge->startPos.x, bridge->endPos.y - bridge->startPos.y);
            echobridgeNormalize(&direction);
            Vector oldPos;
            oldPos = echobridgePlayerPos;
            vectorAdd(&echobridgePlayerPos, direction.x * 1.5, direction.y * 1.5);
            echobridgePlayerOnIsland = false;

            float movementDistance = distanceTo(&oldPos, echobridgePlayerPos.x, echobridgePlayerPos.y);
            int multiplier = echobridgeMultiplierNow;
            int movementScore = (int)floor(movementDistance * multiplier * difficulty);
            score += movementScore;

            if (distanceTo(&echobridgePlayerPos, bridge->endPos.x, bridge->endPos.y) < 8) {
              vectorSet(&echobridgePlayerPos, bridge->endPos.x, bridge->endPos.y);
              echobridgeCurrentIslandIdx = bridge->targetIslandIdx;
              echobridgePlayerOnIsland = true;
              echobridgeActiveBridgeIdx = -1;

              float completedBridgeLength = distanceTo(&bridge->startPos, bridge->endPos.x, bridge->endPos.y);
              float materialRecovery = completedBridgeLength * 0.2 * multiplier;
              echobridgeMaterialGauge = fmin(80, echobridgeMaterialGauge + materialRecovery);
              addScore(floor(materialRecovery), echobridgePlayerPos.x, echobridgePlayerPos.y);

              if (bridge->hasStartIsland) {
                echobridgeIslands[bridge->startIslandIdx].isAlive = false;
              }

              play(SELECT);
              bridge->isAlive = false;
            }
          }
        }
      }
    }
  }

  FOR_EACH(echobridgeIslands, ri) {
    ASSIGN_ARRAY_ITEM(echobridgeIslands, ri, EchobridgeIsland, island4);
    SKIP_IS_NOT_ALIVE(island4);
    if (island4->pos.y > 120) {
      island4->isAlive = false;
    }
  }

  FOR_EACH(echobridgeBridges, rbi) {
    ASSIGN_ARRAY_ITEM(echobridgeBridges, rbi, EchobridgeBridge, bridge3);
    SKIP_IS_NOT_ALIVE(bridge3);
    bool startOffScreen = bridge3->startPos.y > 120;
    bool endOffScreen = bridge3->endPos.y > 120;
    if (startOffScreen && endOffScreen) {
      if (rbi == echobridgeActiveBridgeIdx) {
        echobridgeActiveBridgeIdx = -1;
      }
      bridge3->isAlive = false;
    }
  }

  if (echobridgePlayerPos.y >= 100) {
    play(EXPLOSION);
    gameOver();
  }

  FOR_EACH(echobridgeIslands, di) {
    ASSIGN_ARRAY_ITEM(echobridgeIslands, di, EchobridgeIsland, island5);
    SKIP_IS_NOT_ALIVE(island5);

    if (island5->hasBeenUsed) {
      color = LIGHT_GREEN;
    } else {
      color = GREEN;
    }
    box(island5->pos.x, island5->pos.y, island5->size.x, island5->size.y);

    color = YELLOW;
    rect(island5->pos.x - island5->size.x / 2, island5->pos.y + island5->size.y / 2 - 2,
         island5->size.x, 4);

    color = LIGHT_CYAN;
    float waveTime = ticks * 0.05;
    float islandLeft = island5->pos.x - island5->size.x / 2;
    float islandBottom = island5->pos.y + island5->size.y / 2;
    float islandSeed = island5->pos.x * 0.1 + island5->pos.y * 0.05;

    TIMES(6, wi5) {
      float waveSeed = islandSeed + wi5 * 1.7;
      float waveX = islandLeft + (island5->size.x / 5) * (wi5 - 0.5) + sin(waveSeed) * 0.5;
      float waveY = islandBottom + 2 + sin(waveTime + wi5 * 0.8 + waveSeed) * 0.8;
      float waveSize = 0.5 + sin(waveTime + wi5 * 0.3 + waveSeed * 0.5) * 0.2 + sin(waveSeed * 2) * 0.1;
      rect(waveX, waveY, waveSize * 5, waveSize * 3);
    }
  }

  color = LIGHT_BLUE;
  float maxBridgeLength = echobridgeMaterialGauge / 0.25;
  thickness = 1;
  arc(echobridgePlayerPos.x, echobridgePlayerPos.y, maxBridgeLength, 0, M_PI * 2);

  color = BLUE;
  Vector indicatorEnd;
  vectorSet(&indicatorEnd, echobridgePlayerPos.x, echobridgePlayerPos.y);
  addWithAngle(&indicatorEnd, echobridgeRotationAngle, 15);
  thickness = 1;
  line(echobridgePlayerPos.x, echobridgePlayerPos.y, indicatorEnd.x, indicatorEnd.y);

  color = BLUE;
  box(echobridgePlayerPos.x, echobridgePlayerPos.y, echobridgePlayerSize, echobridgePlayerSize);

  COUNT_IS_ALIVE(echobridgeBridges, echobridgeFinalBridgeCount);
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(echobridgeFinalBridgeCount));
  text(multText, 3, 9);

  color = BLUE;
  rect(5, 90, floor((echobridgeMaterialGauge / 80) * 40), 3);
  color = BLACK;
  thickness = 1;
  line(5, 90, 45, 90);
  thickness = 1;
  line(5, 93, 45, 93);
  thickness = 1;
  line(5, 90, 5, 93);
  thickness = 1;
  line(45, 90, 45, 93);
}

void addGameEchoBridge() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
