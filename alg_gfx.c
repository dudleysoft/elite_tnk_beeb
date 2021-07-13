/**
 *
 * Elite - The New Kind.
 *
 * Allegro version of Graphics routines.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 * Routines for drawing anti-aliased lines and circles by T.Harte.
 *
 **/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

// #include "allegro.h"
#include "beebScreen/beebScreen.h"
#include "bitmap.h"
#include "font.h"

#include "config.h"
#include "gfx.h"
#include "alg_data.h"
#include "elite.h"
#include "keyboard.h"
#include "swis.h"

typedef Bitmap BITMAP;
typedef Font FONT;

typedef union {
	void *dat;
	BITMAP *bmp;
	FONT *font;
} DATAFILE;

#define load_bitmap LoadBMP

unsigned char *gfx_screen;
volatile int frame_count;
DATAFILE *datafile;
BITMAP *scanner_image;

typedef int *PALETTE;

#define MAX_POLYS	100

static int start_poly;
static int total_polys;

#define SCREEN_W 256
#define SCREEN_H 256

struct poly_data
{
	int z;
	int no_points;
	int face_colour;
	int point_list[16];
	int next;
};

static struct poly_data poly_chain[MAX_POLYS];

void blit(BITMAP *bmp,unsigned char *buffer,int sx,int sy,int dx,int dy,int w,int h)
{
	Rect src={sx,sy,w,h};
	Rect dest={dx,dy,0,0};
	BlitBMP(bmp,&src,buffer,&dest);
}

void line(unsigned char *buffer, int x1,int y1,int x2,int y2,int col)
{
	int dx = (x2-x1)<<8;
	int dy = (y2-y1)<<8;
	int len = abs(dx)>abs(dy) ? abs(dx)>>8 : abs(dy)>>8;
	
	// Early out for 0 length lines (just plot the last pixel)
	if (len == 0)
	{
		gfx_plot_pixel(x1,y1,col);
		return;
	}

	dx /= len;
	dy /= len;
	int x = x1<<8;
	int y = y1<<8;
	int l = clipLeftX << 8;
	int r = clipRightX << 8;
	int t = clipTopY << 8;
	int b = clipBottomY << 8;

	for(int i=0;i<len;++i)
	{
		if (x>=l && x<r && y>=t && y<b)
		{
			int off = ((y>>8)*SCREEN_W + (x>>8));
			buffer[off] = col;
			buffer[off^1] = col;
		}
		x+=dx;
		y+=dy;
	}
}

void line_xor(unsigned char *buffer, int x1,int y1,int x2,int y2,int col)
{
	int dx = (x2-x1)<<8;
	int dy = (y2-y1)<<8;
	int len = dx>dy ? dx>>8 : dy>>8;
	
	// Early out for 0 length lines (just plot the last pixel)
	if (len == 0)
	{
		// gfx_fast_plot_pixel(x1,y1,col);
		return;
	}

	dx /= len;
	dy /= len;
	int x = x1<<8;
	int y = y1<<8;
	int l = clipLeftX << 8;
	int r = clipRightX << 8;
	int t = clipTopY << 8;
	int b = clipBottomY << 8;

	for(int i=0;i<len;++i)
	{
		if (x>=l && x<r && y>=t && y<b)
		{
			int off = ((y>>8)*SCREEN_W + (x>>8));
			buffer[off] ^= col;
			//buffer[off^1] ^= col;
		}
		x+=dx;
		y+=dy;
	}
}

void fill_line(unsigned char *buffer,int l,int r,int y,int col)
{
	if (y<clipTopY || y>clipBottomY)
	{
		return;
	}
	if (l<clipLeftX)
	{
		l=clipLeftX;
	}
	if (r>clipRightX)
	{
		r=clipRightX;
	}
	unsigned char *ptr=&buffer[y*SCREEN_W+l];
	for(int x=l;x<=r;++x)
	{
		*ptr++=col;
	}
}

void triangle(unsigned char *buffer,int x1,int y1,int x2,int y2,int x3,int y3,int col)
{
	// Sort so that we always go y1<y2<y3
	if (y3<y1)
	{
		int tx=x1;
		int ty=y1;
		x1=x3;
		y1=y3;
		x3=tx;
		y3=ty;
	}
	if (y2<y1)
	{
		int tx=x1;
		int ty=y1;
		x1=x2;
		y1=y2;
		x2=tx;
		y2=ty;
	}
	if (y3<y2)
	{
		int tx=x2;
		int ty=y2;
		x2=x3;
		y2=y3;
		x3=tx;
		y3=ty;
	}

	int l,r;

	// Flat line
	if (y1==y3)
	{
		l=x1<x2? x1:x2;
		l=l<x3 ? l:x3;
		r=x1>x2? x1:x2;
		r=r>x3 ? r:x3;
		line(buffer,l,y1,r,y2,col);
		return;
	}

	int y=y1;

	// Find the lowest point
	int b=y2>y3? y2:y3;

	int h=b-y;
	int dl,dr;

	// Flat top?
	if (y2==y1)
	{
		// Left is X1 (even if it's not the left)
		l=x1<<8;
		dl=((x3-x1)<<8) / (y3-y1);
		r=x2<<8;
		dr=((x3-x2)<<8) / (y3-y1);
	}
	else
	{
		l=r=x1<<8;
		dl=((x2-x1)<<8) / (y2-y1);
		dr=((x3-x1)<<8) / (y3-y1);
	}

	while(y<=b)
	{
		if (l<r)
		{
			fill_line(buffer,l>>8,r>>8,y,col);
		}
		else
		{
			fill_line(buffer,r>>8,l>>8,y,col);
		}
		l+=dl;
		r+=dr;
		y++;
		if (y==y2)
		{
			dl=((x3<<8)-l) / (y3-y);
		}
	}
}

void textout(unsigned char *buffer,void *font,char *txt, int tx,int ty,int col)
{
	FONT_Render((Font*)font, tx , ty , txt, 0,col);
}

void textout_centre(unsigned char *buffer,void *font,char *txt, int tx,int ty,int col)
{
	FONT_Render((Font*)font, tx , ty , txt, FONT_HCENTER,col);
}


void frame_timer (void)
{
	frame_count++;
}

enum DataKind {
	KindNone,
	KindBmp,
	KindMidi,
	KindFont
};

char *dataFiles[]={
		"data/blake.bmp",
		"data/danube.mid",
		"data/ecm.bmp",
		"data/verd2.bmp",
		"data/verd4.bmp",
		"data/elitetx3.bmp",
		NULL,
		"data/greendot.bmp",
		"data/missgrn.bmp",
		"data/missred.bmp",
		"data/missyell.bmp",
		"data/reddot.bmp",
		"data/safe.bmp"
};

enum DataKind dataKind[] ={
	KindBmp,KindMidi,KindBmp,KindFont,KindFont,
	KindBmp,KindNone,KindBmp,KindBmp,
	KindBmp,KindBmp,KindBmp,KindBmp
};

DATAFILE *load_datafile(char *unused)
{
	DATAFILE *data=malloc(sizeof(DATAFILE) * 13);
	for(int i=0;i<13;i++)
	{
		switch(dataKind[i])
		{
		case KindBmp:
			printf("File: %s\n",dataFiles[i]);
			data[i].bmp=LoadBMP(dataFiles[i], NULL);
			SetKey(data[i].bmp,0,0,0);
			break;
		case KindMidi:
			data[i].dat = NULL;
			break;
		case KindFont:
			printf("File: %s\n",dataFiles[i]);
			data[i].font=FONT_Load(dataFiles[i], 0);
			break;
		default:
			data[i].dat = NULL;
		}
	}

	return data;
}

void unload_datafile(DATAFILE *file)
{
	for(int i=0;i<13;i++)
	{
		if (file[i].dat)
		{
			free(file[i].dat);
		}
	}
}

int the_palette[256];
unsigned char palMap[256];
int nula = FALSE;

int gfx_graphics_startup (void)
{
	int rv;

	printf("Loading data.\n");
	datafile = load_datafile("elite.dat");
	if (!datafile)
	{
		beebScreen_Quit();
      	printf("Error loading %s!\n", "elite.dat");
      	return 1;
	}

	printf("Loading Scanner.\n");
	scanner_image = load_bitmap(scanner_filename, the_palette);
	if (!scanner_image)
	{
		beebScreen_Quit();
		printf("Error reading scanner bitmap file.\n");
		return 1;
	}

	if (nula)
	{
		beebScreen_Init(2,BS_INIT_NULA);
	}
	else
	{
		beebScreen_Init(2, 0);
	}
	beebScreen_SetGeometry(128,256, TRUE);

	beebScreen_SetDefaultNulaRemapColours();

	if (nula)
	{
		beebScreen_CreatePalMap(the_palette,256,palMap);
	}
	else
	{
		beebScreen_SendPal(the_palette,256);
	}

	/* Create the screen buffer bitmap */
	gfx_screen = malloc (SCREEN_W * SCREEN_H);

	beebScreen_SetBuffer(gfx_screen, BS_BUFFER_FORMAT_8BPP, SCREEN_W, SCREEN_H);

	// beebScreen_ClearScreens(TRUE);

	// Draw scanner and border
	blit (scanner_image, gfx_screen, 0, 0, 0, 192, scanner_image->w, scanner_image->h);
	line (gfx_screen, 0, 0, 0, 192, GFX_COL_WHITE);
	line (gfx_screen, 0, 0, 255, 0, GFX_COL_WHITE);
	line (gfx_screen, 255, 0, 255, 192, GFX_COL_WHITE);

	/* Install a timer to regulate the speed of the game... */
	
	return 0;
}


void gfx_graphics_shutdown (void)
{
	FreeBMP(scanner_image);
	free(gfx_screen);
	unload_datafile(datafile);

	beebScreen_Quit();
	_VDU(22);
	_VDU(7);
	printf("Goodbye!\n");
}


/*
 * Blit the back buffer to the screen.
 */

void gfx_update_screen (void)
{
	int nulaPal[16];

	if (nula)
	{
		beebScreen_CreateDynamicPalette(the_palette,palMap,256,nulaPal,16);
	}
	
	beebScreen_Flip();
	if (nula)
	{
		beebScreen_SendPal(nulaPal,16);
	}
}


void gfx_acquire_screen (void)
{
	// acquire_bitmap (gfx_screen);
}


void gfx_release_screen (void)
{
	// release_bitmap(gfx_screen);
}

void gfx_fast_plot_pixel (int x, int y, int col)
{
	int off = ((y*SCREEN_W) + x);
	gfx_screen[off] = col;
	gfx_screen[off^1] = col;
}

void gfx_plot_pixel (int x, int y, int col)
{
	if (x>clipLeftX && x<clipRightX && y>clipTopY && y<clipBottomY)
	{
		int off = ((y*SCREEN_W) + x);
		gfx_screen[off] = col;
		gfx_screen[off^1] = col;
	}
}

// void gfx_draw_filled_circle (int cx, int cy, int radius, int circle_colour)
// {
// 	// TODO - Circle Fill
// 	gfx_draw_circle(cx,cy,radius,circle_colour);
// }


#define AA_BITS 3
#define AA_AND  7
#define AA_BASE 235

#define trunc(x) ((x) & ~65535)
#define frac(x) ((x) & 65535)
#define invfrac(x) (65535-frac(x))
#define plot(x,y,c) gfx_plot_pixel((x), (y), (c)+AA_BASE)

/*
 * Draw anti-aliased wireframe circle.
 * By T.Harte.
 */

void gfx_draw_aa_circle(int cx, int cy, int radius)
{
	int x,y;
	int s;
	int sx, sy;

	cx += GFX_X_OFFSET;
	cy += GFX_Y_OFFSET;

	radius >>= (16 - AA_BITS);

	x = radius;
	s = -radius;
	y = 0;

	while (y <= x)
	{
		//wide pixels
		sx = cx + (x >> AA_BITS); sy = cy + (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx + 1,	sy,	x&AA_AND);

		sy = cy - (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx + 1,	sy,	x&AA_AND);

		sx = cx - (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx - 1,	sy,	x&AA_AND);

		sy = cy + (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx - 1,	sy,	x&AA_AND);

		//tall pixels
		sx = cx + (y >> AA_BITS); sy = cy + (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy + 1,	x&AA_AND);

		sy = cy - (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy - 1,	x&AA_AND);

		sx = cx - (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy - 1,	x&AA_AND);

		sy = cy + (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy + 1,	x&AA_AND);

		s +=	AA_AND+1 + (y << (AA_BITS+1)) + ((1 << (AA_BITS+2))-2);
		y +=	AA_AND+1;

		while(s >= 0)
		{
			s -= (x << 1) + 2;
			x --;
		}
	}
}

/*
 * Draw anti-aliased line.
 * By T.Harte.
 */

#define itofix(a) ((a) << 16)
#define fmul(a,b) ((a) >> 8) *((b) >> 8)
#define fdiv(a,b) ((a) << 8) / ((b) >> 8)
 
void gfx_draw_aa_line (int x1, int y1, int x2, int y2)
{
	int grad, xd, yd;
	int xgap, ygap, xend, yend, xf, yf;
	int brightness1, brightness2, swap;

	int x, y, ix1, ix2, iy1, iy2;

	x1 += itofix(GFX_X_OFFSET);
	x2 += itofix(GFX_X_OFFSET);
	y1 += itofix(GFX_Y_OFFSET);
	y2 += itofix(GFX_Y_OFFSET);

	xd = x2 - x1;
	yd = y2 - y1;

	if (abs(xd) > abs(yd))
	{
		if(x1 > x2)
		{
			swap = x1; x1 = x2; x2 = swap;
			swap = y1; y1 = y2; y2 = swap;
			xd   = -xd;
			yd   = -yd;
		}

		grad = fdiv(yd, xd);

		//end point 1

		xend = trunc(x1 + 32768);
		yend = y1 + fmul(grad, xend-x1);

		xgap = invfrac(x1+32768);

		ix1  = xend >> 16;
		iy1  = yend >> 16;

		brightness1 = fmul(invfrac(yend), xgap);
		brightness2 = fmul(frac(yend), xgap);

		plot(ix1, iy1, brightness1 >> (16-AA_BITS));
		plot(ix1, iy1+1, brightness2 >> (16-AA_BITS));

		yf = yend+grad;

		//end point 2;

		xend = trunc(x2 + 32768);
		yend = y2 + fmul(grad, xend-x2);

		xgap = invfrac(x2 - 32768);

		ix2 = xend >> 16;
		iy2 = yend >> 16;

		brightness1 = fmul(invfrac(yend), xgap);
		brightness2 = fmul(frac(yend), xgap);
      
		plot(ix2, iy2, brightness1 >> (16-AA_BITS));
		plot(ix2, iy2+1, brightness2 >> (16-AA_BITS));

		for(x = ix1+1; x <= ix2-1; x++)
		{
			brightness1 = invfrac(yf);
			brightness2 = frac(yf);

			plot(x, (yf >> 16), brightness1 >> (16-AA_BITS));
			plot(x, 1+(yf >> 16), brightness2 >> (16-AA_BITS));

			yf += grad;
		}
	}
	else
	{
		if(y1 > y2)
		{
			swap = x1; x1 = x2; x2 = swap;
			swap = y1; y1 = y2; y2 = swap;
			xd   = -xd;
			yd   = -yd;
		}

		grad = fdiv(xd, yd);

		//end point 1

		yend = trunc(y1 + 32768);
		xend = x1 + fmul(grad, yend-y1);

		ygap = invfrac(y1+32768);

		iy1  = yend >> 16;
		ix1  = xend >> 16;

		brightness1 = fmul(invfrac(xend), ygap);
		brightness2 = fmul(frac(xend), ygap);

		plot(ix1, iy1, brightness1 >> (16-AA_BITS));
		plot(ix1+1, iy1, brightness2 >> (16-AA_BITS));

		xf = xend+grad;

		//end point 2;

		yend = trunc(y2 + 32768);
		xend = x2 + fmul(grad, yend-y2);

		ygap = invfrac(y2 - 32768);

		ix2 = xend >> 16;
		iy2 = yend >> 16;

		brightness1 = fmul(invfrac(xend), ygap);
		brightness2 = fmul(frac(xend), ygap);
      
		plot(ix2, iy2, brightness1 >> (16-AA_BITS));
		plot(ix2+1, iy2, brightness2 >> (16-AA_BITS));

		for(y = iy1+1; y <= iy2-1; y++)
		{
			brightness1 = invfrac(xf);
			brightness2 = frac(xf);

			plot((xf >> 16), y, brightness1 >> (16-AA_BITS));
			plot(1+(xf >> 16), y, brightness2 >> (16-AA_BITS));

			xf += grad;
		}
	}
}

void gfx_draw_filled_circle(int cx, int cy, int radius,int col)
{
	int x,y;
	int s;
	int sx, sy,ex;

	cx += GFX_X_OFFSET;
	cy += GFX_Y_OFFSET;

	radius <<= AA_BITS;

	x = radius;
	s = -radius;
	y = 0;

	while (y <= x)
	{
		//wide pixels
		sx = cx - (x >> AA_BITS); sy = cy + (y >> AA_BITS);
		ex = cx + (x >> AA_BITS);

		fill_line(gfx_screen,sx,ex,sy,col);

		sy = cy - (y >> AA_BITS);
		fill_line(gfx_screen,sx,ex,sy,col);

		//tall pixels
		sx = cx - (y >> AA_BITS); sy = cy + (x >> AA_BITS);
		ex = cx + (y >> AA_BITS);

		fill_line(gfx_screen,sx,ex,sy,col);

		sy= cy - (x >> AA_BITS);

		fill_line(gfx_screen,sx,ex,sy,col);

		s +=	AA_AND+1 + (y << (AA_BITS+1)) + ((1 << (AA_BITS+2))-2);
		y +=	AA_AND+1;

		while(s >= 0)
		{
			s -= (x << 1) + 2;
			x --;
		}
	}
}

void gfx_draw_outline_circle(int cx, int cy, int radius,int col)
{
	int x,y;
	int s;
	int sx, sy,ex;

	cx += GFX_X_OFFSET;
	cy += GFX_Y_OFFSET;

	radius <<= AA_BITS;

	x = radius;
	s = -radius;
	y = 0;

	while (y <= x)
	{
		//wide pixels
		sx = cx - (x >> AA_BITS); sy = cy + (y >> AA_BITS);
		ex = cx + (x >> AA_BITS);

		gfx_plot_pixel(sx,sy,col);
		gfx_plot_pixel(ex,sy,col);

		sy = cy - (y >> AA_BITS);
		gfx_plot_pixel(sx,sy,col);
		gfx_plot_pixel(ex,sy,col);

		//tall pixels
		sx = cx - (y >> AA_BITS); sy = cy + (x >> AA_BITS);
		ex = cx + (y >> AA_BITS);

		gfx_plot_pixel(sx,sy,col);
		gfx_plot_pixel(ex,sy,col);

		sy= cy - (x >> AA_BITS);

		gfx_plot_pixel(sx,sy,col);
		gfx_plot_pixel(ex,sy,col);

		s +=	AA_AND+1 + (y << (AA_BITS+1)) + ((1 << (AA_BITS+2))-2);
		y +=	AA_AND+1;

		while(s >= 0)
		{
			s -= (x << 1) + 2;
			x --;
		}
	}
}

#undef trunc
#undef frac
#undef invfrac
#undef plot

#undef AA_BITS
#undef AA_AND
#undef AA_BASE


void gfx_draw_circle (int cx, int cy, int radius, int circle_colour)
{
	if (anti_alias_gfx && circle_colour == GFX_COL_WHITE)
		gfx_draw_aa_circle (cx, cy, itofix(radius));
	else	
	 	gfx_draw_outline_circle(cx,cy,radius,circle_colour);
}

void gfx_draw_line (int x1, int y1, int x2, int y2)
{
	// if (y1 == y2)
	// {
	// 	hline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, GFX_COL_WHITE);
	// 	return;
	// }

	// if (x1 == x2)
	// {
	// 	vline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, y2 + GFX_Y_OFFSET, GFX_COL_WHITE);
	// 	return;
	// }

	if (anti_alias_gfx)
	 	gfx_draw_aa_line (itofix(x1), itofix(y1), itofix(x2), itofix(y2));
	 else
		line (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, GFX_COL_WHITE);
}

void gfx_draw_xor_line(int x1,int y1,int x2,int y2,int col)
{
	line_xor (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, col);
}



void gfx_draw_colour_line (int x1, int y1, int x2, int y2, int line_colour)
{
	// if (y1 == y2)
	// {
	// 	hline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, line_colour);
	// 	return;
	// }

	// if (x1 == x2)
	// {
	// 	vline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, y2 + GFX_Y_OFFSET, line_colour);
	// 	return;
	// }

	// if (anti_alias_gfx && (line_colour == GFX_COL_WHITE))
	// 	gfx_draw_aa_line (itofix(x1), itofix(y1), itofix(x2), itofix(y2));
	// else
	line (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, line_colour);
}

void gfx_draw_triangle (int x1, int y1, int x2, int y2, int x3, int y3, int col)
{
	triangle (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET,
				   x3 + GFX_X_OFFSET, y3 + GFX_Y_OFFSET, col);
}



void gfx_display_text (int x, int y, char *txt)
{
	// text_mode (-1);
	textout (gfx_screen, datafile[ELITE_1].dat, txt, (x / (2 / GFX_SCALE)) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, GFX_COL_WHITE);
}


void gfx_display_colour_text (int x, int y, char *txt, int col)
{
	// text_mode (-1);
	textout (gfx_screen, datafile[ELITE_1].dat, txt, (x / (2 / GFX_SCALE)) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, col);
}



void gfx_display_centre_text (int y, char *str, int psize, int col)
{
	int txt_size;
	int txt_colour;
	
	if (psize == 140)
	{
		txt_size = ELITE_2;
		txt_colour = -1;
	}
	else
	{
		txt_size = ELITE_1;
		txt_colour = col;
	}

	// text_mode (-1);
	textout_centre (gfx_screen,  datafile[txt_size].dat, str, (128 * GFX_SCALE) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, txt_colour);
}

void rectfill(unsigned char *buffer,int x,int y,int w,int h,int col)
{
	// Clip the rectangle fill
	if (x < clipLeftX)
	{
		w-=clipLeftX-x;
		x=clipLeftX;
	}

	if (x+w > clipRightX)
	{
		w-=(x+w) - clipRightX;
	}

	for(int i=0;i<h;++i)
	{
		if ((y) < clipTopY)
			continue;

		if ((y) >= clipBottomY)
			break;

		unsigned char *ptr=&buffer[y*SCREEN_H+x];
		for(int x0=0;x0<w;++x0)
		{
			*ptr++=col;
		}
		y++;
	}
}

void gfx_clear_display (void)
{
	rectfill (gfx_screen,  2, 1, 252 , 191 , GFX_COL_BLACK);
}

void gfx_clear_text_area (void)
{
	rectfill (gfx_screen,  2, 170, 252 , 21 , GFX_COL_BLACK);
}


void gfx_clear_area (int tx, int ty, int bx, int by)
{
	rectfill (gfx_screen, tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET,
				   bx , by , GFX_COL_BLACK);
}

void gfx_draw_rectangle (int tx, int ty, int bx, int by, int col)
{
	rectfill (gfx_screen, tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET,
				   bx , by, col);
}


void gfx_display_pretty_text (int tx, int ty, int bx, int by, char *txt)
{
	char strbuf[100];
	char *str;
	char *bptr;
	int len;
	int pos;
	int maxlen;
	
	maxlen = (bx - tx) / 12;

	str = txt;	
	len = strlen(txt);
	
	while (len > 0)
	{
		pos = maxlen;
		if (pos > len)
			pos = len;

		while ((str[pos] != ' ') && (str[pos] != ',') &&
			   (str[pos] != '.') && (str[pos] != '\0'))
		{
			pos--;
		}

		len = len - pos - 1;
	
		for (bptr = strbuf; pos >= 0; pos--)
			*bptr++ = *str++;

		*bptr = '\0';

		// text_mode (-1);
		textout (gfx_screen, datafile[ELITE_1].dat, strbuf, tx/2, ty/2, GFX_COL_WHITE);
		ty += (16 * GFX_SCALE);
	}
}


void gfx_draw_scanner (void)
{
	blit (scanner_image, gfx_screen, 0, 0, GFX_X_OFFSET, 192+GFX_Y_OFFSET, scanner_image->w, scanner_image->h);
}

void gfx_set_clip_region (int tx, int ty, int bx, int by)
{
	SetClip (tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET, bx + GFX_X_OFFSET, by + GFX_Y_OFFSET);
}

void gfx_start_render (void)
{
	start_poly = 0;
	total_polys = 0;
}

void gfx_render_polygon (int num_points, int *point_list, int face_colour, int zavg)
{
	int i;
	int x;
	int nx;
	
	if (total_polys == MAX_POLYS)
		return;

	x = total_polys;
	total_polys++;
	
	poly_chain[x].no_points = num_points;
	poly_chain[x].face_colour = face_colour;
	poly_chain[x].z = zavg;
	poly_chain[x].next = -1;

	for (i = 0; i < 16; i++)
		poly_chain[x].point_list[i] = point_list[i];				

	if (x == 0)
		return;

	if (zavg > poly_chain[start_poly].z)
	{
		poly_chain[x].next = start_poly;
		start_poly = x;
		return;
	} 	

	for (i = start_poly; poly_chain[i].next != -1; i = poly_chain[i].next)
	{
		nx = poly_chain[i].next;
		
		if (zavg > poly_chain[nx].z)
		{
			poly_chain[i].next = x;
			poly_chain[x].next = nx;
			return;
		}
	}	
	
	poly_chain[i].next = x;
}


void gfx_render_line (int x1, int y1, int x2, int y2, int dist, int col)
{
	int point_list[4];
	
	point_list[0] = x1;
	point_list[1] = y1;
	point_list[2] = x2;
	point_list[3] = y2;
	
	gfx_render_polygon (2, point_list, col, dist);
}


void gfx_finish_render (void)
{
	int num_points;
	int *pl;
	int i;
	int col;
	
	if (total_polys == 0)
		return;
		
	for (i = start_poly; i != -1; i = poly_chain[i].next)
	{
		num_points = poly_chain[i].no_points;
		pl = poly_chain[i].point_list;
		col = poly_chain[i].face_colour;

		if (num_points == 2)
		{
			gfx_draw_colour_line (pl[0], pl[1], pl[2], pl[3], col);
			continue;
		}
		
		gfx_polygon (num_points, pl, col); 
	};
}

void polygon(unsigned char *buffer,int count, int *list, int col)
{
	int x=0;
	int y=1;
	for(int i=1;i<count;++i)
	{
		line(buffer,list[x],list[y],list[x+2],list[y+2],col);
		x+=2;
		y+=2;
	}
	
	// Close if we've got more than 2 points
	if (count > 2)
	{
		//Final line to close the poly
		line(buffer,list[x],list[y],list[0],list[1],col);
	}
}

void polygon_aa(int count, int *list)
{
	int x=0;
	int y=1;
	for(int i=1;i<count;++i)
	{
		gfx_draw_aa_line(list[x],list[y],list[x+2],list[y+2]);
		x+=2;
		y+=2;
	}
	
	// Close if we've got more than 2 points
	if (count > 2)
	{
		//Final line to close the poly
		gfx_draw_aa_line(list[x],list[y],list[0],list[1]);
	}
}

void polygon_fill(unsigned char *buffer,int count, int *list, int col)
{
	triangle(buffer,list[0],list[1],list[2],list[3],list[4],list[5],col);
	if (count>3)
	{
		triangle(buffer,list[0],list[1],list[4],list[5],list[6],list[7],col);
		if (count>4)
		{
			triangle(buffer,list[0],list[1],list[6],list[7],list[8],list[9],col);
			if (count>5)
			{
				triangle(buffer,list[0],list[1],list[8],list[9],list[10],list[11],col);
				if (count>6)
				{
					triangle(buffer,list[0],list[1],list[10],list[11],list[12],list[13],col);
					if (count>7)
					{
						triangle(buffer,list[0],list[1],list[12],list[13],list[14],list[15],col);
					}
				}
			}
		}
	}
}

void gfx_polygon (int num_points, int *poly_list, int face_colour)
{
	int i;
	int x,y;
	
	x = 0;
	y = 1;
	for (i = 0; i < num_points; i++)
	{
		poly_list[x] += GFX_X_OFFSET;
		poly_list[y] += GFX_Y_OFFSET;
		x += 2;
		y += 2;
	}
	
	if (num_points == 2 || wireframe)
	{
		if (anti_alias_gfx && face_colour == GFX_COL_WHITE)
			polygon_aa (num_points, poly_list);
		else
			polygon (gfx_screen, num_points, poly_list, face_colour);
	}
	else
	{
		polygon_fill (gfx_screen, num_points, poly_list, face_colour);
	}
}


void gfx_draw_sprite (int sprite_no, int x, int y)
{
	BITMAP *sprite_bmp;
	
	switch (sprite_no)
	{
		case IMG_GREEN_DOT:
			sprite_bmp = datafile[GRNDOT].bmp;
			break;
		
		case IMG_RED_DOT:
			sprite_bmp = datafile[REDDOT].bmp;
			break;
			
		case IMG_BIG_S:
			sprite_bmp = datafile[SAFE].bmp;
			break;
		
		case IMG_ELITE_TXT:
			sprite_bmp = datafile[ELITETXT].bmp;
			break;
			
		case IMG_BIG_E:
			sprite_bmp = datafile[ECM].bmp;
			break;
			
		case IMG_BLAKE:
			sprite_bmp = datafile[BLAKE].bmp;
			break;
		
		case IMG_MISSILE_GREEN:
			sprite_bmp = datafile[MISSILE_G].bmp;
			break;

		case IMG_MISSILE_YELLOW:
			sprite_bmp = datafile[MISSILE_Y].bmp;
			break;

		case IMG_MISSILE_RED:
			sprite_bmp = datafile[MISSILE_R].bmp;
			break;

		default:
			return;
	}

	if (x == -1)
		x = ((256 * GFX_SCALE) - sprite_bmp->w) / 2;
	
	blit (sprite_bmp, gfx_screen, 0, 0, x + GFX_X_OFFSET, y + GFX_Y_OFFSET, sprite_bmp->w, sprite_bmp->h);
}

char fileNames[77][32];

#define FILE_LINES 15

int file_select(char *title, char *path, char *ext)
{
	DIR *dir = opendir("save");
	struct dirent *entry = readdir(dir);
	int used = 0;
	int save = 0;

	char fname[16];
	strcpy(fname,&path[5]);
	int selected = 0;

	if (strcmp(ext,"save")==0)
	{
		save = 1;
		strcpy(fileNames[used++],"<New Save>");
	}
	while(entry)
	{
		if (entry->d_type)
		{
			strcpy(fileNames[used++],entry->d_name);
			// Select the current commander if we've got a file with it's name
			if (save && strcmp(fileNames[used-1],fname)==0)
			{
				selected=used-1;
			}
		}	
		entry = readdir(dir);
	}
	// Change back to the base directory
	_swi(OS_CLI,_IN(0),"DIR ^");

	// used will only be 0 if there are no save files and
	// we chose to load
	if (used == 0)
	{
		return FALSE;
	}

	int input = 0;
	int top = 0;
	int key = 0;
	int frame = 0;
	int enter_pressed = 0;
	char str[32];

	while(1)
	{
		gfx_clear_display();
		gfx_display_centre_text (10, title, 140, GFX_COL_GOLD);
		line (gfx_screen, 0, 36/2, 255, 36/2, GFX_COL_WHITE);
		sprintf(str,"Select a file to %s",ext);
		gfx_display_centre_text (38, str, 120, save ? 249 : GFX_COL_GREEN_4);
		frame++;

		int line = 0;
		int pos = top;
		int y = 72;
		while(line < FILE_LINES && pos < used)
		{
			if (pos == selected)
			{
				gfx_draw_rectangle(78,y/2,100,8,GFX_COL_DARK_RED);
			}
			gfx_display_centre_text(y,fileNames[pos],120,GFX_COL_WHITE);

			y+=16;
			pos++;
			line++;
		}

		if (input)
		{
			sprintf(str,"File: %s%c", fname, frame & 8 ? 0x7f : 32);
			gfx_display_text(16,340,str);
		}

		gfx_update_screen();

		if (input)
		{
			key = readkey();
			int len = strlen(fname);

			if (key>32 && key<127)
			{
				if (len < 10)
				{
					fname[len++]=key;
					fname[len]=0;
				}
			}
			else if (key == 127)
			{
				if (len > 0)
				{
					fname[--len]=0;
				}
			}
			else if (key == 13)
			{
				if (!enter_pressed)
				{
					sprintf(path,"save/%s",fname);
					return TRUE;
				}
			}
			else if (key == 27)
			{
				return FALSE;
			}
			// Catch errant enter presses, we'll wait for enter to not be the key pressed
			if (key != 13)
			{
				enter_pressed = FALSE;
			}
		}
		else
		{
			kbd_poll_keyboard();
			if(kbd_up_pressed)
			{
				if (selected > 0)
				{
					selected--;
				}
				if (selected < top)
				{
					top--;
				}
			}
			if (kbd_down_pressed)
			{
				if (selected < used-1)
				{
					selected++;
				}
				if (selected > top + FILE_LINES-1)
				{
					top++;
				}
			}
			if (kbd_enter_pressed)
			{
				if (save && selected == 0)
				{
					kbd_clear_key_buffer();
					input = TRUE;
					enter_pressed = TRUE;
				}
				else
				{
					sprintf(path,"save/%s",fileNames[selected]);
					return TRUE;
				}
			}
			if (kbd_escape_pressed)
			{
				return FALSE;
			}
		}
	}
}


int gfx_request_file (char *title, char *path, char *ext)
{
	int okay;

	// For now just load/save to the commander name
	okay = file_select(title, path, ext);
	//show_mouse (screen);
	//okay = file_select (title, path, ext);
	//show_mouse (NULL);

	return okay;
}

