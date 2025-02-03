#include <SDL.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglpSDL2.h"

#ifdef USE_UINT64_TIMER
    typedef Uint64 TimerType;
    #define PHASE_MAX (1ULL << 32)
    #define FREQ_SCALE 65536.0f
#else
    typedef Uint32 TimerType;
    #define PHASE_MAX (1UL << 24)
    #define FREQ_SCALE 512.0f
#endif

#include "CInput.h"

//#define DEBUG_MONITORING

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define MAX_NOTES 128
#define AMPLITUDE 10000
#define FADE_OUT_TIME 0.05f    // Fade-out time in seconds

#define DEFAULT_GLOW_SIZE 5
#define DEFAULT_GLOW_INTENSITY 96
#define DEFAULT_OVERLAY 0
#define DEFAULT_GLOW_ENABLED false

// Function to normalize angle to the range [0, 2π)
#define NORMALIZE_ANGLE(angle) (angle = fmodf(angle, 2 * M_PI), (angle < 0) ? (angle += 2 * M_PI) : angle)


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
static int overlay = DEFAULT_OVERLAY;
static int glowSize = DEFAULT_GLOW_SIZE;
static float wscale = 1.0f;
static bool glowEnabled = DEFAULT_GLOW_ENABLED;

typedef struct {
    float frequency; // Frequency of the note in Hz
    float when;      // Time in seconds to start playing the note
    float duration;  // Duration in seconds to play the note
    bool active;     // Whether this note is currently active
} Note;

typedef struct {
    Note notes[MAX_NOTES]; // List of scheduled notes
    int note_count;        // Current number of notes
    TimerType time;           // Current playback time (in samples)
} AudioState;

// Pre-calculated distance lookup table
typedef struct {
    Uint8* distances;  // Lookup table for distances
    int size;         // Size of the table (glowSize * 2 + 1)
} GlowDistanceTable;

// Extended sprite structure to handle multiple scales
typedef struct {
    SDL_Texture *sprite;
    float scaleKey;      // The scale this sprite was created for
    int hash;
    int w;
    int h;
} ScaledSprite;

typedef struct {
    ScaledSprite* sprites;  // Array of sprites at different scales
    int spriteCount;        // Number of sprites for this character
    int maxSprites;         // Maximum sprites allowed per character
} CharacterSprite;

#ifdef DEBUG_MONITORING
typedef struct {
    Uint64 totalDrawCalls;
    Uint64 cacheHits;
    Uint64 cacheMisses;
    Uint64 totalRenderTime;
    Uint64 totalGlowTime;
    size_t peakMemoryUsage;
    int activeScaledSprites;
    float currentScale;
    int currentGlowSize;
} CharacterRenderStats;
static CharacterRenderStats renderStats = {0};
static SDL_atomic_t atomicMemoryUsage = {0};

static void formatSize(size_t bytes, char* buffer, size_t bufferSize) {
    if (bytes < 1024) {
        SDL_snprintf(buffer, bufferSize, "%llu B", (unsigned long long)bytes);
    } else if (bytes < 1024 * 1024) {
        SDL_snprintf(buffer, bufferSize, "%.2f KB", bytes / 1024.0f);
    } else {
        SDL_snprintf(buffer, bufferSize, "%.2f MB", bytes / (1024.0f * 1024.0f));
    }
}

static void updateMemoryUsage(size_t delta, bool increase) {
    int64_t currentValue;
    if (increase) {
        currentValue = SDL_AtomicAdd(&atomicMemoryUsage, delta);
        size_t newUsage = (size_t)currentValue;
        if (newUsage > renderStats.peakMemoryUsage) {
            renderStats.peakMemoryUsage = newUsage;
            char sizeStr[32];
            formatSize(renderStats.peakMemoryUsage, sizeStr, sizeof(sizeStr));
            //SDL_Log("New peak memory usage: %s", sizeStr);
        }
    } else {
        SDL_AtomicAdd(&atomicMemoryUsage, -(int64_t)delta);
    }
}
#endif

// Memory tracking functions with additional logging
static void* trackedMalloc(size_t size) {
    void* ptr = SDL_malloc(size);
    if (ptr) {
#ifdef DEBUG_MONITORING
        updateMemoryUsage(size, true);
        char sizeStr[32];
        formatSize(size, sizeStr, sizeof(sizeStr));
        //SDL_Log("Memory allocated: %s at %p", sizeStr, ptr);
#endif
    }
    return ptr;
}

static void trackedFree(void* ptr, size_t size) {
    if (ptr) {
#ifdef DEBUG_MONITORING
        updateMemoryUsage(size, false);
        char sizeStr[32];
        formatSize(size, sizeStr, sizeof(sizeStr));
        //SDL_Log("Memory freed: %s at %p", sizeStr, ptr);
#endif
        SDL_free(ptr);
    }
}

#ifdef DEBUG_MONITORING

static void logRenderStats() {
    static Uint64 lastLogTime = 0;
    Uint64 currentTime = SDL_GetTicks64();
    
    // Log every 5 seconds
    if (currentTime - lastLogTime > 5000) {
        float hitRate = renderStats.totalDrawCalls > 0 ? 
            (float)renderStats.cacheHits / renderStats.totalDrawCalls * 100.0f : 0.0f;
        float avgRenderTime = renderStats.totalDrawCalls > 0 ? 
            (float)renderStats.totalRenderTime / renderStats.totalDrawCalls : 0.0f;
        
        int64_t currentMemory = SDL_AtomicGet(&atomicMemoryUsage);
        char currentSizeStr[32], peakSizeStr[32];
        formatSize((size_t)currentMemory, currentSizeStr, sizeof(currentSizeStr));
        formatSize(renderStats.peakMemoryUsage, peakSizeStr, sizeof(peakSizeStr));
        
        SDL_Log("\n=== Character Render Statistics ===\n");
        SDL_Log("Cache Hit Rate: %.2f%% (%llu hits, %llu misses)",
                hitRate, renderStats.cacheHits, renderStats.cacheMisses);
        SDL_Log("Average Render Time: %.3f ms", avgRenderTime);
        SDL_Log("Active Scaled Sprites: %d", renderStats.activeScaledSprites);
        SDL_Log("Current Scale: %.2f", renderStats.currentScale);
        SDL_Log("Current Glow Size: %d", renderStats.currentGlowSize);
        SDL_Log("Current Memory Usage: %s", currentSizeStr);
        SDL_Log("Peak Memory Usage: %s\n", peakSizeStr);
        
        lastLogTime = currentTime;
    }
}
#endif

AudioState audio_state = {0};

static CharacterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;
static GlowDistanceTable* distanceTable = NULL;

static GlowDistanceTable* createDistanceTable(int glowSize) {
    GlowDistanceTable* table = (GlowDistanceTable*)trackedMalloc(sizeof(GlowDistanceTable));
    if (!table) return NULL;

    int size = glowSize * 2 + 1;
    table->size = size;
    table->distances = (Uint8*)trackedMalloc(size * size);
    
    if (!table->distances) {
        trackedFree(table, sizeof(GlowDistanceTable));
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

static SDL_Surface* createCharacterSurface(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                                         float scale, int glowRadius, Uint8 glowAlpha,
                                         bool withGlow) {
#ifdef DEBUG_MONITORING
    Uint64 startTime = SDL_GetPerformanceCounter();
#endif
    // Update distance table if needed
    if (withGlow && (!distanceTable || distanceTable->size != (glowRadius * 2 + 1))) {
        if (distanceTable) {
            SDL_free(distanceTable->distances);
            SDL_free(distanceTable);
        }
        distanceTable = createDistanceTable(glowRadius);
    }

    int baseWidth = (int)ceilf((float)CHARACTER_WIDTH * scale);
    int baseHeight = (int)ceilf((float)CHARACTER_HEIGHT * scale);
    int fullWidth = withGlow ? baseWidth + (glowRadius * 2) : baseWidth;
    int fullHeight = withGlow ? baseHeight + (glowRadius * 2) : baseHeight;

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 
        fullWidth, fullHeight, 32, SDL_PIXELFORMAT_RGBA8888);

    if (!surface) return NULL;

    // Clear surface
    SDL_memset(surface->pixels, 0, surface->h * surface->pitch);
    
    int offset = withGlow ? glowRadius : 0;

    // Draw base character using direct memory access for better performance
    SDL_LockSurface(surface);
    Uint32* pixels = (Uint32*)surface->pixels;
    
    for (int yy = 0; yy < CHARACTER_HEIGHT; yy++) {
        int scaledY = (int)((float)yy * scale) + offset;
        for (int xx = 0; xx < CHARACTER_WIDTH; xx++) {
            unsigned char r = grid[yy][xx][0];
            unsigned char g = grid[yy][xx][1];
            unsigned char b = grid[yy][xx][2];
            
            if ((r == 0) && (g == 0) && (b == 0)) continue;
            
            int scaledX = (int)((float)xx * scale) + offset;
            int scaleSize = (int)ceilf(scale);
            
            // Optimized pixel filling for the character
            Uint32 color = SDL_MapRGBA(surface->format, r, g, b, 255);
            for (int sy = 0; sy < scaleSize; sy++) {
                for (int sx = 0; sx < scaleSize; sx++) {
                    int px = scaledX + sx;
                    int py = scaledY + sy;
                    if (px >= 0 && px < fullWidth && py >= 0 && py < fullHeight) {
                        pixels[py * surface->w + px] = color;
                    }
                }
            }
        }
    }

    if (withGlow && distanceTable) {
#ifdef DEBUG_MONITORING
        Uint64 glowStartTime = SDL_GetPerformanceCounter();
#endif
        Uint32* tempPixels = (Uint32*)trackedMalloc(surface->h * surface->pitch);
        if (tempPixels) {
            SDL_memcpy(tempPixels, pixels, surface->h * surface->pitch);

            // Apply glow using the distance lookup table
            int tableSize = distanceTable->size;
            int halfTable = tableSize / 2;

            for (int y = 0; y < baseHeight; y++) {
                for (int x = 0; x < baseWidth; x++) {
                    int srcIdx = (y + offset) * surface->w + (x + offset);
                    Uint32 pixel = tempPixels[srcIdx];
                    Uint8 r, g, b, a;
                    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

                    if (a == 0) continue;

                    // Apply glow using pre-calculated distances
                    for (int gy = 0; gy < tableSize; gy++) {
                        int destY = y + gy - halfTable + offset;
                        if (destY < 0 || destY >= fullHeight) continue;

                        for (int gx = 0; gx < tableSize; gx++) {
                            int destX = x + gx - halfTable + offset;
                            if (destX < 0 || destX >= fullWidth) continue;

                            Uint8 distance = distanceTable->distances[gy * tableSize + gx];
                            if (distance > 0) {
                                Uint8 glowA = (distance * glowAlpha) >> 8;
                                int destIdx = destY * surface->w + destX;
                                
                                Uint32 destPixel = pixels[destIdx];
                                Uint8 existingR, existingG, existingB, existingA;
                                SDL_GetRGBA(destPixel, surface->format, 
                                          &existingR, &existingG, &existingB, &existingA);

                                if (glowA > existingA) {
                                    pixels[destIdx] = SDL_MapRGBA(surface->format, r, g, b, glowA);
                                }
                            }
                        }
                    }
                }
            }
            trackedFree(tempPixels, surface->h * surface->pitch);
        }
#ifdef DEBUG_MONITORING
        Uint64 glowEndTime = SDL_GetPerformanceCounter();
        renderStats.totalGlowTime += (glowEndTime - glowStartTime) * 1000 / SDL_GetPerformanceFrequency();
#endif
    }

    SDL_UnlockSurface(surface);
#ifdef DEBUG_MONITORING
    Uint64 endTime = SDL_GetPerformanceCounter();
    renderStats.totalRenderTime += (endTime - startTime) * 1000 / SDL_GetPerformanceFrequency();
#endif
    return surface;
}

// Add cleanup for monitoring
void cleanupMonitoring() {
#ifdef DEBUG_MONITORING
    char peakSizeStr[32];
    formatSize(renderStats.peakMemoryUsage, peakSizeStr, sizeof(peakSizeStr));
    
    SDL_Log("\n=== Final Render Statistics ===\n");
    SDL_Log("Total Draw Calls: %llu", renderStats.totalDrawCalls);
    SDL_Log("Final Cache Hit Rate: %.2f%%", 
            (float)renderStats.cacheHits / renderStats.totalDrawCalls * 100.0f);
    SDL_Log("Peak Memory Usage: %s", peakSizeStr);
    SDL_Log("Average Render Time: %.3f ms\n", 
            (float)renderStats.totalRenderTime / renderStats.totalDrawCalls);
#endif
}

// Add initialization for monitoring
static void initMonitoring() {
#ifdef DEBUG_MONITORING
    SDL_AtomicSet(&atomicMemoryUsage, 0);
    renderStats = (CharacterRenderStats){0};
    SDL_Log("Monitoring initialized");
#endif
}

static void initCharacterSprite() {
    for (int i = 0; i < MAX_CACHED_CHARACTER_PATTERN_COUNT; i++) {
        characterSprites[i].sprites = NULL;
        characterSprites[i].spriteCount = 0;
        characterSprites[i].maxSprites = 3;  // Cache up to 3 scales per character
    }
    characterSpritesCount = 0;
    distanceTable = NULL;
}

static void resetCharacterSprite() {
    for (int i = 0; i < characterSpritesCount; i++) {
        for (int j = 0; j < characterSprites[i].spriteCount; j++) {
            if (characterSprites[i].sprites[j].sprite) {
                SDL_DestroyTexture(characterSprites[i].sprites[j].sprite);
            }
        }
        if (characterSprites[i].sprites) {
            trackedFree(characterSprites[i].sprites, 
                sizeof(ScaledSprite) * characterSprites[i].maxSprites);
            characterSprites[i].sprites = NULL;
        }
    }
    characterSpritesCount = 0;
    if (distanceTable) {
        if (distanceTable->distances) {
            trackedFree(distanceTable->distances, 
                distanceTable->size * distanceTable->size);
        }
        trackedFree(distanceTable, sizeof(GlowDistanceTable));
        distanceTable = NULL;
    }
}

// Simulate buggy sinf: restricts output to 0, 1, -1 based on 90° increments
float buggySinf(float angle) 
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
   float float_buffer[sample_count];
   memset(float_buffer, 0, sizeof(float_buffer));
   
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
    return sampleToTime(audio_state.time) ;
}


void applyGlowToRect(SDL_Renderer* renderer, SDL_Rect rect, int glowRadius, Uint8 glowAlpha, Uint8 r, Uint8 g, Uint8 b) {
    // Validate inputs
    if (!renderer || glowRadius <= 0 || glowAlpha == 0) {
        return;
    }

    SDL_Rect prevClipRect;
    SDL_RenderGetClipRect(Renderer, & prevClipRect);
    // Store the original draw color and blend mode
    Uint8 originalR, originalG, originalB, originalA;
    SDL_GetRenderDrawColor(renderer, &originalR, &originalG, &originalB, &originalA);
    SDL_BlendMode originalBlendMode;
    SDL_GetRenderDrawBlendMode(renderer, &originalBlendMode);

    // Enable alpha blending
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Draw only the outer borders, no interior filling
    for (int layer = 1; layer <= glowRadius; layer++) {
        // Calculate alpha with a non-linear fade (quadratic falloff)
        float alphaFactor = 1.0f - powf((float)layer / glowRadius, 2.0f);
        Uint8 currentAlpha = (Uint8)(alphaFactor * glowAlpha);

        // Skip if alpha is too low to be visible
        if (currentAlpha <= 0) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer, r, g, b, currentAlpha);

         // Set clip rectangle to control drawing
        SDL_Rect outerClipRect = {
            rect.x - layer, 
            rect.y - layer, 
            rect.w + (layer * 2), 
            rect.h + (layer * 2)
        };
        SDL_RenderSetClipRect(Renderer, &outerClipRect);

        // Top outer border
        SDL_Rect topRect = {
            rect.x - layer, 
            rect.y - layer, 
            rect.w + (layer * 2), 
            1
        };
        SDL_RenderFillRect(Renderer, &topRect);

        // Bottom outer border
        SDL_Rect bottomRect = {
            rect.x - layer, 
            rect.y + rect.h + layer - 1, 
            rect.w + (layer * 2), 
            1
        };
        SDL_RenderFillRect(Renderer, &bottomRect);

        // Left outer border
        SDL_Rect leftRect = {
            rect.x - layer, 
            rect.y - layer, 
            1, 
            rect.h + (layer * 2)
        };
        SDL_RenderFillRect(Renderer, &leftRect);

        // Right outer border
        SDL_Rect rightRect = {
            rect.x + rect.w + layer - 1, 
            rect.y - layer, 
            1, 
            rect.h + (layer * 2)
        };
        SDL_RenderFillRect(Renderer,  &rightRect);
    }

    // Restore the original draw color and blend mode
    SDL_SetRenderDrawColor(renderer, originalR, originalG, originalB, originalA);
    SDL_SetRenderDrawBlendMode(renderer, originalBlendMode);

    SDL_RenderSetClipRect(Renderer, &prevClipRect);
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                     float x, float y, int hash) {
#ifdef DEBUG_MONITORING
    renderStats.totalDrawCalls++;
    renderStats.currentScale = scale;
    renderStats.currentGlowSize = glowSize;
#endif
    CharacterSprite *cp = NULL;
    ScaledSprite *sp = NULL;
    
    // Find existing character entry
    for (int i = 0; i < characterSpritesCount; i++) {
        if (characterSprites[i].sprites && characterSprites[i].sprites[0].hash == hash) {
            cp = &characterSprites[i];
            // Find closest scale
            float bestScaleDiff = FLT_MAX;
            for (int j = 0; j < cp->spriteCount; j++) {
                float scaleDiff = fabsf(cp->sprites[j].scaleKey - scale);
                if (scaleDiff < bestScaleDiff) {
                    bestScaleDiff = scaleDiff;
                    sp = &cp->sprites[j];
                }
            }
            // If scale difference is too large, don't use cached version
            if (bestScaleDiff > 0.1f) {
                sp = NULL;
            }
            break;
        }
    }

#ifdef DEBUG_MONITORING
    if (sp) {
        renderStats.cacheHits++;
    } else {
        renderStats.cacheMisses++;
    }
#endif
    // Create new character entry if needed
    if (!cp) {
#ifdef DEBUG_MONITORING
        renderStats.activeScaledSprites++;
#endif               
        if (characterSpritesCount >= MAX_CACHED_CHARACTER_PATTERN_COUNT) {
            return;  // Cache full
        }
        cp = &characterSprites[characterSpritesCount++];
        cp->sprites = (ScaledSprite*)trackedMalloc(sizeof(ScaledSprite) * cp->maxSprites);
        if (!cp->sprites) return;
        cp->spriteCount = 0;
        
        // Add new scaled sprite
        if (cp->spriteCount < cp->maxSprites) {
            SDL_Surface *surface = createCharacterSurface(grid, scale, glowEnabled && !isInMenu ? glowSize : 0, 
                DEFAULT_GLOW_INTENSITY, glowEnabled && !isInMenu);

            if (surface) {
                sp = &cp->sprites[cp->spriteCount++];
                sp->sprite = SDL_CreateTextureFromSurface(Renderer, surface);
                sp->scaleKey = scale;
                sp->hash = hash;
                sp->w = surface->w;
                sp->h = surface->h;
                SDL_FreeSurface(surface);
            }
        }
    }

    if (sp && sp->sprite) {
        SDL_Rect dst = {offsetX, offsetY, viewW, viewH};
        SDL_RenderSetClipRect(Renderer, &dst);

        SDL_Rect dst2 = {
            (int)((float)(offsetX + x*scale)) - (glowEnabled && !isInMenu ? glowSize : 0), 
            (int)((float)(offsetY + y*scale)) - (glowEnabled && !isInMenu ? glowSize : 0), 
            sp->w, 
            sp->h
        };

        SDL_RenderCopy(Renderer, sp->sprite, NULL, &dst2);
    }
#ifdef DEBUG_MONITORING
    logRenderStats();
#endif    
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
    if(glowEnabled && !isInMenu)
        applyGlowToRect(Renderer, dst2, glowSize >> 1, DEFAULT_GLOW_INTENSITY, (Uint8)r, (Uint8)g, (Uint8)b);
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
        float wscalex = (float)WINDOW_WIDTH / (float)DEFAULT_WINDOW_WIDTH;
        float wscaley = (float)WINDOW_HEIGHT / (float)DEFAULT_WINDOW_HEIGHT;
        wscale = (wscaley < wscalex) ? wscaley : wscalex;
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
    float gScaleX =  (float)w / 100.0f ;
    float gScaleY =  (float)h / 100.0f ;
    float gScale;
    if (gScaleY > gScaleX)
        gScale = gScaleY;
    else
        gScale = gScaleX;
    glowSize = (float)DEFAULT_GLOW_SIZE / gScale * wscale ;
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

    if ((!GameInput->PrevButtons.ButX) && (GameInput->Buttons.ButX))
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
            if(glowEnabled)
            {
                glowEnabled = false;
                resetCharacterSprite();
            }
            else
            {
                overlay = 0;
                glowEnabled = true;
                resetCharacterSprite();
            }
        }
    }



	updateFrame();
    if(!isInMenu && (overlay == 1))
    {
        SDL_Rect dst;
        SDL_Rect clipRect = {offsetX, offsetY, viewW, viewH};
        SDL_RenderSetClipRect(Renderer, &clipRect);
              
        // Always ensure minimum 1 pixel
        float pixelSize = ceilf(1.0f * wscale);
        
        // Draw vertical lines
        for (float x = 0; x < viewW; x += 2.0f * pixelSize)
        {
            dst.x = offsetX + x;
            dst.y = offsetY;
            dst.w = pixelSize;
            dst.h = viewH;
            SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(Renderer, &dst);
        }

         // Draw vertical lines
        for (float x = 0; x < viewW; x += pixelSize * 2.0f)
        {
            dst.x = offsetX + (int)x;
            dst.y = offsetY;
            dst.w = (int)pixelSize;
            dst.h = viewH;
            SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(Renderer, &dst);
        }

        // Draw horizontal lines
        for (float y = 0; y < viewH; y += pixelSize * 2.0f)
        {
            dst.x = offsetX;
            dst.y = offsetY + (int)y;
            dst.w = viewW;
            dst.h = (int)pixelSize;
            SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(Renderer, &dst);
        }
    }
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
        initMonitoring();
        float wscalex = (float)WINDOW_WIDTH / (float)DEFAULT_WINDOW_WIDTH;
        float wscaley = (float)WINDOW_HEIGHT / (float)DEFAULT_WINDOW_HEIGHT;
        wscale = (wscaley < wscalex) ? wscaley : wscalex;
        glowSize = (float)DEFAULT_GLOW_SIZE * wscale;

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
        cleanupMonitoring();
        SDL_Quit();
    }
    else
    {
        SDL_Log("Couldn't initialise SDL!\n");
    }
    return 0;

}