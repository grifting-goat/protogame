#include "overlay.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/glad.h>
#include <string.h>
#include "stb_image.h"

//mostly AI i need to rewrite this

static TTF_Font* g_font = NULL;

bool overlay_init(const char* font_path, int pt_size) {
	if (!TTF_Init()) return false;
	g_font = TTF_OpenFont(font_path, pt_size);
	return g_font != NULL;
}

void overlay_quit(void) {
	if (g_font) {
		TTF_CloseFont(g_font);
		g_font = NULL;
	}
	TTF_Quit();
}

bool overlay_create_text(OverlayText* overlay, const char* text, int x, int y) {
	if (!overlay || !g_font || !text) return false;
	overlay->color = (SDL_Color){255,255,255,255};
	SDL_Surface* surf = TTF_RenderText_Blended(g_font, text, 0, overlay->color);
	if (!surf) return false;

	SDL_Surface* rgba = SDL_CreateSurface(surf->w, surf->h, SDL_PIXELFORMAT_RGBA32);
	if (!rgba) {
		SDL_DestroySurface(surf);
		return false;
	}

	SDL_Rect area = {0, 0, surf->w, surf->h};
	if (!SDL_BlitSurface(surf, &area, rgba, &area)) {
		SDL_DestroySurface(rgba);
		SDL_DestroySurface(surf);
		return false;
	}

	overlay->width = rgba->w;
	overlay->height = rgba->h;

    overlay->x = x;
    overlay->y = y;

	if (overlay->tex_id) {
		glDeleteTextures(1, &overlay->tex_id);
		overlay->tex_id = 0;
	}

	glGenTextures(1, &overlay->tex_id);
	glBindTexture(GL_TEXTURE_2D, overlay->tex_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	SDL_DestroySurface(rgba);
	SDL_DestroySurface(surf);
	return true;
}

void overlay_render_text(const OverlayText* overlay, int window_w, int window_h) {
	if (!overlay || !overlay->tex_id) return;
	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, window_w, window_h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, overlay->tex_id);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2i(overlay->x, overlay->y);
		glTexCoord2f(1,0); glVertex2i(overlay->x+overlay->width, overlay->y);
		glTexCoord2f(1,1); glVertex2i(overlay->x+overlay->width, overlay->y+overlay->height);
		glTexCoord2f(0,1); glVertex2i(overlay->x, overlay->y+overlay->height);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void overlay_destroy_text(OverlayText* overlay) {
	if (!overlay || !overlay->tex_id) return;
	glDeleteTextures(1, &overlay->tex_id);
	overlay->tex_id = 0;
}

bool overlay_create_image(OverlayImage* overlay, const char* image_path, int x, int y) {
	if (!overlay || !image_path) return false;

	int width = 0;
	int height = 0;
	int channels = 0;

	stbi_set_flip_vertically_on_load(1);
	unsigned char* data = stbi_load(image_path, &width, &height, &channels, 0);
	if (!data) {
		return false;
	}

	GLenum format = GL_RGB;
	if (channels == 4) {
		format = GL_RGBA;
	} else if (channels == 1) {
		format = GL_RED;
	}

	overlay->width = width;
	overlay->height = height;
	overlay->x = x;
	overlay->y = y;

	if (overlay->tex_id) {
		glDeleteTextures(1, &overlay->tex_id);
		overlay->tex_id = 0;
	}

	glGenTextures(1, &overlay->tex_id);
	glBindTexture(GL_TEXTURE_2D, overlay->tex_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
	return true;
}

void overlay_render_image(const OverlayImage* overlay, int window_w, int window_h) {
	if (!overlay || !overlay->tex_id) return;

	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, window_w, window_h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, overlay->tex_id);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(overlay->x, overlay->y);
		glTexCoord2f(1, 0); glVertex2i(overlay->x + overlay->width, overlay->y);
		glTexCoord2f(1, 1); glVertex2i(overlay->x + overlay->width, overlay->y + overlay->height);
		glTexCoord2f(0, 1); glVertex2i(overlay->x, overlay->y + overlay->height);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void overlay_destroy_image(OverlayImage* overlay) {
	if (!overlay || !overlay->tex_id) return;
	glDeleteTextures(1, &overlay->tex_id);
	overlay->tex_id = 0;
}


