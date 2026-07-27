#include "cglp.h"

static char *title = "STAGE SEPARATION";
static char *description = "[Tap] Staging";

// This game never calls char()/text() - no custom character patterns
// needed, following the same "single blank placeholder" convention as
// other character-less ports.
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
    .viewSizeX = 100, .viewSizeY = 150, .soundSeed = 5, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
  float currentStage;
  float stageHeight;
  float stageBurnTime;
} Rocket;
static Rocket rocket;

typedef struct {
  Vector pos;
  Vector vel;
  float height;
  bool isAlive;
} Stage;
#define STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT 32
static Stage separatedStages[STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT];
static int separatedStageIndex;

typedef struct {
  Vector pos;
  float size;
  Vector vel;
  bool isAlive;
} Debris;
#define STAGESEPARATION_MAX_DEBRIS_COUNT 32
static Debris debris[STAGESEPARATION_MAX_DEBRIS_COUNT];
static int debrisIndex;

#define STAGESEPARATION_STAR_COUNT 200
static Vector stars[STAGESEPARATION_STAR_COUNT];

static float nextDebrisSpawn;
static float cameraY;
static float zoom;

// JS also kept a "background" object ({skyColor, starDensity}) initialized
// alongside the rest of the game state, but it's never read anywhere else
// in update() - dead state upstream too, so it's simply not ported here.

static void worldToScreen(Vector *pos, Vector *result) {
  result->x = (pos->x - 50) / zoom + 50;
  result->y = (pos->y - cameraY) / zoom;
}

static void update() {
  if (!ticks) {
    vectorSet(&rocket.pos, 50, 90);
    vectorSet(&rocket.vel, 0, -0.5);
    rocket.currentStage = 4;
    rocket.stageHeight = 10;
    rocket.stageBurnTime = 100;
    INIT_UNALIVED_ARRAY(debris);
    debrisIndex = 0;
    INIT_UNALIVED_ARRAY(separatedStages);
    separatedStageIndex = 0;
    TIMES(STAGESEPARATION_STAR_COUNT, i) {
      stars[i].x = rnd(-200, 300);
      stars[i].y = rnd(-1000, 0);
    }
    nextDebrisSpawn = 99;
    cameraY = 0;
    zoom = 1;
  }
  float altitude = -rocket.pos.y;
  addScore(-rocket.vel.y * 0.1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  float targetCameraY = rocket.pos.y - 130 * zoom;
  cameraY += clamp((targetCameraY - cameraY) * 0.1, -99, 0);
  zoom = clamp(1 + altitude / 9999, 1, 3);
  color = BLACK;
  TIMES(STAGESEPARATION_STAR_COUNT, si) {
    Vector *star = &stars[si];
    float screenX = (star->x - 50) / zoom + 50;
    float screenY = fmod(star->y - cameraY, 1000) / zoom;
    if (screenX >= 0 && screenX <= 100 && screenY >= 0 && screenY <= 150) {
      box(screenX, screenY, 1, 1);
    }
  }

  rocket.vel.y -= 0.1 * (rocket.stageBurnTime / 60);
  rocket.vel.y += 0.05;
  rocket.pos.y += rocket.vel.y * difficulty;
  if (rocket.stageBurnTime > 0) {
    rocket.stageBurnTime -= difficulty;
  }
  if (input.isJustPressed && rocket.currentStage > 2) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(separatedStages, separatedStageIndex, Stage, ns);
    vectorSet(&ns->pos, rocket.pos.x,
              rocket.pos.y + rocket.stageHeight * (rocket.currentStage - 1));
    vectorSet(&ns->vel, rnd(0, 0.5) * RNDPM(), rocket.vel.y + 0.5);
    ns->height = rocket.stageHeight;
    ns->isAlive = true;
    separatedStageIndex =
        wrap(separatedStageIndex + 1, 0, STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT);
    rocket.currentStage--;
    rocket.vel.y -= 0.5;
    rocket.stageBurnTime = 60;
  }
  rocket.currentStage = clamp(rocket.currentStage + 0.02 * difficulty, 0, 4);
  if (rocket.currentStage > 2) {
    color = RED;
  } else {
    color = LIGHT_RED;
  }
  Vector rocketScreenPos;
  worldToScreen(&rocket.pos, &rocketScreenPos);
  rect(rocketScreenPos.x - 5 / zoom, rocketScreenPos.y, 10 / zoom,
       rocket.stageHeight * rocket.currentStage / zoom);
  if (rocket.stageBurnTime > 0) {
    particle(rocketScreenPos.x,
             rocketScreenPos.y + rocket.stageHeight * rocket.currentStage / zoom, 1,
             rocket.stageBurnTime / 20 / zoom, M_PI_2, 0.3);
  }
  if (rocketScreenPos.y > 150) {
    play(EXPLOSION);
    gameOver();
  }

  color = PURPLE;
  FOR_EACH(separatedStages, si2) {
    ASSIGN_ARRAY_ITEM(separatedStages, si2, Stage, s);
    SKIP_IS_NOT_ALIVE(s);
    s->vel.y += 0.1;
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    Vector stageScreenPos;
    worldToScreen(&s->pos, &stageScreenPos);
    rect(stageScreenPos.x - 5 / zoom, stageScreenPos.y, 10 / zoom, s->height / zoom);
    if (s->pos.y > cameraY + 200 * zoom) {
      s->isAlive = false;
      continue;
    }
  }

  nextDebrisSpawn += rocket.vel.y * difficulty;
  if (nextDebrisSpawn < 0) {
    int side = rndi(0, 2);
    float spawnX;
    if (side) {
      spawnX = 50 + 70 * zoom;
    } else {
      spawnX = 50 - 70 * zoom;
    }
    ASSIGN_ARRAY_ITEM(debris, debrisIndex, Debris, nd);
    vectorSet(&nd->pos, spawnX, cameraY - rnd(0, 200 * zoom));
    nd->size = rnd(5, 15);
    float velX;
    if (side) {
      velX = -1 * rnd(0.5, 0.8);
    } else {
      velX = 1 * rnd(0.5, 0.8);
    }
    vectorSet(&nd->vel, velX, rnd(-15, 0));
    vectorMul(&nd->vel, zoom / 2);
    nd->isAlive = true;
    debrisIndex = wrap(debrisIndex + 1, 0, STAGESEPARATION_MAX_DEBRIS_COUNT);
    nextDebrisSpawn = rnd(120, 160) / sqrt(zoom);
  }
  color = LIGHT_BLACK;
  FOR_EACH(debris, di) {
    ASSIGN_ARRAY_ITEM(debris, di, Debris, d);
    SKIP_IS_NOT_ALIVE(d);
    vectorAdd(&d->pos, d->vel.x * difficulty, d->vel.y * difficulty);
    Vector debrisScreenPos;
    worldToScreen(&d->pos, &debrisScreenPos);
    Collision dc = arc(VEC_XY(debrisScreenPos), d->size / zoom, 0, M_PI * 2);
    if (dc.isColliding.rect[RED] || dc.isColliding.rect[LIGHT_RED]) {
      play(EXPLOSION);
      gameOver();
      // JS explicitly "return false" here (collide but don't remove the
      // debris) - end() has already stopped the game at this point either
      // way, so just skip the removal check below rather than kill it.
      continue;
    }
    if (d->pos.x < 50 - 70 * zoom || d->pos.x > 50 + 70 * zoom) {
      d->isAlive = false;
      continue;
    }
  }
}

void addGameStageSeparation() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
