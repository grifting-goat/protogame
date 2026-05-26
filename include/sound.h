#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#define SOUND_MAX_CLIPS 32
#define SOUND_MAX_NAME  32
#define SOUND_MAX_ACTIVE_STREAMS 64

typedef struct {
    char name[SOUND_MAX_NAME];
    Uint8* data;
    uint32_t length;
} SoundClip;

typedef struct {
    bool initialized;

    SDL_AudioDeviceID device;
    SDL_AudioSpec device_spec;
    SDL_AudioStream* stream;
    SDL_AudioStream* active_streams[SOUND_MAX_ACTIVE_STREAMS];
    uint32_t active_stream_count;

    SoundClip clips[SOUND_MAX_CLIPS];
    uint32_t clip_count;
} SoundSystem;

bool sound_init(SoundSystem* s);
void sound_shutdown(SoundSystem* s);

int  sound_load_wav(SoundSystem* s, const char* name, const char* path);
int  sound_find(const SoundSystem* s, const char* name);
int  sound_sync_loader(SoundSystem* s);

void sound_play(const SoundSystem* s, int clip_index, float volume);
void sound_play_name(const SoundSystem* s, const char* name, float volume);




#endif //SOUND_H