#include "cglp.h"

static char *title = "PULSE STREAM";
static char *description = "[Tap] Push back \ndrifting objects.\nDon't lose 3 lanes!";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 8, .isDarkColor = false};

#define LANE_COUNT 5
#define TAPS_TO_RECOVER 36
#define SEQUENCE_LEN 15

typedef struct {
  float y;
  float width;
  float height;
} Lane;
static Lane lanes[LANE_COUNT];

typedef struct {
  Vector pos;
  Vector velocity;
  float amplitude;
  int laneIndex;
  float tapExpansion;
  bool isAlive;
} Wave;
// Spawn interval is a constant 36 frames and lifetime is a constant
// ~137/0.8 frames (not difficulty-scaled), so concurrent count stays near a
// constant ~4 - sized with generous headroom.
#define MAX_WAVE_COUNT 32
static Wave waves[MAX_WAVE_COUNT];
static int waveIndex;

typedef struct {
  Vector pos;
  Vector velocity;
  float size;
  int laneIndex;
  bool isAlive;
} Object;
// Spawn probability is 0.01*difficulty per tick (uncapped, linear in
// difficulty) and an object that never gets pushed by a wave just decays
// toward a near-stop without ever reaching the despawn edges, so population
// can hit the old 128-slot cap within a normal ~15-20 minute session;
// raised for headroom.
#define MAX_OBJECT_COUNT 1024
static Object objects[MAX_OBJECT_COUNT];
static int objectIndex;

static float streamPhase;
static int bubbleSpawnIndex;
static bool disabledLanes[LANE_COUNT];
static int laneRecoveryProgress[LANE_COUNT];
static int lastTapTime;
static int autoTapTimer;
static float rapidTapRate;
static int laneSequence[SEQUENCE_LEN];
static int laneIndex;

static void shuffleSequence() {
  int i;
  for (i = SEQUENCE_LEN - 1; i > 0; i--) {
    int j = rndi(0, i + 1);
    int tmp = laneSequence[i];
    laneSequence[i] = laneSequence[j];
    laneSequence[j] = tmp;
  }
}

static void createLaneSequence() {
  int idx = 0;
  int repeat;
  for (repeat = 0; repeat < 3; repeat++) {
    int lane;
    for (lane = 0; lane < LANE_COUNT; lane++) {
      laneSequence[idx] = lane;
      idx++;
    }
  }
  shuffleSequence();
}

static void update() {
  // Never reads a Collision result - pushes/lane logic are decided by lane/position math.
  hasCollision = false;
  if (!ticks) {
    lanes[0].y = 18;
    lanes[1].y = 36;
    lanes[2].y = 54;
    lanes[3].y = 72;
    lanes[4].y = 90;
    int li;
    for (li = 0; li < LANE_COUNT; li++) {
      lanes[li].width = 100;
      lanes[li].height = 9;
    }

    INIT_UNALIVED_ARRAY(waves);
    waveIndex = 0;
    INIT_UNALIVED_ARRAY(objects);
    objectIndex = 0;

    ASSIGN_ARRAY_ITEM(objects, objectIndex, Object, initialObj);
    vectorSet(&initialObj->pos, 98, lanes[2].y + rnd(0, 2) * RNDPM());
    vectorSet(&initialObj->velocity, -0.2, 0);
    initialObj->size = 3;
    initialObj->laneIndex = 2;
    initialObj->isAlive = true;
    objectIndex = wrap(objectIndex + 1, 0, MAX_OBJECT_COUNT);

    streamPhase = 0;
    bubbleSpawnIndex = 0;
    int di;
    for (di = 0; di < LANE_COUNT; di++) {
      disabledLanes[di] = false;
      laneRecoveryProgress[di] = 0;
    }
    lastTapTime = ticks;
    autoTapTimer = 0;
    rapidTapRate = 0;
    createLaneSequence();
    laneIndex = 0;
  }

  streamPhase += 0.05;

  if (ticks % 36 == 0) {
    int selectedLane = laneSequence[laneIndex];
    laneIndex++;

    if (laneIndex >= SEQUENCE_LEN) {
      createLaneSequence();
      laneIndex = 0;
      selectedLane = laneSequence[laneIndex];
      laneIndex++;
    }

    if (disabledLanes[selectedLane]) {
      bool foundActiveLane = false;
      int i;
      for (i = 0; i < LANE_COUNT; i++) {
        selectedLane = (selectedLane + i) % LANE_COUNT;
        if (!disabledLanes[selectedLane]) {
          foundActiveLane = true;
          break;
        }
      }
      if (!foundActiveLane) {
        selectedLane = -1;
      }
    }

    if (selectedLane != -1) {
      ASSIGN_ARRAY_ITEM(waves, waveIndex, Wave, nw);
      vectorSet(&nw->pos, -5, lanes[selectedLane].y);
      vectorSet(&nw->velocity, 0.8, 0);
      nw->amplitude = rnd(1, 1.7);
      nw->laneIndex = selectedLane;
      nw->tapExpansion = 0;
      nw->isAlive = true;
      waveIndex = wrap(waveIndex + 1, 0, MAX_WAVE_COUNT);
    }
  }

  if (rnd(0, 1) < 0.01 * difficulty) {
    int selectedLane;
    if (rnd(0, 1) < 0.7) {
      selectedLane = bubbleSpawnIndex;
      bubbleSpawnIndex = (bubbleSpawnIndex + 1) % LANE_COUNT;
    } else {
      selectedLane = rndi(0, LANE_COUNT);
    }

    if (disabledLanes[selectedLane]) {
      bool foundActiveLane = false;
      int i;
      for (i = 0; i < LANE_COUNT; i++) {
        selectedLane = (selectedLane + 1) % LANE_COUNT;
        if (!disabledLanes[selectedLane]) {
          foundActiveLane = true;
          break;
        }
      }
      if (!foundActiveLane) {
        selectedLane = -1;
      }
    }

    if (selectedLane != -1) {
      COUNT_IS_ALIVE(objects, objectAliveCount);
      if (objectAliveCount < MAX_OBJECT_COUNT) {
        ASSIGN_ARRAY_ITEM(objects, objectIndex, Object, no);
        vectorSet(&no->pos, 98, lanes[selectedLane].y + rnd(0, 2) * RNDPM());
        vectorSet(&no->velocity, rnd(-0.3, -0.2), 0);
        no->size = rnd(2.5, 4);
        no->laneIndex = selectedLane;
        no->isAlive = true;
        objectIndex = wrap(objectIndex + 1, 0, MAX_OBJECT_COUNT);
      }
    }
  }

  bool simulatedTap = false;
  if (input.isJustPressed) {
    lastTapTime = ticks;
    autoTapTimer = 0;
  } else {
    if (ticks - lastTapTime >= 600) {
      autoTapTimer++;
      if (autoTapTimer >= 60) {
        simulatedTap = true;
        autoTapTimer = 0;
      }
    }
  }

  bool tapNow = input.isJustPressed || simulatedTap;
  if (tapNow) {
    rapidTapRate += 0.3;
    FOR_EACH(waves, wi) {
      ASSIGN_ARRAY_ITEM(waves, wi, Wave, w);
      SKIP_IS_NOT_ALIVE(w);
      w->tapExpansion = 1.5;
    }

    int li2;
    for (li2 = 0; li2 < LANE_COUNT; li2++) {
      if (disabledLanes[li2]) {
        laneRecoveryProgress[li2]++;
        if (laneRecoveryProgress[li2] >= TAPS_TO_RECOVER) {
          disabledLanes[li2] = false;
          laneRecoveryProgress[li2] = 0;
          play(POWER_UP);
        }
      }
    }

    play(CLICK);
  }
  rapidTapRate *= 0.98;

  color = CYAN;
  FOR_EACH(waves, wi2) {
    ASSIGN_ARRAY_ITEM(waves, wi2, Wave, wave);
    SKIP_IS_NOT_ALIVE(wave);
    vectorAdd(&wave->pos, VEC_XY(wave->velocity));
    wave->pos.y = lanes[wave->laneIndex].y;

    if (wave->tapExpansion > 0) {
      wave->tapExpansion -= 0.08;
      if (wave->tapExpansion < 0) {
        wave->tapExpansion = 0;
      }
    }

    float waveHeight = sin(streamPhase + wave->pos.x * 0.1) * wave->amplitude;
    float expandedRadius = wave->amplitude * 3 + wave->tapExpansion * 4;
    arc(wave->pos.x, wave->pos.y + waveHeight, expandedRadius, 0, M_PI * 2);

    if (wave->pos.x > 105) {
      wave->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(objects, oi) {
    ASSIGN_ARRAY_ITEM(objects, oi, Object, obj);
    SKIP_IS_NOT_ALIVE(obj);
    vectorAdd(&obj->pos, VEC_XY(obj->velocity));

    if (tapNow) {
      bool accelerated = false;
      FOR_EACH(waves, wi3) {
        ASSIGN_ARRAY_ITEM(waves, wi3, Wave, pulse);
        SKIP_IS_NOT_ALIVE(pulse);
        if (obj->laneIndex == pulse->laneIndex &&
            distanceTo(&obj->pos, VEC_XY(pulse->pos)) < pulse->amplitude * 6) {
          obj->velocity.x += 0.8;
          obj->velocity.y += rnd(0, 0.2) * RNDPM();
          accelerated = true;
          particle(VEC_XY(obj->pos), 4, 1, 0, 2 * M_PI);
          addScore(1, VEC_XY(obj->pos));
          play(HIT);
        }
      }
      if (!accelerated) {
        obj->velocity.x -= 0.1 * (0.25 + rapidTapRate);
      }
    }

    obj->velocity.x *= 0.993;
    obj->velocity.y -= obj->velocity.y * 0.01;
    obj->velocity.y *= 0.9;

    float targetY = lanes[obj->laneIndex].y;
    obj->pos.y += (targetY - obj->pos.y) * 0.05;

    box(VEC_XY(obj->pos), obj->size, obj->size * 2);

    if (obj->pos.x < 0) {
      particle(VEC_XY(obj->pos), 8, 2, 0, M_PI);
      play(EXPLOSION);

      int failedLaneIndex = obj->laneIndex;
      disabledLanes[failedLaneIndex] = true;
      laneRecoveryProgress[failedLaneIndex] = 0;

      FOR_EACH(objects, oi2) {
        ASSIGN_ARRAY_ITEM(objects, oi2, Object, otherObj);
        SKIP_IS_NOT_ALIVE(otherObj);
        if (otherObj->laneIndex == failedLaneIndex) {
          otherObj->isAlive = false;
        }
      }
      FOR_EACH(waves, wi4) {
        ASSIGN_ARRAY_ITEM(waves, wi4, Wave, otherWave);
        SKIP_IS_NOT_ALIVE(otherWave);
        if (otherWave->laneIndex == failedLaneIndex) {
          otherWave->isAlive = false;
        }
      }
      continue;
    }

    if (obj->pos.x > 100) {
      addScore(2, 95, obj->pos.y);
      particle(95, obj->pos.y, 6, 2, M_PI, M_PI / 4);
      play(COIN);
      obj->isAlive = false;
      continue;
    }
  }

  int disabledCount = 0;
  int dci;
  for (dci = 0; dci < LANE_COUNT; dci++) {
    if (disabledLanes[dci]) {
      disabledCount++;
    }
  }
  if (disabledCount >= 3) {
    play(EXPLOSION);
    gameOver();
  }

  int lci;
  for (lci = 0; lci < LANE_COUNT; lci++) {
    Lane *streamCurrent = &lanes[lci];
    if (disabledLanes[lci]) {
      float recoveryProgress = (float)laneRecoveryProgress[lci] / TAPS_TO_RECOVER;
      float recoveryWidth = recoveryProgress * streamCurrent->width;

      if (recoveryWidth > 0) {
        color = BLUE;
        rect(0, streamCurrent->y, recoveryWidth, streamCurrent->height);
      }
      if (recoveryWidth < streamCurrent->width) {
        color = LIGHT_BLUE;
        rect(recoveryWidth, streamCurrent->y, streamCurrent->width - recoveryWidth,
             streamCurrent->height);
      }
    } else {
      color = BLUE;
      rect(0, streamCurrent->y, streamCurrent->width, streamCurrent->height);
    }
  }
}

void addGamePulseStream() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
