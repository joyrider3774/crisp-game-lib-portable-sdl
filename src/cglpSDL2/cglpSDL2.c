#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL2.h"
#include "float.h"

#define SAMPLE_RATE 44100
#define NUM_SAMPLES 512
#define numOs 64

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static int keys[BUTTON_COUNT];
static int prevKeys[BUTTON_COUNT];
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static int origViewW = DEFAULT_WINDOW_WIDTH;
static int origViewH = DEFAULT_WINDOW_HEIGHT;
static Uint64 frameticks = 0;
static double frameTime = 0.0f;
static Uint64 soundticks = 0;
static double soundTime = 0.0f;
static unsigned char clearColorR = 0;
static unsigned char clearColorG = 0;
static unsigned char clearColorB = 0;
static float audioVolume = 0.75f;
static float oscVolume = 0.75f;
static int offsetX = 0 ;
static int offsetY = 0;
static int soundOn = 0;
static SDL_AudioDeviceID audioDevice = 0;

SDL_Renderer *Renderer = NULL;
SDL_Window *SdlWindow = NULL;

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
    SDL_Texture *sprite;
    int hash;
    int w;
    int h;
} CharaterSprite;

static CharaterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;

static void initCharacterSprite() {
    for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
        characterSprites[i].sprite = NULL;
        characterSprites[i].w = 0;
        characterSprites[i].h = 0;
    }
    characterSpritesCount = 0;
}

static void resetCharacterSprite() {
    for (int i = 0; i < characterSpritesCount; i++) {
        SDL_DestroyTexture(characterSprites[i].sprite);
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
        .step_size = (2.0f * M_PI) / (((float)sampleRate / (freq))),
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

    if(when + duration < soundTime)
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
    SDL_Log("md_playTone - no free oscililator\n");
}

void md_stopTone() 
{
    if (soundOn != 1)
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
    SDL_LockAudioDevice(audioDevice);
    memset(stream, 0, len);
    Sint16* fstream = (Sint16*)stream; 
    for (int i = 0; i < len>>1; i++) 
    {          
        fstream[i] = 0;
        int numPlaying = 0;
        Sint64 tmp = 0;
        for(int j = 0; j < numOs; j++)
        {          
            //bad ? placed it inside the loop so soundTime keeps updating as well during the loop 
            Uint64 soundEndTicks = SDL_GetPerformanceCounter();
            Uint64 soundPerf = soundEndTicks - soundticks;
            soundticks = SDL_GetPerformanceCounter();
            soundTime += (double)soundPerf / (double)SDL_GetPerformanceFrequency(); 
            if (os[j].when <= soundTime)
            {
                if(os[j].when + os[j].duration >= soundTime)
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
    SDL_UnlockAudioDevice(audioDevice);
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
	audioDevice = SDL_OpenAudioDevice(NULL, 0, &as, &audiospec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE );
    if(audioDevice == 0)
		return -1;

	if(audiospec.format != AUDIO_S16SYS)
    {
        SDL_CloseAudioDevice(audioDevice);
		return -1;
    }

    for(int i = 0; i < numOs; i++)
        os[i] = oscillate(SAMPLE_RATE, 0, oscVolume, 0, 0);
    soundticks = SDL_GetPerformanceCounter();
    SDL_PauseAudioDevice(audioDevice, 0);
	return 1;
}


float md_getAudioTime() 
{   
    return (float)(soundTime);
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                      float x, float y, int hash) 
{
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
        SDL_Surface *tmp = SDL_CreateRGBSurface(0, (int)ceilf((float)CHARACTER_WIDTH * scale), (int)ceilf((float)CHARACTER_HEIGHT * scale), 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
        if(tmp)
        {
            SDL_Surface *tmp2 = SDL_ConvertSurfaceFormat(tmp, SDL_PIXELFORMAT_RGBA8888, 0);        
            SDL_FreeSurface(tmp);
            if(tmp2)
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
                        Uint32 color = SDL_MapRGB(tmp2->format, (Uint8)r, (Uint8)g, (Uint8)b);
                        SDL_FillRect(tmp2, &dstChar, color);                        
                    }
                }  
                cp->sprite = SDL_CreateTextureFromSurface(Renderer, tmp2);
                cp->w = tmp2->w;
                cp->h = tmp2->h;
                if(cp->sprite)      
                    characterSpritesCount++;
                SDL_FreeSurface(tmp2);
            }
        }
    }

    if(cp && cp->sprite)
    {
        SDL_Rect dst = {offsetX , offsetY, viewW, viewH};
        SDL_RenderSetClipRect(Renderer, &dst);
        SDL_Rect dst2 = {(int)((float)(offsetX + x*scale)), (int)((float)(offsetY + y*scale)), cp->w, cp->h};
        SDL_RenderCopy(Renderer, cp->sprite, NULL, &dst2);
    }
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b)
{
    SDL_Rect dst = {offsetX , offsetY, viewW, viewH};
    SDL_RenderSetClipRect(Renderer, &dst);
    SDL_Rect dst2 = { (int)(offsetX + x * scale) , (int)(offsetY + y * scale), (int)ceilf(w * scale), (int)ceilf(h  * scale)};
    SDL_SetRenderDrawColor(Renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_RenderFillRect(Renderer, &dst2);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    //clear screen also in case we resize window
    md_clearScreen(clearColorR, clearColorG, clearColorB);
    SDL_Rect dst = {offsetX, offsetY, viewW, viewH};
    SDL_RenderSetClipRect(Renderer, &dst);
    SDL_SetRenderDrawColor(Renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_Rect dst2 = {offsetX , offsetY, viewW, viewH};
    SDL_RenderFillRect(Renderer, &dst2);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    clearColorR = r;
    clearColorG = g;
    clearColorB = b;

    SDL_Rect dst = {0 , 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderSetClipRect(Renderer, &dst);
    SDL_SetRenderDrawColor(Renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_RenderClear(Renderer);
}

static int resizingEventWatcher(void* data, SDL_Event* event) {
  if (event->type == SDL_WINDOWEVENT &&
      event->window.event == SDL_WINDOWEVENT_RESIZED) {
    SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
    if (win == (SDL_Window*)data) {
        SDL_GetWindowSize(SdlWindow, &WINDOW_WIDTH , &WINDOW_HEIGHT);
        md_initView(origViewW, origViewH);
        md_clearScreen(clearColorR, clearColorG, clearColorB);
    }
  }
  return 0;
}

void md_initView(int w, int h) 
{
    origViewW = w;
    origViewH = h;
    float xScale = (float)WINDOW_WIDTH / w;
    float yScale = (float)WINDOW_HEIGHT / h;
    if (yScale < xScale)
        scale = yScale;
    else
        scale = xScale;
    viewW = (int)ceilf((float)w * scale);
    viewH = (int)ceilf((float)h * scale);
    offsetX = (int)(WINDOW_WIDTH - viewW) >> 1;
    offsetY = (int)(WINDOW_HEIGHT - viewH) >> 1;
    SDL_Rect dst = {offsetX , offsetY, viewW, viewH};
    SDL_RenderSetClipRect(Renderer, &dst);  
    resetCharacterSprite();
}

void md_consoleLog(char* msg) 
{ 
    SDL_Log(msg); 
}

void update() {    
    for(int i = 0; i < BUTTON_COUNT; i++)
        prevKeys[i] = keys[i];
    
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_KEYDOWN)
        {
            switch(event.key.keysym.sym)
            {
                case BUTTON_A:
                    keys[BUTTON_A_INDEX] = 1;
                    break;
                case BUTTON_B:
                    keys[BUTTON_B_INDEX] = 1;
                    break;
                case BUTTON_LEFT:
                    keys[BUTTON_LEFT_INDEX] = 1;
                    break;
                case BUTTON_RIGHT:
                    keys[BUTTON_RIGHT_INDEX] = 1;
                    break;
                case BUTTON_DOWN:
                    keys[BUTTON_DOWN_INDEX] = 1;
                    break;
                case BUTTON_UP:
                    keys[BUTTON_UP_INDEX] = 1;
                    break;
                case BUTTON_MENU:
                    keys[BUTTON_MENU_INDEX] = 1;
                    break;
                case BUTTON_VOLDOWN:
                    keys[BUTTON_VOLDOWN_INDEX] = 1;
                    break;
                case BUTTON_VOLUP:
                    keys[BUTTON_VOLUP_INDEX] = 1;
                    break;
                default:
                    break;
            }
        }

        if(event.type == SDL_KEYUP)
        {
            switch(event.key.keysym.sym)
            {
                case BUTTON_A:
                    keys[BUTTON_A_INDEX] = 0;
                    break;
                case BUTTON_B:
                    keys[BUTTON_B_INDEX] = 0;
                    break;
                case BUTTON_LEFT:
                    keys[BUTTON_LEFT_INDEX] = 0;
                    break;
                case BUTTON_RIGHT:
                    keys[BUTTON_RIGHT_INDEX] = 0;
                    break;
                case BUTTON_DOWN:
                    keys[BUTTON_DOWN_INDEX] = 0;
                    break;
                case BUTTON_UP:
                    keys[BUTTON_UP_INDEX] = 0;
                    break;
                case BUTTON_MENU:
                    keys[BUTTON_MENU_INDEX] = 0;
                    break;
                case BUTTON_VOLDOWN:
                    keys[BUTTON_VOLDOWN_INDEX] = 0;
                    break;
                case BUTTON_VOLUP:
                    keys[BUTTON_VOLUP_INDEX] = 0;
                    break;
                default:
                    break;
            }
        }

        if(event.type == SDL_QUIT)
            quit = 1;
    }
    
    setButtonState(keys[BUTTON_LEFT_INDEX] == 1, keys[BUTTON_RIGHT_INDEX] == 1, keys[BUTTON_UP_INDEX] == 1,
        keys[BUTTON_DOWN_INDEX] == 1, keys[BUTTON_B_INDEX] == 1, keys[BUTTON_A_INDEX] == 1);
    
    if ((prevKeys[BUTTON_MENU_INDEX] == 0) && (keys[BUTTON_MENU_INDEX] == 1))
    {
        if (!isInMenu)
            goToMenu();
        else
            quit = 1;
    }
 
    if ((prevKeys[BUTTON_VOLDOWN_INDEX] == 0) && (keys[BUTTON_VOLDOWN_INDEX] == 1))
    {
        audioVolume -= 0.05f;
        if(audioVolume < 0.0f)
            audioVolume = 0.0f;    
    }

    if ((prevKeys[BUTTON_VOLUP_INDEX] == 0) && (keys[BUTTON_VOLUP_INDEX] == 1))
    {
        audioVolume += 0.05f;
        if(audioVolume > 1.0f)
            audioVolume = 1.0f;     
    }

    updateFrame();
    SDL_RenderPresent(Renderer);
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
    printf("  -a: Use hardware accelerated rendering (default is software)\n");
}

int main(int argc, char **argv)
{
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
        
        if(strcasecmp(argv[i], "-a") == 0)
            useHWSurface = true;
        
        if(strcasecmp(argv[i], "-ns") == 0)
			noAudioInit = true;

        if(strcasecmp(argv[i], "-w") == 0)
            if(i+1 < argc)
                WINDOW_WIDTH = atoi(argv[i+1]);
        
        if(strcasecmp(argv[i], "-h") == 0)
            if(i+1 < argc)
                WINDOW_HEIGHT = atoi(argv[i+1]);
        
    }


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0)
    {
        SDL_Log("SDL Succesfully initialized\n");

        Uint32 WindowFlags = SDL_WINDOW_RESIZABLE;
        if (fullScreen)
        {
            WindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }

        SdlWindow = SDL_CreateWindow("Crisp Game Lib Portable Sdl2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, WindowFlags);

        if (SdlWindow)
        {
            SDL_AddEventWatch(resizingEventWatcher, SdlWindow);
            Uint32 flags = 0;
            if (useHWSurface == 0)
                flags |= SDL_RENDERER_SOFTWARE;
            else
                flags |= SDL_RENDERER_ACCELERATED;

            SDL_Log("Succesfully Set %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
            SDL_ShowCursor(SDL_DISABLE);
            Renderer = SDL_CreateRenderer(SdlWindow, -1, flags);
            if (Renderer)
            {
                SDL_RendererInfo rendererInfo;
                SDL_GetRendererInfo(Renderer, &rendererInfo);
                SDL_Log("Using Renderer:%s\n", rendererInfo.name);
                SDL_Log("Succesfully Created Buffer\n");      
                if(!noAudioInit)
                    soundOn = InitAudio();
                if(soundOn == 1)
                    SDL_Log("Succesfully opened audio\n");
                else
                    SDL_Log("Failed to open audio\n");
                initCharacterSprite();
                initGame();
                while(quit == 0)
                {
                    frameticks = SDL_GetPerformanceCounter();
                    update();                
                    if(quit == 0)
                    {
                        Uint64 frameEndTicks = SDL_GetPerformanceCounter();
                        Uint64 FramePerf = frameEndTicks - frameticks;
                        frameTime = FramePerf / (double)SDL_GetPerformanceFrequency() * 1000.0f;
                        double delay = 1000.0f / FPS - frameTime;
                        if (delay > 0.0f)
                            SDL_Delay((Uint32)(delay));                            
                    }
                }           
                resetCharacterSprite();
                SDL_DestroyRenderer(Renderer);
            }
            else
            {
                SDL_Log("Failed to created Renderer!\n");
            }
            SDL_DestroyWindow(SdlWindow);
        }		
        else
        {
            SDL_Log("Failed to create SDL_Window %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
        }
        SDL_Quit();
    }
    else
    {
        SDL_Log("Couldn't initialise SDL!\n");
    }
return 0;

}