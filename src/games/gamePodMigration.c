#include "cglp.h"
#include "string.h"

static char *title = "POD MIGRATION";
static char *description = "[Tap] Change direction";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  lll ",
        "l lccl",
        "llclcl",
        "lcllll",
        " lwwwl",
        "  lll ",
    },
    {
        "  ll  ",
        " lppl ",
        "llpll ",
        " lll  ",
            "      ",
        "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = false};

static Vector leadWhalePos;
static Vector leadWhaleKnockbackVel;
static int leadWhaleKnockbackFrames;

typedef struct {
  Vector pos;
  bool isDestroyed;
} Whale;
#define POD_WHALE_COUNT 3
static Whale podWhales[POD_WHALE_COUNT];

typedef struct {
  Vector pos;
  Vector size;
  float jaggedness;
  int spawnTime;
  float emergeDuration;
  float lifespan;
  float sinkDuration;
  bool isAlive;
} Ice;
// targetIceCount = 3 + floor(difficulty/2) grows unbounded with difficulty (parity with the old
// cap of 48 hit at difficulty~90, i.e. ~90 min in) - raised for long sessions.
#define MAX_ICE_COUNT 256
static Ice iceFloes[MAX_ICE_COUNT];
static int iceIndex;

static int direction;
static float speed;
static int respawnTimer;
static float multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&leadWhalePos, 20, 50);
    vectorSet(&leadWhaleKnockbackVel, 0, 0);
    leadWhaleKnockbackFrames = 0;
    vectorSet(&podWhales[0].pos, 10, 45);
    podWhales[0].isDestroyed = false;
    vectorSet(&podWhales[1].pos, 10, 55);
    podWhales[1].isDestroyed = false;
    vectorSet(&podWhales[2].pos, 5, 50);
    podWhales[2].isDestroyed = false;
    INIT_UNALIVED_ARRAY(iceFloes);
    iceIndex = 0;
    direction = 0;
    speed = 1;
    respawnTimer = 0;
    multiplier = 1;
  }

  float currentSpeed = speed + (difficulty - 1) * 0.3;
  float baseIceSpawnRate = 0.01 + (difficulty - 1) * 0.005;

  int targetIceCount = 3 + (int)floor(difficulty / 2);
  COUNT_IS_ALIVE(iceFloes, curIceCount);
  float iceCountMultiplier = 1;
  if (curIceCount < targetIceCount) {
    iceCountMultiplier = 3;
  } else if (curIceCount < targetIceCount + 2) {
    iceCountMultiplier = 2;
  }
  float iceSpawnRate = baseIceSpawnRate * iceCountMultiplier;

  multiplier *= 0.998;
  multiplier = fmax(multiplier, 1);

  color = LIGHT_BLUE;
  for (int wy = 0; wy < 100; wy += 12) {
    for (int wx = 0; wx < 100; wx += 15) {
      float waveHeight = sin(ticks * 0.01 + wx * 0.1 + wy * 0.05) * 0.8;
      rect(wx, wy + waveHeight, 9, 2);
    }
  }

  respawnTimer++;
  int respawnInterval = 300;
  if (respawnTimer >= respawnInterval) {
    int destroyedIdx = -1;
    TIMES(POD_WHALE_COUNT, fi) {
      if (destroyedIdx < 0 && podWhales[fi].isDestroyed) {
        destroyedIdx = fi;
      }
    }
    if (destroyedIdx >= 0) {
      podWhales[destroyedIdx].isDestroyed = false;
      particle(VEC_XY(podWhales[destroyedIdx].pos), 6, 1, 0, M_PI * 2);
      play(COIN);
      respawnTimer = 0;
    }
  }

  if (input.isJustPressed) {
    direction = (direction + 1) % 4;
  }

  Vector vel;
  vectorSet(&vel, 0, 0);
  if (direction == 0) {
    vel.x = currentSpeed;
  } else if (direction == 1) {
    vel.y = currentSpeed;
  } else if (direction == 2) {
    vel.x = -currentSpeed;
  } else {
    vel.y = -currentSpeed;
  }

  bool canMove = true;

  TIMES(POD_WHALE_COUNT, wi) {
    Vector target;
    if (wi == 0) {
      target = leadWhalePos;
    } else {
      target = podWhales[wi - 1].pos;
    }
    Vector offset;
    vectorSet(&offset, target.x - podWhales[wi].pos.x, target.y - podWhales[wi].pos.y);
    vectorMul(&offset, 0.05);
    vectorAdd(&podWhales[wi].pos, VEC_XY(offset));
  }

  color = BLACK;
  characterOptions.isMirrorX = direction >= 2;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", VEC_XY(leadWhalePos));
  color = LIGHT_BLUE;
  particle(VEC_XY(leadWhalePos), 1, 1, vectorAngle(&vel) + M_PI, 0.2);

  color = BLACK;
  TIMES(POD_WHALE_COUNT, wi2) {
    Vector target2;
    if (wi2 == 0) {
      target2 = leadWhalePos;
    } else {
      target2 = podWhales[wi2 - 1].pos;
    }
    Whale *whale = &podWhales[wi2];
    characterOptions.isMirrorX = whale->pos.x > target2.x;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    if (!whale->isDestroyed) {
      if (character("b", VEC_XY(whale->pos)).isColliding.character['a']) {
        whale->isDestroyed = true;
        particle(VEC_XY(whale->pos), 4, 2, 0, M_PI * 2);
        play(EXPLOSION);
        respawnTimer = 0;
      }
    } else {
      color = TRANSPARENT;
      character("b", VEC_XY(whale->pos));
      color = BLACK;
    }
  }

  if (rnd(0, 1) < iceSpawnRate) {
    Vector newIcePos;
    vectorSet(&newIcePos, rnd(10, 90), rnd(10, 90));
    float safeDistance = 20;
    bool isSafe = true;
    TIMES(POD_WHALE_COUNT, wi3) {
      if (distanceTo(&newIcePos, VEC_XY(podWhales[wi3].pos)) < safeDistance) {
        isSafe = false;
      }
    }
    if (isSafe) {
      play(CLICK);
      ASSIGN_ARRAY_ITEM(iceFloes, iceIndex, Ice, ice);
      ice->pos = newIcePos;
      vectorSet(&ice->size, rnd(9, 16), rnd(6, 10));
      ice->jaggedness = rnd(0.3, 0.8);
      ice->spawnTime = ticks;
      ice->emergeDuration = 150;
      ice->lifespan = rnd(500, 600);
      ice->sinkDuration = 120;
      ice->isAlive = true;
      iceIndex = wrap(iceIndex + 1, 0, MAX_ICE_COUNT);
    }
  }

  FOR_EACH(iceFloes, icei) {
    ASSIGN_ARRAY_ITEM(iceFloes, icei, Ice, ice);
    SKIP_IS_NOT_ALIVE(ice);
    bool iceDestroyed = false;

    float ageFrames = ticks - ice->spawnTime;
    float emergeProgress = fmin(ageFrames / ice->emergeDuration, 1);

    float sinkStartTime = ice->lifespan - ice->sinkDuration;
    float heightMultiplier = 1;
    bool isFullyEmerged = emergeProgress >= 1;
    bool isSinking = ageFrames >= sinkStartTime;

    if (ageFrames >= ice->lifespan) {
      ice->isAlive = false;
      continue;
    } else if (isSinking) {
      float sinkProgress = (ageFrames - sinkStartTime) / ice->sinkDuration;
      float sinkEase = pow(sinkProgress, 2);
      heightMultiplier = 1 - sinkEase;
    }

    float emergeEase = 1 - pow(1 - emergeProgress, 3);

    float finalHeightMultiplier = emergeEase * heightMultiplier;
    float visibleHeight = ice->size.y * finalHeightMultiplier;
    float emergeOffset = ice->size.y - visibleHeight;

    int jaggedParts = (int)floor(ice->size.x / 4);

    color = LIGHT_BLUE;
    TIMES(jaggedParts, jp1) {
      float partWidth = ice->size.x / jaggedParts;
      float waveOffset = sin(ticks * 0.02 + jp1 * 0.5) * 1.5;
      float partHeight = ice->size.y * 0.7 * finalHeightMultiplier;
      if (partHeight > 0) {
        rect(ice->pos.x - ice->size.x / 2 + jp1 * partWidth - 1,
             ice->pos.y + emergeOffset - partHeight / 2 + 2 + waveOffset,
             partWidth + 2, partHeight);
      }
    }

    color = CYAN;
    bool iceCollidesA = false;
    bool iceCollidesB = false;
    TIMES(jaggedParts, jp2) {
      float partWidth = ice->size.x / jaggedParts;
      float heightVariation = ice->jaggedness * (sin(ice->pos.x * 0.1 + jp2) * 2);
      float fullPartHeight = ice->size.y + heightVariation;
      float partHeight = fullPartHeight * finalHeightMultiplier;
      if (partHeight > 0) {
        Collision partCollision =
            rect(ice->pos.x - ice->size.x / 2 + jp2 * partWidth - 1,
                 ice->pos.y + emergeOffset - partHeight / 2, partWidth + 2, partHeight);
        if (partCollision.isColliding.character['a']) {
          iceCollidesA = true;
        }
        if (partCollision.isColliding.character['b']) {
          iceCollidesB = true;
        }
      }
    }

    color = LIGHT_CYAN;
    TIMES(jaggedParts, jp3) {
      float partWidth3 = (ice->size.x / jaggedParts) * 0.8;
      float heightVariation3 = ice->jaggedness * (sin(ice->pos.x * 0.1 + jp3) * 1);
      float fullPartHeight3 = ice->size.y * 0.6 + heightVariation3;
      float partHeight3 = fullPartHeight3 * finalHeightMultiplier;
      if (partHeight3 > 0) {
        rect(ice->pos.x - ice->size.x / 2 + jp3 * (ice->size.x / jaggedParts) - 1,
             ice->pos.y + emergeOffset - partHeight3 / 2 - 1, partWidth3, partHeight3);
      }
    }

    if (iceCollidesB) {
      particle(VEC_XY(ice->pos), 5, 2, 0, M_PI);
      float points = round(multiplier);
      addScore(points, VEC_XY(ice->pos));
      multiplier += 1;
      play(POWER_UP);
      iceDestroyed = true;
    }

    if (!iceDestroyed && isFullyEmerged && !isSinking) {
      if (iceCollidesA) {
        canMove = false;
      }
    }

    if (iceDestroyed) {
      ice->isAlive = false;
      continue;
    }
  }

  if (leadWhaleKnockbackFrames > 0) {
    vectorAdd(&leadWhalePos, VEC_XY(leadWhaleKnockbackVel));
    vectorMul(&leadWhaleKnockbackVel, 0.88);
    leadWhaleKnockbackFrames--;
  }

  if (canMove) {
    vectorAdd(&leadWhalePos, VEC_XY(vel));
  } else {
    vectorSet(&leadWhaleKnockbackVel, vel.x * -1.8, vel.y * -1.8);
    leadWhaleKnockbackFrames = 25;
    play(HIT);
  }

  bool allPodWhalesDestroyed = true;
  TIMES(POD_WHALE_COUNT, wi4) {
    if (!podWhales[wi4].isDestroyed) {
      allPodWhalesDestroyed = false;
    }
  }
  if (allPodWhalesDestroyed) {
    play(EXPLOSION);
    gameOver();
  }

  leadWhalePos.x = clamp(leadWhalePos.x, 3, 97);
  leadWhalePos.y = clamp(leadWhalePos.y, 3, 97);

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)round(multiplier)));
  text(multText, 3, 9);
}

void addGamePodMigration() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
