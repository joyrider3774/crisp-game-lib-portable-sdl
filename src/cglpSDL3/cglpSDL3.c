#if defined _WIN32 || defined __CYGWIN__
    #include <windows.h>
    #undef TRANSPARENT
#endif
#include <stdlib.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <string.h>
#include <math.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL3.h"
#include "CInput.h"

#ifdef USE_UINT64_TIMER
    typedef Uint64 TimerType;
    #define PHASE_MAX (1ULL << 32)
    #define FREQ_SCALE 65536.0f
#else
    typedef Uint32 TimerType;
    #define PHASE_MAX (1UL << 24)
    #define FREQ_SCALE 512.0f
#endif

//PI constants
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
    #define M_PI_2 1.57079632679489661923
#endif

#ifndef M_PI_4
    #define M_PI_4 0.78539816339744830962
#endif



#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define SOUND_CHANNELS 1       // do not change it only supports MONO but sdl might convert
#define MAX_NOTES 128
#define AMPLITUDE 10000
#define FADE_OUT_TIME 0.05f    // Fade-out time in seconds

#define DEFAULT_GLOW_SIZE 6 * DEFAULT_WINDOW_WIDTH / 240
#define DEFAULT_GLOW_INTENSITY 96
#define DEFAULT_OVERLAY 0
#define DEFAULT_GLOW_ENABLED false

#define FPS_SAMPLES 10

#define PIXELFORMAT SDL_PIXELFORMAT_RGBA32

// Function to normalize angle to the range [0, 2π)
#define NORMALIZE_ANGLE(angle) (angle = fmodf(angle, 2 * M_PI), (angle < 0) ? (angle += 2 * M_PI) : angle)

static float mouseX, mouseY;
static int prevRealMouseX = 0, prevRealMouseY = 0;
static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static int realViewW = DEFAULT_WINDOW_WIDTH;
static int realViewH = DEFAULT_WINDOW_HEIGHT;
static int origViewW = DEFAULT_WINDOW_WIDTH;
static int origViewH = DEFAULT_WINDOW_HEIGHT;
static Uint64 frameticks = 0;
static double frameTime = 0.0f;
static unsigned char clearColorR = 0;
static unsigned char clearColorG = 0;
static unsigned char clearColorB = 0;
static float audioVolume = 1.00f;
static int offsetX = 0;
static int offsetY = 0;
static int soundOn = 0;
static int useBugSound = 1;
static SDL_AudioStream *audioStream = NULL;
static SDL_AudioDeviceID audioDeviceId = 0;
CInput *GameInput;
SDL_Renderer *Renderer = NULL;
SDL_Window *SdlWindow = NULL;
static SDL_AudioSpec audiospec = {0};
static int audiospec_sample_frames;
static int overlay = DEFAULT_OVERLAY;
static int glowSize = DEFAULT_GLOW_SIZE;
static float wscale = 1.0f;
static bool glowEnabled = DEFAULT_GLOW_ENABLED;
static SDL_Surface *view = NULL;
static SDL_Texture *viewTexture = NULL;
static bool nodelay = false;
static char startgame[100] = {0};

static int fpsSamples[FPS_SAMPLES];
static bool showfps = false;
static float avgfps = 0;
static int framecount = 0;
static int lastfpstime = 0;
static int fpsAvgCount = 0;

static Uint64 lastCrtTime = 0;
static bool scaledDrawing = true;

typedef struct {
    float frequency; // Frequency of the note in Hz
    float when;      // Time in seconds to start playing the note
    float duration;  // Duration in seconds to play the note
    bool active;     // Whether this note is currently active
} Note;

typedef struct {
    Note notes[MAX_NOTES];    // List of scheduled notes
    int note_count;           // Current number of notes
    TimerType time;           // Current playback time (in samples)
} AudioState;

typedef struct 
{
    SDL_Surface *sprite;
    int hash;
} CharaterSprite;

typedef struct {
    Uint8* distances;  // Lookup table for distances
    int size;         // Size of the table (glowSize * 2 + 1)
} GlowDistanceTable;

typedef struct {
    SDL_Surface* scanlineSurface;
    int screenHeight;
    int screenWidth;
    int screenOffsetX;
    int screenOffsetY;
    float scrollOffset;
    int scanlineSpacing;
    float scanlineFps;
} CRTEffect;

typedef struct {
    char title[100];
    int overlay;
    bool glowEnabled;
    bool isDarkColor;
} gameOverlay;

gameOverlay gameOverLays[MAX_GAME_COUNT];

CRTEffect* crtEffect = NULL;

static GlowDistanceTable* distanceTable = NULL;

AudioState audio_state = {0};

static CharaterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;

static void logMessage(SDL_PRINTF_FORMAT_STRING const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#if defined _WIN32 || defined __CYGWIN__
    vprintf(fmt, ap);
#else
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);
#endif    
    va_end(ap);   
}

static void resetGame(Game *game)
{
    if((SDL_strlen(game->title) == 0) || (game->update == NULL) )
        return;

    int freeIndex = -1;
    for (int i = 0; i < gameCount; i++)
    {
        if((freeIndex == -1) && (SDL_strlen(gameOverLays[i].title) == 0))
            freeIndex = i;

        if ((SDL_strlen(gameOverLays[i].title) > 0) && (SDL_strcmp(game->title, gameOverLays[i].title) == 0 ))
        {
            if(scaledDrawing)
            {
                overlay = gameOverLays[i].overlay;
                glowEnabled = gameOverLays[i].glowEnabled;
            }
            else
            {
                overlay = 0;
                glowEnabled = false;
            }
            game->options.isDarkColor = gameOverLays[i].isDarkColor;
            return;
        }
    } 

    //no match found add new game
    if(freeIndex > -1)
    {
        gameOverLays[freeIndex].overlay = overlay = 0;
        gameOverLays[freeIndex].glowEnabled = glowEnabled = false;
        memset(gameOverLays[freeIndex].title, 0, 100*sizeof(char));
        strcpy(gameOverLays[freeIndex].title, game->title);
        gameOverLays[freeIndex].isDarkColor = game->options.isDarkColor;
    }

}

static CRTEffect* CreateCRTEffect(int screenWidth, int screenHeight, int screenOffsetX, int screenOffsetY,
    int scanlineSpacing, int scanelineThickness, float scanlineFps, 
    Uint8 scanlineR, Uint8 scanlineG, Uint8 scanlineB, Uint8 scanlineA) {
    
    CRTEffect* effect = (CRTEffect*)SDL_malloc(sizeof(CRTEffect));
    if (!effect) return NULL;

    effect->screenHeight = screenHeight;
    effect->screenWidth = screenWidth;
    effect->screenOffsetX = screenOffsetX;
    effect->screenOffsetY = screenOffsetY;
    effect->scrollOffset = 0.0f;
    effect->scanlineSpacing = scanlineSpacing;
    effect->scanlineFps = scanlineFps;

    effect->scanlineSurface = SDL_CreateSurface(screenWidth, screenHeight, PIXELFORMAT);

    if (!effect->scanlineSurface) {
        SDL_free(effect);
        return NULL;
    }

    // Set the alpha blend mode
    if (!SDL_SetSurfaceBlendMode(effect->scanlineSurface, SDL_BLENDMODE_BLEND)) 
    {
        SDL_DestroySurface(effect->scanlineSurface);
        SDL_free(effect);
        return NULL;
    }

    // Clear to transparent
    SDL_FillSurfaceRect(effect->scanlineSurface, NULL, SDL_MapSurfaceRGBA(effect->scanlineSurface, 0, 0, 0, 0));

    // Draw the scanlines
    SDL_Rect lineRect = {0, 0, screenWidth, scanelineThickness};
    Uint32 lineColor = SDL_MapSurfaceRGBA(effect->scanlineSurface, scanlineR, scanlineG, scanlineB, scanlineA);

    for (int y = 0; y < screenHeight; y += scanlineSpacing) {
        lineRect.y = y;
        SDL_FillSurfaceRect(effect->scanlineSurface, &lineRect, lineColor);
    }

    return effect;
}

static void UpdateCRTEffect(CRTEffect* effect, float deltaTime)
{
    if (!effect) return;

    effect->scrollOffset += effect->scanlineFps * deltaTime;
    
    if (effect->scrollOffset >= effect->scanlineSpacing) {
        effect->scrollOffset = 0.0f;
    }
}

static void RenderCRTEffect(SDL_Surface* screenSurface, CRTEffect* effect)
{
    if (!effect || !screenSurface) return;

    int offsetY = (int)effect->scrollOffset;
    
    // First part: from offset to end of screen
    SDL_Rect srcRect1 = {
        0,
        offsetY,
        effect->screenWidth,
        effect->screenHeight - offsetY
    };
    
    SDL_Rect dstRect1 = {
        effect->screenOffsetX,
        effect->screenOffsetY,
        effect->screenWidth,
        effect->screenHeight - offsetY
    };

    SDL_BlitSurface(effect->scanlineSurface, &srcRect1, screenSurface, &dstRect1);
    
    // Second part: wrap around from top of texture
    if (offsetY > 0) {
        SDL_Rect srcRect2 = {
            0,
            0,
            effect->screenWidth,
            offsetY
        };
        
        SDL_Rect dstRect2 = {
            effect->screenOffsetX,
            effect->screenOffsetY + effect->screenHeight - offsetY,
            effect->screenWidth,
            offsetY
        };

        SDL_BlitSurface(effect->scanlineSurface, &srcRect2, screenSurface, &dstRect2);
    }
}

static void DestroyCRTEffect(CRTEffect* effect)
{
    if (!effect) return;
    
    if (effect->scanlineSurface) {
        SDL_DestroySurface(effect->scanlineSurface);
    }
    SDL_free(effect);
}

static void loadGameOverlays()
{
    //initialize
    for (int i = 0; i < gameCount; i++)
    {
        memset(gameOverLays[i].title, 0, 100 * sizeof(char));
        gameOverLays[i].overlay = 0;
        gameOverLays[i].glowEnabled = false;
        gameOverLays[i].isDarkColor = false;
    }
    onResetGame = resetGame;
    //load
    char fileName[FILENAME_MAX];
    sprintf(fileName,"%s/.cglpoverlays.dat",SDL_getenv("HOME") == NULL ? ".": SDL_getenv("HOME"));
    FILE *fp;
    fp = fopen(fileName, "rb");
    if(fp)
    {
        int i = 0;
        while (!feof(fp) && (i < gameCount))
        {
            fread(gameOverLays[i].title, sizeof(char), 100, fp);
            fread(&gameOverLays[i].overlay, sizeof(int), 1, fp);
            fread(&gameOverLays[i].glowEnabled, sizeof(bool), 1, fp);
            fread(&gameOverLays[i].isDarkColor, sizeof(bool), 1, fp);
            i++;
        }
        fclose(fp);
    }
}

static void saveGameOverlays()
{
    char fileName[FILENAME_MAX];
    sprintf(fileName,"%s/.cglpoverlays.dat", SDL_getenv("HOME") == NULL ? ".": SDL_getenv("HOME"));
    FILE *fp;
    fp = fopen(fileName, "wb");
    if(fp)
    {
        for (int i = 0; i < gameCount; i++)
        {
            if(SDL_strlen(gameOverLays[i].title) > 0)
            {
                fwrite(gameOverLays[i].title, sizeof(char), 100, fp);
                fwrite(&gameOverLays[i].overlay, sizeof(int), 1, fp);
                fwrite(&gameOverLays[i].glowEnabled, sizeof(bool), 1, fp);
                fwrite(&gameOverLays[i].isDarkColor, sizeof(bool), 1, fp);
            }
        }
        fclose(fp);
    }
}

static void loadHighScores()
{
    char fileName[FILENAME_MAX];
    sprintf(fileName,"%s/.cglpscore.dat", SDL_getenv("HOME") == NULL ? ".": SDL_getenv("HOME"));
    FILE *fp;
    fp = fopen(fileName, "rb");
    if(fp)
    {
        int i = 0;
        while (!feof(fp) && (i < gameCount))
        {
            fread(hiScores[i].title, sizeof(char), 100, fp);
            fread(&hiScores[i].hiScore, sizeof(int), 1, fp);
            i++;
        }
        fclose(fp);
    }
}

static void saveHighScores()
{
    char fileName[FILENAME_MAX];
    sprintf(fileName,"%s/.cglpscore.dat", SDL_getenv("HOME") == NULL ? ".": SDL_getenv("HOME"));
    FILE *fp;
    fp = fopen(fileName, "wb");
    if(fp)
    {
        for (int i = 0; i < gameCount; i++)
        {
            if(SDL_strlen(hiScores[i].title) > 0)
            {                
                fwrite(hiScores[i].title, sizeof(char), 100, fp);
                fwrite(&hiScores[i].hiScore, sizeof(int), 1, fp);
            }
        }
        fclose(fp);
    }
}

static GlowDistanceTable* createDistanceTable(int glowSize) {
    GlowDistanceTable* table = (GlowDistanceTable*)SDL_malloc(sizeof(GlowDistanceTable));
    if (!table) return NULL;

    int size = glowSize * 2 + 1;
    table->size = size;
    table->distances = (Uint8*)SDL_malloc((size_t)size * size);
    
    if (!table->distances) {
        SDL_free(table);
        return NULL;
    }

    // Pre-calculate distances using integer arithmetic
    int centerX = glowSize;
    int centerY = glowSize;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            // Fast integer-based distance approximation
            int dist = (dx * dx + dy * dy);
            // Convert to 0-255 range based on max possible distance
            int maxDist = glowSize * glowSize;
            int scaledDist = (dist * 255) / (maxDist == 0 ? 1 : maxDist);
            if (scaledDist > 255) scaledDist = 255;
            table->distances[y * size + x] = 255 - scaledDist;
        }
    }
    
    return table;
}

static void initCharacterSprite() {
    for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
        characterSprites[i].sprite = NULL;
    }
    characterSpritesCount = 0;
    distanceTable = NULL;  // Initialize distance table pointer
}

static void resetCharacterSprite() {
    for (int i = 0; i < characterSpritesCount; i++) {
        SDL_DestroySurface(characterSprites[i].sprite);
        characterSprites[i].sprite = NULL;
    }
    characterSpritesCount = 0;
    
    if (distanceTable) {
        SDL_free(distanceTable->distances);
        SDL_free(distanceTable);
        distanceTable = NULL;
    }
}

// Simulate buggy sinf: restricts output to 0, 1, -1 based on 90° increments
static float buggySinf(float angle)
{
    NORMALIZE_ANGLE(angle);  // Normalize angle to [0, 2π)

    // Map angle to nearest 90 (p/2 radians)
    if (angle < M_PI_4 || angle >= (2 * M_PI - M_PI_4)) 
    {
        return 0.0f;  // Closest to 0 or 360
    } else if (angle < (M_PI_2 + M_PI_4)) {
        return 1.0f;  // Closest to 90
    } else if (angle < (M_PI + M_PI_4)) {
        return 0.0f;  // Closest to 180
    } else if (angle < (3 * M_PI_2 + M_PI_4)) {
        return -1.0f; // Closest to 270
    }

    return 0.0f;  // Default fallback
}


static TimerType timeToSample(float t) { return (TimerType)(t * SAMPLE_RATE); }
static float sampleToTime(TimerType s) { return (float)s / SAMPLE_RATE; }

// Sine wave oscillator function
static float generateSineWave(float frequency, TimerType ticks) 
{
    // Calculate fixed-point frequency representation
    TimerType freq_fixed = (TimerType)((frequency * PHASE_MAX) / SAMPLE_RATE);
    
    // Calculate phase using fixed-point arithmetic
    TimerType phase = (ticks * freq_fixed) & (PHASE_MAX - 1);
    
    // Convert phase to float angle
    float phase_float = (2.0f * M_PI * phase) / PHASE_MAX;
    
    return useBugSound ? buggySinf(phase_float) : sinf(phase_float);
}

// Audio callback
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioState *audio_state = (AudioState *)userdata;
    Sint16 *buffer = (Sint16 *)stream;
    int sample_count = (len / sizeof(Sint16));
    // Intermediate float buffer to accumulate the summed waveforms
    float* float_buffer = (float*)SDL_malloc(sample_count * sizeof(float));
    if (float_buffer == NULL)
        return;
 
    memset(float_buffer, 0, sample_count * sizeof(float));
   
    // Track active notes
    int active_note_count = 0;


    for (int i = 0; i < audio_state->note_count; i++) 
    {
        Note *note = &audio_state->notes[i];
       
        // Convert note start time to current time context
        TimerType note_start_sample = timeToSample(note->when);
        float current_sample_time = sampleToTime(audio_state->time);
       
        if (!note->active && current_sample_time >= note->when) 
        {
            note->active = true;
        }

        if (note->active) 
        {
            // Determine if note should be deactivated
            if (current_sample_time > note->when + note->duration + FADE_OUT_TIME) 
            {
                note->active = false; // Mark note as inactive after fade-out
                continue; // Skip to the next note
            }
           
            // Sum of all active notes' waveforms
            for (int j = 0; j < sample_count; j++) 
            {
                TimerType current_sample = audio_state->time + j;
                float sample_time = sampleToTime(current_sample);
                float amplitude = audioVolume;

                float note_end_time = note->when + note->duration;

                // Fade out ending notes
                if (sample_time > note_end_time) 
                {
                    float fade_progress = (sample_time - note_end_time) / FADE_OUT_TIME;
                    amplitude *= (1.0f - fade_progress);
                    if (amplitude < 0.0f) 
                        amplitude = 0.0f;
                }
				
			    // Add this note's waveform to the float buffer
                // Use sample time for wave generation
                float_buffer[j] += generateSineWave(note->frequency, current_sample) * AMPLITUDE * amplitude;
            }
        }

        // Always add notes that are either active or scheduled for the future
        if (note->active || (note_start_sample > audio_state->time)) 
        {
            audio_state->notes[active_note_count++] = *note;
        }
    }

    // Update the note count to reflect only active notes
    audio_state->note_count = active_note_count;

    // Find the maximum amplitude in the float buffer and normalize
    float max_amplitude = 0.0f;
    for (int i = 0; i < sample_count; i++) 
    {
        if (float_buffer[i] > max_amplitude) 
            max_amplitude = float_buffer[i];
        if (float_buffer[i] < -max_amplitude) 
            max_amplitude = -float_buffer[i];
    }

    // If the maximum amplitude exceeds the allowed range, scale it down
    if (max_amplitude > 32767.0f) 
    {
        float scale_factor = 32767.0f / max_amplitude;
        for (int i = 0; i < sample_count; i++) 
        {
	        // Normalize and directly convert to Sint16
            buffer[i] = (Sint16)(float_buffer[i] * scale_factor);
        }
    } 
    else 
    {
        // If there's no clipping, just convert to Sint16 directly
        for (int i = 0; i < sample_count; i++) 
        {
            buffer[i] = (Sint16)float_buffer[i];
        }
    }

    audio_state->time += sample_count;
    SDL_free(float_buffer);
}

void SDLCALL SDL3AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    if (additional_amount > 0) 
    {
        Uint8 *data = SDL_stack_alloc(Uint8, additional_amount);
        if (data) 
        {
            audio_callback(userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}

static void schedule_note(AudioState *audio_state, float frequency, float when, float duration)
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


static int InitAudio()
{
	SDL_AudioSpec spec = {0};
    spec.freq = SAMPLE_RATE;
    spec.format = SDL_AUDIO_S16; // Use signed 16-bit audio
    spec.channels = SOUND_CHANNELS;
    
    //no more buffer size in spec
    char buf[10];
    SDL_itoa(BUFFER_SIZE, buf, 10);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, buf);
	
    audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, SDL3AudioCallback, &audio_state);
    if(audioStream == NULL)
    {
        logMessage("Failed to get audioStream: %s\n", SDL_GetError());
		return -1;
    }

    audioDeviceId = SDL_GetAudioStreamDevice(audioStream);
    if(audioDeviceId == 0)
    {
        logMessage("Failed to get DeviceId!\n");
        return -1;
    }

  
    if (SDL_GetAudioDeviceFormat(audioDeviceId, &audiospec, &audiospec_sample_frames)) 
    {
        logMessage("Requested audio specs: format: %d, freq: %d, channels: %d, frames:%d\n", spec.format, spec.freq, spec.channels, BUFFER_SIZE);
        logMessage("Obtained audio specs:  format: %d, freq: %d, channels: %d, frames:%d\n", audiospec.format, audiospec.freq, audiospec.channels, audiospec_sample_frames);         
    }
    else
    {
        logMessage("Failed to get Audio Device Format %s\n", SDL_GetError());
    }
       
    SDL_ResumeAudioDevice(audioDeviceId);
   
	return 1;
}


float md_getAudioTime() 
{ 
    return sampleToTime(audio_state.time);
}

static void applyGlowToRect(SDL_Surface* surface, SDL_Rect rect, int glowRadius, Uint8 glowAlpha,
                     Uint8 r, Uint8 g, Uint8 b) {
    if (!surface || glowRadius <= 0 || glowAlpha == 0) {
        return;
    }

    SDL_Surface* tempSurface = SDL_CreateSurface(rect.w + (glowRadius * 2), rect.h + (glowRadius * 2), PIXELFORMAT);

    if (!tempSurface) return;

    // Clear temp surface
    SDL_FillSurfaceRect(tempSurface, NULL, SDL_MapSurfaceRGBA(tempSurface, 0, 0, 0, 0));

    SDL_LockSurface(tempSurface);
    Uint32* pixels = (Uint32*)tempSurface->pixels;
    
    // For each glow layer
    for (int layer = glowRadius; layer > 0; layer--) {
        // Calculate alpha with quadratic falloff
        float alphaFactor = 1.0f - powf((float)layer / glowRadius, 2.0f);
        Uint8 currentAlpha = (Uint8)(alphaFactor * glowAlpha);
        
        if (currentAlpha <= 0) continue;

        // Draw top outer border
        SDL_Rect topRect = {
            glowRadius - layer,
            glowRadius - layer,
            rect.w + (layer * 2),
            1
        };

        // Draw glow borders in the temp surface
        for (int y = topRect.y; y < topRect.y + 1; y++) {
            for (int x = topRect.x; x < topRect.x + topRect.w; x++) {
                if (x >= 0 && x < tempSurface->w && y >= 0 && y < tempSurface->h) {
                    pixels[y * tempSurface->w + x] = SDL_MapSurfaceRGBA(tempSurface, r, g, b, currentAlpha);
                }
            }
        }

        // Bottom outer border
        SDL_Rect bottomRect = {
            glowRadius - layer,
            glowRadius + rect.h + layer - 1,
            rect.w + (layer * 2),
            1
        };

        for (int y = bottomRect.y; y < bottomRect.y + 1; y++) {
            for (int x = bottomRect.x; x < bottomRect.x + bottomRect.w; x++) {
                if (x >= 0 && x < tempSurface->w && y >= 0 && y < tempSurface->h) {
                    pixels[y * tempSurface->w + x] = SDL_MapSurfaceRGBA(tempSurface, r, g, b, currentAlpha);
                }
            }
        }

        // Left outer border
        SDL_Rect leftRect = {
            glowRadius - layer,
            glowRadius - layer,
            1,
            rect.h + (layer * 2)
        };

        for (int y = leftRect.y; y < leftRect.y + leftRect.h; y++) {
            for (int x = leftRect.x; x < leftRect.x + 1; x++) {
                if (x >= 0 && x < tempSurface->w && y >= 0 && y < tempSurface->h) {
                    pixels[y * tempSurface->w + x] = SDL_MapSurfaceRGBA(tempSurface, r, g, b, currentAlpha);
                }
            }
        }

        // Right outer border
        SDL_Rect rightRect = {
            glowRadius + rect.w + layer - 1,
            glowRadius - layer,
            1,
            rect.h + (layer * 2)
        };

        for (int y = rightRect.y; y < rightRect.y + rightRect.h; y++) {
            for (int x = rightRect.x; x < rightRect.x + 1; x++) {
                if (x >= 0 && x < tempSurface->w && y >= 0 && y < tempSurface->h) {
                    pixels[y * tempSurface->w + x] = SDL_MapSurfaceRGBA(tempSurface, r, g, b, currentAlpha);
                }
            }
        }
    }
    
    SDL_UnlockSurface(tempSurface);

    // Blit the temp surface to the target surface
    SDL_Rect dstRect = {
        rect.x - glowRadius,
        rect.y - glowRadius,
        tempSurface->w,
        tempSurface->h
    };
    SDL_BlitSurface(tempSurface, NULL, surface, &dstRect);
    SDL_DestroySurface(tempSurface);
}


// Update glow application to use distance table
static void applyGlowToCharacterPixel(SDL_Surface* surface, int centerX, int centerY,
                              Uint8 r, Uint8 g, Uint8 b, 
                              int glowRadius, Uint8 glowAlpha) {
    if (!surface || glowRadius <= 0) return;

    // Update distance table if needed
    if (!distanceTable || distanceTable->size != (glowRadius * 2 + 1)) {
        if (distanceTable) {
            SDL_free(distanceTable->distances);
            SDL_free(distanceTable);
        }
        distanceTable = createDistanceTable(glowRadius);
        if (!distanceTable) return;
    }

    SDL_LockSurface(surface);
    Uint32* pixels = (Uint32*)surface->pixels;
    
    int tableSize = distanceTable->size;
    int halfTable = tableSize / 2;

    // For each pixel in the glow area
    for (int dy = -glowRadius; dy <= glowRadius; dy++) {
        int y = centerY + dy;
        if (y < 0 || y >= surface->h) continue;

        for (int dx = -glowRadius; dx <= glowRadius; dx++) {
            int x = centerX + dx;
            if (x < 0 || x >= surface->w) continue;

            // Skip the center pixel
            if (dx == 0 && dy == 0) continue;

            // Get pre-calculated distance value
            int tableX = dx + halfTable;
            int tableY = dy + halfTable;
            Uint8 distance = distanceTable->distances[tableY * tableSize + tableX];

            if (distance > 0) {
                Uint8 layerAlpha = (distance * glowAlpha) >> 8;
                int idx = y * surface->w + x;
                
                Uint32 existing = pixels[idx];
                Uint8 er, eg, eb, ea;
                SDL_GetRGBA(existing, SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), &er, &eg, &eb, &ea);

                // Only update if new alpha is higher
                if (layerAlpha > ea) {
                    pixels[idx] = SDL_MapSurfaceRGBA(surface, r, g, b, layerAlpha);
                }
            }
        }
    }

    SDL_UnlockSurface(surface);
}

static SDL_Surface* createCharacterSurface(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                                  float scale, int glowRadius, Uint8 glowAlpha,
                                  bool withGlow) {
    int baseWidth = (int)ceilf((float)CHARACTER_WIDTH * scale);
    int baseHeight = (int)ceilf((float)CHARACTER_HEIGHT * scale);
    int fullWidth = withGlow ? baseWidth + (glowRadius * 2) : baseWidth;
    int fullHeight = withGlow ? baseHeight + (glowRadius * 2) : baseHeight;
    
    SDL_Surface* surface = SDL_CreateSurface(fullWidth, fullHeight, PIXELFORMAT);
    
    if (!surface) return NULL;

    // Clear surface
    SDL_FillSurfaceRect(surface, NULL, SDL_MapSurfaceRGBA(surface, 0, 0, 0, 0));
    
    int offset = withGlow ? glowRadius : 0;

    // First pass: Apply glow for each non-empty character pixel
    if (withGlow) {
        for (int yy = 0; yy < CHARACTER_HEIGHT; yy++) {
            for (int xx = 0; xx < CHARACTER_WIDTH; xx++) {
                unsigned char r = grid[yy][xx][0];
                unsigned char g = grid[yy][xx][1];
                unsigned char b = grid[yy][xx][2];
                
                if ((r == 0) && (g == 0) && (b == 0)) continue;

                // Calculate center position for this character pixel
                int centerX = (int)((float)xx * scale) + offset + (int)(scale / 2);
                int centerY = (int)((float)yy * scale) + offset + (int)(scale / 2);

                // Apply glow around this pixel
                applyGlowToCharacterPixel(surface, centerX, centerY, r, g, b, 
                                        glowRadius, glowAlpha);
            }
        }
    }

    // Second pass: Draw the actual character pixels
    for (int yy = 0; yy < CHARACTER_HEIGHT; yy++) {
        for (int xx = 0; xx < CHARACTER_WIDTH; xx++) {
            unsigned char r = grid[yy][xx][0];
            unsigned char g = grid[yy][xx][1];
            unsigned char b = grid[yy][xx][2];
            
            if ((r == 0) && (g == 0) && (b == 0)) continue;

            SDL_Rect dstChar = {
                (int)((float)xx * scale) + offset,
                (int)((float)yy * scale) + offset,
                (int)ceilf(scale),
                (int)ceilf(scale)
            };

            // Draw the actual pixel at full opacity
            Uint32 color = SDL_MapSurfaceRGBA(surface, r, g, b, 255);
            SDL_FillSurfaceRect(surface, &dstChar, color);
        }
    }

    return surface;
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                     float x, float y, int hash) {
    if(!view) return;

    CharaterSprite *cp = NULL;
    for (int i = 0; i < characterSpritesCount; i++) {
        if ((characterSprites[i].hash == hash) && (characterSprites[i].sprite)) {
            cp = &characterSprites[i];
            break;
        }
    }
    
    if (cp == NULL) {
        cp = &characterSprites[characterSpritesCount];
        cp->hash = hash;

        SDL_Surface* tempSurface = createCharacterSurface(grid, scale, glowEnabled && !isInMenu ? glowSize: 0,
                DEFAULT_GLOW_INTENSITY, glowEnabled && !isInMenu);

        if (tempSurface) {
            cp->sprite = tempSurface;
            
            if (cp->sprite) {
                characterSpritesCount++;
            }
        }
    }

    if(cp && cp->sprite) {
        SDL_Rect dst = {
            (int)((float)x * scale) - (glowEnabled && !isInMenu ? glowSize : 0),
            (int)((float)y * scale) - (glowEnabled && !isInMenu ? glowSize : 0),
            cp->sprite->w,
            cp->sprite->h
        };
        SDL_BlitSurface(cp->sprite, NULL, view, &dst);
    }
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b) {
    if(!view) return;

    //adjust for different behaviour between sdl and js in case of negative width / height
    if(w < 0.0f) {
        x += w;
        w *= -1.0f;
    }
    if(h < 0.0f) {
        y += h;
        h *= -1.0f;
    }

    SDL_Rect rect = {
        (int)(x * scale),
        (int)(y * scale),
        (int)ceilf(w * scale),
        (int)ceilf(h * scale)
    };

    // Apply glow first
    if(glowEnabled && !isInMenu && !isInGameOver)
        applyGlowToRect(view, rect, glowSize >> 1, DEFAULT_GLOW_INTENSITY, r, g, b);

    // Draw the main rectangle
    Uint32 color = SDL_MapSurfaceRGBA(view, r, g, b, 255);
    SDL_FillSurfaceRect(view, &rect, color);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    if(!view)
        return;
    
	//clear screen also in case we resize window
    md_clearScreen(clearColorR, clearColorG, clearColorB);
    
	Uint32 color = SDL_MapSurfaceRGBA(view, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_FillSurfaceRect(view, NULL, color);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    clearColorR = r;
    clearColorG = g;
    clearColorB = b;

    SDL_Rect dst = {0 , 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderClipRect(Renderer, &dst);
    SDL_SetRenderDrawColor(Renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_RenderClear(Renderer);
}

bool SDLCALL resizingEventWatcher(void *userdata, SDL_Event * event){
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
    SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
    if (win == (SDL_Window*)userdata) {
        md_initView(origViewW, origViewH);
        md_clearScreen(clearColorR, clearColorG, clearColorB);
    }
  }
  return 0;
}

static void cleanupView() {
    if (viewTexture) {
        SDL_DestroyTexture(viewTexture);
        viewTexture = NULL;
    }
    if (view) {
        SDL_DestroySurface(view);
        view = NULL;
    }
}

void md_initView(int w, int h) 
{
    if(!Renderer)
        return;

    SDL_GetWindowSizeInPixels(SdlWindow, &WINDOW_WIDTH , &WINDOW_HEIGHT);
    float wscalex = (float)WINDOW_WIDTH / (float)DEFAULT_WINDOW_WIDTH;
    float wscaley = (float)WINDOW_HEIGHT / (float)DEFAULT_WINDOW_HEIGHT;
    wscale = (wscaley < wscalex) ? wscaley : wscalex;

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
    realViewW = viewW;
    realViewH = viewH;
    
    float gScaleX =  (float)w / 100.0f ;
    float gScaleY =  (float)h / 100.0f ;
    float gScale;
    if (gScaleY > gScaleX)
        gScale = gScaleY;
    else
        gScale = gScaleX;
    glowSize = (float)DEFAULT_GLOW_SIZE / gScale * wscale ;

    mouseX = (realViewW >> 1);
    mouseY = (realViewH >> 1);

    if(!scaledDrawing)
    {
        viewW = w;
        viewH = h;
        glowSize = 2.0f;
        scale = 1.0f;
    }
    
    // logMessage("md_initView: window size: %dx%d requested view size: %dx%d adjusted view size %dx%d scale:%3f\n", WINDOW_WIDTH, WINDOW_HEIGHT, w, h, viewW, viewH, scale);

    // Cleanup existing resources
    cleanupView();
    
    // Create new surface
    view = SDL_CreateSurface(viewW, viewH, PIXELFORMAT);
    if (view) 
    {
        // Create texture from surface
        viewTexture = SDL_CreateTexture(Renderer, 
            PIXELFORMAT,
            SDL_TEXTUREACCESS_STREAMING,
            viewW, viewH);
        if(viewTexture)
            SDL_SetTextureScaleMode(viewTexture, SDL_SCALEMODE_NEAREST);
        if(crtEffect)
        {
            DestroyCRTEffect(crtEffect);
            crtEffect = NULL;
        }
        Game g = getGame(currentGameIndex);
        crtEffect = CreateCRTEffect(viewW, viewH, 0, 0, 6*wscale, 3*wscale, 10, 
            g.options.isDarkColor ? 40 : 128 , g.options.isDarkColor ? 40 : 128 , g.options.isDarkColor ? 40 : 128, g.options.isDarkColor ? 55 : 45);
    }
    resetCharacterSprite();
}


void md_consoleLog(char* msg) 
{ 
    logMessage(msg); 
}

static void update() {
    CInput_Update(GameInput);
    if(GameInput->Buttons.ButQuit)
        quit = 1;
    
    bool mouseUsed = getGame(currentGameIndex).usesMouse;
    setButtonState(!mouseUsed && (GameInput->Buttons.ButLeft || GameInput->Buttons.ButDpadLeft), 
        !mouseUsed && (GameInput->Buttons.ButRight || GameInput->Buttons.ButDpadRight),
        !mouseUsed && (GameInput->Buttons.ButUp || GameInput->Buttons.ButDpadUp),
        !mouseUsed && (GameInput->Buttons.ButDown || GameInput->Buttons.ButDpadDown), 
        GameInput->Buttons.ButB, GameInput->Buttons.ButA);
    
    if (mouseUsed)
    {
        if(GameInput->Buttons.ButRight)
            mouseX += WINDOW_WIDTH /100;
        
        if(GameInput->Buttons.ButLeft)
            mouseX -= WINDOW_WIDTH /100;
            
        if(GameInput->Buttons.ButUp)
            mouseY -= WINDOW_HEIGHT /100;
    
        if(GameInput->Buttons.ButDown)
            mouseY += WINDOW_HEIGHT /100;

        mouseX = clamp(mouseX, 0, WINDOW_WIDTH - 2*offsetX -1);
        mouseY = clamp(mouseY, 0, WINDOW_HEIGHT - 2*offsetY -1);

        if((prevRealMouseX != GameInput->Buttons.MouseX) || (prevRealMouseY != GameInput->Buttons.MouseY))
        {
            mouseX = ((GameInput->Buttons.MouseX - offsetX));
            mouseY = ((GameInput->Buttons.MouseY - offsetY));
            prevRealMouseX = GameInput->Buttons.MouseX;
            prevRealMouseY = GameInput->Buttons.MouseY;
        }

        setMousePos(mouseX / scale, mouseY / scale);
    }

    if ((!GameInput->PrevButtons.ButBack) && (GameInput->Buttons.ButBack))
    {
        if (!isInMenu && (startgame[0] == 0))
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

    if ((!GameInput->PrevButtons.ButX) && (GameInput->Buttons.ButX))
    {
        if(!isInMenu && scaledDrawing)
        {
            if (overlay == 0)
            {
                if(glowEnabled)
                {
                    glowEnabled = false;
                    resetCharacterSprite();
                }
                else
                {
                    overlay = 1;
                    glowEnabled = true;
                    resetCharacterSprite();
                }
            }
            else 
            {
                if(overlay == 1)
                {
                    if(glowEnabled)
                    {
                        glowEnabled = false;
                        resetCharacterSprite();
                    }
                    else
                    {
                        overlay = 2;
                        glowEnabled = false;
                        resetCharacterSprite();                    
                    }
                }
                else
                {
                    if (overlay == 2)
                    {
                        glowEnabled = true;
                        resetCharacterSprite();
                        overlay = 0;
                        
                    }
                }
            }
            //remember
            Game g = getGame(currentGameIndex);
            if((SDL_strlen(g.title) > 0) && (g.update != NULL))
            {         
                for (int i = 0; i < gameCount; i++)
                {
                    if (SDL_strcmp(g.title, gameOverLays[i].title) == 0 )
                    {
                        gameOverLays[i].overlay = overlay;
                        gameOverLays[i].glowEnabled = glowEnabled;
                    }
                }
            }
        }
    }

    updateFrame();

    // Draw a Little Cursor
    if (mouseUsed && !isInGameOver)
    {
        Uint32 col = SDL_MapRGB(view->format, 255, 105, 180);
        SDL_Rect dstHorz = {(int)(mouseX-3*wscale), (int)(mouseY-1*wscale), 7*wscale,3*wscale};
        SDL_FillRect(view, &dstHorz, col);
        SDL_Rect dstVert = {(int)(mouseX-1*wscale), (int)(mouseY-3*wscale), 3*wscale,7*wscale};
        SDL_FillRect(view, &dstVert, col);
    }

    if(scaledDrawing && !isInMenu && (overlay == 1))
    {
        SDL_Rect dst = { 0 };

        // Always ensure minimum 1 pixel
        float pixelSize = ceilf(1.0f * wscale);
        
         // Draw vertical lines
        for (float x = 0; x < viewW; x += pixelSize * 2.0f)
        {
            dst.x = (int)x;
            dst.y = 0;
            dst.w = (int)pixelSize;
            dst.h = viewH;
            SDL_FillSurfaceRect(view, &dst, SDL_MapSurfaceRGBA(view, 0,0,0,255));
        }

        // Draw horizontal lines
        for (float y = 0; y < viewH; y += pixelSize * 2.0f)
        {
            dst.x = 0;
            dst.y = (int)y;
            dst.w = viewW;
            dst.h = (int)pixelSize;
            SDL_FillSurfaceRect(view, &dst, SDL_MapSurfaceRGBA(view, 0,0,0,255));
        }
    }
    if(showfps)
    {
        char fpsText[10];
        sprintf(fpsText, "%.2f", avgfps);
        int prev = color;
        CharacterOptions prevCharOptions = characterOptions;
        characterOptions.isMirrorX = false;
        characterOptions.isMirrorY = false;
        characterOptions.rotation = 0;
        color = BLACK;
        rect(0,0,SDL_strlen(fpsText)*6, 6);
        color = WHITE;
        text(fpsText, 2, 3);
        characterOptions = prevCharOptions;
        color = prev;
    }
    // Update texture from surface
    if (view && viewTexture) {
        
        Uint64 currentCrtTime = SDL_GetTicks();
        float deltaTime = (currentCrtTime - lastCrtTime) / 1000.0f;
        lastCrtTime = currentCrtTime;
        if(scaledDrawing && !isInGameOver && (overlay == 2) && !isInMenu)
        {
            UpdateCRTEffect(crtEffect, deltaTime);
            RenderCRTEffect(view, crtEffect);
        }
        SDL_UpdateTexture(viewTexture, NULL, view->pixels, view->pitch);
        
        // Clear renderer
        SDL_SetRenderDrawColor(Renderer, clearColorR, clearColorG, clearColorB, 255);
        SDL_RenderClear(Renderer);
        
        // Draw the view texture
        SDL_FRect dst = {offsetX, offsetY, realViewW, realViewH};
        SDL_RenderTexture(Renderer, viewTexture, NULL, &dst);

        // Present the renderer
        SDL_RenderPresent(Renderer);
    }
    if ((!GameInput->PrevButtons.ButStart) && (GameInput->Buttons.ButStart))
    {
        if(!isInMenu)
        {
            Game g = getGame(currentGameIndex);
            if((SDL_strlen(g.title) > 0) && (g.update != NULL))
            {         
                for (int i = 0; i < gameCount; i++)
                {
                    if (SDL_strcmp(g.title, gameOverLays[i].title) == 0 )
                    {
                        gameOverLays[i].isDarkColor = !gameOverLays[i].isDarkColor;
                        restartGame(currentGameIndex);
                        break;
                    }
                }
            }
        }
    }
}

static void printHelp(char* exe)
{
    char* binaryName = SDL_strrchr(exe, '/');
    if (binaryName == NULL)
    {
        binaryName = SDL_strrchr(exe, '\\');
        if(binaryName == NULL)
            binaryName = exe;
    }
    if(binaryName)
        ++binaryName;

    printf("Crisp Game Lib Portable Sdl 3 Version\n");
    printf("Usage: %s [-w <WIDTH>] [-h <HEIGHT>] [-f] [-ns] [-a] [-fps] [-nd] [-g <GAMENAME>] [-ms] [-cgl] [-nsd] [CGL file]  \n", binaryName);
    printf("\n");
    printf("Commands:\n");
    printf("  -w <WIDTH>: use <WIDTH> as window width\n");
    printf("  -h <HEIGHT>: use <HEIGHT> as window height\n");
    printf("  -f: Run fullscreen\n");
    printf("  -ns: No Sound\n");
    printf("  -a: Use hardware accelerated rendering (default is software)\n");
    printf("  -fps: Show fps\n");
    printf("  -nd: no fps delay (run as fast as possible)\n");
    printf("  -list: List game names to be used with -g option\n");
    printf("  -g <GAMENAME>: run game <GAMENAME> only\n");
    printf("  -ms: Make screenshot of every game\n");
    printf("  -cgl: Generate .cgl files for all games\n");
    printf("  -nsd: No scaled drawing (scales the view, instead of drawing scaled, is faster but disables overlays, glow etc)\n");
    printf("  CGL file: Pass a .cgl file to launch a game directly\n");
}

void SDL_Cleanup()
{
    SDL_Event Event;
    while(SDL_PollEvent(&Event))
        SDL_Delay(1);
    SDL_Delay(250);
    
    SDL_Quit();
}

int main(int argc, char** argv)
{
//attach to potential console when using -mwindows so we can get output in a cmd / msys prompt 
//but see no console window when running from explorer start menu or so
#if defined _WIN32 || defined __CYGWIN__
    if(AttachConsole((DWORD)-1))
    {
        freopen("CON", "w", stderr);
        freopen("CON", "w", stdout);
    }
#endif
    bool fullScreen = false;
    bool useHWSurface = false;
    bool noAudioInit = false;
    bool makescreenshots = false;
    scaledDrawing = true;
    for (int i = 0; i < argc; i++)
    {
        //logMessage("param %d %s\n", i, argv[i]);
        char* ext = strrchr(argv[i], '.');
        if (ext != NULL)
        {
            if (SDL_strcmp(ext, ".cgl") == 0)
            {
                memset(startgame, 0, 100);
                char* gamestart = strrchr(argv[i], '/');
                if (gamestart == NULL)
                    gamestart = strrchr(argv[i], '\\');
                if (gamestart != NULL)
                {
                    gamestart++;
                    for (char* j = gamestart; j < ext; j++)
                        startgame[j - gamestart] = SDL_toupper(*j);
                }
            }
        }
                    
        if(SDL_strcmp(argv[i], "-cgl") == 0)
        {
            initGame();
            for(int i = 0; i < gameCount; i++)
            {
                Game g = getGame(i);
                if(SDL_strlen(g.title) > 0 && (g.update != NULL))
                {
                        
                    char filename[512];
                    sprintf(filename, "./%s.cgl", g.title);
                    FILE *f = fopen(filename, "w");
                    if(f)
                    {
                        fwrite(g.title, sizeof(char), SDL_strlen(g.title), f);
                        fclose(f);
                    }
                }
            }
            return 0;
        }

        if((SDL_strcmp(argv[i], "-?") == 0) || (SDL_strcmp(argv[i], "--?") == 0) || 
            (SDL_strcmp(argv[i], "/?") == 0) || (SDL_strcmp(argv[i], "-help") == 0) || (SDL_strcmp(argv[i], "--help") == 0))
        {
            printHelp(argv[0]);
            return 0;
        }

        if(SDL_strcmp(argv[i], "-f") == 0)
            fullScreen = true;
        
        if(SDL_strcmp(argv[i], "-a") == 0)
            useHWSurface = true;
        
        if(SDL_strcmp(argv[i], "-fps") == 0)
            showfps = true;
        
        if(SDL_strcmp(argv[i], "-ns") == 0)
			noAudioInit = true;

        if(SDL_strcmp(argv[i], "-nd") == 0)
			nodelay = true;

        if(SDL_strcmp(argv[i], "-w") == 0)
            if(i+1 < argc)
                WINDOW_WIDTH = SDL_atoi(argv[i+1]);
        
        if(SDL_strcmp(argv[i], "-h") == 0)
            if(i+1 < argc)
                WINDOW_HEIGHT = SDL_atoi(argv[i+1]);
        
        if(SDL_strcmp(argv[i], "-g") == 0)
            if(i+1 < argc)
            {
                memset(startgame, 0, 100);
                strcpy(startgame, argv[i+1]);
            }

        if(SDL_strcmp(argv[i], "-nsd") == 0)
            scaledDrawing = false;
        
        if(SDL_strcmp(argv[i], "-list") == 0)
        {
            initGame();
            quit = 1;
            int counter = 0;
            for (int i = 1; i < gameCount; i++)
            {
                if(getGame(i).update != NULL)
                {
                    counter++;
                    logMessage("%d. %s\n", counter, getGame(i).title);
                }
            }
            return 0;
        }
        
        if(SDL_strcmp(argv[i], "-ms") == 0)
            makescreenshots = true;
    }

    if (SDL_Init(SDL_INIT_VIDEO))
    {
        logMessage("SDL Succesfully initialized\n");
        atexit(SDL_Cleanup);
        Uint32 WindowFlags = SDL_WINDOW_RESIZABLE;
        
        SdlWindow = SDL_CreateWindow("Crisp Game Lib Portable Sdl 3", WINDOW_WIDTH, WINDOW_HEIGHT, WindowFlags);

        if (SdlWindow)
        {
            if (fullScreen)
            {
                SDL_SetWindowFullscreen(SdlWindow, true);
            }
            SDL_AddEventWatch(resizingEventWatcher, SdlWindow);
            logMessage("Succesfully Set %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
            if (useHWSurface == 0)
                Renderer = SDL_CreateRenderer(SdlWindow, SDL_SOFTWARE_RENDERER);
            else
                Renderer = SDL_CreateRenderer(SdlWindow, NULL);
            
            if (Renderer)
            {
            
                logMessage("Using Renderer:%s\n", SDL_GetRendererName(Renderer));
                char RenderDriverNames[1000];
                memset(RenderDriverNames, 0, 1000);
                for (int i = 0; i < SDL_GetNumRenderDrivers(); i++)
                {
                    if(i > 0)
                        strcat(RenderDriverNames, ",");
                    strcat(RenderDriverNames, SDL_GetRenderDriver(i));
                }
                logMessage("Available Renders: %s\n",RenderDriverNames);
                logMessage("Succesfully Created Buffer\n");
                initCharacterSprite();
                initGame();
                if(makescreenshots)
                {
                    quit = 1;
                    for (int i = 1; i < gameCount; i++)
                    {
                        if(getGame(i).update == NULL)
                            continue;
                        restartGame(i);
                        setButtonState(false,false,false,false,false,false);
                        updateFrame();
                        setButtonState(false,false,false,false,false,true);
                        updateFrame();
                        setButtonState(false,false,false,false,false,false);
                        for (int j = 0; j < 140; j++)
                            updateFrame();
                        char filename[512];
                        sprintf(filename, "./%s.bmp", getGame(i).title);
                        SDL_SaveBMP(view, filename);
                    }
                }
                if(!noAudioInit)
                {
                    if(!SDL_InitSubSystem(SDL_INIT_AUDIO))
                        logMessage("Failed to open audio: %s\n", SDL_GetError());
                    else
                    {
                        const char* audioName = SDL_GetCurrentAudioDriver();
                        if(audioName)
                            logMessage("Using Audio Driver:%s\n", audioName);
                        char AudioDriverNames[1000];
                        memset(AudioDriverNames, 0, 1000);
                        for (int i = 0; i < SDL_GetNumAudioDrivers(); i++)
                        {
                            audioName = SDL_GetAudioDriver(i);
                            if (audioName)
                            {
                                if(i > 0)
                                    strcat(AudioDriverNames, ",");
                                strcat(AudioDriverNames, audioName);
                            }
                        }
                        logMessage("Available Audio Drivers: %s\n", AudioDriverNames);
                        soundOn = InitAudio();
                        if(soundOn == 1)
                            logMessage("Succesfully opened audio\n");
                        else
                            logMessage("Failed to open audio:%s\n", SDL_GetError());
                    }
                }
                loadHighScores();
                loadGameOverlays();
	            if (startgame[0] != 0)
	            {
	                logMessage("Start Game: %s\n", startgame);
	                bool found = false;
	                for (int i = 0; i < gameCount; i++)
	                {
	                    if(SDL_strcmp(startgame, getGame(i).title) == 0)
	                    {
	                        found = true;
	                        restartGame(i);
	                        break;
	                    }
	                }
	                if(!found)
	                    memset(startgame, 0, 100);
	            }             
                SDL_GetMouseState(&prevRealMouseX, &prevRealMouseY);   
                GameInput = CInput_Create();
                int skip = 10;
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
                        if (!nodelay && (delay > 0.0f))
                            SDL_Delay((Uint32)(delay)); 
                    }
                    if (showfps)
                    {
                        if(skip > 0)
                        {
                            skip--;
                            lastfpstime = SDL_GetTicks();
                        }
                        else
                        {
                            framecount++;
                            if(SDL_GetTicks() - lastfpstime >= 1000)
                            {
                                for (int i = FPS_SAMPLES-1; i > 0; i--)
                                    fpsSamples[i] = fpsSamples[i-1];
                                fpsSamples[0] = framecount;
                                fpsAvgCount++;
                                if(fpsAvgCount > FPS_SAMPLES)
                                    fpsAvgCount = FPS_SAMPLES;
                                int fpsSum = 0;
                                for (int i = 0; i < fpsAvgCount; i++)
                                    fpsSum += fpsSamples[i];
                                avgfps = (float)fpsSum / (float)fpsAvgCount;
                                framecount = 0;
                                lastfpstime = SDL_GetTicks();
                            }
                        }
                    }
                }           
                CInput_Destroy(GameInput);
                resetCharacterSprite();
                if(crtEffect)
                    DestroyCRTEffect(crtEffect);
                SDL_DestroyRenderer(Renderer);
                saveHighScores();
                saveGameOverlays();
                if(soundOn)
                {
                    SDL_PauseAudioStreamDevice(audioStream);
                    SDL_CloseAudioDevice(audioDeviceId);
                    SDL_QuitSubSystem(SDL_INIT_AUDIO);
                }
            }
            else
            {
                logMessage("Failed to created Renderer!\n");
            }
            SDL_DestroyWindow(SdlWindow);
        }		
        else
        {
            logMessage("Failed to create SDL_Window %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
        }
    }
    else
    {
        logMessage("Couldn't initialise SDL!\n");
    }
    return 0;

}