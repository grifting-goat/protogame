#include "sound.h"

#include <string.h>

//gpt generated boiler plate idc to learn how to do this rn

static float clampf(float x, float lo, float hi) {
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

static void sound_cleanup_finished_streams(SoundSystem* s) {
	if (!s) {
		return;
	}

	for (uint32_t i = 0; i < s->active_stream_count;) {
		SDL_AudioStream* stream = s->active_streams[i];
		int queued = stream ? SDL_GetAudioStreamQueued(stream) : 0;

		if (!stream || queued <= 0) {
			if (stream) {
				SDL_UnbindAudioStream(stream);
				SDL_DestroyAudioStream(stream);
			}

			s->active_streams[i] = s->active_streams[s->active_stream_count - 1];
			s->active_streams[s->active_stream_count - 1] = NULL;
			s->active_stream_count--;
		} else {
			i++;
		}
	}
}

static void sound_recycle_active_stream(SoundSystem* s, uint32_t index) {
	if (!s || index >= s->active_stream_count) {
		return;
	}

	SDL_AudioStream* stream = s->active_streams[index];
	if (stream) {
		SDL_UnbindAudioStream(stream);
		SDL_DestroyAudioStream(stream);
	}

	s->active_streams[index] = s->active_streams[s->active_stream_count - 1];
	s->active_streams[s->active_stream_count - 1] = NULL;
	s->active_stream_count--;
}


bool sound_init(SoundSystem* s) {
	if (!s) {return false;}
	memset(s, 0, sizeof(*s));

	if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
		if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
			SDL_Log("sound_init: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
			return false;
		}
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.format = SDL_AUDIO_F32;
	want.channels = 2;
	want.freq = 48000;

	s->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want);
	if (s->device == 0) {
		SDL_Log("sound_init: SDL_OpenAudioDevice failed: %s", SDL_GetError());
		return false;
	}

	if (!SDL_GetAudioDeviceFormat(s->device, &s->device_spec, NULL)) {
		s->device_spec = want;
		SDL_Log("sound_init: SDL_GetAudioDeviceFormat failed, using requested spec: %s", SDL_GetError());
	}

	s->stream = SDL_CreateAudioStream(&s->device_spec, &s->device_spec);
	if (!s->stream) {
		SDL_Log("sound_init: SDL_CreateAudioStream failed: %s", SDL_GetError());
		SDL_CloseAudioDevice(s->device);
		s->device = 0;
		return false;
	}

	if (!SDL_BindAudioStream(s->device, s->stream)) {
		SDL_Log("sound_init: SDL_BindAudioStream failed: %s", SDL_GetError());
		SDL_DestroyAudioStream(s->stream);
		s->stream = NULL;
		SDL_CloseAudioDevice(s->device);
		s->device = 0;
		return false;
	}

	SDL_ResumeAudioDevice(s->device);

	s->initialized = true;
	return true;

}

void sound_shutdown(SoundSystem* s) {
	if (!s) {
		return;
	}

	for (uint32_t i = 0; i < s->clip_count; i++) {
		SDL_free(s->clips[i].data);
		s->clips[i].data = NULL;
		s->clips[i].length = 0;
		s->clips[i].name[0] = '\0';
	}
	s->clip_count = 0;

	if (s->stream) {
		SDL_DestroyAudioStream(s->stream);
		s->stream = NULL;
	}

	for (uint32_t i = 0; i < s->active_stream_count; i++) {
		if (s->active_streams[i]) {
			SDL_UnbindAudioStream(s->active_streams[i]);
			SDL_DestroyAudioStream(s->active_streams[i]);
			s->active_streams[i] = NULL;
		}
	}
	s->active_stream_count = 0;

	if (s->device != 0) {
		SDL_CloseAudioDevice(s->device);
		s->device = 0;
	}
	s->initialized = false;
}

int sound_load_wav(SoundSystem* s, const char* name, const char* path) {
	if (!s || !s->initialized || !name || !path) {return -1;}

	int existing = sound_find(s, name);

	if (existing >= 0) {return existing;}

	if (s->clip_count >= SOUND_MAX_CLIPS) {
        SDL_Log("sound_load_wav: clip limit reached (%u)", SOUND_MAX_CLIPS);
        return -1;
	}

	SDL_AudioSpec src_spec;
	SDL_zero(src_spec);
	Uint8* src_data = NULL;
	Uint32 src_len = 0;

	if (!SDL_LoadWAV(path, &src_spec, &src_data, &src_len)) {
		SDL_Log("sound_load_wav: SDL_LoadWAV failed for '%s': %s", path, SDL_GetError());
		return -1;
	}

	Uint8* conv_data = NULL;
	int conv_len = 0;
	if (!SDL_ConvertAudioSamples(&src_spec, src_data, (int)src_len, &s->device_spec, &conv_data, &conv_len)) {
		SDL_Log("sound_load_wav: SDL_ConvertAudioSamples failed for '%s': %s", path, SDL_GetError());
		SDL_free(src_data);
		return -1;
	}

	SDL_free(src_data);

	if (!conv_data || conv_len <= 0) {
		SDL_free(conv_data);
		SDL_Log("sound_load_wav: converted data empty for '%s'", path);
		return -1;
	}

	uint32_t idx = s->clip_count;
	SoundClip* clip = &s->clips[idx];
	SDL_strlcpy(clip->name, name, sizeof(clip->name));
	clip->data = conv_data;
	clip->length = (uint32_t)conv_len;
	
	s->clip_count++;
	return (int)idx;
}


int sound_find(const SoundSystem* s, const char* name) {
	if (!s || !name) {
		return -1;
	}

	for (uint32_t i = 0; i < s->clip_count; i++) {
		if (strcmp(s->clips[i].name, name) == 0) {
			return (int)i;
		}
	}
	return -1;

}

int sound_sync_loader(SoundSystem* s) {
	if (!s || !s->initialized) {
		return -1;
	}

	static const struct {
		const char* name;
		const char* path;
	} sync_sounds[] = {
		[SOUND_DEATH]      = {"death",      "wilhelmscream.wav"},
		[SOUND_OOF]        = {"oof",        "death2.wav"},
		[SOUND_BLUNDER]    = {"blunder",    "blunder.wav"},
		[SOUND_MUSKET]     = {"musket",     "musket.wav"},
		[SOUND_MACE]       = {"mace",       "swordslash.wav"},
		[SOUND_MACE_HIT]   = {"mace_hit",   "mace.wav"},
		[SOUND_MACE_SMASH] = {"mace_smash", "mace_smash.wav"},
		[SOUND_HURT]       = {"hurt",       "classic_hurt.wav"},
		[SOUND_DASH]       = {"dash",       "swordlunge.wav"},
		[SOUND_JOIN]       = {"join",       "button.wav"},
		[SOUND_CANT]       = {"cant",       "clickfast.wav"},
		[SOUND_KILL]       = {"kill",       "victory.wav"},
	};

	int loaded_count = 0;
	for (uint32_t i = 0; i < (uint32_t)(sizeof(sync_sounds) / sizeof(sync_sounds[0])); i++) {
		if (sound_load_wav(s, sync_sounds[i].name, sync_sounds[i].path) >= 0) {
			loaded_count++;
		}
	}

	return loaded_count;
}

void sound_play(const SoundSystem* s, int clip_index, float volume) {
	if (!s || !s->initialized || s->device == 0) {
		return;
	}

	SoundSystem* sys = (SoundSystem*)s;
	sound_cleanup_finished_streams(sys);

	if (clip_index < 0 || (uint32_t)clip_index >= s->clip_count) {
		return;
	}

	const SoundClip* clip = &s->clips[(uint32_t)clip_index];
	if (!clip->data || clip->length == 0) {
		return;
	}

	float gain = clampf(volume, 0.0f, 1.0f);
	if (gain <= 0.001f) {
		return;
	}

	if (sys->active_stream_count >= SOUND_MAX_ACTIVE_STREAMS) {
		sound_cleanup_finished_streams(sys);
	}
	if (sys->active_stream_count >= SOUND_MAX_ACTIVE_STREAMS) {
		// Under bursty SFX, recycle one currently active stream to keep playback responsive.
		sound_recycle_active_stream(sys, 0);
	}

	SDL_AudioStream* one_shot = SDL_CreateAudioStream(&s->device_spec, &s->device_spec);
	if (!one_shot) {
		SDL_Log("sound_play: SDL_CreateAudioStream failed: %s", SDL_GetError());
		return;
	}

	if (!SDL_BindAudioStream(s->device, one_shot)) {
		SDL_Log("sound_play: SDL_BindAudioStream failed: %s", SDL_GetError());
		SDL_DestroyAudioStream(one_shot);
		return;
	}

	if (!SDL_SetAudioStreamGain(one_shot, gain)) {
		SDL_Log("sound_play: SDL_SetAudioStreamGain failed: %s", SDL_GetError());
		SDL_UnbindAudioStream(one_shot);
		SDL_DestroyAudioStream(one_shot);
		return;
	}

	if (!SDL_PutAudioStreamData(one_shot, clip->data, (int)clip->length)) {
		SDL_Log("sound_play: SDL_PutAudioStreamData failed: %s", SDL_GetError());
		SDL_UnbindAudioStream(one_shot);
		SDL_DestroyAudioStream(one_shot);
		return;
	}

	if (!SDL_FlushAudioStream(one_shot)) {
		SDL_Log("sound_play: SDL_FlushAudioStream failed: %s", SDL_GetError());
	}

	sys->active_streams[sys->active_stream_count++] = one_shot;

}
void sound_play_id(const SoundSystem* s, SoundID id, float volume) {
	if (id < 0 || id >= SOUND_COUNT) { return; }
	sound_play(s, (int)id, volume);
}