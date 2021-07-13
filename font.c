// File:	DiceMan\\Font.cpp
// Purpose:	Lumdum Dare 48 Hour Contest Game
// Theme:	Random
// Author:	James Watson

#include "bitmap.h"
#include "font.h"
#include <stdlib.h>

extern unsigned char *gfx_screen;

Font *FONT_Load(char *szFname,int kern)
{
	Font *font=malloc(sizeof(Font));
	int key;
	int idx,x,y,w,h;

	font->bmp=LoadBMP(szFname, NULL);

	if (font->bmp==NULL)
	{
		free(font);
		return NULL;
	}
	key = SetKey(font->bmp,255,0,255);

	font->width=malloc(128);

	idx=32;
	font->w=w=font->bmp->w>>4;
	font->h=h=font->bmp->h/6;

	for(y=0;y<h*6;y+=h)
	{
		for(x=0;x<w*16;x+=w)
		{
			char *p=(char *)font->bmp->data;
			int x0,y0;
			int w0=0;

			for(y0=0;y0<h;y0++)
			{
				x0=0;
				while((x0<w)&&(p[((y+y0)*font->bmp->w)+(x+x0)]==key))
					x0++;

				if (x0==w)
					continue;

				while((x0<w)&&(p[((y+y0)*font->bmp->w)+(x+x0)]!=key))
					x0++;

				if (x0>w0)
					w0=x0;
			}
			if (idx==32)
				w0=w>>1;

			font->width[idx++]=w0;
		}
	}

	return font;
}

int FONT_Length(Font *font,char *str)
{
	int w=0;
	char *p=str;

	while(*p)
		w+=font->width[*p++];

	return w;
}

void FONT_Render(Font *font,int x,int y,char *str,int flags,int col)
{
	int w;
	char *p=str;
	Rect rect;
	Rect src;

	// Catch Error if we haven't loaded a font
	if (font==NULL)
		return;

	src.w=font->w;
	src.h=font->h;

	w=0;

	while(*p)
		w+=font->width[*p++]+2;

	p=str;

	if (flags & FONT_HCENTER)
		rect.x=x-(w>>1);
	else if (flags & FONT_RIGHT)
		rect.x=x-w;
	else
		rect.x=x;
	if (flags & FONT_VCENTER)
		rect.y=y-(font->h>>1);
	else if (flags & FONT_BOTTOM)
		rect.y=y-font->h;
	else
		rect.y=y;

	while(*p)
	{
		src.x=(*p&15)*font->w;
		src.y=((*p-32)>>4)*font->h;
		BlitBMPCol(font->bmp,&src,gfx_screen,&rect,col);

		rect.x+=font->width[*p++]+2;
	}
}
