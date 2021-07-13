#include <stdlib.h>
#include "beebScreen/beebScreen.h"
#include "bitmap.h"

#pragma pack(1)

typedef struct {
	char id0,id1;
	int size;
	int reserved;
	int img_offset;
	int extra_size;
	int width;
	int height;
	short planes;
	short bpp;
	int compression;
	int img_size;
	int r0,r1,r2,r3;
} BMP_Header;

char pathDest[256];

char *remapPath(const char *src)
{
	char *dest = pathDest;
	int sinceDir=0;
	while(*src)
	{
		switch(*src)
		{
		case '\\':
		case '/':
			*dest++='.';
			sinceDir = 0;
			break;
		case '.':
			if (sinceDir<10)
			{
				*dest++='/';
				sinceDir++;
			}
			break;
		default:
			if (sinceDir<10)
			{
				*dest++=*src;
				sinceDir++;
			}
			break;
		}
		src++;
	}
	*dest=0;
	return pathDest;
}

Bitmap *LoadBMP(const char *fname,int *pal)
{
    FILE *fhand = fopen(remapPath(fname),"rb");
    if (fhand == NULL)
    {
        return NULL;
    }

    Bitmap *bmp = malloc(sizeof(Bitmap));
    BMP_Header header;
    
    fread(&header,1,sizeof(BMP_Header),fhand);

    bmp->w = header.width;
    bmp->h = header.height;
    bmp->key = -1;
    bmp->pal = malloc(4<<header.bpp);
    bmp->data = malloc(header.width * header.height);

    // Find the actual start of the colour table
    fseek(fhand,header.extra_size + 14,SEEK_SET);
    // Read into pal data
    fread(bmp->pal,4,256,fhand);
    fseek(fhand,header.img_offset,SEEK_SET);

    // Read backwards into our buffer
    for(int y=header.height-1;y>=0;--y)
    {
        unsigned char *ptr = &bmp->data[y*header.width];
        fread(ptr,1,header.width,fhand);
        
        // Make sure we're 32bit aligned on each row
        if (header.width%4)
        {
            fseek(fhand,4-(header.width%4),SEEK_CUR);
        }
    }
    fclose(fhand);

    if (pal)
    {
        beebScreen_SetNulaPal((int*)bmp->pal,pal,256,beebScreen_extractBGR888);
    }

    return bmp;
}

int colourDist(int a,int b,int weight)
{
    int d=a-b;
    return d*d*weight;
}

int FindColour(Bitmap *bmp,int r,int g, int b)
{
    unsigned char *ptr=bmp->pal;
    for(int i=0;i<256;++i)
    {
        if (ptr[2]==r && ptr[1]==g &&ptr[0]==b)
        {
            return i;
        }
        ptr+=4;
    }
    // Find closest colour
    int dist = colourDist(bmp->pal[2],r,3) + colourDist(bmp->pal[1],g,5) + colourDist(bmp->pal[0],b,1);
    int found = 0;
    ptr=&bmp->pal[4];
    for(int i=1;i<256;++i)
    {
        int newDist = colourDist(ptr[2],r,3) + colourDist(ptr[1],g,5) + colourDist(ptr[0],b,1);
        if (newDist < dist)
        {
            dist = newDist;
            found = i;
        }
        ptr+=4;
    }
    return found;
}

int SetKey(Bitmap *bmp, int r,int g, int b)
{
    unsigned char *ptr=bmp->pal;
    for(int i=0;i<256;++i)
    {
        if (ptr[2]==r && ptr[1]==g &&ptr[0]==b)
        {
            bmp->key=i;
            return i;
        }
        ptr+=4;
    }
    return -1;
}

void FreeBMP(Bitmap *bmp)
{
    if (bmp->pal)
    {
        free(bmp->pal);
    }
    if (bmp->data)
    {
        free(bmp->data);
    }
    free(bmp);
}

int clipLeftX=0;
int clipRightX=256;
int clipTopY=0;
int clipBottomY=256;

void SetClip(int left,int top,int right,int bottom)
{
    clipLeftX=left;
    clipTopY=top;
    clipRightX=right;
    clipBottomY=bottom;
}

void BlitBMP(Bitmap *bmp,Rect *src,unsigned char *buffer,Rect *dest)
{
    int dx=dest->x;
    int dy=dest->y;
    int w = src ? src->w: bmp->w;
    int h = src ? src->h : bmp->h;
    int sx = src ? src->x : 0;
    int sy = src ? src->y : 0;

    for(int y=dy;y<dy+h;y++,sy++)
    {
        // Break out if we're off screen
        if (y<clipTopY)
            continue;

        if (y>=clipBottomY)
            break;

        // get out data pointers
        unsigned char *in = &bmp->data[(sy * bmp->w) + sx];
        unsigned char *ptr = &buffer[y*256];

        // Loop across the row
        for(int x=dx;x<dx+w;x++,in++)
        {
            // Off screen, or the colour key
            if (x<clipLeftX || *in==bmp->key)
                continue;

            if (x>=clipRightX)
                break;

            ptr[x]=*in;
        }
    }
}

void BlitBMPCol(Bitmap *bmp,Rect *src,unsigned char *buffer,Rect *dest,int col)
{
    int dx=dest->x;
    int dy=dest->y;
    int w = src ? src->w: bmp->w;
    int h = src ? src->h : bmp->h;
    int sx = src ? src->x : 0;
    int sy = src ? src->y : 0;

    for(int y=dy;y<dy+h;y++,sy++)
    {
        // Break out if we're off screen
        if (y<clipTopY)
            continue;

        if (y>=clipBottomY)
            break;

        // get out data pointers
        unsigned char *in = &bmp->data[(sy * bmp->w) + sx];
        unsigned char *ptr = &buffer[y*256];

        // Loop across the row
        for(int x=dx;x<dx+w;x++,in++)
        {
            // Off screen, or the colour key
            if (x<clipLeftX || *in==bmp->key)
                continue;

            if (x>=clipRightX)
                break;

            ptr[x]=*in & col;
        }
    }
}