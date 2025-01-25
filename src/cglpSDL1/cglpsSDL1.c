#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL1.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE ((SAMPLE_RATE / FPS) +1)
#define numOs 64

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static int keys[SDLK_LAST];
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static Uint32 frameticks = 0;
static float frameTime = 0.0f;
static Uint32 clearColor = 0;
static int prevMenu = 0;
static float audioVolume = 0.50f;
static int osIndex = 0;
SDL_Surface *screen, *view;

static SDL_AudioSpec audiospec;



typedef struct {
  float current_step;
  float step_size;
  float volume;
  float duration;
  float when;
  int freq;
} oscillator;

oscillator oscillate(float sampleRate, float freq, float volume, float duration, float when) 
{
  oscillator o = {
      .current_step = 0,
      .volume = volume,
      .step_size = (2 * M_PI) / (((float)sampleRate / freq)),
      .duration = duration,
      .when = when,
      .freq = freq,
  };
  return o;
}

float next(oscillator *os) {
  float ret = sinf(os->current_step);
  os->current_step += os->step_size;
  return ret;
}

oscillator os[numOs];


void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                      float x, float y, int hash) 
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
            SDL_Rect dst = {(Sint16)(x*scale + (float)xx*scale), (Sint16)(y*scale + (float)yy*scale), (Uint16)(ceil(scale)), (Uint16)(ceil(scale))};
            Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
            SDL_FillRect(view, &dst, color);
        }
    }
}


void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b)
{
  SDL_Rect dst = { (Sint16)(x * scale) , (Sint16)(y * scale), (Uint16)ceil(w * scale), (Uint16)ceil(h  * scale)};
  Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
  SDL_FillRect(view, &dst, color);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(view, NULL, color);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    clearColor = SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(screen, NULL, clearColor);
}

void md_playTone(float freq, float duration, float when) 
{
    if(when + duration <= (float)SDL_GetTicks() / 1000.0f)
        return;

    os[osIndex] = oscillate(SAMPLE_RATE, freq, audioVolume, duration, when);
    osIndex++;
    if(osIndex == numOs)
        osIndex = 0;

}

void md_stopTone() 
{
    for(int i = 0; i < numOs; i++)
    {
        os[i].current_step = 0;
        os[i].duration = 0;
        os[i].step_size = 0;
        os[i].when = 0;
        os[i].volume = 0;
        os[i].freq = 0;
    }
    osIndex = 0;
}

float md_getAudioTime() 
{ 
    return (float)SDL_GetTicks() / 1000.0f;
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

    SDL_Rect clipRect = {0, 0, viewW, viewH};
    SDL_SetClipRect(view, &clipRect);
}

void md_consoleLog(char* msg) 
{ 
    printf(msg); 
}

static void audioCallBack(void *ud, Uint8 *stream, int len)
{
    memset(stream, 0, len);
    Sint16* fstream = (Sint16*)stream;
    for (int i = 0; i < len/2; i++) 
    {       
        fstream[i] = 100;  
        for(int j = 0; j < numOs; j++)
        {
            float time = (float)SDL_GetTicks()/1000.0f;
            if (os[j].when <= time)
            {
                if(os[j].when + os[j].duration >= time)
                {
                    float v = next(&os[j]);
                    fstream[i] = (Sint16)(clamp((float)fstream[i] + truncf(clamp(v * 32767.0, -32767.0, 32767.0)), -32767.0, 32767.0));
                }
                else
                {
                    os[j].current_step = 0;
                    os[j].duration = 0;
                    os[j].step_size = 0;
                    os[j].when = 0;
                    os[j].volume = 0;
                    os[j].freq = 0;
                }
            } 
        }
        fstream[i] = (Sint16)((float)fstream[i] * audioVolume);
    }

    // for (int j = 0; j < numOs; j++)
    // {
    //     SDL_MixAudio((Uint8*)stream, (Uint8*)Buffer[j], len, 0);
    // }
    
}


int InitAudio()
{
	SDL_AudioSpec as;
	as.format = AUDIO_S16;
    as.channels = 1;
    as.freq = SAMPLE_RATE;
    as.samples = BUFFER_SIZE;
	as.callback = &audioCallBack;
	if(SDL_OpenAudio(&as, &audiospec) < 0)
		return -1;

	if(audiospec.format != AUDIO_S16SYS)
		return -1;

    for(int i = 0; i < numOs; i++)
        os[i] = oscillate(SAMPLE_RATE, 0, 0.75f, 0, 0);

    SDL_PauseAudio(0);
	return 0;
}

void update() {
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
    updateFrame();    
    SDL_Rect src = {0, 0, viewW, viewH};
    SDL_Rect dst = {(WINDOW_WIDTH - viewW) >> 1, (WINDOW_HEIGHT - viewH) >> 1, viewW, viewH};
    SDL_BlitSurface(view, &src, screen, &dst);
    SDL_Flip(screen);
    if ((prevMenu == 0) && (keys[BUTTON_MENU] == 1))
    {
        if (!isInMenu)
            goToMenu();
        else
            quit = 1;
    }
    prevMenu = keys[BUTTON_MENU];
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0)
	{
        printf("SDL Succesfully initialized\n");
		bool fullScreen = false;
        bool useHWSurface = false;
		for (int i=0; i < argc; i++)
		{
			if(strcasecmp(argv[i], "-f") == 0)
				fullScreen = true;
            
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
            view = SDL_DisplayFormat(screen);
            if(InitAudio() == 0)
            {
                printf("Succesfully opened audio\n");    
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
            }
            SDL_FreeSurface(view);
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