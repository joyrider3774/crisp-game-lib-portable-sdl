#include "cglp.h"
#include "string.h"

static char *title = "SWITCHBOARD";
static char *description = "[Hold] Connect calls";

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

#define SWITCHBOARD_PLUG_COUNT 9

typedef struct {
  int id;
  Vector pos;
  bool active;
} Plug;
static Plug plugs[SWITCHBOARD_PLUG_COUNT];

typedef struct {
  int currentPlugIndex;
  float angle;
  float speed;
} Cursor;
static Cursor cursor;

typedef struct {
  int from;
  int to;
  float timeLimit;
  float maxTimeLimit;
  bool isAlive;
} Request;
// Every request/connection ties up 2 of the 9 plugs, so at most ~4 of
// either can ever coexist - sized generously above that hard ceiling.
#define SWITCHBOARD_MAX_REQUEST_COUNT 16
static Request requests[SWITCHBOARD_MAX_REQUEST_COUNT];
static int requestIndex;

typedef struct {
  int from;
  int to;
  Vector fromPos;
  Vector toPos;
  float duration;
  float timeLeft;
  bool isAlive;
} Connection;
#define SWITCHBOARD_MAX_CONNECTION_COUNT 16
static Connection connections[SWITCHBOARD_MAX_CONNECTION_COUNT];
static int connectionIndex;

typedef struct {
  bool active;
  Vector startPos;
  Vector currentPos;
  int startPlugId;
  int endPlugId;
} ConnLine;
static ConnLine connLine;

static int contractsLost;
static float nextRequestTimer;
static float multiplier;

static void generateNewConnectionRequest() {
  bool busy[SWITCHBOARD_PLUG_COUNT];
  TIMES(SWITCHBOARD_PLUG_COUNT, bi) { busy[bi] = false; }
  FOR_EACH(connections, ci) {
    ASSIGN_ARRAY_ITEM(connections, ci, Connection, c);
    SKIP_IS_NOT_ALIVE(c);
    busy[c->from - 1] = true;
    busy[c->to - 1] = true;
  }
  FOR_EACH(requests, ri) {
    ASSIGN_ARRAY_ITEM(requests, ri, Request, r);
    SKIP_IS_NOT_ALIVE(r);
    busy[r->from - 1] = true;
    busy[r->to - 1] = true;
  }

  int availablePlugIds[SWITCHBOARD_PLUG_COUNT];
  int availableCount = 0;
  TIMES(SWITCHBOARD_PLUG_COUNT, plugIdx) {
    if (!busy[plugIdx]) {
      availablePlugIds[availableCount] = plugIdx + 1;
      availableCount++;
    }
  }

  int activeAvailablePlugs[SWITCHBOARD_PLUG_COUNT];
  int activeAvailableCount = 0;
  TIMES(availableCount, ai) {
    int id = availablePlugIds[ai];
    if (plugs[id - 1].active) {
      activeAvailablePlugs[activeAvailableCount] = id;
      activeAvailableCount++;
    }
  }

  if (activeAvailableCount >= 1 && availableCount >= 2) {
    int fromIndex = rndi(0, activeAvailableCount);
    int fromId = activeAvailablePlugs[fromIndex];

    int toId;
    do {
      int toIndex = rndi(0, availableCount);
      toId = availablePlugIds[toIndex];
    } while (toId == fromId);

    float timeLimit = floor(rnd(600, 1200) / sqrt(difficulty));

    ASSIGN_ARRAY_ITEM(requests, requestIndex, Request, nr);
    nr->from = fromId;
    nr->to = toId;
    nr->timeLimit = timeLimit;
    nr->maxTimeLimit = timeLimit;
    nr->isAlive = true;
    requestIndex = wrap(requestIndex + 1, 0, SWITCHBOARD_MAX_REQUEST_COUNT);
  }
}

static void update() {
  // Never reads a Collision result - connections are decided by distance/angle math.
  hasCollision = false;
  float centerX = 50;
  float centerY = 50;
  float radius = 30;

  if (!ticks) {
    TIMES(SWITCHBOARD_PLUG_COUNT, i) {
      float angle = (i * M_PI * 2) / 9 - M_PI / 2;
      plugs[i].id = i + 1;
      vectorSet(&plugs[i].pos, centerX + cos(angle) * radius, centerY + sin(angle) * radius);
      plugs[i].active = true;
    }

    cursor.currentPlugIndex = 0;
    cursor.angle = -M_PI / 2;
    cursor.speed = 0.03;

    INIT_UNALIVED_ARRAY(requests);
    requestIndex = 0;
    ASSIGN_ARRAY_ITEM(requests, requestIndex, Request, initR);
    initR->from = 3;
    initR->to = 7;
    initR->timeLimit = 900;
    initR->maxTimeLimit = 900;
    initR->isAlive = true;
    requestIndex = wrap(requestIndex + 1, 0, SWITCHBOARD_MAX_REQUEST_COUNT);

    nextRequestTimer = 180;

    connLine.active = false;
    vectorSet(&connLine.startPos, 0, 0);
    vectorSet(&connLine.currentPos, 0, 0);
    connLine.startPlugId = -1;
    connLine.endPlugId = -1;

    INIT_UNALIVED_ARRAY(connections);
    connectionIndex = 0;
    contractsLost = 0;
    multiplier = 1;
  }

  if (input.isPressed) {
    cursor.angle += cursor.speed * sqrt(difficulty) * 0.8;
  } else {
    cursor.angle += cursor.speed * sqrt(difficulty) * 1.4;
  }
  if (cursor.angle >= M_PI * 2 - M_PI / 2) {
    cursor.angle = -M_PI / 2;
  }

  float cursorX = centerX + cos(cursor.angle) * radius;
  float cursorY = centerY + sin(cursor.angle) * radius;

  float minDistance = 999999;
  int closestPlugIndex = 0;
  TIMES(SWITCHBOARD_PLUG_COUNT, cpi) {
    float d = distanceTo(&plugs[cpi].pos, cursorX, cursorY);
    if (d < minDistance) {
      minDistance = d;
      closestPlugIndex = cpi;
    }
  }
  cursor.currentPlugIndex = closestPlugIndex;

  multiplier -= (multiplier - 1) * 0.001;

  nextRequestTimer -= 1;
  if (nextRequestTimer <= 0) {
    generateNewConnectionRequest();
    nextRequestTimer = floor(rnd(100, 200) / sqrt(difficulty));
  }

  // Unlike upstream's splice-during-iterate quirk, the isAlive ring buffer checks every request once.
  FOR_EACH(requests, ti) {
    ASSIGN_ARRAY_ITEM(requests, ti, Request, r);
    SKIP_IS_NOT_ALIVE(r);
    r->timeLimit -= 1;
    if (r->timeLimit <= 0) {
      plugs[r->from - 1].active = false;
      contractsLost++;
      play(LASER);
      if (contractsLost >= 3) {
        gameOver();
      }
      r->isAlive = false;
      continue;
    }
  }

  if (input.isJustPressed && !connLine.active) {
    float minD = 999999;
    int closestId = -1;
    TIMES(SWITCHBOARD_PLUG_COUNT, fpi) {
      if (plugs[fpi].active) {
        float d = distanceTo(&plugs[fpi].pos, cursorX, cursorY);
        if (d < minD) {
          minD = d;
          closestId = plugs[fpi].id;
        }
      }
    }
    if (closestId != -1) {
      connLine.active = true;
      vectorSet(&connLine.startPos, plugs[closestId - 1].pos.x, plugs[closestId - 1].pos.y);
      connLine.startPlugId = closestId;
      vectorSet(&connLine.currentPos, plugs[closestId - 1].pos.x, plugs[closestId - 1].pos.y);
      play(SELECT);
    }
  }

  if (input.isPressed && connLine.active) {
    vectorSet(&connLine.currentPos, cursorX, cursorY);
  }

  if (input.isJustReleased && connLine.active) {
    float minD2 = 999999;
    int closestId2 = -1;
    TIMES(SWITCHBOARD_PLUG_COUNT, epi) {
      float d = distanceTo(&plugs[epi].pos, cursorX, cursorY);
      if (d < minD2) {
        minD2 = d;
        closestId2 = plugs[epi].id;
      }
    }

    if (closestId2 != -1) {
      connLine.endPlugId = closestId2;

      bool requestMatched = false;
      FOR_EACH(requests, mri) {
        ASSIGN_ARRAY_ITEM(requests, mri, Request, req);
        SKIP_IS_NOT_ALIVE(req);
        bool isCorrectConnection = (connLine.startPlugId == req->from && closestId2 == req->to) ||
                                    (connLine.startPlugId == req->to && closestId2 == req->from);
        if (isCorrectConnection) {
          Plug *fromPlug = &plugs[req->from - 1];
          Plug *toPlug = &plugs[req->to - 1];
          float duration = floor(rnd(100, 200) / sqrt(difficulty));

          ASSIGN_ARRAY_ITEM(connections, connectionIndex, Connection, nc);
          nc->from = req->from;
          nc->to = req->to;
          vectorSet(&nc->fromPos, fromPlug->pos.x, fromPlug->pos.y);
          vectorSet(&nc->toPos, toPlug->pos.x, toPlug->pos.y);
          nc->duration = duration;
          nc->timeLeft = duration;
          nc->isAlive = true;
          connectionIndex = wrap(connectionIndex + 1, 0, SWITCHBOARD_MAX_CONNECTION_COUNT);

          if (!fromPlug->active) {
            fromPlug->active = true;
            contractsLost = (int)fmax(0, contractsLost - 1);
          }
          if (!toPlug->active) {
            toPlug->active = true;
            contractsLost = (int)fmax(0, contractsLost - 1);
          }

          float scoreGained = round(multiplier);
          addScore(scoreGained, cursorX, cursorY);

          multiplier++;

          // Upstream's 7 custom "connectingN.mp3" clips have no equivalent; substituted with COIN.
          play(COIN);
          req->isAlive = false;
          requestMatched = true;
        }
      }

      if (!requestMatched) {
        FOR_EACH(requests, wri) {
          ASSIGN_ARRAY_ITEM(requests, wri, Request, req2);
          SKIP_IS_NOT_ALIVE(req2);
          if (connLine.startPlugId == req2->from || closestId2 == req2->from) {
            Plug *fromPlug2 = &plugs[req2->from - 1];
            fromPlug2->active = false;
            contractsLost++;
            play(LASER);
            if (contractsLost >= 3) {
              gameOver();
            }
            req2->isAlive = false;
          }
        }
      }
    }

    connLine.active = false;
    vectorSet(&connLine.startPos, 0, 0);
    vectorSet(&connLine.currentPos, 0, 0);
    connLine.startPlugId = -1;
    connLine.endPlugId = -1;
  }

  FOR_EACH(connections, uci) {
    ASSIGN_ARRAY_ITEM(connections, uci, Connection, c);
    SKIP_IS_NOT_ALIVE(c);
    c->timeLeft -= 1;
    if (c->timeLeft <= 0) {
      c->isAlive = false;
      play(CLICK);
      continue;
    }
  }

  // Draw plugs
  FOR_EACH(plugs, dpi) {
    ASSIGN_ARRAY_ITEM(plugs, dpi, Plug, plug);
    color = BLACK;
    box(VEC_XY(plug->pos), 7, 7);

    if (plug->active) {
      color = WHITE;
    } else {
      color = LIGHT_BLACK;
    }
    box(VEC_XY(plug->pos), 4, 4);

    float toCenterAngle = atan2(centerY - plug->pos.y, centerX - plug->pos.x);
    Vector numberPos;
    vectorSet(&numberPos, plug->pos.x, plug->pos.y);
    addWithAngle(&numberPos, toCenterAngle, 10);

    color = BLACK;
    // Vircon32 port note: upstream draws this with an isSmallText option
    // this engine has no small-font variant for - drawn at normal size.
    text(intToChar(plug->id), VEC_XY(numberPos));
  }

  // Display all connection requests
  FOR_EACH(requests, dri) {
    ASSIGN_ARRAY_ITEM(requests, dri, Request, r);
    SKIP_IS_NOT_ALIVE(r);
    Plug *fromPlug = &plugs[r->from - 1];

    float fromCenterAngle = atan2(fromPlug->pos.y - centerY, fromPlug->pos.x - centerX);
    Vector requestPos;
    vectorSet(&requestPos, fromPlug->pos.x, fromPlug->pos.y);
    addWithAngle(&requestPos, fromCenterAngle, 10);

    color = RED;
    text(intToChar(r->to), VEC_XY(requestPos));

    float timeProgress = r->timeLimit / (1000 / sqrt(difficulty));
    float barWidth = 10 * timeProgress;

    color = RED;
    rect(requestPos.x - 5, requestPos.y + 4, barWidth, 1);
  }

  // Draw cursor
  color = BLUE;
  box(cursorX, cursorY, 6, 6);

  // Draw active connection lines
  FOR_EACH(connections, dci) {
    ASSIGN_ARRAY_ITEM(connections, dci, Connection, c);
    SKIP_IS_NOT_ALIVE(c);
    color = CYAN;
    float distance = distanceTo(&c->fromPos, VEC_XY(c->toPos));
    float sag = distance * 0.15;

    float midX = (c->fromPos.x + c->toPos.x) / 2;
    float midY = (c->fromPos.y + c->toPos.y) / 2 + sag;

    int segments = 10;
    thickness = 3;
    TIMES(segments, si) {
      float t1 = (float)si / segments;
      float t2 = (float)(si + 1) / segments;

      float p1x = (1 - t1) * (1 - t1) * c->fromPos.x + 2 * (1 - t1) * t1 * midX + t1 * t1 * c->toPos.x;
      float p1y = (1 - t1) * (1 - t1) * c->fromPos.y + 2 * (1 - t1) * t1 * midY + t1 * t1 * c->toPos.y;
      float p2x = (1 - t2) * (1 - t2) * c->fromPos.x + 2 * (1 - t2) * t2 * midX + t2 * t2 * c->toPos.x;
      float p2y = (1 - t2) * (1 - t2) * c->fromPos.y + 2 * (1 - t2) * t2 * midY + t2 * t2 * c->toPos.y;

      line(p1x, p1y, p2x, p2y);
    }

    Plug *fromPlug = &plugs[c->from - 1];
    float fromCenterAngle = atan2(fromPlug->pos.y - centerY, fromPlug->pos.x - centerX);
    Vector barPos;
    vectorSet(&barPos, fromPlug->pos.x, fromPlug->pos.y);
    addWithAngle(&barPos, fromCenterAngle, 10);
    barPos.x -= 5;
    barPos.y += 4;

    float timeProgress = c->timeLeft / (500 / sqrt(difficulty));
    float barWidth = 10 * timeProgress;

    color = BLUE;
    rect(VEC_XY(barPos), barWidth, 1);
  }

  // Draw connection line in progress
  if (connLine.active) {
    color = YELLOW;
    thickness = 2;
    line(VEC_XY(connLine.startPos), VEC_XY(connLine.currentPos));
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)round(multiplier)));
  text(multText, 3, 9);
}

void addGameSwitchboard() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
