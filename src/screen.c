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
#include <errno.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdarg.h>

#include "defines.h"
#include "autodefs.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif


#include "log.h"
#include "ncolor.h"
#include "events.h"
#include "cevents.h"
#include "common.h"
#include "screen.h"
extern screen *screenlist;
extern screen *currentscreen;

int screenupdated;

/* screen ***********************************************************************************************/

screen *add_screen (void *screenptr, int type){
	screen *current = screenlist;
	screen *new = NULL;
	
	screenupdated = 1;
        if (screenlist==NULL){
                current=calloc(sizeof(screen), 1);
                if (current==NULL){
			plog("Cannot allocate screen memory in add_screen(:1)\n");   
			exit(MEMALLOC_ERR);   
		}
		current->next=NULL;
		current->prev=NULL;
		current->screen=screenptr;
		current->type=type;
		current->scrollpos=0;
		current->scrolling=0;
		current->update=U_ALL;
		current->hidden=0;
		screenlist = current; 
		return(current);
		
	}
	else{
		while(current!=NULL){
			if (current->next==NULL){
				new=calloc(sizeof(screen), 1);
                		if (new==NULL){
					plog("Cannot allocate screen memory in add_screen(:2)\n");   
					exit(MEMALLOC_ERR);   
				}
				current->next=new;
				new->next=NULL;
				new->prev=current;
				new->type=type;
				new->screen=screenptr;
				new->scrollpos=0;
				new->scrolling=0;
				new->update=U_ALL;
				new->hidden=0;

				break;
			}
			current=current->next;
		}		
		return(new);
	}	
}

void remove_screen(screen *Current){
	screen *Previous;
	screen *Next;

	screenupdated = 1;
	Previous = Current->prev;
	Next = Current->next;
          
	if (Next != NULL) Next->prev = Previous;
	if (Previous != NULL) Previous->next = Next;
	if (screenlist == Current) screenlist = Next;
        free (Current);
}

void hide_screen(screen *Current, int hide){
	if (Current != NULL) Current->hidden = hide;
}

int print_screen(WINDOW *win, char *buffer){
	return (print_screen_opt(win, buffer, COLS-2, 0, A_NORMAL, O_ALL));
}

int print_screen_opt(WINDOW *win, char *buffer, int linelen, int indent, int attr_def, int options){

	int strbuffer[MAXDATASIZE];
	int attrbuffer[MAXDATASIZE];
	char colorcode[7];

	int i, j, line, linepos, linesize, strpos, strlength, printed;
	int bgcolor, fgcolor, fg_set, bg_set;

	int attr_bold=0;
	int attr_ul=0;
	int attr_rev=0;
	int attr_color=0;
	int attr_current;
	

	// do this in two steps.
	// first create two arrays one with characters and the other with attributes
	// and populate them by parsing the string

	if (buffer != NULL && win != NULL){

		for (i=0, j=0; i<strlen(buffer); i++){
		
			//printf("%d, %c\n", buffer[i], buffer[i]);
		
			// if character is a ^B, set attribute to BOLD
			if (buffer[i]==2 && options&O_BOLD) attr_bold = (attr_bold + 1) & 1;

			// if character is a ^_, set attribute to UNDERLINE
			else if (buffer[i]==31 && options&O_UNDERLINE) attr_ul = (attr_ul + 1) & 1;

			// if character is a ^O, set attribute to REVERSE
			else if (buffer[i]==15 && options&O_REVERSE) attr_rev = (attr_rev + 1) & 1;

			// if character is a ^C, color info follows
			else if (buffer[i]==3 && options&O_COLOR){
				bzero(colorcode,7);
				if (isdigit((int)buffer[i+1]) && isdigit((int)buffer[i+2])){
					strncpy(colorcode, &buffer[i+1], 2);
					i=i+2;
				}
				else if (isdigit((int)buffer[i+1])){
					strncpy(colorcode, &buffer[i+1], 1);
					i++;
				}	
				else{
					strcpy(colorcode, "0");
				}	
				fg_set=sscanf(colorcode, "%d", &fgcolor);
	
				if (fg_set && buffer[i+1]==','){		
					bzero(colorcode,7);
					if (isdigit((int)buffer[i+2]) && isdigit((int)buffer[i+3])){
						strncpy(colorcode, &buffer[i+1], 2);
						i=i+3;
					}
					else{
						strncpy(colorcode, &buffer[i+1], 1);
						i=i+2;
					}
					bg_set=sscanf(colorcode, "%d", &bgcolor);
				}
				if (fg_set > 0 && fgcolor && (options & O_NOMONOCHROME)) attr_color = COLOR_PAIR(mirc_palette(fgcolor, bgcolor));
				else attr_color = 0;
			}				
			else {
				attr_current = attr_def;
				attr_current = attr_current | attr_color;
				if (attr_ul) attr_current = attr_current | A_UNDERLINE;
				if (attr_bold) attr_current = attr_current | A_BOLD;
				if (attr_rev) attr_current = attr_current | A_REVERSE;

				strbuffer[j] = (buffer[i]&0xff);
				attrbuffer[j] = attr_current;

				//wattrset(win, attrbuffer[j]);
				//waddch(win, strbuffer[j]);
				j++;
			}
			strbuffer[j] = 0;

		}

		// now print the decoded characters to the screen
		
		printed = 0;
		strlength = j;
		strpos = 0;
		line = 0;

		while(strpos < strlength){
			if (line==0) linesize = linelen;
			else linesize = linelen-indent;

			if ((strpos+linesize) < strlength){
				for (linepos=linesize; linepos>0; linepos--){
					if (strbuffer[strpos+linepos]==' ') break;
				}

				// if the string is longer than the line width of the display
				// print only to the end of the last word
				if (linepos==0) linepos=linesize;
				if (line>0 && (options&O_INDENT)){ 
					wattrset(win, attr_def);	
					for (i=0; i<indent; i++) waddch(win, ' ');
				}
				for (i=0; i<linepos; i++){
					if (isprint((unsigned char)strbuffer[strpos+i]) || strbuffer[strpos+i] == '\n'){
						wattrset(win, attrbuffer[strpos+i]);	
						waddch(win, strbuffer[strpos+i]);
						printed++;
					}
				}
				waddch (win, '\n');
				strpos=strpos+linepos+1;
			}
			else {
				if (line && options&O_INDENT){ 
					wattrset(win, attr_def);	
					for (i=0; i<indent; i++){
						waddch(win, ' ');
						printed++;
					}
				}
				for (i=0; strpos+i < j; i++){
					if (isprint((unsigned char)strbuffer[strpos+i]) || strbuffer[strpos+i] == '\n'){
						wattrset(win, attrbuffer[strpos+i]);	
						waddch(win, strbuffer[strpos+i]);
						printed++;
					}
				}
				strpos = strlength;
			}
			line++;
		}
	}
	return(printed);
}

void print_all(char *buffer){
	screen *current;

        current=screenlist;
        while(current!=NULL){
		if (current->type==SERVER) print_server (current->screen, buffer);
		else if (current->type==CHANNEL) print_channel (current->screen, buffer);
		else if (current->type==CHAT) print_chat (current->screen, buffer);
		else if (current->type==DCCCHAT) print_dcc_chat (current->screen, buffer);
		else if (current->type==HELP) print_help (current->screen, buffer);
		current=current->next;
	}
}

void print_all_attrib(char *buffer, int attrib){
	screen *current;

        current=screenlist;
        while(current!=NULL){
		if (current->type==SERVER) print_server_attrib (current->screen, buffer, attrib);
		else if (current->type==CHANNEL) print_channel_attrib (current->screen, buffer, attrib);
		else if (current->type==CHAT) print_chat_attrib (current->screen, buffer, attrib);
		else if (current->type==DCCCHAT) print_dcc_chat_attrib (current->screen, buffer, attrib);
		else if (current->type==HELP) print_help_attrib (current->screen, buffer, attrib);
		current=current->next;
	}
}
     
void vprint_all_attrib(int attrib, char *template, ...){
        va_list ap;
        char stringt[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(stringt, template, ap);
        va_end(ap);

	print_all_attrib(stringt, attrib);
}

void vprint_all(char *template, ...){
        va_list ap;
        char string[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(string, template, ap);
        va_end(ap);
	print_all(string);
}

void scroll_message_screen(screen *screen, int lines){
	screen->scrollpos = (screen->scrollpos) + lines;
	screen->scrolling = 1;
	if (screen->scrollpos > BUFFERLINES) screen->scrollpos = BUFFERLINES;
	else if (screen->scrollpos < 0) screen->scrollpos=0;
}

void set_message_scrolling(screen *screen, int scroll){	
	screen->scrolling=scroll;
}

void redraw_screen(screen *current){
	if (current->type==SERVER) redraw_server_screen(current->screen);
	else if (current->type==CHANNEL) redraw_channel_screen(current->screen);
	else if (current->type==TRANSFER) redraw_transfer_screen(current->screen);
	else if (current->type==CHAT) redraw_chat_screen(current->screen);
	else if (current->type==DCCCHAT) redraw_dccchat_screen(current->screen);
	else if (current->type==LIST) redraw_list_screen(current->screen);
}
	
void refresh_screen(screen *current){
	if ((current->type)==SERVER) refresh_server_screen(current->screen);
	else if ((current->type)==CHANNEL) refresh_channel_screen(current->screen);
	else if ((current->type)==CHAT) refresh_chat_screen(current->screen);
	else if ((current->type)==DCCCHAT) refresh_dccchat_screen(current->screen);
	else if ((current->type)==TRANSFER) refresh_transfer_screen(current->screen);
	else if (current->type==LIST) refresh_list_screen(current->screen);

}

void set_update_status(screen *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_update_status(screen *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}

int update_status(screen *S){
	if (S != NULL) return (S->update);
	return(0);
}

void set_screen_update_status(screen *scr, int update){
        if (scr->type==SERVER) set_server_update_status(scr->screen, update);
        else if (scr->type==TRANSFER) set_transfer_update_status(scr->screen, update);
        else if (scr->type==LIST) set_list_update_status(scr->screen, update);
        else if (scr->type==CHANNEL) set_channel_update_status(scr->screen, update);
        else if (scr->type==CHAT) set_chat_update_status(scr->screen, update);
        else if (scr->type==DCCCHAT) set_dccchat_update_status(scr->screen, update);
}
              
screen *select_next_screen(screen *currentscreen){
        screen *nextscreen;

	nextscreen = currentscreen;
	while(1){
		nextscreen = nextscreen->next;
		if (nextscreen == NULL) nextscreen = screenlist;
		if (nextscreen->hidden == 0) break;
	}
        
	refresh_screen(nextscreen);
        set_screen_update_status(nextscreen, U_ALL_REFRESH);
        set_inputline_update_status(inputline, U_ALL_REFRESH);
        set_menuline_update_status(menuline, U_ALL_REFRESH);   
        set_statusline_update_status(statusline, U_ALL_REFRESH);
	return(nextscreen);
}

screen *select_prev_screen(screen *currentscreen){
        screen *current, *nextscreen;

	nextscreen = currentscreen;
	while(1){
		nextscreen = nextscreen->prev;
	        if (nextscreen == NULL){
	                current = screenlist;
	                while(current != NULL){
	                        if (current->next == NULL) nextscreen = current;
	                        current = current->next;
			}
		}
		if (nextscreen->hidden == 0) break;		
	}

        refresh_screen(nextscreen);
        set_screen_update_status(nextscreen, U_ALL_REFRESH);
        set_inputline_update_status(inputline, U_ALL_REFRESH);  
        set_menuline_update_status(menuline, U_ALL_REFRESH);
        set_statusline_update_status(statusline, U_ALL_REFRESH);
	return(nextscreen);
}

screen *select_screen(screen *screen){
        // currentmenusline->current = -1;
        if (currentscreen->type == CHANNEL) ((channel *)(currentscreen->screen))->selecting = 0;

        refresh_screen(screen);
        set_screen_update_status(screen, U_ALL_REFRESH);
        set_inputline_update_status(inputline, U_ALL_REFRESH);  
        set_menuline_update_status(menuline, U_ALL_REFRESH);
        set_statusline_update_status(statusline, U_ALL_REFRESH);
	
	return(screen);
}

int process_screen_events(screen *screen, int key){

	/* take care of the page scrolling keys */
	/* scroll forward or (control-f) */
	if (key == KEY_SF || key == KEY_A1 || key == 6){
		scroll_message_screen(screen, +1);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(E_NONE);
	}
	/* scroll back or (control-r) */
	else if (key == KEY_SR || key == KEY_C1 || key == 18){
		scroll_message_screen(screen, -1);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(E_NONE);
	}
	/* scroll back a page or (control-y) */
	else if (key == KEY_PPAGE || key == KEY_A3 || key == 25){
		scroll_message_screen(screen, 4-LINES);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(E_NONE);
	}
	/* scroll forward a page or (control-v) */
	else if (key == KEY_NPAGE || key == KEY_C3 || key == 22){
		scroll_message_screen(screen, LINES-4);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(E_NONE);
	}

	// if the key is esc disable screen scrolling
	else if (key==27){
		set_message_scrolling(screen, 0);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(E_NONE);		
	}

	// if the key is enter disable screen scrolling but return the key
	else if (key==KEY_ENTER || key==10){
		set_message_scrolling(screen, 0);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(key);		
	}
	else if (key == 2 || key == 31 || key == 15 || key == 3 || key >= 0x20){
		set_message_scrolling(currentscreen, 0);
		set_screen_update_status(screen, U_MAIN_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
		return(key);		
	}
	return(key);
}                



/* server *************************************************************************************************/


int create_server_screen(server *S){
	if (S==NULL) return (0);

	S->message = newpad(BUFFERLINES, COLS);
	scrollok(S->message, TRUE);
	wclear(S->message);
	touchwin(S->message);
	
	// box(S->message,0,0);
	return(1);

}

int delete_server_screen(server *S){
	if (S==NULL) return (0);
	delwin(S->message);
	return(1);
}

server *add_server(char *servername, int port, char *nick, char *user, char *host, char *domain, char *name){
	server *new;

	new=(server *)malloc(sizeof(server));
	if (new==NULL){
		exit(1);
	}
	strcpy(new->server, servername);
	new->port = port;
	new->update = 0;
	new->connect_status = -1;
	new->active = 0;
	strcpy(new->nick, nick);
	strcpy(new->user, user);
	strcpy(new->host, host);
	strcpy(new->domain, domain);
	strcpy(new->name, name);
	currentscreen = add_screen((void*)new, SERVER);
	create_server_screen(new);
	return(new);
}

int redraw_server_screen(server *S){
	if (S==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(S->message, BUFFERLINES, COLS);
	#endif
	touchwin(S->message);
	return(0);
}

void refresh_server_screen(server *S){
	touchwin(S->message);
	wrefresh(S->message);
}

void print_server(server *S, char *buffer){
	if (S!=NULL) print_screen_opt(S->message, buffer, COLS-2, 0, A_NORMAL, O_ALL);
	set_server_update_status(S, U_MAIN_REFRESH);
}

void print_server_attrib(server *S, char *buffer, int attrib){
	if (S!=NULL) print_screen_opt(S->message, buffer, COLS-2, 0, attrib, O_ALL);
	set_server_update_status(S, U_MAIN_REFRESH);
}

void vprint_server_attrib(server *S, int attrib, char *template, ...){
	va_list ap;
	int i, length;
	int currposx, currposy;
	char stringt[MAXDATASIZE];

	va_start(ap, template);
	vsprintf(stringt, template, ap);
	va_end(ap);

	print_server_attrib(S, stringt, attrib);
}

void vprint_server(server *S, char *template, ...){
	va_list ap;
	int i, length;
	int currposx, currposy;
	char string[MAXDATASIZE];

	va_start(ap, template);
	vsprintf(string, template, ap);
	va_end(ap);

	print_server(S, string);
}

int server_update_status(server *S){
	if (S != NULL) return (S->update);
	return(0);
}

void set_server_update_status(server *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_server_update_status(server *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}


/* channel *************************************************************************************************/


int create_channel_screen(channel *C){
	if (C==NULL) return (0);

	C->message = newpad(BUFFERLINES, COLS-USERWINWIDTH-1);
	C->user = newwin(LINES-3, USERWINWIDTH, 1, COLS-USERWINWIDTH);
	C->vline = newwin(LINES-3, 1, 1, COLS-USERWINWIDTH-1); 
	scrollok(C->message, TRUE);
	scrollok(C->user, FALSE);
	wattron(C->vline, A_REVERSE | MENU_COLOR);
	wclear(C->message);
	wclear(C->user);
	mvwvline(C->vline, 0, 0, ' ', LINES-3);
	return(1);
}

int delete_channel_screen(channel *C){	 
	if (C==NULL) return (0);
	delwin(C->message);
	delwin(C->user);
	delwin(C->vline);
	return(1);
}

channel *add_channel(char *channelname, server *server){
	channel *new;

	new=(channel *)calloc(sizeof(channel), 1);
	if (new==NULL){
		exit(1);
	}
	strcpy(new->channel, channelname);
	new->server=server;
	new->update = 0;
	strcpy(new->topic, "");
	new->userlist=NULL;
	new->top=NULL;
	new->selected=NULL;
	new->selecting = 0;
	currentscreen = add_screen((void*)new, CHANNEL);
	create_channel_screen(new);
	return(new);
}

void refresh_channel_screen(channel *C){
	touchwin(C->message);
	touchwin(C->user);
	touchwin(C->vline);
	wrefresh(C->message);
	wrefresh(C->user);
	wrefresh(C->vline);
}

int redraw_channel_screen(channel *C){
	if (C==NULL) return (0);

	#ifdef RESIZE_CAPABLE
	wresize(C->message, BUFFERLINES, COLS-USERWINWIDTH-1);
	wresize(C->user, LINES-3, USERWINWIDTH);
	wresize(C->vline, LINES-3, 1);
	#endif
	mvwin(C->user, 1, COLS-USERWINWIDTH);
	mvwin(C->vline, 1, COLS-USERWINWIDTH-1); 
	wattron(C->vline, A_REVERSE | MENU_COLOR);
	mvwvline(C->vline, 0, 0, ' ', LINES-3);

	refresh_user_list(C);
	touchwin(C->message);
	touchwin(C->user);
	touchwin(C->vline);
	return(1);
}

void printmsg_channel(channel *C, char *nick, char *buffer){
	char temp[MAXDATASIZE];

	if (C != NULL){
		// Print the message. If this is the current channel refresh its window
		sprintf(temp, "%c<%s>%c %s\n", 2, nick, 2, buffer);
		print_screen_opt(C->message, temp, COLS-USERWINWIDTH-3, 5, A_NORMAL, O_ALL);
		set_channel_update_status(C, U_MAIN_REFRESH);
	}
}


void print_channel(channel *C, char *buffer){
	if (C != NULL){
		print_screen_opt(C->message, buffer, COLS-USERWINWIDTH-3, 0, A_NORMAL, O_ALL);
		set_channel_update_status(C, U_MAIN_REFRESH);
	}
}

void print_channel_attrib(channel *C, char *buffer, int attrib){
	if (C != NULL){
		print_screen_opt(C->message, buffer, COLS-USERWINWIDTH-3, 0, attrib, O_ALL);
		set_channel_update_status(C, U_MAIN_REFRESH);
	}
}

void printmymsg_channel(channel *C, char *buffer){
	if (C != NULL){
		printmsg_channel(C, ((server *)(C->server))->nick, buffer);
	}
}

void vprint_channel_attrib(channel *C, int attrib, char *template, ...){
        va_list ap;
        int i, length;
        int currposx, currposy;
        char stringt[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(stringt, template, ap);
        va_end(ap);

	print_channel_attrib(C, stringt, attrib);
}

void vprint_channel(channel *C, char *template, ...){
        va_list ap;
        int i, length;
        int currposx, currposy;
        char string[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(string, template, ap);
        va_end(ap);

	print_channel(C, string);
}

void refresh_user_list(channel *C){
	user *current;
	char scratch[MAXNICKDISPLEN+3];
	int pos, i;

	werase(C->user);
	touchwin(C->user);
	current = C->top;
	
	pos = 0;	
	while(current!=NULL && pos <= LINES){
		if (current->op) strcpy(scratch, " @");
		else if (current->voice) strcpy(scratch, " +");
		else strcpy(scratch, " ");

		strncat(scratch, current->nick, MAXNICKDISPLEN);
		for (i=strlen(scratch); i<=MAXNICKDISPLEN; i++) scratch[i] = ' ';
		scratch[i] = 0;
		strcat(scratch, " ");
		if (current == C->selected && C->selecting){
        		wattron(C->user, A_REVERSE);
        		wattron(C->user, A_BOLD);
			// sprintf(scratch, " %-10s", current->nick);
			mvwaddstr(C->user, pos, 0, scratch);
        		wattrset(C->user, A_NORMAL);
		}
		else{
			// sprintf(scratch, " %-10s", current->nick);
			mvwaddstr(C->user, pos, 0, scratch);
		} 
		current=current->next;
		pos++;
	}
}

int user_win_offset(channel *C){
	user *current;
	int pos;

	current = C->top;	
	pos = 0;	
	while(current != NULL && current != C->selected){
		pos++;
		current = current->next;	
	}
	return(pos);
}
	
void end_channel(channel *C){
	// if (C->active) sendcmd_server(C->server, "PART", "", C->channel, (C->server)->nick);
	remove_all_users(C);
	delete_channel_screen(C);
	free (C);
}

int add_user(channel *C, char *nick, int op, int voice){
	user *current, *last, *new;
	char tempnick[MAXNICKLEN + 3];
	char heldnick[MAXNICKLEN + 3] ;
	char *nickA;
	char *nickB;

	int order;
	int num, topnum, selectednum, insertednum;
	int lower, higher, lorank, hirank;

	if (C == NULL) return(0);

	new = calloc(sizeof(user), 1);
	if (new==NULL){
		plog ("Cannot allocate userlist memory in add_user(:1)");
		exit (0);
	}

	if (nick[0] == '@' || nick[0] == '+') strcpy(new->nick, nick + 1);
	else strcpy(new->nick, nick);
	new->op = op;
	new->voice = voice;
	if (nick[0] == '@') new->op |= 1;
	else if (nick[0] == '+') new->voice |= 1;

	if (new->op) heldnick[0] = 1;
	else if (new->voice) heldnick[0] = 2;
	else heldnick[0] = ' ';
	heldnick[1] = 0;
	strcat(heldnick, new->nick);

	if (C->userlist == NULL){
		new->prev = NULL;
		new->next = NULL;
		C->userlist = new;
		C->top = new;
		C->selected = new;
		// vprint_all("starting new list with %s\n", heldnick);
		set_channel_update_status(C, U_USER_REFRESH);
		return(1);
	}
	else {
		num = 0;
		current = C->userlist;
		while(current != NULL){
			lower = 0;
			higher = 0;

			/* compare new to the previous and next entry */
			if (current->prev == NULL) lower = 1;
			else {
				if (current->prev->op) tempnick[0] = 1;
				else if (current->prev->voice) tempnick[0] = 2;
				else tempnick[0] = ' ';
				tempnick[1] = 0;
				strcat(tempnick, current->prev->nick);
				if (strcmp(heldnick, tempnick) > 0) lower = 1;

			}

			if (current->op) tempnick[0] = 1;
			else if (current->voice) tempnick[0] = 2;
			else tempnick[0] = ' ';
			tempnick[1] = 0;
			strcat(tempnick, current->nick);
			if (strcmp(heldnick, tempnick) < 0) higher = 1;

			/* if the new entry should lie between the previous and current entry, insert it there */
			if (lower && higher){
				if (current->prev != NULL){ 
					(current->prev)->next = new;
					new->prev = current->prev;
					// vprint_all("inserting between %s and %s\n", current->prev->nick, current->nick);
				}
				else {
					new->prev = NULL;
					C->userlist = new;
					// vprint_all("inserting at start before %s\n", current->nick);
				}

				new->next = current;
				current->prev = new;
				insertednum = num;
				break;
			}
			last = current;

			current = current->next;
			num++;
		}

		/* if current is NULL, the new entry belongs at the end of the list */
		if (current == NULL){
			new->prev = last;
			new->next = NULL;
			last->next = new;
			insertednum = num;
			// vprint_all("inserting at end after %s\n", last->nick);
		}

		num = 0;
		topnum = 0;
		selectednum = 0;

		/* count the position of the selected and top entry */
		current = C->userlist;
		while(current != NULL){
			if (current == C->top) topnum = num; 
			if (current == C->selected) selectednum = num; 
			current = current->next;
			num++;
		}

		/* if top and selected entries appear before the new entry, do nothing */
		// if (selectednum <= insertednum && topnum <= insertednum) {}

		/* if selected entry is above the top entry, move top to selected */  
		if (topnum > selectednum) C->top = C->selected;

		/* if selected entry is below the visible area, move the top down */
		else if (selectednum >= topnum + LINES - 3 && C->top != NULL){
			if (C->top->next != NULL) C->top = C->top->next;
		}
	}

	set_channel_update_status(C, U_USER_REFRESH);
	return(1);
}

int remove_user(channel *C, char *nick){
	user *current, *next, *previous;

	if (C == NULL) return(0);
	current=C->userlist;
	while (current!=NULL){
		//if (strcmp(current->nick, nick)==0 || ((current->nick[0]=='@' || current->nick[0]=='+') && 
		//	strcmp(current->nick + 1, nick)==0)){

		if (strcmp(current->nick, nick) == 0){
			previous = current->prev;
			next = current->next;
          
			if (next != NULL) next->prev = previous;
			if (previous != NULL) previous->next = next;

			if (C->userlist == current) C->userlist = next;
			if (current == C->top) C->top = (C->top)->prev;
			if (C->top == NULL) C->top = C->userlist;
			if (current == C->selected) C->selected = (C->selected)->prev;	  
			if (C->selected == NULL) C->selected = C->userlist;
        		free (current);
			set_channel_update_status(C, U_USER_REFRESH);
			return(1);
		}
		current=current->next;
	}
	return(0);
}

char get_user_status(channel *C, char *nick, int *op, int *voice){
	user *current;
	char retval;

	if (C == NULL || nick == NULL) return(0);
	current=C->userlist;
	*op = 0;
	*voice = 0;

	while (current!=NULL){
		//if (strcmp(current->nick, nick)==0) return (0);

		//else if (current->nick[0]=='@' && strcmp(current->nick + 1, nick)==0) return ('@');
		//else if (current->nick[0]=='+' && strcmp(current->nick + 1, nick)==0) return ('+');			
		
		if (strcmp(current->nick, nick) == 0){
			retval = 0;
			if (current->voice){
				*voice = 1;
				retval = '+';
			}
			if (current->op){
				*op = 1; 
				retval = '@';
			}
			return (retval);
		}
		current=current->next;
	}
	return(0);
}

int change_user_status(channel *C, char *nick, char *mode){
	char newnick[MAXNICKLEN];
	char currmode;
	int voice;
	int op;

	if (C == NULL || nick == NULL) return(0);

	currmode = get_user_status(C, nick, &op, &voice);	
	if (remove_user(C, nick)){
		if (strcmp(mode, "+o") == 0) op = 1;
		else if (strcmp(mode, "-o") == 0) op = 0;
		else if (strcmp(mode, "+v") == 0) voice = 1;
		else if (strcmp(mode, "-v") == 0) voice = 0;
		add_user(C, nick, op, voice);
		return(1);
	}
	return(0);
}	
			

void quit_user(char *nick){
        screen *current;
                                
        current=screenlist;
        while(current!=NULL){
                if (current->type==CHANNEL) remove_user (current->screen, nick);
                current=current->next;
        }
}

void remove_all_users(channel *C){
	user *current, *next;

	current=C->userlist;
	while (current!=NULL){
		next=current->next;
        	free (current);
		current=next;
	}
	set_channel_update_status(C, U_USER_REFRESH);
}

void select_next_user(channel *C){
        user *current, *topuser;
	int topnum, i;

	topnum = -1;
	i = 0;

	/* select the next user */
	if ((C->selected)->next != NULL){
		C->selected = C->selected->next;
	}

	/* make sure that the window dimensions still display the user */
        
	current = C->userlist;
        topuser = C->top;
	if (topuser == NULL) topuser = C->userlist; 

	while(current!=NULL){
		if (current == C->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((C->top)->next != NULL) C->top = (C->top)->next;
			topnum++;
		}
                if (current == C->selected){
			if (topnum == -1) C->top = C->selected;
			break;
		}
                current=current->next;
		i++; 
        }
	set_channel_update_status(C, U_USER_REFRESH);
}

void select_prev_user(channel *C){
        user *current, *topuser;
	int topnum, i;

	topnum = -1;
	i = 0;

	/* select the previous user */
	if ((C->selected)->prev != NULL){
		C->selected = C->selected->prev;
	}

	/* make sure that the window dimensions still display the user */
      
	current = C->userlist;
        topuser = C->top;
	if (topuser == NULL) topuser = C->userlist; 

	while(current!=NULL){
		if (current == C->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((C->top)->prev != NULL) C->top = (C->top)->prev;
			topnum++;
		}
                if (current == C->selected){
			if (topnum == -1) C->top = C->selected;
			break;
		}
                current=current->next;
		i++; 
        }
	set_channel_update_status(C, U_USER_REFRESH);

}

void select_next_user_by_key(channel *C, int key){
        user *current, *topuser;
	int found, topnum, i;

	topnum = -1;
	i = 0;


	/* find the user that starts with character key */
	if (C->selected->next != NULL) current = C->selected->next;
	else current = C->userlist;

	found = 0;
	while (current != NULL){
		if (key == current->nick[0]){
			found = 1;
			C->selected = current;
			break;
		}
		current = current->next;
	}
	/* if match not found, start again from top */ 
	if (!found){
		current = C->userlist;
		while (current != NULL){
			if (key == current->nick[0]){
				found = 1;
				C->selected = current;
				break;
			}
			current = current->next;
		}
	}	

	/* make sure that the window dimensions still display the user */
        
	current = C->userlist;
        topuser = C->top;
	if (topuser == NULL) topuser = C->userlist; 

	while(current!=NULL){
		if (current == C->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((C->top)->next != NULL) C->top = (C->top)->next;
			topnum++;
		}
                if (current == C->selected){
			if (topnum == -1) C->top = C->selected;
			break;
		}
                current=current->next;
		i++; 
        }
	set_channel_update_status(C, U_USER_REFRESH);
}

char *selected_channel_nick(channel *C){
	if (C != NULL){
		if ((C->selected) != NULL){
			return((C->selected)->nick);
		}
	}
	return(NULL);
}



int channel_update_status(channel *S){
	if (S != NULL) return (S->update);
	return(0);
}

void set_channel_update_status(channel *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_channel_update_status(channel *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}


/* chat ****************************************************************************************************/

int create_chat_screen(chat *C){
	if (C==NULL) return (0);

	C->message = newpad(BUFFERLINES, COLS);
	scrollok(C->message, TRUE);
	wclear(C->message);
	return(1);
}

int delete_chat_screen(chat *C){
	if (C==NULL) return (0);
	delwin(C->message);
	return(1);
}

int redraw_chat_screen(chat *C){
	if (C==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(C->message, BUFFERLINES, COLS);
	#endif
	touchwin(C->message);
	return(1);
}

void refresh_chat_screen(chat *C){
        touchwin(C->message);
        wrefresh(C->message); 
}


chat *add_chat(char *chatname, server *server){
        chat *new;
                        
        new=(chat *)calloc(sizeof(chat), 1);
        if (new==NULL){
                exit(1);
        }
        new->server=server;
	new->update = 0;
        strcpy(new->nick, chatname);
        currentscreen = add_screen((void*)new, CHAT);
	create_chat_screen(new);
        return(new);
}

void end_chat(chat *C){
	delete_chat_screen(C);
	free (C);
}


void print_chat(chat *C, char *buffer){
        if (C!=NULL){
		print_screen_opt(C->message, buffer, COLS-2, 0, A_NORMAL, O_ALL);
		set_chat_update_status(C, U_MAIN_REFRESH);
	}
}

void print_chat_attrib(chat *C, char *buffer, int attrib){
        if (C!=NULL){
		print_screen_opt(C->message, buffer, COLS-2, 0, attrib, O_ALL);
		set_chat_update_status(C, U_MAIN_REFRESH);
	}
}

void printmsg_chat(chat *C, char *nick, char *buffer){
	char scratch[MAXDATASIZE];
	if (C!=NULL){
		sprintf(scratch, "%c<%s>%c %s\n", 2, nick, 2, buffer);
		print_screen_opt(C->message, scratch, COLS-2, 5, A_NORMAL, O_ALL);
		set_chat_update_status(C, U_MAIN_REFRESH);
	}
}

void printmymsg_chat(chat *C, char *buffer){
	if (C!=NULL){
		printmsg_chat(C, ((server *)(C->server))->nick, buffer);
	}
}

void vprint_chat_attrib(chat *C, int attrib, char *template, ...){
	va_list ap;
	int i, length;
	int currposx, currposy;
	char stringt[MAXDATASIZE];

	va_start(ap, template);
	vsprintf(stringt, template, ap);
	va_end(ap);

	print_chat_attrib(C, stringt, attrib);
}

void vprint_chat(chat *C, char *template, ...){
        va_list ap;
        int i, length;
        int currposx, currposy;
        char string[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(string, template, ap);
        va_end(ap);

	print_chat(C, string);
}

int chat_update_status(chat *S){
	if (S != NULL) return (S->update);
	return(0);
}

void set_chat_update_status(chat *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_chat_update_status(chat *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}

/* dcc chat ************************************************************************************************/

int create_dccchat_screen(dcc_chat *D){
        if (D==NULL) return (0);
        D->message = newpad(BUFFERLINES, COLS);
        scrollok(D->message, TRUE);
        wclear(D->message);
        return(1);
}

int delete_dccchat_screen(dcc_chat *D){
        if (D==NULL) return (0);
        delwin(D->message);
        return(1);
}

dcc_chat *add_incoming_dcc_chat(char *nick, char *dest, server *s, unsigned long hostip, unsigned short port){
	dcc_chat *new;

	new=(dcc_chat *)calloc(sizeof(dcc_chat), 1);
	if (new==NULL){
		exit(1);
	}
	strcpy(new->nick, nick);
	strcpy(new->dest, dest);
	new->hostip=hostip;
	new->port=port;
	new->type=DCC_CHAT;
	new->update = 0;
	new->serverstatus = 0;
	new->server = s;
	new->allowed = 0;
	new->direction = DCC_RECEIVE;
	currentscreen = add_screen((void*)new, DCCCHAT);
	create_dccchat_screen(new);
	return(new);
}

dcc_chat *add_outgoing_dcc_chat(char *nick, char *dest, server *s){
	dcc_chat *new;

	new=(dcc_chat *)calloc(sizeof(dcc_chat), 1);
	if (new==NULL){
		exit(1);
	}
	strcpy(new->nick, nick);
	strcpy(new->dest, dest);
	new->hostip=0;
	new->port=0;
	new->type=DCC_CHAT;
	new->update = 0;
	new->serverstatus = 0;
	new->server = s;
	new->allowed = 1;
	new->direction = DCC_SEND;
	currentscreen = add_screen((void*)new, DCCCHAT);
	create_dccchat_screen(new);
	return(new);
}

void refresh_dccchat_screen(dcc_chat *C){
	touchwin(C->message);
	wrefresh(C->message);
}


int redraw_dccchat_screen(dcc_chat *D){
	if (D==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(D->message, BUFFERLINES, COLS);
	#endif
	touchwin(D->message);
	return(0);
}

void printmsg_dcc_chat(dcc_chat *C, char *nick, char *buffer){
	char scratch[MAXDATASIZE];					
	if (C!=NULL){
		sprintf(scratch, "%c<%s>%c %s\n", 2, nick, 2, buffer);
		print_screen_opt(C->message, scratch, COLS-2, 5, A_NORMAL, O_ALL);
		set_dccchat_update_status(C, U_MAIN_REFRESH);
	}
}

void printmymsg_dcc_chat(dcc_chat *D, char *buffer){
	if (D!=NULL){
		 printmsg_dcc_chat(D, D->dest, buffer);
	}
}

void print_dcc_chat(dcc_chat *C, char *buffer){
	if (C!=NULL){
		print_screen_opt(C->message, buffer, COLS-2, 0, A_NORMAL, O_ALL);
		set_dccchat_update_status(C, U_MAIN_REFRESH);
	}
}

void print_dcc_chat_attrib(dcc_chat *C, char *buffer, int attrib){
	if (C!=NULL){
		print_screen_opt(C->message, buffer, COLS-2, 0, attrib, O_ALL);
		set_dccchat_update_status(C, U_MAIN_REFRESH);
	}
}

void vprint_dcc_chat_attrib(dcc_chat *C, int attrib, char *template, ...){
	va_list ap;
	int i, length;
	int currposx, currposy;
	char stringt[MAXDATASIZE];

	va_start(ap, template);
	vsprintf(stringt, template, ap);
	va_end(ap);

	print_dcc_chat_attrib(C, stringt, attrib);
}

void vprint_dcc_chat(dcc_chat *C, char *template, ...){
        va_list ap;
        int i, length;
        int currposx, currposy;
        char string[MAXDATASIZE];

        va_start(ap, template);
        vsprintf(string, template, ap);
        va_end(ap);

	print_dcc_chat(C, string);
}

void end_dccchat(dcc_chat *D){
	close(D->dccfd);
	delete_dccchat_screen(D);
	free (D);
}

void disconnect_dccchat(dcc_chat *D){
	close(D->dccfd);
	D->active = 0;	
	print_dcc_chat(D, "Disconnected.\n");
}

int dccchat_update_status(dcc_chat *S){
	if (S != NULL) return (S->update);
	return (0);
}

void set_dccchat_update_status(dcc_chat *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_dccchat_update_status(dcc_chat *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}

/* transfer **********************************************************************************************/



int create_transfer_screen(transfer *T){
	if (T==NULL) return (0);
	T->message = newwin(LINES-3, COLS, 1, 0);
	werase(T->message);
	return(0);
}

transfer *add_transfer(char *name){
	transfer *new;

	new=(transfer *)calloc(sizeof(transfer), 1);
	if (new==NULL){
		exit(1);
	}
	new->dcclist = NULL;
	new->dcclisttop = NULL;
	strcpy(new->name, name);
	add_screen((void*)new, TRANSFER);
	create_transfer_screen(new);
	return(new);
}

int redraw_transfer_screen(transfer *T){
	if (T==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(T->message, LINES-3, COLS);
	#endif
	touchwin(T->message);
	return(0);
}

void refresh_transfer_screen(transfer *T){
	if (T==NULL) return;
	touchwin(T->message);
	wrefresh(T->message);
}

int transfer_update_status(transfer *S){
	if (S != NULL) return (S->update);
	return(0);
}

void set_transfer_update_status(transfer *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_transfer_update_status(transfer *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}



/* channel list ******************************************************************************************/

int create_list_screen(list *L){
	if (L==NULL) return (0);
	L->message = newwin(LINES-3, COLS, 1, 0);
        scrollok(L->message, FALSE);
        wclear(L->message);
	return(1);
}

list *add_list(server *server){
	list *new;

	new=(list *)calloc(sizeof(list), 1);
	if (new==NULL){
		exit(1);
	}
	new->active = 0;
	new->list = NULL;
	new->top = NULL;
	new->selected = NULL;
	new->server = server;
	new->usinglist = LIST_SORT_CHANNEL;
	strcpy(new->servername, server->server);
	currentscreen = add_screen((void*)new, LIST);
	create_list_screen(new);
	set_list_update_status(new, U_ALL);
	return(new);
}

int redraw_list_screen(list *L){
	if (L==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(L->message, LINES-3, COLS);
	#endif
	touchwin(L->message);
	return(0);
}

void refresh_channel_list(list *L){
	list_channel *current;
	char scratch[MAXDATASIZE];
	int i, posy, posx;

	werase(L->message);
	touchwin(L->message);
	current = L->top;
	
	posy = 0;	
	while(current != NULL && posy < LINES - 3){
		strcpy(scratch, current->channel);
		if (current == L->selected){
			
			// print channel name and fill gaps
			posx = print_list_pos_attrib(L, scratch, 0, posy, A_REVERSE, O_COLOR | O_REVERSE);
        		if (posx < 24){
				for (i = 0; i < 24 - posx; i++) scratch[i] = ' ';
				scratch[i] = 0;
				posx = print_list_pos_attrib(L, scratch, posx, posy, A_REVERSE, O_COLOR | O_REVERSE);
			}
			else {
				posx = print_list_pos_attrib(L, ".. ", 21, posy, A_REVERSE, O_COLOR | O_REVERSE);
			}
			
			// print number of users
			sprintf(scratch, "%6d  ", current->users);
			posx = print_list_pos_attrib(L, scratch, 24, posy, A_REVERSE, O_COLOR | O_REVERSE);				

			// channel description
			posx = print_list_pos_attrib(L, current->description, 32, posy, A_REVERSE, O_COLOR | O_REVERSE);
			
			//select bar gap filler
			for (i = 0; i < COLS - 32 - posx; i++) scratch[i] = ' ';
			scratch[i] = 0;
			posx = print_list_pos_attrib(L, scratch, 32 + posx, posy, A_REVERSE, O_COLOR | O_REVERSE);

		}
		else{
			// print channel name
			posx = print_list_pos_attrib(L, scratch, 0, posy, A_NORMAL, O_NOMONOCHROME | O_COLOR);
			if (posx >= 24){
				posx = print_list_pos_attrib(L, ".. ", 21, posy, A_NORMAL, O_NOMONOCHROME | O_COLOR);
			}

			// print number of users
			sprintf(scratch, "%6d  ", current->users);
			posx = print_list_pos_attrib(L, scratch, 24, posy, A_NORMAL, O_NOMONOCHROME | O_COLOR);
	
			// channel description
			posx = print_list_pos_attrib(L, current->description, 32, posy, A_NORMAL, O_NOMONOCHROME | O_COLOR);

		} 
		current = current->viewnext;
		posy++;
	}
	set_list_update_status(L, U_ALL_REFRESH);
}

int add_list_channel(list *L, char *channel, int users, char *description, int type){
	list_channel *current, *last, *new;
	int topnum, selectednum, i;

	if (L == NULL) return(0);
	new = calloc(sizeof(list_channel), 1);
	if (new == NULL){
		plog ("Cannot allocate channel list memory in add_list_channel(:1)");
		exit (0);
	}
	strcpy(new->channel, channel);
	strcpy(new->description, description);
	new->users = users;

	// unsorted
	if (type == 0){
		if (L->alphalist != NULL) (L->alphalist)->alphaprev = new;
		if (L->userlist != NULL) (L->userlist)->userprev = new;
		new->alphaprev = NULL;
		new->alphanext = L->list;
		new->userprev = NULL;
		new->usernext = L->list;
		new->viewprev = NULL;
		new->viewnext = L->list;
		L->view = new;
		L->list = new;
		L->top = new;
		L->selected = new;
		set_list_update_status(L, U_ALL_REFRESH);
	}

	// sorted
	if (type == 1){
		// beginning of list;
		if (L->alphalist == NULL || L->userlist == NULL){
			new->alphaprev = NULL;
			new->alphanext = NULL;
			new->userprev = NULL;
			new->usernext = NULL;
			new->viewprev = NULL;
			new->viewnext = NULL;
			L->view = new;
			L->list = new;
			L->alphalist = new;
			L->userlist = new;
			L->top = new;
			L->selected = new;
			set_list_update_status(L, U_ALL_REFRESH);
			return(1);
		}

		/* insert into alphabetical (main) list */
		current = L->alphalist;
		last = L->alphalist;

		i = 0;
		selectednum = -1;
		topnum = -1;

		while (current != NULL){
			if (current == L->selected) selectednum = i;
			else if (current == L->top) topnum = i;

			if (strcmp(current->channel, channel) > 0){
				new->alphaprev = current->alphaprev;
				new->alphanext = current;
				if (current->alphaprev != NULL) (current->alphaprev)->alphanext = new;

				if (L->usinglist == LIST_SORT_CHANNEL){		
					new->viewprev = current->alphaprev;
					new->viewnext = current;
					if (current->alphaprev != NULL) (current->alphaprev)->viewnext = new;
					current->viewprev = new;
					if (L->view == current) L->view = new;
					if (L->top == current) L->top = new;
					if (L->selected == current) L->selected = new;
					if (is_channel_in_list_view(L, new)) set_list_update_status(L, U_ALL_REFRESH);
					if (topnum >= 0 && selectednum == -1 && L->top->viewnext != NULL) L->top = L->top->viewnext;
				}

				current->alphaprev = new;
				if (L->alphalist == current) L->alphalist = new;

				break;
			}
			last = current;
			current = current->alphanext;
			i++;
		}

		/* end of alpha list */
		if (current == NULL){
			last->alphanext = new;
			new->alphaprev = last;
			new->alphanext = NULL;

			if (L->usinglist == LIST_SORT_CHANNEL){
				last->viewnext = new;
				new->viewprev = last;
				new->viewnext = NULL;		
				if (is_channel_in_list_view(L, new)) set_list_update_status(L, U_ALL_REFRESH);
			}
		}

		/* insert into user sorted list */
		current = L->userlist;
		last = L->userlist;
		i = 0;
		while (current != NULL){
			if (current == L->selected) selectednum = i;
			else if (current == L->top) topnum = i;

			if (current->users > users){
				new->userprev = current->userprev;
				new->usernext = current;
				if (current->userprev != NULL) (current->userprev)->usernext = new;

				if (L->usinglist == LIST_SORT_USERS){		
					new->viewprev = current->userprev;
					new->viewnext = current;
					if (current->userprev != NULL) (current->userprev)->viewnext = new;
					current->viewprev = new;
					if (L->view == current) L->view = new;
					if (L->top == current) L->top = new;
					if (L->selected == current) L->selected = new;
					if (is_channel_in_list_view(L, new)) set_list_update_status(L, U_ALL_REFRESH);
					if (topnum >= 0 && selectednum == -1 && L->top->viewnext != NULL) L->top = L->top->viewnext;
				}
				current->userprev = new;
				if (L->userlist == current) L->userlist = new;

				break;
			}
			last = current;
			current = current->usernext;
			i++;
		}
		if (current == NULL){
			last->usernext = new;
			new->userprev = last;
			new->usernext = NULL;

			if (L->usinglist == LIST_SORT_USERS){
				last->viewnext = new;
				new->viewprev = last;
				new->viewnext = NULL;		
				if (is_channel_in_list_view(L, new)) set_list_update_status(L, U_ALL_REFRESH);
			}
		}
	}
	return(1);
}

int is_channel_in_list_view(list *L, list_channel *channel){
	list_channel *current;
	int counter;

	if (L == NULL) return(0);
	current = L->top;
	counter = 0;

	while((current != NULL) && (counter < LINES - 3)){
		if (current == channel) return(1);
		counter++;					
		if (L->usinglist == LIST_SORT_USERS)	current = current->usernext;
		else current = current->alphanext;
	}
	return(0);
}

void select_next_list_channel(list *L){
        list_channel *current, *topchannel;
	int topnum, i;

	topnum = -1;
	i = 0;

	// select the next channel
	if (L->selected != NULL){
		if ((L->selected)->viewnext != NULL){
			L->selected = L->selected->viewnext;
		}
	}

	// make sure that the window dimensions still display the list
        
	current = L->view;
        topchannel = L->top;
	if (topchannel == NULL) topchannel = L->view; 

	while(current != NULL){
		if (current == L->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((L->top)->viewnext != NULL) L->top = (L->top)->viewnext;
			topnum++;
		}
                if (current == L->selected){
			if (topnum == -1) L->top = L->selected;
			break;
		}
                current = current->viewnext;
		i++; 
        }
	set_list_update_status(L, U_MAIN_REFRESH);
}


void select_prev_list_channel(list *L){
        list_channel *current, *topchannel;
	int topnum, i;

	topnum = -1;
	i = 0;

	// select the previous user
	if (L->selected != NULL){
		if ((L->selected)->viewprev != NULL){
			L->selected = L->selected->viewprev;
		}
	}

	// make sure that the window dimensions still display the user
        
	current = L->view;
        topchannel = L->top;
	if (topchannel == NULL && L->usinglist == LIST_SORT_USERS) topchannel = L->userlist; 
	else if (topchannel == NULL) topchannel = L->alphalist; 

	while(current!=NULL){
		if (current == L->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((L->top)->viewprev != NULL) L->top = (L->top)->viewprev;
			topnum++;
		}
                if (current == L->selected){
			if (topnum == -1) L->top = L->selected;
			break;
		}
                current=current->viewnext;
		i++; 
        }
	set_list_update_status(L, U_MAIN_REFRESH);

}

void select_next_list_channel_by_key(list *L, int key){
        list_channel *current, *topchannel;
	int topnum, i;
	int found;
	
	topnum = -1;
	i = 0;

	if (L == NULL) return;

	/* select the next channel name that starts with key */
	if (L->selected != NULL && (L->selected)->viewnext != NULL) current = L->selected->viewnext;
	else current = L->view;

	found = 0;
	while (current != NULL){
		if (key == current->channel[1]){
			found = 1;
			L->selected = current;
			break;
		}
		current = current->viewnext;
	}

	/* if the key is not found in the first scan scan once more from top */
	if (!found){
		current = L->view;
		while (current != NULL){
			if (key == current->channel[1]){
				found = 1;
				L->selected = current;
				break;
			}
			current = current->viewnext;
		}
	}

	/* make sure that the window dimensions still display the selected channel */
        
	current = L->view;
        topchannel = L->top;
	if (topchannel == NULL) topchannel = L->view; 

	while(current != NULL){
		if (current == L->top) topnum = i;
                if (topnum != -1 && i >= topnum + (LINES - 3)){
			if ((L->top)->viewnext != NULL) L->top = (L->top)->viewnext;
			topnum++;
		}
                if (current == L->selected){
			if (topnum == -1) L->top = L->selected;
			break;
		}
                current = current->viewnext;
		i++; 
        }
	set_list_update_status(L, U_MAIN_REFRESH);
}

void select_prev_list_channel_page(list *L){
	int i;
	for (i = 0; i < LINES - 4; i++) select_prev_list_channel(L);
}

void select_next_list_channel_page(list *L){
	int i;
	for (i = 0; i < LINES - 4; i++) select_next_list_channel(L);
}


void apply_list_view(list *L, char *search, int minusers, int maxusers, int sorttype){
        list_channel *current, *last, *next, *outer;
	int topnum, i, j, selected, searchon;

	topnum = -1;
	i = 0;

	if (L->list == NULL) return;
        wclear(L->message);

	/* now eliminate the non selected channels */
	if (strcmp(search, "") == 0) searchon = 0;
	else searchon = 1;

	L->view = NULL;
	L->top = NULL;
	L->selected = NULL;
	L->usinglist = sorttype;
	if (L->usinglist == LIST_SORT_USERS) current = L->userlist;
	else current = L->alphalist;

	last = NULL;
	while (current != NULL){
		if (current->users >= minusers || minusers == 0){
			if (current->users <= maxusers || maxusers == 0){
				if (searchon == 1){
					if (strstr(current->description, search) != NULL){
						if (L->view == NULL){
							L->view = current;
							L->top = current;
							L->selected = current;			
						}	
						if (last != NULL) last->viewnext = current;
						current->viewprev = last;
						current->viewnext = NULL;
						last = current;
					}
				}
				else {
					if (L->view == NULL){
						L->view = current;
						L->top = current;
						L->selected = current;			
					}
					if (last != NULL) last->viewnext = current;
					current->viewprev = last;
					current->viewnext = NULL;
					last = current;
				}
			}
		} 	
		if (L->usinglist == LIST_SORT_USERS) current = current->usernext;
		else current = current->alphanext;
	}
	set_list_update_status(L, U_MAIN_REFRESH);
}


char *selected_list_channel(list *L){
	if (L != NULL){
		if ((L->selected) != NULL){
			return((L->selected)->channel);
		}
	}
	return(NULL);
}

void refresh_list_screen(list *L){
	refresh_channel_list(L);
	touchwin(L->message);
	wrefresh(L->message);
}

void print_list(list *L, char *buffer){
	if (L!=NULL){
		print_screen_opt(L->message, buffer, COLS-3, 0, A_NORMAL, O_ALL);
		set_list_update_status(L, U_MAIN_REFRESH);
	}
}

int print_list_pos_attrib(list *L, char *buffer, int x, int y, int attrib, int options){
	int len;

	if (L!=NULL){
		wmove(L->message, y, x);
		len = print_screen_opt(L->message, buffer, COLS - 2, 0, attrib, options);
		set_list_update_status(L, U_MAIN_REFRESH);
		return(len);
	}
	return(0);
}

int list_update_status(list *L){
	return (L->update);
}

void set_list_update_status(list *L, int update){
	L->update = L->update | update;
}

void unset_list_update_status(list *L, int update){
	L->update = L->update & (0xffff ^ update);        
}

/* help *************************************************************************************************/


int create_help_screen(help *H){
	if (H==NULL) return (0);

	H->message = newpad(BUFFERLINES, COLS);
	scrollok(H->message, TRUE);
	wclear(H->message);
	touchwin(H->message);
	// box(H->message,0,0);
	return(1);

}

int delete_help_screen(help *H){
	if (H==NULL) return (0);
	delwin(H->message);
	return(1);
}

help *add_help(char *helpname, char *subname, char *filename){
	help *new;

	new=(help *)malloc(sizeof(help));
	if (new==NULL){
		exit(1);
	}
	strcpy(new->name, helpname);
	strcpy(new->subname, subname);
	currentscreen = add_screen((void*)new, HELP);
	create_help_screen(new);
	print_help_file(new, filename);
	set_help_update_status(new, U_ALL);
	return(new);
}

int redraw_help_screen(help *H){
	if (H==NULL) return (0);
	#ifdef RESIZE_CAPABLE
	wresize(H->message, BUFFERLINES, COLS);
	#endif
	touchwin(H->message);
	return(0);
}

void refresh_help_screen(help *H){
	touchwin(H->message);
	wrefresh(H->message);
}

void print_help(help *H, char *buffer){
	if (H!=NULL) print_screen_opt(H->message, buffer, COLS-2, 0, A_NORMAL, O_ALL);
	set_help_update_status(H, U_MAIN_REFRESH);
}

void print_help_attrib(help *H, char *buffer, int attrib){
	if (H!=NULL) print_screen_opt(H->message, buffer, COLS-2, 0, attrib, O_ALL);
	set_help_update_status(H, U_MAIN_REFRESH);
}

int help_update_status(help *H){
	return (H->update);
}

void set_help_update_status(help *H, int update){
	H->update = H->update | update;
}

void unset_help_update_status(help *H, int update){
	H->update = H->update & (0xffff ^ update);        
}

int print_help_file(help *H, char *filename){
	FILE *fp;
	char line[4096];
	char path[4096];

	strcpy(path, INSTALL_PATH);
	strcat(path, "/help/");
	strcat(path, filename);

	fp = fopen(path, "rb");

	/* if the default install path doesn't contain the help files, try current directory */	
	if (fp == NULL){
		strcpy(path, "./help/");
		strcat(path, filename);
		fp = fopen(path, "rb");
		if (fp == NULL){
			vprint_all_attrib(ERROR_COLOR, "Error displaying help file %s.\n", path);
			return(0);
		}
	}
	while(1){
		fgets(line, 4095, fp);
		if (ferror(fp)){
			fclose(fp);
			return(0);
		}
		else if (feof(fp)) break;
		print_help(H, line);
	}
	fclose(fp);
	return(1);
}
				

/* input and status **************************************************************************************/


inputwin *create_input_screen(){
	inputwin *new;
	WINDOW *W;

	new = malloc(sizeof(inputwin));
	W = newwin(1, COLS, LINES-1, 0);
	keypad(W, TRUE);
	intrflush(W, FALSE);
	meta(W, TRUE);
	touchwin(W);
	if (new != NULL){
		new->inputline = W;
		new->update = U_ALL_REFRESH;
		new->cursorpos = 0;
		new->head = NULL;
		new->current = NULL;
	}
	return(new);
}

void redraw_input_screen(inputwin *I){
	mvwin(I->inputline, LINES-1, 0);
	#ifdef RESIZE_CAPABLE
	wresize(I->inputline, 1, COLS);
	#endif
	keypad(I->inputline, TRUE);
	meta(I->inputline, TRUE);
	touchwin(I->inputline);
}

void move_input_cursor(inputwin *I, int spaces){
	if (I->cursorpos + spaces <= strlen(I->inputbuffer)){
		I->cursorpos+=spaces;
		if (I->cursorpos > I->cursorstart + COLS - 1) I->cursorstart = I->cursorpos - COLS;
		else if (I->cursorpos < I->cursorstart) I->cursorstart = I->cursorpos;

		if (I->cursorpos >= MAXDATASIZE){
			I->cursorpos = MAXDATASIZE-1;
		}
		else if (I->cursorpos < 0){
			I->cursorpos = 0;
		}
	}
}

void set_input_buffer(inputwin *I, char *buffer){
	I->cursorstart = 0;
	strncpy(I->inputbuffer, buffer, MAXDATASIZE);
	I->cursorpos = strlen(buffer);
	if (I->cursorpos > I->cursorstart + COLS - 1) I->cursorstart = I->cursorpos - COLS + 1;
}

void backspace_input_buffer(inputwin *I){
	if (I->cursorpos > 0){  
		I->cursorpos--;
                strcpy(&(I->inputbuffer)[I->cursorpos], &(I->inputbuffer)[I->cursorpos+1]);
		if (I->cursorpos < I->cursorstart) I->cursorstart = I->cursorpos;
        }
}

void delete_input_buffer(inputwin *I){
	if (strlen(&(I->inputbuffer)[I->cursorpos]) > 0){
		strcpy(&(I->inputbuffer)[I->cursorpos], &(I->inputbuffer)[I->cursorpos+1]);
	}
}

void add_input_buffer(inputwin *I, int value){
	char scratch[MAXDATASIZE];

	if (I->cursorpos < MAXDATASIZE){
		strcpy(scratch, &(I->inputbuffer)[I->cursorpos]);
	        (I->inputbuffer)[I->cursorpos] = value;
	        strcpy(&(I->inputbuffer)[I->cursorpos+1], scratch);
	        I->cursorpos++;
		if (I->cursorpos > I->cursorstart + COLS - 1) I->cursorstart = I->cursorpos - COLS + 1;
	}
}

void append_input_buffer(inputwin *I, char *string){
	char scratch[MAXDATASIZE];

	if (I->cursorpos + strlen(string) < MAXDATASIZE){
		strcpy(scratch, &(I->inputbuffer)[I->cursorpos]);
	        strcpy(&(I->inputbuffer)[I->cursorpos], string);
	        strcpy(&(I->inputbuffer)[I->cursorpos + strlen(string)], scratch);
	        I->cursorpos += strlen(string);
		if (I->cursorpos > I->cursorstart + COLS - 1) I->cursorstart = I->cursorpos - COLS + 1;
	}
}
	
int process_inputline_events(inputwin *I, int event){
	char buffer[MAXDATASIZE];
	
	// for cursor control on the inputline
	if (event == KEY_LEFT){
		move_input_cursor(I, -1);
		return(E_NONE);
	}

	else if (event == KEY_RIGHT){
		move_input_cursor(I, 1);
		return(E_NONE);
	}
               
	// for command recall on the inputline
	else if (event == KEY_UP){
		//I->current = prev_inputline_entry(I->current, buffer);
		if (prev_inputline_entry(I, buffer)) set_input_buffer(I, buffer);
		return(E_NONE);
	}
                        
	else if (event == KEY_DOWN){
		if (next_inputline_entry(I, buffer)) set_input_buffer(I, buffer);
		return(E_NONE);
	}

	// backspace (ctrl-h (dec 8) or 0x7f)
	else if (event == 0x7f || event == 8 || event == KEY_BACKSPACE){
		backspace_input_buffer(I);
		return(E_NONE);
	}

	else if (event == KEY_DC){
		delete_input_buffer(I);
		return(E_NONE);
	}

	// ctrl-a to beginning of line
	else if (event == 1 || event == KEY_HOME || event == KEY_A1){
		I->cursorpos = 0;
		I->cursorstart = 0;
		return(E_NONE);
	}

	// ctrl-e to end of line
	else if (event == 5 || event == KEY_END || event == KEY_C1){
		I->cursorpos = strlen(I->inputbuffer);
		if (I->cursorpos > I->cursorstart + COLS - 1) I->cursorstart = I->cursorpos - COLS + 1;
		return(E_NONE);
	}
	
	else if (event == 2 || event == 31 || event == 15 || event == 3 || (event >= 0x20 && event < 0x7e)){
                add_input_buffer(I, event);
        	return(E_NONE);
	}
	return (event);
}


int add_inputline_entry(inputwin *I, char *str_buffer){
	inputline_entry *current;
	char *strptr;
	
	if (strlen(str_buffer) > 0){
		current = calloc(sizeof(inputline_entry), 1);
		strptr = calloc(strlen(str_buffer) + 1, 1);

		if (current==NULL || strptr==NULL){
			plog("Cannot allocate input buffer memeory in add_inputline_entry()");
			exit(0);
		}   
          	current->buffer = strptr;
		strcpy(strptr, str_buffer);
		current->next = I->head;
		current->prev = NULL;
		if (I->head != NULL) (I->head)->prev = current; 
		I->head = current;
		I->current = NULL;
		return (1);
	}
	return (0);
}

int prev_inputline_entry(inputwin *I, char *buffer){
	if (I->current != NULL){
		if (I->current->next != NULL){
			I->current = I->current->next; 
			strncpy(buffer, I->current->buffer, MAXDATASIZE);
			return(1);
		}
	}
	else if (I->head != NULL){
		I->current = I->head; 
		strncpy(buffer, I->current->buffer, MAXDATASIZE);
		return(1);
	}
	beep();
	return(0);
}

int next_inputline_entry(inputwin *I, char *buffer){
	if (I->current != NULL){
		if (I->current->prev != NULL){
			I->current = I->current->prev; 
			strncpy(buffer, I->current->buffer, MAXDATASIZE);
		}
		else{
			I->current = NULL; 
			strcpy(buffer, "");
		}
		return(1);
	}
	strcpy(buffer, "");
	return(0);
}

int print_inputline(inputwin *inputline){
	int i, j;
	int bgcolor, fgcolor, fg_set, bg_set;
	char colorcode[8];
	char *buffer;
	WINDOW *inputwin;

	int attr_bold;
	int attr_ul;
	int attr_rev;
	int attr_color;
	int attr_current;

	buffer = inputline->inputbuffer;	
	inputwin = inputline->inputline;

	if (inputline != NULL && buffer != NULL){

		attr_color = 0;
		attr_bold = 0;
		attr_ul = 0;
		attr_rev = 0;
		attr_color = 0;
		attr_current = 0;
		
		// now print the string on the line
		for (i=inputline->cursorstart, j=0; i<strlen(buffer) && j < COLS; i++){
		
			// if character is a ^B, set attribute to BOLD and print a B to indicate change
			if (buffer[i]==2){ 
				attr_bold = attr_bold ^ 1;
				wattrset(inputwin, A_BOLD);
				mvwaddch(inputwin, 0, j, 'B');
				j++;
			}
			// if character is a ^_, set attribute to UNDERLINE
			else if (buffer[i]==31){
				attr_ul = attr_ul ^ 1;
				wattrset(inputwin, A_BOLD);
				mvwaddch(inputwin, 0, j, 'U');
				j++;
			}
			// if character is a ^O, set attribute to REVERSE
			else if (buffer[i]==15){
				attr_rev = attr_rev ^ 1;
				wattrset(inputwin, A_BOLD);
				mvwaddch(inputwin, 0, j, 'R');
				j++;
			}
			// if character is a ^C, color info follows
			else if (buffer[i]==3){
				wattrset(inputwin, A_BOLD);
				mvwaddch(inputwin, 0, j, 'C');
				wattrset(inputwin, A_NORMAL);
				j++;

				bzero(colorcode,7);
				if (isdigit((int)buffer[i+1]) && isdigit((int)buffer[i+2])){
					strncpy(colorcode, &buffer[i+1], 2);
					mvwaddch(inputwin, 0, j, buffer[i+1]);
					mvwaddch(inputwin, 0, j+1, buffer[i+2]);
					i=i+2;
					j=j+2;

				}
				else{
					strncpy(colorcode, &buffer[i+1], 1);
					mvwaddch(inputwin, 0, j, buffer[i+1]);
					j++;
					i++;
				}	
				fg_set = sscanf(colorcode, "%d", &fgcolor);
	
				if (fg_set && buffer[i+1]==','){		
					mvwaddch(inputwin, 0, j, ',');
					j++;

					bzero(colorcode,7);
					if (isdigit((int)buffer[i+2]) && isdigit((int)buffer[i+3])){
						mvwaddch(inputwin, 0, j, buffer[i+2]);
						mvwaddch(inputwin, 0, j+1, buffer[i+3]);
						strncpy(colorcode, &buffer[i+2], 2);
						i=i+3;
						j=j+2;

					}
					else{
						mvwaddch(inputwin, 0, j, buffer[i+2]);
						strncpy(colorcode, &buffer[i+2], 1);
						i=i+2;
						j++;
					}
					bg_set = sscanf(colorcode, "%d", &bgcolor);
				}
				if (fg_set > 0 && fgcolor) attr_color = COLOR_PAIR(mirc_palette(fgcolor, bgcolor));
				else attr_color = 0;
			}				
			else {
				attr_current = A_NORMAL;
				attr_current = attr_current | attr_color;
				if (attr_ul) attr_current = attr_current | A_UNDERLINE;
				if (attr_bold) attr_current = attr_current | A_BOLD;
				if (attr_rev) attr_current = attr_current | A_REVERSE;
		
				wattrset(inputwin, attr_current);
				mvwaddch(inputwin, 0, j, buffer[i]);
				j++;
			}
		}
		
		// clear the line now

		wattrset(inputwin, A_NORMAL);
		for (i=j; i < COLS; i++){
			mvwaddch(inputwin, 0, i, ' ');
		}
		if (inputline->cursorpos >=0) mvwaddstr(inputwin, 0, inputline->cursorpos - inputline->cursorstart, "");
		else mvwaddstr(inputwin, 0, j, "");

		wrefresh(inputwin);
	}	
	return(j);
}



void set_inputline_update_status(inputwin *S, int update){
	if (S !=NULL) S->update = S->update | update;
}

void unset_inputline_update_status(inputwin *S, int update){
	if (S !=NULL) S->update = S->update & (0xffff ^ update);        
}

int inputline_update_status(inputwin *S){
	if (S != NULL) return (S->update);
	return(0);
}



menuwin *create_menu_screen(){
	menuwin *new;
	WINDOW *W;
	
	new = malloc(sizeof(menuwin));
	W = newwin(1, COLS, 0, 0);
	wattron(W, A_REVERSE);
	mvwhline(W, 0, 0, ' ', COLS);
	if (new != NULL){
		new->menuline = W;
		new->update = U_ALL_REFRESH;
	}
	return(new);
}

void redraw_menu_screen(menuwin *M){
	#ifdef RESIZE_CAPABLE
	wresize(M->menuline, 1, COLS);
	#endif
	wattron(M->menuline, A_REVERSE);
	mvwhline(M->menuline, 0, 0, ' ', COLS);
	touchwin(M->menuline);
	M->update = 0;
}

void set_menuline_update_status(menuwin *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_menuline_update_status(menuwin *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}

int menuline_update_status(menuwin *S){
	if (S != NULL) return (S->update);
	return(0);
}



statuswin *create_status_screen(){
	statuswin *new;
	WINDOW *W;

	new = malloc(sizeof(statuswin));
	W = newwin(1, COLS, LINES-2, 0);
	wattron(W, A_REVERSE);	
	mvwhline(W, 0, 0, ' ', COLS);
	keypad(W, TRUE);
	touchwin(W);
	if (new != NULL){
		new->statusline = W;
		new->update = U_ALL_REFRESH;
	}
	return(new);
}

void redraw_status_screen(statuswin *S){
	mvwin(S->statusline, LINES-2, 0);
	#ifdef RESIZE_CAPABLE
	wresize(S->statusline, 1, COLS);
	#endif
	wattron(S->statusline, A_REVERSE);	
	mvwhline(S->statusline, 0, 0, ' ', COLS);
	touchwin(S->statusline);
	S->update = 0;
}

void set_statusline_update_status(statuswin *S, int update){
	if (S != NULL) S->update = S->update | update;
}

void unset_statusline_update_status(statuswin *S, int update){
	if (S != NULL) S->update = S->update & (0xffff ^ update);        
}

int statusline_update_status(statuswin *S){
	if (S != NULL) return (S->update);
	return(0);	
}


/* misc **************************************************************************************************/

void progress_bar(WINDOW *win, int posy, int posx, int size, int percent){
	int to_fill; 
	int i;

	to_fill = (size*percent)/100;
	wattrset(win, make_color(PROGRESS_COLOR_F, PROGRESS_COLOR_B));
	wattron(win, A_REVERSE);
	wattron(win, A_BOLD);
		for (i=0; i<size; i++){
		if (i==to_fill+1){
			if (has_colors){
				wattrset(win, make_color(PROGRESS_COLOR_F, PROGRESS_COLOR_B));
				wattron(win, A_REVERSE);
			}
			else {
				wattrset(win, A_NORMAL);
			}
		}
	mvwaddch(win, posy, posx+i, ' ');
	}
	wattrset(win, A_NORMAL);
}
server *server_by_name(char *servername){
	screen *current;

	current = server_screen_by_name(servername);
	if (current != NULL) return (current->screen);
	return (NULL);
}

screen *server_screen_by_name(char *servername){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==SERVER){
			if (strcmp(((server *)(current->screen))->server, servername)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

channel *channel_by_name(char *channelname){
	screen *current;

	current = channel_screen_by_name(channelname);
	if (current != NULL) return (current->screen);
	return (NULL);
}

screen *channel_screen_by_name(char *channelname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==CHANNEL){
			if (strcasecmp(((channel *)(current->screen))->channel, channelname)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

chat *chat_by_name(char *chatname){
	screen *current;

	current = chat_screen_by_name(chatname);
	if (current != NULL) return (current->screen);
	return (NULL);
}

screen *chat_screen_by_name(char *chatname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==CHAT){
			if (strcasecmp(((chat *)(current->screen))->nick, chatname)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

dcc_chat *dcc_chat_by_name(char *chatname){
	screen *current;

	current = dcc_chat_screen_by_name(chatname);
	if (current != NULL) return (current->screen);
	return (NULL);
}

screen *dcc_chat_screen_by_name(char *chatname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==DCCCHAT){
			if (strcasecmp(((dcc_chat *)(current->screen))->nick, chatname)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

list *list_by_name(char *listname){
	screen *current;

	current = list_screen_by_name(listname);
	if (current != NULL) return (current->screen);
	return (NULL);
}

list *active_list_by_name(char *listname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==LIST){
			if (((list *)(current->screen))->active){
				if (strcasecmp(((list *)(current->screen))->server->server, listname)==0){
					break;
				}
			}
		}
		current=current->next;
	}
	return(current->screen);
}

screen *list_screen_by_name(char *listname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==LIST){
			if (strcasecmp(((list *)(current->screen))->server->server, listname)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

list *list_by_server(server *server){
	screen *current;
	list *currentlist=NULL;

	// find which chat this chatname belongs to
	current=screenlist;
	while(current!=NULL){
		if (current->type==LIST){
			if (((list *)(current->screen))->server == server){
				currentlist = (list *)(current->screen);
				break;
			}
		}
		current=current->next;
	}
	return(currentlist);
}

list *active_list_by_server(server *server){
	screen *current;
	list *currentlist=NULL;

	// find which chat this chatname belongs to
	current=screenlist;
	while(current!=NULL){
		if (current->type==LIST){
			if (((list *)(current->screen))->active){
				if (((list *)(current->screen))->server == server){
					currentlist = (list *)(current->screen);
					break;
				}
			}
		}
		current=current->next;
	}
	return(currentlist);
}
 
transfer *transfer_by_name(char *name){
	screen *current;

	current = transfer_screen_by_name(name);
	if (current != NULL) return (current->screen);
	return (NULL);
}

screen *transfer_screen_by_name(char *name){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==TRANSFER){
			if (strcmp(((transfer *)(current->screen))->name, name)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

