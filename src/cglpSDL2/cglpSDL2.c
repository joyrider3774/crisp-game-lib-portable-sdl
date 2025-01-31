#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL2.h"
#include "CInput.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define MAX_NOTES 128
#define AMPLITUDE 10000
#define FADE_OUT_TIME 0.05f    // Fade-out time in seconds

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static int origViewW = DEFAULT_WINDOW_WIDTH;
static int origViewH = DEFAULT_WINDOW_HEIGHT;
static Uint64 frameticks = 0;
static double frameTime = 0.0f;
static unsigned char clearColorR = 0;
static unsigned char clearColorG = 0;
static unsigned char clearColorB = 0;
static float audioVolume = 1.00f;
static int offsetX = 0 ;
static int offsetY = 0;
static int soundOn = 0;
static int useBugSound = 1;
static SDL_AudioDeviceID audioDevice = 0;
CInput *GameInput;
SDL_Renderer *Renderer = NULL;
SDL_Window *SdlWindow = NULL;
static SDL_AudioSpec audiospec = {0};

typedef struct {
    float frequency; // Frequency of the note in Hz
    float when;      // Time in seconds to start playing the note
    float duration;  // Duration in seconds to play the note
    bool active;     // Whether this note is currently active
} Note;

typedef struct {
    Note notes[MAX_NOTES]; // List of scheduled notes
    int note_count;        // Current number of notes
    float time;            // Current playback time in seconds
} AudioState;

typedef struct 
{
    SDL_Texture *sprite;
    int hash;
    int w;
    int h;
} CharaterSprite;

AudioState audio_state = {0};

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

// Function to normalize angle to the range [0, 2π)
#define NORMALIZE_ANGLE(angle) (angle = fmodf(angle, 2 * M_PI), (angle < 0) ? (angle += 2 * M_PI) : angle)


// Simulate buggy sinf: restricts output to 0, 1, -1 based on 90° increments
float buggy_sinf(float angle) 
{
    NORMALIZE_ANGLE(angle);  // Normalize angle to [0, 2π)

    // Map angle to nearest 90° (π/2 radians)
    if (angle < M_PI_4 || angle >= (2 * M_PI - M_PI_4)) 
    {
        return 0.0f;  // Closest to 0° or 360°
    } else if (angle < (M_PI_2 + M_PI_4)) {
        return 1.0f;  // Closest to 90°
    } else if (angle < (M_PI + M_PI_4)) {
        return 0.0f;  // Closest to 180°
    } else if (angle < (3 * M_PI_2 + M_PI_4)) {
        return -1.0f; // Closest to 270°
    }

    return 0.0f;  // Default fallback
}

// Sine wave oscillator function
float generate_sine_wave(float frequency, float time) 
{
    if(useBugSound == 1)
        return buggy_sinf(2.0f * M_PI * frequency * time);
    else
        return sinf(2.0f * M_PI * frequency * time);
}

// Audio callback
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioState *audio_state = (AudioState *)userdata;
    int16_t *buffer = (int16_t *)stream;
    int sample_count = len / sizeof(int16_t);

    // Intermediate float buffer to accumulate the summed waveforms
    float float_buffer[sample_count];
    for (int i = 0; i < sample_count; i++) {
        float_buffer[i] = 0.0f;
    }

    // Track active notes
    int active_note_count = 0;

    // Sum of all active notes' waveforms
    for (int i = 0; i < audio_state->note_count; i++) {
        Note *note = &audio_state->notes[i];

        if (!note->active && audio_state->time >= note->when) {
            note->active = true; // Activate the note when the time comes
        }

        if (note->active) {
            float note_end = note->when + note->duration;
            if (audio_state->time > note_end + FADE_OUT_TIME) {
                note->active = false; // Mark note as inactive after fade-out
                continue; // Skip to the next note
            }

            // Generate audio for active notes
            for (int j = 0; j < sample_count; j++) 
            {
                int numtones = 0;
                float t = audio_state->time + (float)j / SAMPLE_RATE;
                if (t >= note->when) {
                    float amplitude = audioVolume; // Apply global volume
                    numtones++;
                    // Apply fade-out if the note is in its fade-out period
                    if (t > note_end) {
                        float fade_time = t - note_end;
                        amplitude *= (1.0f - (fade_time / FADE_OUT_TIME));
                        if (amplitude < 0.0f) amplitude = 0.0f; // Ensure no negative values
                    }

                    // Add this note's waveform to the float buffer
                    float_buffer[j] += generate_sine_wave(note->frequency, t) * AMPLITUDE * amplitude;
                }
                float_buffer[j]  = float_buffer[j]  / numtones;
                
            }
        }

        // Always add notes that are either active or scheduled for the future
        if (note->active || (note->when > audio_state->time)) {
            audio_state->notes[active_note_count++] = *note;
        }
    }

    // Update the note count to reflect only active notes
    audio_state->note_count = active_note_count;

    // Find the maximum amplitude in the float buffer and normalize
    float max_amplitude = 0.0f;
    for (int i = 0; i < sample_count; i++) {
        if (float_buffer[i] > max_amplitude) max_amplitude = float_buffer[i];
        if (float_buffer[i] < -max_amplitude) max_amplitude = -float_buffer[i];
    }

    // If the maximum amplitude exceeds the allowed range, scale it down
    if (max_amplitude > 32767.0f) {
        float scale_factor = 32767.0f / max_amplitude;
        for (int i = 0; i < sample_count; i++) {
            // Normalize and directly convert to int16_t
            buffer[i] = (int16_t)(float_buffer[i] * scale_factor);
        }
    } else {
        // If there's no clipping, just convert to int16_t directly
        for (int i = 0; i < sample_count; i++) {
            buffer[i] = (int16_t)float_buffer[i];
        }
    }

    // Update the current playback time
    audio_state->time += (float)sample_count / SAMPLE_RATE;
}

void schedule_note(AudioState *audio_state, float frequency, float when, float duration) 
{
    if (audio_state->note_count >= MAX_NOTES) 
    {
        return;
    }
    
    Note *note = &audio_state->notes[audio_state->note_count++];
    note->frequency = frequency;
    note->when = when;
    note->duration = duration;
    note->active = false;
}


void md_playTone(float freq, float duration, float when) 
{
    if(soundOn != 1)
        return;
   
    schedule_note(&audio_state, freq, when, duration);
}

void md_stopTone() 
{
    if (soundOn != 1)
        return;

    for (int i = 0; i < audio_state.note_count; i++) 
    {
        audio_state.notes[i].duration = 0;
    }
}


int InitAudio()
{
	SDL_AudioSpec spec = {0};
    spec.freq = SAMPLE_RATE;
    spec.format = AUDIO_S16SYS; // Use signed 16-bit audio
    spec.channels = 1;         // Mono audio
    spec.samples = BUFFER_SIZE;
    spec.callback = audio_callback;
    spec.userdata = &audio_state;
	audioDevice = SDL_OpenAudioDevice(NULL, 0, &spec, &audiospec, 0 );
    if(audioDevice == 0)
		return -1;

	if(audiospec.format != AUDIO_S16SYS)
    {
        SDL_CloseAudioDevice(audioDevice);
		return -1;
    }


    SDL_PauseAudioDevice(audioDevice, 0);
	return 1;
}


float md_getAudioTime() 
{   
    return audio_state.time ;
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
    //adjust for different behaviour between sdl and js in case of negative width / height
    if(w < 0.0f)
    {
        x += w;
        w *= -1.0f; 
    }

    if(h < 0.0f)
    {
        y += h;
        h *= -1.0f; 
    }

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
    CInput_Update(GameInput);
    if(GameInput->Buttons.ButQuit)
        quit = 1;
    
    setButtonState(GameInput->Buttons.ButLeft || GameInput->Buttons.ButDpadLeft, GameInput->Buttons.ButRight || GameInput->Buttons.ButDpadRight,
        GameInput->Buttons.ButUp || GameInput->Buttons.ButDpadUp, GameInput->Buttons.ButDown || GameInput->Buttons.ButDpadDown, 
        GameInput->Buttons.ButB, GameInput->Buttons.ButA);

    float mouseX = ((GameInput->Buttons.MouseX - offsetX) / scale);
    float mouseY = ((GameInput->Buttons.MouseY - offsetY) / scale);
    setMousePos(mouseX, mouseY);
    
    if ((!GameInput->PrevButtons.ButBack) && (GameInput->Buttons.ButBack))
    {
        if (!isInMenu)
            goToMenu();
        else
            quit = 1;
    }
 
    if ((!GameInput->PrevButtons.ButLB) && (GameInput->Buttons.ButLB))
    {
        audioVolume -= 0.05f;
        if(audioVolume < 0.0f)
            audioVolume = 0.0f;    
    }

    if ((!GameInput->PrevButtons.ButRB) && (GameInput->Buttons.ButRB))
    {
        audioVolume += 0.05f;
        if(audioVolume > 1.0f)
            audioVolume = 1.0f;     
    }

    if ((!GameInput->PrevButtons.ButY) && (GameInput->Buttons.ButY))
    {
        if(useBugSound == 1)
            useBugSound = 0;
        else
            useBugSound = 1;
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


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) == 0)
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
            Renderer = SDL_CreateRenderer(SdlWindow, -1, flags);
            if (Renderer)
            {
                SDL_RendererInfo rendererInfo;
                SDL_GetRendererInfo(Renderer, &rendererInfo);
                SDL_Log("Using Renderer:%s\n", rendererInfo.name);
                SDL_Log("Succesfully Created Buffer\n");      
                if(!noAudioInit)
                {
                    soundOn = InitAudio();
                    if(soundOn == 1)
                        SDL_Log("Succesfully opened audio\n");
                    else
                        SDL_Log("Failed to open audio\n");
                }
                initCharacterSprite();
                initGame();
                GameInput = CInput_Create();
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
                CInput_Destroy(GameInput);
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