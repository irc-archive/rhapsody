/*****************************************************************************/
/*                                                                           */
/*  Copyright (C) 2004 Adrian Gonera                                         */
/*                                                                           */
/*  This file is part of Rhapsody.                                           */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif
#include "ncolor.h"
#include "common.h"

int HAS_COLORS;	
int CHANGE_COLORS;


/*
 0 white
 1 black
 2 blue     (navy)
 3 green
 4 red
 5 brown    (maroon)
 6 purple
 7 orange   (olive)
 8 yellow
 9 lt.green (lime)
 10 teal    (a kinda green/blue cyan)
 11 lt.cyan (cyan ?) (aqua)
 12 lt.blue (royal)
 13 pink    (light purple) (fuschia)
 14 grey
 15 lt.grey (silver)

*/

int mirc_color_palette_8[16]={
	COLOR_BLACK,
	COLOR_WHITE,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_RED,
	COLOR_RED,
	COLOR_CYAN,
	COLOR_YELLOW,
	COLOR_YELLOW,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_CYAN,
	COLOR_CYAN,
	COLOR_MAGENTA,
	COLOR_WHITE,
	COLOR_WHITE
};


int begin_color(){

	if ((HAS_COLORS = has_colors())){
		start_color();
		init_color_palette();
		init_pair(0, DEFAULT_COLOR_F, DEFAULT_COLOR_B);

		#ifdef DEBUG
			printf("Terminal has color support = YES\r\n");
			printf("Terminal has colors = %d\r\n", COLORS);
			printf("Terminal has color pairs = %d\r\n", COLOR_PAIRS);
		#endif		

		CHANGE_COLORS = can_change_color();
		
		#ifdef DEBUG
			if (CHANGE_COLORS) printf("Terminal can change color = YES\r\n");
			else printf("Terminal can change color = NO\r\n");
		#endif
	}
	else {
		#ifdef DEBUG
			printf("Terminal has color support = NO\n");
		#endif
	}
	return(0);
}

void init_color_palette(){

	if (CHANGE_COLORS){		
		init_color(C_WHITE, 1000, 1000, 1000);
		init_color(C_BLACK, 0, 0, 0);
		init_color(C_BLUE, 0, 0, 1000);
		init_color(C_GREEN, 0, 1000, 0);
		init_color(C_RED, 1000, 0, 0);
		init_color(C_BROWN, 1000, 0, 1000);
		init_color(C_LBLUE, 250, 250, 1000);
		init_color(C_YELLOW, 1000, 500, 500);
		init_color(C_LGREEN, 250, 1000, 250);
		init_color(C_LGREEN2, 500, 1000, 500);
		init_color(C_AQUA, 500, 500, 1000);
		init_color(C_LBLUE2, 500, 500, 1000);
		init_color(C_PURPLE, 0, 500, 1000);
		init_color(C_GREY, 250, 250, 250);
		init_color(C_LGREY, 500, 500, 500);

		init_pair(-1, C_WHITE, C_WHITE);
		init_pair(0, C_WHITE, C_WHITE);
		init_pair(1, C_BLACK, C_WHITE);
		init_pair(2, C_BLUE, C_WHITE);	
		init_pair(3, C_GREEN, C_WHITE);
		init_pair(4, C_RED, C_WHITE);
		init_pair(5, C_BROWN, C_WHITE);
		init_pair(6, C_LBLUE, C_WHITE);
		init_pair(7, C_ORANGE, C_WHITE);
		init_pair(8, C_YELLOW, C_WHITE);
		init_pair(9, C_LGREEN, C_WHITE);
		init_pair(10, C_LGREEN2, C_WHITE);
		init_pair(11, C_AQUA, C_WHITE);
		init_pair(12, C_LBLUE2, C_WHITE);
		init_pair(13, C_PURPLE, C_WHITE);
		init_pair(14, C_GREY, C_WHITE);
		init_pair(15, C_LGREY, C_WHITE);
	}

/*	Curses Colors	Integer Value
	COLOR_BLACK	0
	COLOR_BLUE	1
	COLOR_GREEN	2
	COLOR_CYAN	3
	COLOR_RED	4
	COLOR_MAGENTA	5
	COLOR_YELLOW	6
	COLOR_WHITE	7
*/

	// init the color palette, mirc style using only the eight availble colors
	else {
		int i, j;
		for (i=0; i<16; i++){
			for (j=0; j<16; j++){
				init_pair(i*16+j, mirc_color_palette_8[j], mirc_color_palette_8[i]);				
			}
		}	
	}
}


