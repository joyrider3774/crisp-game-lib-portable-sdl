#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL1.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define MAX_NOTES 128
#define AMPLITUDE 10000
#define FADE_OUT_TIME 0.05f    // Fade-out time in seconds

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
static float audioVolume = 1.00f;
static SDL_Surface *screen = NULL, *view = NULL;
static int soundOn = 0;

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
    SDL_Surface *sprite;
    int hash;
} CharaterSprite;

AudioState audio_state = {0};

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

// Sine wave oscillator function
float generate_sine_wave(float frequency, float time) {
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
            for (int j = 0; j < sample_count; j++) {
                float t = audio_state->time + (float)j / SAMPLE_RATE;
                if (t >= note->when) {
                    float amplitude = audioVolume; // Apply global volume

                    // Apply fade-out if the note is in its fade-out period
                    if (t > note_end) {
                        float fade_time = t - note_end;
                        amplitude *= (1.0f - (fade_time / FADE_OUT_TIME));
                        if (amplitude < 0.0f) amplitude = 0.0f; // Ensure no negative values
                    }

                    // Add this note's waveform to the float buffer
                    float_buffer[j] += generate_sine_wave(note->frequency, t) * AMPLITUDE * amplitude;
                }
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

	if(SDL_OpenAudio(&spec, &audiospec) < 0)
		return -1;

	if(audiospec.format != AUDIO_S16SYS)
    {
        SDL_CloseAudio();
		return -1;
    }

    SDL_PauseAudio(0);
	return 1;
}


float md_getAudioTime() 
{ 
    return audio_state.time;
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
            {
                soundOn = InitAudio();
                if(soundOn == 1)
                    printf("Succesfully opened audio\n");
                else
                    printf("Failed to open audio\n");
            }
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