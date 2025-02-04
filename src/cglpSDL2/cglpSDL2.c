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

#define DEFAULT_GLOW_SIZE 6 * DEFAULT_WINDOW_WIDTH / 240
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
static SDL_Surface *view = NULL;
static SDL_Texture *viewTexture = NULL;

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

static GlowDistanceTable* distanceTable = NULL;

AudioState audio_state = {0};

static CharaterSprite characterSprites[MAX_CACHED_CHARACTER_PATTERN_COUNT];
static int characterSpritesCount;

static GlowDistanceTable* createDistanceTable(int glowSize) {
    GlowDistanceTable* table = (GlowDistanceTable*)SDL_malloc(sizeof(GlowDistanceTable));
    if (!table) return NULL;

    int size = glowSize * 2 + 1;
    table->size = size;
    table->distances = (Uint8*)SDL_malloc(size * size);
    
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
float buggySinf(float angle) 
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
    return sampleToTime(audio_state.time);
}

void applyGlowToRect(SDL_Surface* surface, SDL_Rect rect, int glowRadius, Uint8 glowAlpha,
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
void applyGlowToCharacterPixel(SDL_Surface* surface, int centerX, int centerY, 
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
SDL_Surface* createCharacterSurface(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
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
            cp->sprite = tempSurface;
            
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
    if(glowEnabled && !isInMenu)
        applyGlowToRect(view, rect, glowSize >> 1, DEFAULT_GLOW_INTENSITY, r, g, b);

    // Draw the main rectangle
    Uint32 color = SDL_MapRGBA(view->format, r, g, b, 255);
    SDL_FillRect(view, &rect, color);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    if(!view)
        return;
    
	//clear screen also in case we resize window
    md_clearScreen(clearColorR, clearColorG, clearColorB);
    
	Uint32 color = SDL_MapRGB(view->format, (Uint8)r, (Uint8)g, (Uint8)b);
    SDL_FillRect(view, NULL, color);
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
        SDL_FreeSurface(view);
        view = NULL;
    }
}

void md_initView(int w, int h) 
{
    SDL_GetRendererOutputSize(Renderer, &WINDOW_WIDTH , &WINDOW_HEIGHT);
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
    
    // Cleanup existing resources
    cleanupView();
    
    // Create new surface
    view = SDL_CreateRGBSurfaceWithFormat(0, viewW, viewH, 32, SDL_PIXELFORMAT_RGBA8888);
    if (view) {
        // Create texture from surface
        viewTexture = SDL_CreateTexture(Renderer, 
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      viewW, viewH);
    }
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

    // Update texture from surface
    if (view && viewTexture) {
        SDL_UpdateTexture(viewTexture, NULL, view->pixels, view->pitch);
        
        // Clear renderer
        SDL_SetRenderDrawColor(Renderer, clearColorR, clearColorG, clearColorB, 255);
        SDL_RenderClear(Renderer);
        
        // Draw the view texture
        SDL_Rect dst = {offsetX, offsetY, viewW, viewH};
        SDL_RenderCopy(Renderer, viewTexture, NULL, &dst);
        
        // Present the renderer
        SDL_RenderPresent(Renderer);
    }
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