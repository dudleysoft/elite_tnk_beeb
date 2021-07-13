/*
 * Elite - The New Kind.
 *
 * Allegro version of the keyboard routines.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 */

/*
 * keyboard.c
 *
 * Code to handle keyboard input.
 */

#include <stdlib.h>
#include <string.h>

#include "beebScreen/beebScreen.h"
#include "bbckeycodes.h"
#include "swis.h"
 
// #include "allegro.h"

int kbd_F1_pressed;
int kbd_F2_pressed;
int kbd_F3_pressed;
int kbd_F4_pressed;
int kbd_F5_pressed;
int kbd_F6_pressed;
int kbd_F7_pressed;
int kbd_F8_pressed;
int kbd_F9_pressed;
int kbd_F10_pressed;
int kbd_F11_pressed;
int kbd_F12_pressed;
int kbd_y_pressed;
int kbd_n_pressed;
int kbd_fire_pressed;
int kbd_ecm_pressed;
int kbd_energy_bomb_pressed;
int kbd_hyperspace_pressed;
int kbd_ctrl_pressed;
int kbd_jump_pressed;
int kbd_escape_pressed;
int kbd_dock_pressed;
int kbd_d_pressed;
int kbd_origin_pressed;
int kbd_find_pressed;
int kbd_fire_missile_pressed;
int kbd_target_missile_pressed;
int kbd_unarm_missile_pressed;
int kbd_pause_pressed;
int kbd_resume_pressed;
int kbd_inc_speed_pressed;
int kbd_dec_speed_pressed;
int kbd_up_pressed;
int kbd_down_pressed;
int kbd_left_pressed;
int kbd_right_pressed;
int kbd_enter_pressed;
int kbd_backspace_pressed;
int kbd_space_pressed;

int key[256];

int kbd_keyboard_startup (void)
{
//	set_keyboard_rate(2000, 2000);
	return 0;
}

int kbd_keyboard_shutdown (void)
{
	return 0;
}

#define KEYS 37

int keys[]={
	BBC_f0,
	BBC_f1,
	BBC_f2,
	BBC_f3,
	BBC_f4,
	BBC_f5,
	BBC_f6,
	BBC_f7,
	BBC_f8,
	BBC_f9,

	BBC_y,
	BBC_n,

    BBC_a,
	BBC_e,
    BBC_TAB,
	BBC_h,
	BBC_CTRL,
	BBC_j,
	BBC_ESC,

    BBC_c,
	BBC_d,
	BBC_o,
	BBC_f,

	BBC_m,
	BBC_t,
	BBC_u,
	BBC_p,
	BBC_r,
	BBC_SPACE,
	BBC_SLASH,
	BBC_s,
	BBC_x,
	BBC_LESS,
	BBC_PERIOD,
	BBC_RETURN,
	BBC_DELETE,
	BBC_AT
};

void poll_keyboard()
{
	for(int i=0;i<KEYS;++i)
	{
		key[keys[i]]=beebScreen_ScanKey(keys[i]);
	}
}

void kbd_poll_keyboard (void)
{
	poll_keyboard();

	kbd_F1_pressed = key[BBC_f0];
	kbd_F2_pressed = key[BBC_f1];
	kbd_F3_pressed = key[BBC_f2];
	kbd_F4_pressed = key[BBC_f3];
	kbd_F5_pressed = key[BBC_f4];
	kbd_F6_pressed = key[BBC_f5];
	kbd_F7_pressed = key[BBC_f6];
	kbd_F8_pressed = key[BBC_f7];
	kbd_F9_pressed = key[BBC_f8];
	kbd_F10_pressed = key[BBC_f9];
	kbd_F11_pressed = key[BBC_AT];
	kbd_F12_pressed = key[255];

	kbd_y_pressed = key[BBC_y];
	kbd_n_pressed = key[BBC_n];

    kbd_fire_pressed = key[BBC_a];
	kbd_ecm_pressed = key[BBC_e];
    kbd_energy_bomb_pressed = key[BBC_TAB];
	kbd_hyperspace_pressed = key[BBC_h];
	kbd_ctrl_pressed = (key[BBC_CTRL]);
	kbd_jump_pressed = key[BBC_j];
	kbd_escape_pressed = key[BBC_ESC];

    kbd_dock_pressed = key[BBC_c];
	kbd_d_pressed = key[BBC_d];
	kbd_origin_pressed = key[BBC_o];
	kbd_find_pressed = key[BBC_f];

	kbd_fire_missile_pressed = key[BBC_m];
	kbd_target_missile_pressed = key[BBC_t];
	kbd_unarm_missile_pressed = key[BBC_u];
	
	kbd_pause_pressed = key[BBC_p];
	kbd_resume_pressed = key[BBC_r];
	
	kbd_inc_speed_pressed = key[BBC_SPACE];
	kbd_dec_speed_pressed = key[BBC_SLASH];
	
	kbd_up_pressed = key[BBC_s];
	kbd_down_pressed = key[BBC_x];
	kbd_left_pressed = key[BBC_LESS];
	kbd_right_pressed = key[BBC_PERIOD];
	
	kbd_enter_pressed = key[BBC_RETURN];
	kbd_backspace_pressed = key[BBC_DELETE];
	kbd_space_pressed = key[BBC_SPACE];

}


int kbd_read_key (void)
{
	int keynum;
	int keyasc;

	kbd_enter_pressed = 0;
	kbd_backspace_pressed = 0;
	
	keynum = readkey();
	keyasc = keynum & 255;

	if (keyasc == 13)
	{
		kbd_enter_pressed = 1;
		return 0;
	} 

	if (keyasc == 127)
	{
		kbd_backspace_pressed = 1;
		return 0;
	} 

	return keyasc;
}

void kbd_clear_key_buffer (void)
{
    _swi(OS_Byte,_INR(0,1),15,0);
}
 
