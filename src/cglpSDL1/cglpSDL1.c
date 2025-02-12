#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL1.h"
#include <math.h>

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

#define DEFAULT_GLOW_SIZE 6 * DEFAULT_WINDOW_WIDTH / 240
#define DEFAULT_GLOW_INTENSITY 96
#define DEFAULT_OVERLAY 0
#define DEFAULT_GLOWENABLED false

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define MAX_NOTES 128
#define AMPLITUDE 10000
#define FADE_OUT_TIME 0.05f    // Fade-out time in seconds

#define FPS_SAMPLES 10

#define MAX(a,b) ((a) > (b) ? (a) : (b))

// Function to normalize angle to the range [0, 2pi]
#define NORMALIZE_ANGLE(angle) (angle = fmodf(angle, 2 * M_PI), (angle < 0) ? (angle += 2 * M_PI) : angle)

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static int quit = 0;
static int keys[SDLK_LAST];
static int prevKeys[SDLK_LAST];
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static int origViewW = DEFAULT_WINDOW_WIDTH;
static int origViewH = DEFAULT_WINDOW_HEIGHT;
static Uint32 frameticks = 0;
static Uint32 frameTime = 0;
static unsigned char clearColorR = 0;
static unsigned char clearColorG = 0;
static unsigned char clearColorB = 0;
static Uint32 clearColor = 0;
static float audioVolume = 1.00f;
static int offsetX = 0;
static int offsetY = 0;
static SDL_Surface *screen = NULL, *view = NULL;
static int soundOn = 0;
static int useBugSound = 1;
static int overlay = DEFAULT_OVERLAY;
static int glowSize = DEFAULT_GLOW_SIZE;
static float wscale = 1.0f;
static bool glowEnabled = DEFAULT_GLOWENABLED;
static Uint32 videoFlags = SDL_SWSURFACE;
static bool nodelay = false;
static SDL_AudioSpec audiospec = {0};
static int startgame = -1;

static int fpsSamples[FPS_SAMPLES];
static bool showfps = false;
static float avgfps = 0;
static int framecount = 0;
static int lastfpstime = 0;
static int fpsAvgCount = 0;

static Uint32 lastCrtTime = 0;

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

static void resetGame(Game *game)
{
    if((strlen(game->title) == 0) || (game->update == NULL) )
        return;

    int freeIndex = -1;
    for (int i = 0; i < gameCount; i++)
    {
        if((freeIndex == -1) && (strlen(gameOverLays[i].title) == 0))
            freeIndex = i;

        if ((strlen(gameOverLays[i].title) > 0) && (strcmp(game->title, gameOverLays[i].title) == 0 ))
        {
            overlay = gameOverLays[i].overlay;
            glowEnabled = gameOverLays[i].glowEnabled;
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

    // Create main surface directly in screen format
    effect->scanlineSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 
        screenWidth, screenHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);

    if (!effect->scanlineSurface) {
        SDL_free(effect);
        return NULL;
    }

    // Set surface alpha properties
    SDL_SetAlpha(effect->scanlineSurface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    // Clear to transparent
    SDL_FillRect(effect->scanlineSurface, NULL, SDL_MapRGBA(effect->scanlineSurface->format, 0, 0, 0, 0));

    // Draw the scanlines
    SDL_Rect lineRect = {0, 0, screenWidth, scanelineThickness};
    Uint32 lineColor = SDL_MapRGBA(effect->scanlineSurface->format, scanlineR, scanlineG, scanlineB, scanlineA);

    for (int y = 0; y < screenHeight; y += scanlineSpacing) {
        lineRect.y = y;
        SDL_FillRect(effect->scanlineSurface, &lineRect, lineColor);
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
        SDL_FreeSurface(effect->scanlineSurface);
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
            if(strlen(gameOverLays[i].title) > 0)
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
            if(strlen(hiScores[i].title) > 0)
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
        SDL_FreeSurface(characterSprites[i].sprite);
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
   int sample_count = len / sizeof(Sint16);
   // Intermediate float buffer to accumulate the summed waveforms
   float* float_buffer = (float*)malloc(sample_count * sizeof(float));
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
   free(float_buffer);
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
    return sampleToTime(audio_state.time);
}

static void applyGlowToRect(SDL_Surface* surface, SDL_Rect rect, int glowRadius, Uint8 glowAlpha,
                     Uint8 r, Uint8 g, Uint8 b) {
    if (!surface || glowRadius <= 0 || glowAlpha == 0) {
        return;
    }

    SDL_Surface* tempSurface = SDL_CreateRGBSurface(surface->flags,
        rect.w + (glowRadius * 2), rect.h + (glowRadius * 2),
        32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);

    if (!tempSurface) return;

    // Clear temp surface
    SDL_FillRect(tempSurface, NULL, SDL_MapRGBA(tempSurface->format, 0, 0, 0, 0));

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
                    pixels[y * tempSurface->w + x] = SDL_MapRGBA(tempSurface->format, r, g, b, currentAlpha);
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
                    pixels[y * tempSurface->w + x] = SDL_MapRGBA(tempSurface->format, r, g, b, currentAlpha);
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
                    pixels[y * tempSurface->w + x] = SDL_MapRGBA(tempSurface->format, r, g, b, currentAlpha);
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
                    pixels[y * tempSurface->w + x] = SDL_MapRGBA(tempSurface->format, r, g, b, currentAlpha);
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
    SDL_FreeSurface(tempSurface);
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
                SDL_GetRGBA(existing, surface->format, &er, &eg, &eb, &ea);

                // Only update if new alpha is higher
                if (layerAlpha > ea) {
                    pixels[idx] = SDL_MapRGBA(surface->format, r, g, b, layerAlpha);
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
    
    SDL_Surface* surface = SDL_CreateRGBSurface(SDL_SWSURFACE, fullWidth, fullHeight,
        32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    
    if (!surface) return NULL;

    // Clear surface
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
    
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
                (Sint16)((float)xx * scale) + offset,
                (Sint16)((float)yy * scale) + offset,
                (Uint16)ceilf(scale),
                (Uint16)ceilf(scale)
            };

            // Draw the actual pixel at full opacity
            Uint32 color = SDL_MapRGBA(surface->format, r, g, b, 255);
            SDL_FillRect(surface, &dstChar, color);
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
            cp->sprite = SDL_DisplayFormatAlpha(tempSurface);
            SDL_FreeSurface(tempSurface);
            
            if (cp->sprite) {
                characterSpritesCount++;
            }
        }
    }

    if(cp && cp->sprite) {
        SDL_Rect dst = {
            (Sint16)((float)x * scale) - (glowEnabled && !isInMenu ? glowSize : 0),
            (Sint16)((float)y * scale) - (glowEnabled && !isInMenu ? glowSize : 0),
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
        (Sint16)(x * scale),
        (Sint16)(y * scale),
        (Uint16)ceilf(w * scale),
        (Uint16)ceilf(h * scale)
    };

    // Apply glow first
    if(glowEnabled && !isInMenu && !isInGameOver)
        applyGlowToRect(view, rect, glowSize >> 1, DEFAULT_GLOW_INTENSITY, r, g, b);

    // Draw the main rectangle
    Uint32 color = SDL_MapRGBA(view->format, r, g, b, 255);
    SDL_FillRect(view, &rect, color);
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
    clearColorR = r;
    clearColorG = g;
    clearColorB = b;
    clearColor = SDL_MapRGB(screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(screen, NULL, clearColor);
}

void md_initView(int w, int h) 
{
    if(!screen)
        return;
    
    WINDOW_WIDTH = screen->w;
    WINDOW_HEIGHT = screen->h;
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

    float gScaleX =  (float)w / 100.0f ;
    float gScaleY =  (float)h / 100.0f ;
    float gScale;
    if (gScaleY > gScaleX)
        gScale = gScaleY;
    else
        gScale = gScaleX;
    glowSize = (float)DEFAULT_GLOW_SIZE / gScale * wscale ;

    if(view)
    {
        SDL_FreeSurface(view);
        view = NULL;
    }

    SDL_Surface* tmp = SDL_CreateRGBSurface(screen->flags, viewW, viewH, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    if(tmp)
    {
        view = SDL_DisplayFormatAlpha(tmp);
        SDL_FreeSurface(tmp);

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
    printf(msg); 
}

static void update() 
{
    for(int i = 0; i < SDLK_LAST; i++)
        prevKeys[i] = keys[i];

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if (event.type == SDL_VIDEORESIZE)
        {
            if(screen)
                SDL_FreeSurface(screen);
            screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, videoFlags);
            md_initView(origViewW, origViewH);
            md_clearScreen(clearColorR, clearColorG, clearColorB);
        }

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

    int tmpX, tmpY;
    Uint8 butState = SDL_GetMouseState(&tmpX, &tmpY);

    setButtonState(keys[BUTTON_LEFT] == 1, keys[BUTTON_RIGHT] == 1, keys[BUTTON_UP] == 1,
        keys[BUTTON_DOWN] == 1, (keys[BUTTON_B] == 1) || (butState & SDL_BUTTON(3)), (keys[BUTTON_A] == 1) || (butState & SDL_BUTTON(1)));
    
    
    float mouseX = ((tmpX - offsetX) / scale);
    float mouseY = ((tmpY - offsetY) / scale);
    setMousePos(mouseX, mouseY);

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
        if (!isInMenu && (startgame == -1))
            goToMenu();
        else
            quit = 1;
    }

    if ((prevKeys[BUTTON_SOUNDSWITCH] == 0) && (keys[BUTTON_SOUNDSWITCH] == 1))
    {
        if(useBugSound == 1)
            useBugSound = 0;
        else
            useBugSound = 1;
    }

    if ((prevKeys[BUTTON_GLOWSWITCH] == 0) && (keys[BUTTON_GLOWSWITCH] == 1))
    {
        if(!isInMenu)
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
            if((strlen(g.title) > 0) && (g.update != NULL))
            {         
                for (int i = 0; i < gameCount; i++)
                {
                    if (strcmp(g.title, gameOverLays[i].title) == 0 )
                    {
                        gameOverLays[i].overlay = overlay;
                        gameOverLays[i].glowEnabled = glowEnabled;
                    }
                }
            }
        }
    }
    
    updateFrame();  
    if(!isInMenu && (overlay == 1))
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
            SDL_FillRect(view, &dst, SDL_MapRGB(view->format, 0,0,0));
        }

        // Draw horizontal lines
        for (float y = 0; y < viewH; y += pixelSize * 2.0f)
        {
            dst.x = 0;
            dst.y = (int)y;
            dst.w = viewW;
            dst.h = (int)pixelSize;
            SDL_FillRect(view, &dst, SDL_MapRGB(view->format, 0,0,0));
        }
    }
    if(showfps)
    {
        char fpsText[10];
        sprintf(fpsText, "%.2f", avgfps);
        int prev = color;
        color = BLACK;
        rect(0,0,strlen(fpsText)*6, 6);
        color = WHITE;
        text(fpsText, 2, 3);
        color = prev;
    }

    Uint32 currentCrtTime = SDL_GetTicks();
    float deltaTime = (currentCrtTime - lastCrtTime) / 1000.0f;
    lastCrtTime = currentCrtTime;
    if(!isInGameOver && (overlay == 2) && !isInMenu)
    {
        UpdateCRTEffect(crtEffect, deltaTime);
        RenderCRTEffect(view, crtEffect);
    }

    SDL_Rect src = {0, 0, viewW, viewH};
    SDL_Rect dst = {offsetX, offsetY, viewW, viewH};
    SDL_BlitSurface(view, &src, screen, &dst);
    SDL_Flip(screen);

    if ((prevKeys[BUTTON_DARKSWITCH] == 0) && (keys[BUTTON_DARKSWITCH] == 1))
    {
        if(!isInMenu)
        {
            Game g = getGame(currentGameIndex);
            if((strlen(g.title) > 0) && (g.update != NULL))
            {         
                for (int i = 0; i < gameCount; i++)
                {
                    if (strcmp(g.title, gameOverLays[i].title) == 0 )
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

    printf("Crisp Game Lib Portable Sdl2 Version\n");
    printf("Usage: %s <command1> <command2> ...\n", binaryName);
    printf("\n");
    printf("Commands:\n");
    printf("  -w <WIDTH>: use <WIDTH> as window width\n");
    printf("  -h <HEIGHT>: use <HEIGHT> as window height\n");
    printf("  -f: Run fullscreen\n");
    printf("  -ns: No Sound\n");
    printf("  -a: Use hardware (accelerated) surfaces\n");
    printf("  -fps: Show fps\n");
    printf("  -nd: no fps delay (run as fast as possible)\n");
    printf("  -list: List game names to be used with -g option\n");
    printf("  -g <GAMENAME>: run game <GAMENAME> only\n");
    printf("  -ms: Make screenshot of every game\n");
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO) == 0)
	{
        printf("SDL Succesfully initialized\n");
		bool fullScreen = false;
        bool useHWSurface = false;
        bool noAudioInit = false;
        bool makescreenshots = false;
		for (int i=0; i < argc; i++)
		{
        	if((strcmp(argv[i], "-?") == 0) || (strcmp(argv[i], "--?") == 0) || 
            	(strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "-help") == 0) || (strcmp(argv[i], "--help") == 0))
        	{
                printHelp(argv[0]);
                return 0;
            }

            if(strcmp(argv[i], "-f") == 0)
                fullScreen = true;
            
            if(strcmp(argv[i], "-a") == 0)
                useHWSurface = true;
            
            if(strcmp(argv[i], "-fps") == 0)
                showfps = true;
            
            if(strcmp(argv[i], "-ns") == 0)
                noAudioInit = true;

            if(strcmp(argv[i], "-nd") == 0)
                nodelay = true;

            if(strcmp(argv[i], "-w") == 0)
                if(i+1 < argc)
                    WINDOW_WIDTH = atoi(argv[i+1]);
            
            if(strcmp(argv[i], "-h") == 0)
                if(i+1 < argc)
                    WINDOW_HEIGHT = atoi(argv[i+1]);
            
            if(strcmp(argv[i], "-g") == 0)
                if(i+1 < argc)
                    startgame = i+1;
            
            if(strcmp(argv[i], "-list") == 0)
            {
                initGame();
                quit = 1;
                int counter = 0;
                for (int i = 1; i < gameCount; i++)
                {
                    if(getGame(i).update != NULL)
                    {
                        counter++;
                        printf("%d. %s\n", counter, getGame(i).title);
                    }
                }
                return 0;
            }
            
            if(strcmp(argv[i], "-ms") == 0)
                makescreenshots = true;
        }

		videoFlags = SDL_SWSURFACE;
        if(useHWSurface)
            videoFlags = SDL_HWSURFACE;

		if(fullScreen)
			videoFlags |= SDL_FULLSCREEN;
        else
            videoFlags |= SDL_RESIZABLE;
        
        //needed for scanline effect
        videoFlags |= SDL_SRCALPHA;

        screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT, 0, videoFlags);
		if(screen)
		{
			SDL_WM_SetCaption( "Crisp Game Lib Portable Sdl1", NULL);
			printf("Succesfully Set %dx%d\n",WINDOW_WIDTH, WINDOW_HEIGHT);
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
                if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
                    printf("Failed to open audio: %s\n", SDL_GetError());
                else
                {
                    soundOn = InitAudio();
                    if(soundOn == 1)
                        printf("Succesfully opened audio\n");
                    else
                        printf("Failed to open audio\n");
                }
            } 
            if (startgame > 1)
            {
                int tmp = startgame;
                startgame = -1;
                for (int i = 0; i < gameCount; i++)
                {
                    if(strcmp(argv[tmp], getGame(i).title) == 0)
                    {
                        startgame = i;
                        restartGame(i);
                        break;
                    }
                }
            }
            loadHighScores();
            loadGameOverlays();
            int skip = 10;
            while(quit == 0)
            {
                frameticks = SDL_GetTicks();
                update();                
                if(quit == 0)
                {
                    frameTime = SDL_GetTicks() - frameticks;
                    double delay = 1000.0f / FPS - frameTime;
                    if (!nodelay && (delay > 0.0f))
                        SDL_Delay((Uint32)(delay)); 
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
            } 
        
            resetCharacterSprite();
            if(view)
                SDL_FreeSurface(view);
            if(screen)
			    SDL_FreeSurface(screen);
            saveHighScores();
            saveGameOverlays();
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