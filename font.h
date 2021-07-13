// File:	DiceMan\\Font.h
// Purpose:	Lumdum Dare 48 Hour Contest Game
// Theme:	Random
// Author:	James Watson

#ifndef _FONT_H
#define _FONT_H

#define FONT_NORMAL		0
#define FONT_HCENTER	1
#define FONT_VCENTER	2
#define FONT_RIGHT		4
#define FONT_BOTTOM		8

typedef struct {
	Bitmap *bmp;
	char *width;
	int w,h;
} Font;

Font *FONT_Load(char *szFname,int kern);
void FONT_Render(Font *font,int x,int y,char *str,int flags,int col);
int FONT_Length(Font *font,char *str);

#endif