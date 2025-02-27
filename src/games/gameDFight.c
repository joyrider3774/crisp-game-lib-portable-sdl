#include "cglp.h"

static char *title = "D FIGHT";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " l    ",
        "llll  ",
        " l    ",
        "l l   ",
        "l  l  ",
        "      ",
    },
    {
        " l    ",
        "llll  ",
        " l    ",
        "  l   ",
        " l    ",
        "      ",
    },
    {
        " ll   ",
        "llllll",
        " ll   ",
        " ll   ",
        "ll ll ",
        "ll  ll",
    },
    {
        " ll   ",
        "llllll",
        " ll   ",
        " ll   ",
        "  ll  ",
        " ll   ",
    }
};

static int charactersCount = 4;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 800, .isDarkColor = false};

typedef struct {
  float x;
  bool isAlive;
} Dome;

typedef struct Agent {
  Vector pos;
  Vector vel;
  struct Agent* target;
  float targetDistance;
  float aimLength;
  float fireLength;
  int moveTicks;
  int mirrorX;
  char type;  // 'p' for player, 'a' for ally, 'e' for enemy
  bool isAlive;
} Agent;

#define DFIGHT_MAX_DOME_COUNT 20
static Dome domes[DFIGHT_MAX_DOME_COUNT];
static int domeIndex;
static float nextDomeDist;

#define DFIGHT_MAX_AGENT_COUNT 50
static Agent agents[DFIGHT_MAX_AGENT_COUNT];
static int agentIndex;
static float nextAllyAgentTicks;
static float nextEnemyAgentTicks;
static int enemyCount;
static int allyCount;
static const float domeRadius = 40;
static const float maxAimDistance = 99;

// Forward declarations
static Agent* getNearest(Agent* a);
static void addAgent(char type);

static void update() {
  if (!ticks) {
    // Initialize domes
    INIT_UNALIVED_ARRAY(domes);
    domeIndex = 0;
    
    // Add first dome
    ASSIGN_ARRAY_ITEM(domes, domeIndex, Dome, d);
    d->x = 100;
    d->isAlive = true;
    domeIndex = wrap(domeIndex + 1, 0, DFIGHT_MAX_DOME_COUNT);
    
    nextDomeDist = 0;
    
    // Initialize agents
    INIT_UNALIVED_ARRAY(agents);
    agentIndex = 0;
    
    // Add player agent
    ASSIGN_ARRAY_ITEM(agents, agentIndex, Agent, a);
    vectorSet(&a->pos, 99, 87);
    vectorSet(&a->vel, 0, 0);
    a->target = NULL;
    a->targetDistance = 0;
    a->aimLength = 0;
    a->fireLength = 0;
    a->moveTicks = 0;
    a->mirrorX = 1;
    a->type = 'p';  // player
    a->isAlive = true;
    agentIndex = wrap(agentIndex + 1, 0, DFIGHT_MAX_AGENT_COUNT);
    
    nextAllyAgentTicks = 9;
    nextEnemyAgentTicks = 0;
    enemyCount = 0;
    allyCount = 0;
  }
  
  const float sd = sqrtf(difficulty);
  const float scr = sd * 0.2;
  
  // Update domes
  nextDomeDist -= scr;
  if (nextDomeDist < 0) {
    ASSIGN_ARRAY_ITEM(domes, domeIndex, Dome, d);
    d->x = 200 + domeRadius;
    d->isAlive = true;
    domeIndex = wrap(domeIndex + 1, 0, DFIGHT_MAX_DOME_COUNT);
    nextDomeDist += rnd(70, 150);
  }
  
  color = YELLOW;
  thickness = 3;  // Set proper thickness for domes
  FOR_EACH(domes, i) {
    ASSIGN_ARRAY_ITEM(domes, i, Dome, d);
    SKIP_IS_NOT_ALIVE(d);
    
    d->x -= scr;
    arc(d->x, 90, domeRadius, M_PI, M_PI * 2);
    
    d->isAlive = d->x >= -domeRadius;
  }
  thickness = 1;  // Reset thickness to default
  
  // Draw ground
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10);
  
  // Update ally agents
  nextAllyAgentTicks--;
  if (nextAllyAgentTicks < 0) {
    addAgent('a');  // ally
    allyCount++;
    nextAllyAgentTicks = (rnd(120, 150) * allyCount) / sd;
  }
  
  // Update enemy agents
  if (enemyCount == 0) {
    nextEnemyAgentTicks = 0;
  }
  nextEnemyAgentTicks--;
  if (nextEnemyAgentTicks < 0) {
    addAgent('e');  // enemy
    enemyCount++;
    nextEnemyAgentTicks += (rnd(60, 80) * enemyCount) / sd;
  }
  
  // Reset counters
  allyCount = 0;
  enemyCount = 0;
  
  // Process agents
  Vector p;
  vectorSet(&p, 0, 0);
  
  FOR_EACH(agents, i) {
    ASSIGN_ARRAY_ITEM(agents, i, Agent, a);
    SKIP_IS_NOT_ALIVE(a);
    
    if (a->type == 'p') {  // player
      a->pos.x += (clamp(input.pos.x, 3, 197) - a->pos.x) * 0.2;
    } else {
      if (a->type == 'a') {  // ally
        allyCount++;
      } else {  // enemy
        enemyCount++;
      }
      
      a->vel.y += difficulty * 0.05;
      
      if (a->pos.x < 3 && a->vel.x < 0) {
        a->pos.x = 3;
        a->vel.x *= -1;
      }
      
      if (a->pos.x > 197 && a->vel.x > 0) {
        a->pos.x = 197;
        a->vel.x *= -1;
      }
      
      if (a->pos.y >= 87) {
        a->pos.y = 87;
        a->vel.y = 0;
        if (rnd(0, 1) < 0.01) {
          a->vel.y = -rnd(3, 6) * sd;
        }
      }
    }
    
    vectorAdd(&a->pos, a->vel.x, a->vel.y);
    vectorMul(&a->vel, 0.9);
    
    if (a->target == NULL) {
      a->target = getNearest(a);
      continue;
    }
    
    float d = distanceTo(&a->pos, VEC_XY(a->target->pos));
    float an = angleTo(&a->pos, VEC_XY(a->target->pos));
    a->mirrorX = fabsf(an) < M_PI / 2 ? 1 : -1;
    
    if (!a->target->isAlive || d > maxAimDistance) {
      a->target = NULL;
      a->aimLength = 0;
      a->fireLength = 0;
      continue;
    }
    
    if (a->type != 'p') {  // not player
      float vx = -1;
      if (a->pos.x > a->target->pos.x) {
        vx *= -1;
      }
      if (d > a->targetDistance) {
        vx *= -1;
      }
      a->vel.x += vx * sd * 0.1;
    }
    
    if (a->fireLength > 0) {
      // Increase fire length
      a->fireLength += sd * 3;
      
      if (a->type == 'e') {  // enemy
        color = RED;
      } else {
        color = BLUE;
      }
      
      vectorSet(&p, VEC_XY(a->pos));
      addWithAngle(&p, an, a->fireLength);
      
      barCenterPosRatio = 1.0;
      thickness = 4;  // Thicker for shooting beam
      Collision c = bar(VEC_XY(p), 9, an);
      thickness = 1;  // Reset to default
      a->aimLength = d;
      
      if (c.isColliding.rect[YELLOW] || p.y > 90 || a->fireLength > maxAimDistance) {
        a->fireLength = 0;
        a->aimLength = 0;
        
        if (c.isColliding.rect[YELLOW]) {
          play(HIT);
          particle(VEC_XY(p), 9, 3, an, 0.3);
        }
      }
    } else {
      a->aimLength += sd * (
        a->type == 'e' ? 0.4 : 
        a->type == 'a' ? 0.2 : 
        0.9
      ) * d * 0.03;
      
      if (a->aimLength > d) {
        play(LASER);
        a->fireLength = 5;
      }
      
      if (a->type == 'e') {  // enemy
        color = RED;
      } else if (a->type == 'a') {  // ally
        color = BLUE;
      } else {  // player
        color = GREEN;
      }
      
      vectorSet(&p, VEC_XY(a->pos));
      addWithAngle(&p, an, a->aimLength);
      text("x", VEC_XY(p));
      
      if (a->type == 'e') {  // enemy
        color = LIGHT_RED;
      } else {
        color = LIGHT_BLUE;
      }
      
      barCenterPosRatio = 0.0;
      thickness = 1;  // Thin for aiming line
      bar(VEC_XY(a->pos), a->aimLength, an);
    }
  }
  
  // Check collisions and remove agents
  FOR_EACH(agents, i) {
    ASSIGN_ARRAY_ITEM(agents, i, Agent, a);
    SKIP_IS_NOT_ALIVE(a);
    
    if (a->type == 'e') {  // enemy
      color = PURPLE;
    } else if (a->type == 'a') {  // ally
      color = BLUE;
    } else {  // player
      color = GREEN;
    }
    
    a->moveTicks++;
    
    characterOptions.isMirrorX = a->mirrorX < 0;
    
    char charToUse[2];
    charToUse[0] = a->type == 'p' ? 'c' : 'a';
    charToUse[0] += a->pos.y < 87 ? 0 : (a->moveTicks / 15) % 2;
    charToUse[1] = '\0';
    
    Collision c = character(charToUse, VEC_XY(a->pos));
    
    if (a->type == 'e' && c.isColliding.rect[BLUE]) {  // enemy hit by ally
      play(POWER_UP);
      particle(VEC_XY(a->pos), 5, 1, 0, M_PI * 2);
      addScore(1, VEC_XY(a->pos));
      a->isAlive = false;
    } else if (a->type == 'a' && c.isColliding.rect[RED]) {  // ally hit by enemy
      play(COIN);
      particle(VEC_XY(a->pos), 5, 1, 0, M_PI * 2);
      a->isAlive = false;
    } else if (a->type == 'p' && c.isColliding.rect[RED]) {  // player hit by enemy
      play(EXPLOSION);
      gameOver();
    }
  }
  
  // Reset the bar center position ratio to default
  barCenterPosRatio = 0.5;
}

static void addAgent(char type) {
  float x = rnd(0, 1) < 0.3 ? -3 : 203;
  float vx = (x < 0 ? 1 : -1) * sqrtf(difficulty);
  int mx = x < 0 ? 1 : -1;
  
  ASSIGN_ARRAY_ITEM(agents, agentIndex, Agent, a);
  vectorSet(&a->pos, x, 87);
  vectorSet(&a->vel, vx, 0);
  a->target = NULL;
  a->targetDistance = rnd(30, 60);
  a->aimLength = 0;
  a->fireLength = 0;
  a->moveTicks = 0;
  a->mirrorX = mx;
  a->type = type;
  a->isAlive = true;
  agentIndex = wrap(agentIndex + 1, 0, DFIGHT_MAX_AGENT_COUNT);
}

static Agent* getNearest(Agent* a) {
  float nd = maxAimDistance;
  Agent* nearest = NULL;
  
  FOR_EACH(agents, i) {
    ASSIGN_ARRAY_ITEM(agents, i, Agent, aa);
    SKIP_IS_NOT_ALIVE(aa);
    
    if (a == aa) {
      continue;
    }
    
    // Skip allies looking at allies or player
    if ((a->type == 'p' || a->type == 'a') && (aa->type == 'p' || aa->type == 'a')) {
      continue;
    }
    
    // Skip enemies looking at enemies
    if (a->type == 'e' && aa->type == 'e') {
      continue;
    }
    
    float d = distanceTo(&a->pos, VEC_XY(aa->pos));
    if (d < nd) {
      nd = d;
      nearest = aa;
    }
  }
  
  return nearest;
}

void addGameDFight() {
  addGame(title, description, characters, charactersCount, options, true, update);
}