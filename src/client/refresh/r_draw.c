/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Drawing of all images that are not textures
 *
 * =======================================================================
 */

#include "header/local.h"

image_t *draw_chars;
extern qboolean scrap_dirty;
extern unsigned r_rawpalette[256];
void Scrap_Upload(void);


void Draw_InitLocal(void) {
	/* don't bilerp characters and crosshairs */
	Cvar_Get("gl_nolerp_list", "pics/conchars.pcx pics/ch1.pcx pics/ch2.pcx pics/ch3.pcx", 0);

	/* load console characters */
	draw_chars = R_FindImage("pics/conchars.pcx", it_pic);
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void Draw_Char(int x, int y, int num){
	Draw_CharScaled(x, y, num, 1.0f);
}

void Draw_GLTexturedQuad(int x, int y, float width, float height, float tex_x1, float tex_y1, float tex_x2, float
                         tex_y2, int texnum){
	R_Bind(texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(tex_x1, tex_y1);
	glVertex2f(x, y);
	glTexCoord2f(tex_x2, tex_y1);
	glVertex2f(x + width, y);
	glTexCoord2f(tex_x2, tex_y2);
	glVertex2f(x + width, y + height);
	glTexCoord2f(tex_x1, tex_y2);
	glVertex2f(x, y + height);
	glEnd();
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void Draw_CharScaled(int x, int y, int num, float scale) {
	num &= 255;
	if ((num & 127) == 32) {
		return; /* space */
	}

	if (y <= -8) {
		return; /* totally off screen */
	}

	int row = num >> 4;
	int col = num & 15;
	float frow = row * 0.0625;
	float fcol = col * 0.0625;
	float size = 0.0625;
	float scaledSize = 8*scale;

    Draw_GLTexturedQuad(x, y, scaledSize, scaledSize, fcol, frow, fcol + size, frow + size, draw_chars->texnum);
}

image_t * Draw_FindPic(const char *name) {
	char fullname[MAX_QPATH];
	if ((name[0] != '/') && (name[0] != '\\')) {
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		return R_FindImage(fullname, it_pic);
	} else {
		return R_FindImage(name + 1, it_pic);
	}
}

void Draw_GetPicSize(int *w, int *h, char *pic) {
	image_t* gl = Draw_FindPic(pic);
	if (gl){
        *w = gl->width;
        *h = gl->height;
	}else{
		*w = *h = -1;
    }
}

void Draw_StretchPic(int x, int y, int w, int h, char *pic){
	image_t* gl = Draw_FindPic(pic);
	if (!gl){
		VID_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if(scrap_dirty){
		Scrap_Upload();
	}

    Draw_GLTexturedQuad(x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th, gl->texnum);
}


void Draw_Pic(int x, int y, char *pic) {
	Draw_PicScaled(x, y, pic, 1.0f);
}

void Draw_PicScaled(int x, int y, const char *pic, float factor) {
	image_t* gl = Draw_FindPic(pic);
	if (!gl) {
		VID_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty) {
		Scrap_Upload();
	}

	GLfloat w = gl->width*factor;
	GLfloat h = gl->height*factor;
    Draw_GLTexturedQuad(x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th, gl->texnum);
}


/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void Draw_TileClear(int x, int y, int w, int h, char *pic) {
	image_t* image = Draw_FindPic(pic);
	if (!image) {
		VID_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

    float tex_x1 = x / 64.0f;
    float tex_y1 = y / 64.0f;
    float tex_x2 = (x + w) / 64.0f;
    float tex_y2 = (y + h) / 64.0f;
    Draw_GLTexturedQuad(x, y, w, h, tex_x1, tex_y1, tex_x2, tex_y2, image->texnum);
}

void Draw_GLColoredQuad(int x1, int y1, int x2, int y2, float color[]){
	glDisable(GL_TEXTURE_2D);
	glColor4f(color[0], color[1], color[2], color[3]);

	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();

	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
}

/*
 * Fills a box of pixels with a single color
 */
void Draw_Fill(int x, int y, int w, int h, int c) {
	union{
		unsigned c;
		float v[4];
	} color;

	if ((unsigned)c > 255){
		VID_Error(ERR_FATAL, "Draw_Fill: bad color");
	}

    /* NOTE: somethings fishy with this union */
	color.c = d_8to24table[c];
    for(int i = 0; i < 3; i++){
        color.v[i] /= 255.0f;
    }
    color.v[3] = 1.0f;
    Draw_GLColoredQuad(x, y, x + w, y + h, color.v);
}

void Draw_FadeScreen(void) {
	glEnable(GL_BLEND);
    float color[] = {0, 0, 0, 0.8f};
    Draw_GLColoredQuad(0, 0, vid.width, vid.height, color);
	glDisable(GL_BLEND);
}

void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data) {
	unsigned image32[256 * 256];
	unsigned char image8[256 * 256];

	R_Bind(0);

    float hscale = rows <= 256 ? 1 : rows / 256.0f;
    float trows = rows <= 256 ? rows : 256;
	float t = rows * hscale / 256 - 1.0 / 512.0;

	if (!qglColorTableEXT) {
		for (int i = 0; i < trows; i++) {
			int row = (int)(i * hscale);
			if (row > rows) {
				break;
			}
			byte* source = data + cols * row;
			int fracstep = cols * 0x10000 / 256;
			int frac = fracstep >> 1;
            /* TODO: this step is different in these two similar statements */
            /* TODO: refactor and remove duplication */
			unsigned int* dest = &image32[i * 256];
			for (int j = 0; j < 256; j++) {
				dest[j] = r_rawpalette[source[frac >> 16]];
				frac += fracstep;
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	} else {
		for (int i = 0; i < trows; i++) {
			int row = (int)(i * hscale);
			if (row > rows) {
				break;
			}
			byte* source = data + cols * row;
			int fracstep = cols * 0x10000 / 256;
			int frac = fracstep >> 1;
			unsigned char* dest = &image8[i * 256];
			for (int j = 0; j < 256; j++) {
				dest[j] = source[frac >> 16];
				frac += fracstep;
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 256, 256, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, image8);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(1.0 / 512.0, 1.0 / 512.0);
	glVertex2f(x, y);
	glTexCoord2f(511.0 / 512.0, 1.0 / 512.0);
	glVertex2f(x + w, y);
	glTexCoord2f(511.0 / 512.0, t);
	glVertex2f(x + w, y + h);
	glTexCoord2f(1.0 / 512.0, t);
	glVertex2f(x, y + h);
	glEnd();
}

int Draw_GetPalette(void) {
	/* get the palette */
	byte *pic, *pal;
	int width, height;
	LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);

	if (!pal) {
		VID_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	}

	for (int i = 0; i < 256; i++) {
		int r = pal[i * 3 + 0];
		int g = pal[i * 3 + 1];
		int b = pal[i * 3 + 2];

		unsigned int v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff); /* 255 is transparent */

	free(pic);
	free(pal);
	return 0;
}

