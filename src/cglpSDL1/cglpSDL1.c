#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL1.h"

#define SAMPLE_RATE 44100
#define NUM_SAMPLES 512
#define numOs 64

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static int keys[SDLK_LAST];
static int prevKeys[SDLK_LAST];
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static Uint32 frameticks = 0;
static float frameTime = 0.0f;
static Uint32 clearColor = 0;
static float audioVolume = 0.75f;
static float oscVolume = 0.75f;
static SDL_Surface *screen = NULL, *view = NULL;
static int soundOn = 0;

static SDL_AudioSpec audiospec;

typedef struct {
  float current_step;
  float step_size;
  float volume;
  float duration;
  float when;
  int freq;
  float sampleRate;
  int playing;
} oscillator;

static oscillator os[numOs];

typedef struct 
{
    SDL_Surface *sprite;
    int hash;
} CharaterSprite;

static CharaterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;

static void initCharacterSprite() {
    for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
        characterSprites[i].sprite = NULL;
    }
    characterSpritesCount = 0;
}

static void resetCharacterSprite() {
    for (int i = 0; i < characterSpritesCount; i++) {
        SDL_FreeSurface(characterSprites[i].sprite);
        characterSprites[i].sprite = NULL;
    }
    characterSpritesCount = 0;
}

oscillator oscillate(float sampleRate, float freq, float volume, float duration, float when) 
{
    oscillator o = 
    {
        .sampleRate = sampleRate,
        .current_step = 0,
        .volume = volume,
        .step_size = (2 * M_PI) / (((float)sampleRate / (freq))),
        .duration = duration,
        .when = when,
        .freq = freq,
        .playing = 0,
    };
    return o;
}

Sint16 next(oscillator *os) 
{
    os->playing = 1;
    float ret = sinf(os->current_step);
    os->current_step += os->step_size;
    return (Sint16)truncf(clamp(ret * 32767.0f * os->volume, -32767.0f, 32767.0f));
}


void md_playTone(float freq, float duration, float when) 
{
    if(soundOn != 1)
        return;

    if(when + duration < (float)SDL_GetTicks() / 1000.0f)
        return;
    
    for(int i = 0; i < numOs; i++)
    {
        if(os[i].playing == 0)
        {
            os[i] = oscillate(SAMPLE_RATE, freq, oscVolume, duration, when);
            os[i].playing = 1;
            return;
        }
    }
    printf("md_playTone - no free oscililator\n");
}

void md_stopTone() 
{
    if(soundOn != 1)
        return;

    for(int i = 0; i < numOs; i++)
    {
        os[i].current_step = 0;
        os[i].duration = 0;
        os[i].step_size = 0;
        os[i].when = 0;
        os[i].volume = 0;
        os[i].freq = 0;
        os[i].playing = 0;
    }
}

static void audioCallBack(void *ud, Uint8 *stream, int len)
{
    memset(stream, 0, len);
    Sint16* fstream = (Sint16*)stream; 
    for (int i = 0; i < len>>1; i++) 
    {          
        fstream[i] = 0;
        int numPlaying = 0;
        Sint64 tmp = 0;
        for(int j = 0; j < numOs; j++)
        {          
            if (os[j].when <= (float)SDL_GetTicks() / 1000.0f)
            {
                if(os[j].when + os[j].duration >= (float)SDL_GetTicks() / 1000.0f)
                {
                    numPlaying++;
                    tmp = tmp + next(&os[j]);
                }
                else
                {
                    os[j].current_step = 0;
                    os[j].duration = 0;
                    os[j].step_size = 0;
                    os[j].when = 0;
                    os[j].volume = 0;
                    os[j].freq = 0;
                    os[j].playing = 0;
                }
            } 
        }
        if(numPlaying > 0)
            fstream[i] = (Sint16)((clamp(((float)tmp / (float)numPlaying) * audioVolume, -32767.0f, 32767.0f)));
    }
}

int InitAudio()
{
	SDL_AudioSpec as;
	as.format = AUDIO_S16SYS;
    as.channels = 1;
    as.freq = SAMPLE_RATE;
    as.samples = NUM_SAMPLES;
	as.callback = &audioCallBack;
    as.userdata = NULL;
	if(SDL_OpenAudio(&as, &audiospec) < 0)
		return -1;

	if(audiospec.format != AUDIO_S16SYS)
    {
        SDL_CloseAudio();
		return -1;
    }

    for(int i = 0; i < numOs; i++)
        os[i] = oscillate(SAMPLE_RATE, 0, oscVolume, 0, 0);

    SDL_PauseAudio(0);
	return 1;
}


float md_getAudioTime() 
{ 
    return (float)SDL_GetTicks() / 1000.0f;
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                      float x, float y, int hash) 
{
    if(!view)
        return;

    CharaterSprite *cp = NULL;
    for (int i = 0; i < characterSpritesCount; i++)
    {
        if ((characterSprites[i].hash == hash) && (characterSprites[i].sprite)) 
        {
            cp = &characterSprites[i];
            break;
        }
    }
    
    if (cp == NULL)
    {
        cp = &characterSprites[characterSpritesCount];
        cp->hash = hash;
        SDL_Surface *tmp = SDL_CreateRGBSurface(screen->flags, (int)ceilf((float)CHARACTER_WIDTH * scale), (int)ceilf((float)CHARACTER_HEIGHT * scale), 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
        cp->sprite = SDL_DisplayFormatAlpha(tmp);
        SDL_FreeSurface(tmp);
        if(cp->sprite)
        {
            for (int yy = 0; yy < CHARACTER_HEIGHT; yy++)
            {
                for (int xx = 0; xx < CHARACTER_WIDTH; xx++)
                {
                    unsigned char r = grid[yy][xx][0];
                    unsigned char g = grid[yy][xx][1];
                    unsigned char b = grid[yy][xx][2];
                    if ((r == 0) && (g == 0) && (b == 0))
                       continue;
                    SDL_Rect dstChar = {(Sint16)((float)xx*scale), (Sint16)((float)yy*scale), (Uint16)(ceilf(scale)), (Uint16)(ceilf(scale))};
                    Uint32 color = SDL_MapRGB(cp->sprite->format, (Uint8)r, (Uint8)g, (Uint8)b);
                    SDL_FillRect(cp->sprite, &dstChar, color);
                }
            }        
            characterSpritesCount++;
        }
    }

    if(cp && cp->sprite)
    {
        SDL_Rect dst = {(Sint16)((float)x*scale), (Sint16)((float)y*scale), cp->sprite->w, cp->sprite->h};
        SDL_BlitSurface(cp->sprite, NULL, view, &dst);
    }
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b)
{
    if(!view)
        return;

    SDL_Rect dst = { (Sint16)(x * scale) , (Sint16)(y * scale), (Uint16)ceilf(w * scale), (Uint16)ceilf(h  * scale)};
    Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(view, &dst, color);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    if(!view)
        return;

    Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(view, NULL, color);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    if(!screen)
        return;

    clearColor = SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(screen, NULL, clearColor);
}

void md_initView(int w, int h) 
{
    float xScale = (float)WINDOW_WIDTH / w;
    float yScale = (float)WINDOW_HEIGHT / h;
    if (yScale < xScale)
        scale = yScale;
    else
        scale = xScale;
    viewW = w * scale;
    viewH = h * scale;

    if(view)
    {
        SDL_FreeSurface(view);
        view = NULL;
    }

    SDL_Surface *tmp = SDL_CreateRGBSurface(screen->flags, viewW, viewH, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    if(tmp)
    {
        view = SDL_DisplayFormat(tmp);
        SDL_FreeSurface(tmp);
    }

    resetCharacterSprite();
}

void md_consoleLog(char* msg) 
{ 
    printf(msg); 
}

void update() 
{
    for(int i = 0; i < SDLK_LAST; i++)
        prevKeys[i] = keys[i];

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_KEYDOWN)
        {
            keys[event.key.keysym.sym] = 1;
        }

        if(event.type == SDL_KEYUP)
        {
            keys[event.key.keysym.sym] = 0;
        }

        if(event.type == SDL_QUIT)
            quit = 1;
    }

    setButtonState(keys[BUTTON_LEFT] == 1, keys[BUTTON_RIGHT] == 1, keys[BUTTON_UP] == 1,
        keys[BUTTON_DOWN] == 1, keys[BUTTON_B] == 1, keys[BUTTON_A] == 1);
    
    if ((prevKeys[BUTTON_VOLDOWN] == 0) && (keys[BUTTON_VOLDOWN] == 1))
    {
        audioVolume -= 0.05f;
        if(audioVolume < 0.0f)
            audioVolume = 0.0f;    
    }

    if ((prevKeys[BUTTON_VOLUP] == 0) && (keys[BUTTON_VOLUP] == 1))
    {
        audioVolume += 0.05f;
        if(audioVolume > 1.0f)
            audioVolume = 1.0f;     
    }

    if ((prevKeys[BUTTON_MENU] == 0) && (keys[BUTTON_MENU] == 1))
    {
        if (!isInMenu)
            goToMenu();
        else
            quit = 1;
    }

    updateFrame();    
    SDL_Rect src = {0, 0, viewW, viewH};
    SDL_Rect dst = {(WINDOW_WIDTH - viewW) >> 1, (WINDOW_HEIGHT - viewH) >> 1, viewW, viewH};
    SDL_BlitSurface(view, &src, screen, &dst);
    SDL_Flip(screen);

}

void printHelp(char* exe)
{
    char *binaryName = strrchr(exe, '/');
    if (!binaryName)
        binaryName = strrchr(exe, '\\');
    if(binaryName)
        ++binaryName;

    printf("Crisp Game Lib Portable Sdl2 Version\n");
    printf("Usage: %s <command1> <command2> ...\n", binaryName);
    printf("\n");
    printf("Commands:\n");
    printf("  -w <WIDTH>: use <WIDTH> as window width\n");
    printf("  -h <HEIGHT>: use <HEIGHT> as window height\n");
    printf("  -f: Run fullscreen\n");
    printf("  -ns: No Sound\n");
    printf("  -a: Use hardware (accelerated) surfaces\n");
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0)
	{
        printf("SDL Succesfully initialized\n");
		bool fullScreen = false;
        bool useHWSurface = false;
        bool noAudioInit = false;
		for (int i=0; i < argc; i++)
		{
            if((strcasecmp(argv[i], "-?") == 0) || (strcasecmp(argv[i], "--?") == 0) || 
                (strcasecmp(argv[i], "/?") == 0) || (strcasecmp(argv[i], "-help") == 0) || (strcasecmp(argv[i], "--help") == 0))
            {
                printHelp(argv[0]);
                return 0;
            }

			if(strcasecmp(argv[i], "-f") == 0)
				fullScreen = true;

            if(strcasecmp(argv[i], "-ns") == 0)
				noAudioInit = true;
            
            if(strcasecmp(argv[i], "-a") == 0)
				useHWSurface = true;

            if(strcasecmp(argv[i], "-w") == 0)
                if(i+1 < argc)
                    WINDOW_WIDTH = atoi(argv[i+1]);
            
            if(strcasecmp(argv[i], "-h") == 0)
                if(i+1 < argc)
                    WINDOW_HEIGHT = atoi(argv[i+1]);
			
		}

		Uint32 flags = SDL_SWSURFACE;
        if(useHWSurface)
            flags = SDL_HWSURFACE;

		if(fullScreen)
			flags |= SDL_FULLSCREEN;
        
        screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT, 0, flags);
		if(screen)
		{
			SDL_WM_SetCaption( "Crisp Game Lib Portable Sdl1", NULL);
			printf("Succesfully Set %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
            SDL_ShowCursor(SDL_DISABLE);
            if(!noAudioInit)
                soundOn = InitAudio();
            if(soundOn == 1)
                printf("Succesfully opened audio\n");
            else
                printf("Failed to open audio\n");
            initCharacterSprite();
            initGame();
            while(quit == 0)
            {
                frameticks = SDL_GetTicks();
                update();                
                if(quit == 0)
                {
                    frameTime = SDL_GetTicks() - frameticks;
                    float delay = 1000.0f / FPS - frameTime;
                    if (delay > 0.0f)
                        SDL_Delay((Uint32)(delay));
                }
            } 
        
            resetCharacterSprite();
            if(view)
                SDL_FreeSurface(view);
            if(screen)
			    SDL_FreeSurface(screen);
		}
		else
		{
			printf("Failed to Set Videomode %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
		}
		SDL_Quit();
	}
	else
	{
		printf("Couldn't initialise SDL!\n");
	}
	return 0;

}