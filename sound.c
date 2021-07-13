/*
 * Elite - The New Kind.
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder.
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 *
 */

/*
 * sound.c
 */

#include <stdlib.h>
// #include <allegro.h>
#include "sound.h"
#include "alg_data.h" 
#include "swis.h"

#define NUM_SAMPLES 14 

// extern DATAFILE *datafile;

static int sound_on;

// struct sound_sample
// {
//  	void *sample;
// 	char filename[256];
// 	int runtime;
// 	int timeleft;
// };


#define ENVELOPES 4

typedef struct {
	char number;
	char time;
	char pi1;
	char pi2;
	char pi3;
	char pn1;
	char pn2;
	char pn3;
	char aa;
	char ad;
	char as;
	char ar;
	char ala;
	char ald;
} Envelope;

Envelope envelopes[]={
	{1, 1, 0, 111, -8, 4, 1, 8, 8, -2, 0, -1, 126, 44},
  	{2, 1, 14, -18, -1, 44, 32, 50, 6, 1, 0, -2, 120, 126},
	{3, 1, 1, -1, -3, 17, 32, 128, 1, 0, 0, -1, 1, 1},
 	{4, 1, 4, -8, 44, 4, 6, 8, 22, 0, 0, -127, 126, 0}
};

typedef struct {
	short channel;
	short amplitude;
	short pitch;
	short duration;
} Sound;

Sound sounds[]=
{
	{ 0x12,0x01,0x00,0x10 },   // 0  - Lasers fired by us
	{ 0x12,0x02,0x2C,0x08 },   // 1  - We're being hit by lasers
	{ 0x11,0x03,0xF0,0x18 },   // 2 - We died 1 / We made a hit or kill 2
	{ 0x10,0xF1,0x07,0x1A },   // 3 - We died 2 / We made a hit or kill 1
	{ 0x03,0xF1,0xBC,0x01 },   // 4 - Short, high beep
	{ 0x13,0xF4,0x0C,0x08 },   // 5 - Long, low beep
	{ 0x10,0xF1,0x06,0x0C },   // 6 - Missile launched / Ship launched from station
	{ 0x10,0x02,0x60,0x10 },   // 7 - Hyperspace drive engaged
	{ 0x13,0x04,0xC2,0xFF },   // 8 - E.C.M. on
	{ 0x13,0x00,0x00,0x00 }   //  9 - E.C.M. off
};
 
// struct sound_sample sample_list[NUM_SAMPLES] =
// {
// 	{NULL, "launch.wav",    32, 0}, // 6
// 	{NULL, "crash.wav",      7, 0}, // 1
// 	{NULL, "dock.wav",      36, 0}, // 6
// 	{NULL, "gameover.wav",  24, 0}, // -1
// 	{NULL, "pulse.wav",      4, 0}, // 0
// 	{NULL, "hitem.wav",		 4, 0}, // 2
// 	{NULL, "explode.wav",	23, 0}, // -1
// 	{NULL, "ecm.wav",		23, 0}, // 8
// 	{NULL, "missile.wav",	25, 0}, // 6
// 	{NULL, "hyper.wav",	    37, 0}, // 7
// 	{NULL, "incom1.wav",	 4, 0}, // 1
// 	{NULL, "incom2.wav",	 5, 0}, // 1
// 	{NULL, "beep.wav",		 2, 0}, // 4
// 	{NULL, "boop.wav",		 7, 0}, // 5
// };

int sound_lookup[]={
	6,1,6,-1,0,2,-1,8,6,7,1,1,4,5
};

void snd_sound_startup (void)
{
	int i;

 	/* Install a sound driver.. */
	sound_on = 1;

	for(int i=0;i < ENVELOPES; ++i)
	{
		_swi(OS_Word,_INR(0,1),8,&envelopes[i]);
	}
}
 

void snd_sound_shutdown (void)
{
	int i;

	if (!sound_on)
		return;
}

int ecm_timer = 0;

void snd_play_sample (int sample_no)
{
	if (!sound_on)
	{
		return;
	}

	if ((sample_no < 0) || (sample_no >= NUM_SAMPLES))
	{
		return;
	}

	int play=sound_lookup[sample_no];
	// Explosion is followed by
	if (play==-1)
	{
		_swi(OS_Word,_INR(0,1),7,&sounds[3]);
		_swi(OS_Word,_INR(0,1),7,&sounds[3]);
	}
	else
	{
		_swi(OS_Word,_INR(0,1),7,&sounds[play]);
	}
	if (play == 8)
	{
		ecm_timer = 10;
	}
}


void snd_update_sound (void)
{
	// Turn off the ecm sample after a few frames, otherwise it'll play forever
	if (ecm_timer > 0)
	{
		ecm_timer--;
		if (ecm_timer == 0)
		{
			_swi(OS_Word,_INR(0,1),7,&sounds[9]);
		}
	}
	// int i;
	
	// for (i = 0; i < NUM_SAMPLES; i++)
	// {
	// 	if (sample_list[i].timeleft > 0)
	// 		sample_list[i].timeleft--;
	// }
}


void snd_play_midi (int midi_no, int repeat)
{
	// if (!sound_on)
	// 	return;
	
	// switch (midi_no)
	// {
	// 	case SND_ELITE_THEME:
	// 		play_midi (datafile[THEME].dat, repeat);
	// 		break;
		
	// 	case SND_BLUE_DANUBE:
	// 		play_midi (datafile[DANUBE].dat, repeat);
	// 		break;
	// }
}


void snd_stop_midi (void)
{
	// if (sound_on);
	// 	play_midi (NULL, TRUE);
}