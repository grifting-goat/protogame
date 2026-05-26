#ifndef OVERLAY_H
#define OVERLAY_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
	TTF_Font* font;
	SDL_Color color;
	unsigned int tex_id;
	int width;
	int height;
    int x;
	int y;
} OverlayText;

typedef struct {
    unsigned int tex_id;
    int width;
    int height;
    int x;
    int y;
	bool show;
} OverlayImage;


bool overlay_init(const char* font_path, int pt_size);

void overlay_quit(void);

bool overlay_create_text(OverlayText* overlay, const char* text, int x, int y);

void overlay_render_text(const OverlayText* overlay, int window_w, int window_h);

void overlay_destroy_text(OverlayText* overlay);

bool overlay_create_image(OverlayImage* overlay, const char* image_path, int x, int y);

void overlay_render_image(const OverlayImage* overlay, int window_w, int window_h);

void overlay_destroy_image(OverlayImage* overlay);

#endif // OVERLAY_H