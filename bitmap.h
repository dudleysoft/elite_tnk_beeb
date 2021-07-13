// File:	DiceMan\\Bitmap.h
// Purpose:	Lumdum Dare 48 Hour Contest Game - Port to beebScreen
// Theme:	Random
// Author:	James Watson

#ifndef _BITMAP_H
#define _BITMAP_H

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef struct {
    int w;
    int h;
    int key;
    unsigned char *pal;
    unsigned char *data;
} Bitmap;

Bitmap *LoadBMP(const char *fname,int *pal);

int FindColour(Bitmap *bmp,int r,int g, int b);

int SetKey(Bitmap *bmp, int r,int g, int b);

void BlitBMP(Bitmap *bmp,Rect *src,unsigned char *buffer,Rect *dest);
void BlitBMPCol(Bitmap *bmp,Rect *src,unsigned char *buffer,Rect *dest,int col);

void FreeBMP(Bitmap *bmp);

void SetClip(int left,int top,int right,int bottom);

extern int clipLeftX;
extern int clipRightX;
extern int clipTopY;
extern int clipBottomY;

#endif