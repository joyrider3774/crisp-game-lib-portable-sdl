#include "cglp.h"
#include "machineDependent.h"
#include "pd_api.h"
#include "math.h"

#define SYNTH_COUNT 4
#define CRANK_ON_DEGREE 5
#define CRANK_OFF_DEGREE 1
#define CRANK_OFF_DURATION 1

PlaydateAPI* pd;
static PDSynth* synths[SYNTH_COUNT];
static int synthIndex;
static bool isDarkColor;
static int baseColor;
static int viewWidth, viewHeight;
static int crankWay;
static int crankOffTicks;
static float mouseX, mouseY;

typedef struct {
  LCDBitmap* sprite;
  int hash;
} CharaterSprite;
static CharaterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;

static void initCharacterSprite() {
  for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
    characterSprites[i].sprite =
        pd->graphics->newBitmap(CHARACTER_WIDTH, CHARACTER_HEIGHT, kColorClear);
  }
  characterSpritesCount = 0;
}

static void resetCharacterSprite() { characterSpritesCount = 0; }

static void createCharacterImageData(
    unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3]) {
  pd->graphics->fillRect(0, 0, CHARACTER_WIDTH, CHARACTER_HEIGHT, kColorClear);
  int cp = 0;
  for (int y = 0; y < CHARACTER_HEIGHT; y++) {
    for (int x = 0; x < CHARACTER_WIDTH; x++) {
      unsigned char r = grid[y][x][0];
      unsigned char g = grid[y][x][1];
      unsigned char b = grid[y][x][2];
      if (r > 0 || g > 0 || b > 0) {
        pd->graphics->fillRect(x, y, 1, 1, baseColor);
      }
      cp++;
    }
  }
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                      float x, float y, int hash) {
  if (color == TRANSPARENT) {
    return;
  }
  CharaterSprite* cp = NULL;
  for (int i = 0; i < characterSpritesCount; i++) {
    if (characterSprites[i].hash == hash) {
      cp = &characterSprites[i];
      break;
    }
  }
  if (cp == NULL) {
    cp = &characterSprites[characterSpritesCount];
    cp->hash = hash;
    pd->graphics->pushContext(cp->sprite);
    createCharacterImageData(grid);
    pd->graphics->popContext();
    characterSpritesCount++;
  }
  pd->graphics->drawBitmap(cp->sprite, (int)x, (int)y, kBitmapUnflipped);
}

static void loadHighScores()
{
	SDFile* saveStateFile = pd->file->open("savestate.srm", kFileReadData);
    //does not exist
    if (saveStateFile == NULL)
        return;

    int ret = 1;
    int i = 0;
	while ((ret > 0) && (i < gameCount))
	{
		ret = pd->file->read(saveStateFile, hiScores[i].title, sizeof(char) * 100);
		if(ret <= 0)
			break;
		ret = pd->file->read(saveStateFile, &hiScores[i].hiScore, sizeof(int));
        i++;
    }
	
	pd->file->close(saveStateFile);
}

static void saveHighScores()
{
    SDFile* saveStateFile = pd->file->open("savestate.srm", kFileWrite);
    //does not exist
    if (saveStateFile == NULL)
        return;

    for (int i = 0; i < gameCount; i++)
	{
		pd->file->write(saveStateFile, hiScores[i].title, sizeof(char)* 100);
		pd->file->write(saveStateFile, &hiScores[i].hiScore, sizeof(int));
    }
	
	pd->file->close(saveStateFile);
}


static LCDPattern lcdPattern = {0,    0,    0,    0,    0,    0,    0,    0,
                                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned char rotateLeft(unsigned char num, unsigned char rotation) {
  unsigned char dropped;
  rotation %= 8;
  while (rotation--) {
    dropped = (num >> 7) & 1;
    num = (num << 1) | dropped;
  }
  return num;
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b) {
  int xi = (int)x;
  int yi = (int)y;
  int wi = (int)w;
  int hi = (int)h;
  if (abs(wi) <= 4 && abs(hi) <= 4) {
    pd->graphics->fillRect(xi, yi, wi, hi, baseColor);
    return;
  }
  unsigned char p;
  for (int i = 0; i < 8; i++) {
    switch (i % 3) {
      case 0:
        p = (r & 0xf0) | (g >> 4);
        break;
      case 1:
        p = (b & 0xf0) | (r >> 4);
        break;
      case 2:
        p = (g & 0xf0) | (b >> 4);
        break;
    }
    lcdPattern[i] = rotateLeft(p ^ 0xff, i * 3);
  }
  pd->graphics->fillRect(xi, yi, wi, hi, (LCDColor)&lcdPattern);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) {
  pd->graphics->clear(baseColor);
  pd->graphics->fillRect(0, 0, viewWidth, viewHeight,
                         isDarkColor ? kColorBlack : kColorWhite);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b) {
  isDarkColor = r < 128 && g < 182 && b < 128;
  baseColor = isDarkColor ? kColorWhite : kColorBlack;
  pd->graphics->setBackgroundColor(baseColor);
  int xScale = 400 / viewWidth;
  int yScale = 240 / viewHeight;
  int scale = xScale < yScale ? xScale : yScale;
  pd->display->setScale(scale);
  pd->display->setOffset(0, 0);
  pd->graphics->clear(baseColor);
  pd->display->setOffset((pd->display->getWidth() - viewWidth) / 2 * scale,
                         (pd->display->getHeight() - viewHeight) / 2 * scale);
}

void md_playTone(float freq, float duration, float when) {
  pd->sound->synth->playNote(synths[synthIndex], freq, 0.36f, duration,
                             (uint32_t)(when * 44100));
  synthIndex++;
  if (synthIndex >= SYNTH_COUNT) {
    synthIndex = 0;
  }
}

void md_stopTone() {
  for (int i = 0; i < SYNTH_COUNT; i++) {
    pd->sound->synth->noteOff(synths[i], 0);
  }
  synthIndex = 0;
}

float md_getAudioTime() { return (float)pd->sound->getCurrentTime() / 44100; }

void md_initView(int w, int h) {
  viewWidth = w;
  viewHeight = h;
  mouseX = viewWidth >> 1;
  mouseY = viewHeight >> 1;
  resetCharacterSprite();
}

void md_consoleLog(char* msg) { pd->system->logToConsole(msg); }

static PDButtons buttonsCurrent, buttonsPushed, buttonsReleased;

static int update(void* userdata) {
  float cc = pd->system->getCrankChange();
  if (!pd->system->isCrankDocked()) {
    if (cc > CRANK_ON_DEGREE) {
      crankWay = 1;
      crankOffTicks = 0;
    } else if (cc < -CRANK_ON_DEGREE) {
      crankWay = -1;
      crankOffTicks = 0;
    } else if (cc < CRANK_OFF_DEGREE && cc > -CRANK_OFF_DEGREE) {
      crankOffTicks++;
      if (crankOffTicks > CRANK_OFF_DURATION) {
        crankWay = 0;
      }
    }
  } else {
    crankWay = 0;
    crankOffTicks = 0;
  }

  bool mouseUsed = getGame(currentGameIndex).usesMouse;
  pd->system->getButtonState(&buttonsCurrent, &buttonsPushed, &buttonsReleased);
  setButtonState(!mouseUsed && ((buttonsCurrent & kButtonLeft) || crankWay == 1),
                 !mouseUsed && ((buttonsCurrent & kButtonRight) || crankWay == -1),
                 !mouseUsed && ((buttonsCurrent & kButtonUp) || crankWay == 1),
                 !mouseUsed && ((buttonsCurrent & kButtonDown) || crankWay == -1),
                 buttonsCurrent & kButtonB, buttonsCurrent & kButtonA);

  if (mouseUsed)
  {
      if (buttonsCurrent & kButtonRight)
          mouseX += viewWidth / 100;

      if (buttonsCurrent & kButtonLeft)
          mouseX -= viewWidth / 100;

      if (buttonsCurrent & kButtonUp)
          mouseY -= viewHeight / 100;

      if (buttonsCurrent & kButtonDown)
          mouseY += viewHeight / 100;

      mouseX = clamp(mouseX, 0, viewWidth-1);
      mouseY = clamp(mouseY, 0, viewHeight-1);

    
      setMousePos(mouseX, mouseY);
  }
  updateFrame();
  // Draw a Little Cursor
  if (mouseUsed && !isInGameOver)
  {
      pd->graphics->fillEllipse(mouseX-2, mouseY-2, 4, 4, 0, 360, kColorWhite);
      pd->graphics->fillEllipse(mouseX-1, mouseY-1, 2, 2, 0, 360, kColorBlack);
      pd->graphics->fillEllipse(mouseX, mouseY, 1, 1, 0, 360, kColorXOR);
  }
  //pd->system->drawFPS(0, 0);
  if (!isInMenu) {
    if ((buttonsCurrent & kButtonA) && (buttonsCurrent & kButtonB) &&
        (buttonsCurrent & kButtonUp) && (buttonsCurrent & kButtonRight)) {
      saveHighScores();
	  goToMenu();
    }
  }
  return 1;
}

static void init() {
  for (int i = 0; i < SYNTH_COUNT; i++) {
    synths[i] = pd->sound->synth->newSynth();
  }
  synthIndex = 0;
  pd->display->setRefreshRate(FPS);
  crankWay = 0;
  crankOffTicks = 0;
  pd->system->getCrankChange();
  initCharacterSprite();
  initGame();
  loadHighScores();
  pd->system->setUpdateCallback(update, NULL);
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI* _pd, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    pd = _pd;
    init();
  }
  
  if (event == kEventTerminate) {
    saveHighScores();
  }
  return 0;
}
