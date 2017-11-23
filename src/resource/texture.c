/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <png.h>

#include "texture.h"
#include "resource.h"
#include "global.h"
#include "vbo.h"

char* texture_path(const char *name) {
	return strjoin(TEX_PATH_PREFIX, name, TEX_EXTENSION, NULL);
}

bool check_texture_path(const char *path) {
	return strendswith(path, TEX_EXTENSION);
}

typedef struct ImageData {
	int width;
	int height;
	int depth;
	Uint32 *pixels;
} ImageData;

static ImageData* load_png(const char *filename);

void* load_texture_begin(const char *path, unsigned int flags) {
	return load_png(path);
}

void* load_texture_end(void *opaque, const char *path, unsigned int flags) {
	SDL_Surface *surface;
	ImageData *img = opaque;
	Uint32 rmask, gmask, bmask, amask;

	if(!img) {
		return NULL;
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	surface = SDL_CreateRGBSurfaceFrom(img->pixels, img->width, img->height, img->depth * 4, 0, rmask, gmask, bmask, amask);

	if(!surface) {
		log_warn("SDL_CreateRGBSurfaceFrom(): failed: %s", SDL_GetError());
		free(img);
		return NULL;
	}

	Texture *texture = malloc(sizeof(Texture));

	load_sdl_surf(surface, texture);
	free(surface->pixels);
	SDL_FreeSurface(surface);
	free(img);

	return texture;
}

Texture* get_tex(const char *name) {
	return get_resource(RES_TEXTURE, name, RESF_DEFAULT | RESF_UNSAFE)->texture;
}

Texture* prefix_get_tex(const char *name, const char *prefix) {
	char *full = strjoin(prefix, name, NULL);
	Texture *tex = get_tex(full);
	free(full);
	return tex;
}

static ImageData* load_png_p(const char *filename, SDL_RWops *rwops) {
#define PNGFAIL(msg) { log_warn("Couldn't load '%s': %s", filename, msg); return NULL; }
	png_structp png_ptr;
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		PNGFAIL("png_create_read_struct() failed")
	}

	png_setup_error_handlers(png_ptr);

	png_infop info_ptr;
	if(!(info_ptr = png_create_info_struct(png_ptr))) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		PNGFAIL("png_create_info_struct() failed")
	}

	png_infop end_info;
	if(!(end_info = png_create_info_struct(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		PNGFAIL("png_create_info_struct() failed")
	}

	png_init_rwops_read(png_ptr, rwops);
	png_read_info(png_ptr, info_ptr);

	int colortype = png_get_color_type(png_ptr,info_ptr);

	if(colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	if(!(colortype & PNG_COLOR_MASK_ALPHA))
		png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	png_read_update_info(png_ptr, info_ptr);

	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	int depth = png_get_bit_depth(png_ptr, info_ptr);

	png_bytep row_pointers[height];

	Uint32 *pixels = malloc(sizeof(Uint32)*width*height);

	for(int i = 0; i < height; i++)
		row_pointers[i] = (png_bytep)(pixels+i*width);

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, end_info);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	ImageData *img = malloc(sizeof(ImageData));
	img->width = width;
	img->height = height;
	img->depth = depth;
	img->pixels = pixels;

	return img;
#undef PNGFAIL
}

static ImageData* load_png(const char *filename) {
	SDL_RWops *rwops = vfs_open(filename, VFS_MODE_READ);

	if(!rwops) {
		log_warn("VFS error: %s", vfs_get_error());
		return NULL;
	}

	ImageData *img = load_png_p(filename, rwops);

	SDL_RWclose(rwops);
	return img;
}

void load_sdl_surf(SDL_Surface *surface, Texture *texture) {
	glGenTextures(1, &texture->gltex);
	glBindTexture(GL_TEXTURE_2D, texture->gltex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int nw = 2;
	int nh = 2;

	while(nw < surface->w) nw *= 2;
	while(nh < surface->h) nh *= 2;

	Uint32 *tex = calloc(sizeof(Uint32), nw*nh);

	int x, y;

	for(y = 0; y < nh; y++) {
		for(x = 0; x < nw; x++) {
			if(y < surface->h && x < surface->w)
				tex[y*nw+x] = ((Uint32*)surface->pixels)[y*surface->w+x];
			else
				tex[y*nw+x] = '\0';
		}
	}

	texture->w = surface->w;
	texture->h = surface->h;

	texture->truew = nw;
	texture->trueh = nh;


	glTexImage2D(GL_TEXTURE_2D, 0, 4, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);

	free(tex);
}

void free_texture(Texture *tex) {
	glDeleteTextures(1, &tex->gltex);
	free(tex);
}

void draw_texture(float x, float y, const char *name) {
	draw_texture_p(x, y, get_tex(name));
}

void draw_texture_p(float x, float y, Texture *tex) {
	glBindTexture(GL_TEXTURE_2D, tex->gltex);

	glPushMatrix();

	float wq = ((float)tex->w)/tex->truew;
	float hq = ((float)tex->h)/tex->trueh;

	if(x || y)
		glTranslatef(x,y,0);
	if(tex->w != 1 || tex->h != 1)
		glScalef(tex->w,tex->h,1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	if(wq != 1 || hq != 1)
		glScalef(wq, hq, 1);
	glMatrixMode(GL_MODELVIEW);

	draw_quad();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();
}

void draw_texture_with_size_p(float x, float y, float w, float h, Texture *tex) {
	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(w/tex->w, h/tex->h, 1);
	draw_texture_p(0, 0, tex);
	glPopMatrix();
}

void draw_texture_with_size(float x, float y, float w, float h, const char *name) {
	draw_texture_with_size_p(x, y, w, h, get_tex(name));
}

void fill_screen(float xoff, float yoff, float ratio, const char *name) {
	fill_screen_p(xoff, yoff, ratio, 1, get_tex(name));
}

void fill_screen_p(float xoff, float yoff, float ratio, float aspect, Texture *tex) {
	glBindTexture(GL_TEXTURE_2D, tex->gltex);

	float rw = ratio;
	float rh = ratio;

	if(ratio == 0) {
		rw = ((float)tex->w)/tex->truew;
		rh = ((float)tex->h)/tex->trueh;
	}
	rw *= aspect;

	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(xoff,yoff, 0);
		glScalef(rw, rh, 1);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glTranslatef(VIEWPORT_W*0.5, VIEWPORT_H*0.5, 0);
	glScalef(VIEWPORT_W, VIEWPORT_H, 1);

	draw_quad();

	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

// draws a thin, w-width rectangle from point A to point B with a texture that
// moving along the line.
// As with fill_screen, the texture’s dimensions must be powers of two for the
// loop to be gapless.
//
void loop_tex_line_p(complex a, complex b, float w, float t, Texture *texture) {
	complex d = b-a;
	complex c = (b+a)/2;
	glPushMatrix();
	glTranslatef(creal(c),cimag(c),0);
	glRotatef(180/M_PI*carg(d),0,0,1);
	glScalef(cabs(d),w,1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(t, 0, 0);
	glMatrixMode(GL_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D, texture->gltex);

	draw_quad();

	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();
}

void loop_tex_line(complex a, complex b, float w, float t, const char *texture) {
	loop_tex_line_p(a, b, w, t, get_tex(texture));
}
