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
#include <signal.h>
#include <ctype.h>
#include <pwd.h>

#include "autodefs.h"
#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "log.h"
#include "common.h"
#include "events.h"
#include "cevents.h"
#include "screen.h"
#include "network.h"
#include "ncolor.h"
#include "parser.h"
#include "ctcp.h"
#include "dcc.h"
#include "main.h"
#include "cmenu.h"
#include "forms.h"
#include "config.h"
#include "option.h"

// define active form ids
#define CF_NEW_SERVER_CONNECT 1
#define CF_FAVORITE_SERVER_CONNECT 2
#define CF_EDIT_SERVER_FAVORITES 3

#define CF_IDENTITY 10
#define CF_DCC_OPTIONS 11
#define CF_DCC_SEND_OPTIONS 12
#define CF_CLIENT_OPTIONS 13
#define CF_SCRIPT_OPTIONS 14
#define CF_CTCP_OPTIONS 15

#define CF_EDIT_CHANNEL_FAVORITES 20
#define CF_JOIN 21
#define CF_JOIN_FAVORITE 22

#define CF_LIST_OPTIONS 30
#define CF_CHANNEL_SELECT 32

#define CF_EDIT_USER_FAVORITES 40
#define CF_EDIT_USER_IGNORED 41
#define CF_NEW_CHAT 42
#define CF_FAVORITE_CHAT 43
#define CF_NEW_DCC_CHAT 44
#define CF_FAVORITE_DCC_CHAT 45
#define CF_NEW_DCC_SEND 46
#define CF_FAVORITE_DCC_SEND 47
#define CF_DCC_SEND_FILE 48

#define CF_HELP_ABOUT 50

#define CF_FILE_TRANSFER 60
#define CF_FILE_TRANSFER_INFO 61
#define CF_FILE_ALLOW 62

#define CF_EXIT 250

menu *ServerMenu[6];
menu *TransferMenu[4];
menu *ListMenu[4];
menu *ChannelMenu[4];
menu *ChatMenu[4];
menu *DCCChatMenu[4];
menu *HelpMenu[2];
menu *ChannelUserMenu;

menu *currentmenu = NULL;
menu *windowmenu = NULL;
menu *UserMenu = NULL;
menu *UserListMenu = NULL;
menu *CtcpMenu = NULL;
menu *ControlMenu = NULL;
menu *DCCMenu = NULL;

menubar *servermenus;
menubar *channelmenus;
menubar *chatmenus;
menubar *dccchatmenus;
menubar *transfermenus;
menubar *listmenus;
menubar *helpmenus;
menubar *currentmenusline;

int currentmenunum;
int menuchanged;

menuitem *currentmenuitem;

int process_server_events(int key);
int process_channel_events(int key);
int process_chat_events(int key);
int process_dccchat_events(int key);
int process_transfer_events(int key);
int process_list_events(int key);
int process_help_events(int key);

int connect_to_server (server *S);
int disconnect_from_server (server *S);

int inst_alarm_handler();
int inst_resize_handler();
int inst_abrt_handler();
int inst_term_handler();
int inst_pipe_handler();

int draw_menuline_screen(menuwin *menuline, menubar *menubar);
int process_common_form_events(screen *screen, int key);

extern int screenupdated;

/* globals used in this file */
static int currentform = 0;
static char newserver[MAXDATASIZE];
static char newchannel[MAXDATASIZE];
static char newuser[MAXDATASIZE];
static char newfile[MAXDATASIZE];
static dcc_file *newdccfile = NULL;
//static dcc_file *selectedfile = NULL;

menu *build_window_menu(int startx, int starty);
//form *currentform;
int sigkey;

int main(int argc, char *argv[]){
	struct passwd *pwd;
	char buffer[MAXDATASIZE];
	char inputbuffer[MAXDATASIZE];
	char scratch[MAXDATASIZE];
	int serr, maxsfd, sfd;
	int key;
	int i;
	int configstat;
  
	server *S;
	screen *current, *tempscr;
	dcc_chat *currentdccchat;
	dcc_file *currentdcc;
	dcc_file *nextdcc;

	struct timeval timeout;      	
	fd_set readfds, writefds;
	time_t now;

	// inst_ctrlc_handler();
	inst_alarm_handler();
	inst_resize_handler();
	inst_abrt_handler();
	inst_term_handler();
	inst_pipe_handler();

	initscr();
	begin_color();
	
	// use raw mode to catch flow control stuff, ctrl-s, ctrl-q 
	raw();
	noecho();

	erase();
	refresh();

	/* get the controlling loginname and home directory */
	pwd = getpwuid(geteuid());
	strcpy(loginuser, pwd->pw_name);
	strcpy(homepath, pwd->pw_dir);

	/* get the hostname and domain for defaults */
	gethostname(hostname, MAXHOSTLEN);
	getdomainname(domain, MAXDOMLEN);

	sprintf(configfile, "%s/%s", homepath, DEFAULT_CONFIG_FILE);
	//printf("%s:%s:%s", loginuser, homepath, configfile);

	inputline = create_input_screen();
	menuline = create_menu_screen();
	statusline = create_status_screen();

	init_all_menus();
	servermenus = init_menubar(6, 3, 4, ServerMenu);
	channelmenus = init_menubar(3, 3, 4, ChannelMenu);
	chatmenus = init_menubar(3, 3, 4, ChatMenu);
	dccchatmenus = init_menubar(3, 3, 4, DCCChatMenu);
	listmenus = init_menubar(3, 3, 4, ListMenu);
	transfermenus = init_menubar(3, 3, 4, TransferMenu);
	helpmenus = init_menubar(2, 3, 4, HelpMenu);
	currentmenusline = servermenus;

	S = add_server("", 0, "", "", "", "", "");

	vprint_all("%s\n", CODE_ID);
	vprint_all("Reading configuration from %s...", configfile);
	configstat = read_config(configfile, &configuration);

	/* create a transfer screen and hide it by default */
	transferscreen = add_transfer("transfers");
	hide_screen(transfer_screen_by_name("transfers"), 1); 

	set_menuline_update_status(menuline, U_ALL_REFRESH);
	set_statusline_update_status(statusline, U_ALL_REFRESH);

	currentscreen=screenlist;
	// update_current_screen();

	bzero (inputbuffer,MAXDATASIZE);
	resize_occured=0;
	ctrl_c_occured=0;
	alarm_occured=0;

	inputline_head = NULL;
	inputline_current = NULL;
	menuchanged = 0;

	// notimeout(inputline->inputline, TRUE );
	key = 0;
	sigkey = 0;

	windowmenu = build_window_menu(22,1);
	set_input_buffer (inputline, "");

	if (configstat != 0){
		print_all(" Failed. Defaults Loaded.\nUse Ctrl-T to select \002Options\002 from the menu and configure the client.\n");
	}
	else print_all(" OK\n");

	while(1){
		/* if a resize occured redraw all windows asap, if not resized asap bad things will happen */
		if (resize_occured || key == 12){
			close_all_forms();
			endwin();
			refresh();
			
			current=screenlist;
			while(current!=NULL){ 
				set_screen_update_status(current, U_ALL_REFRESH);
				redraw_screen(current);
				current=current->next;
			}
			redraw_input_screen(inputline);
			redraw_menu_screen(menuline);
			redraw_status_screen(statusline);
			set_inputline_update_status(inputline, U_ALL_REFRESH);
			set_menuline_update_status(menuline, U_ALL_REFRESH);
			set_statusline_update_status(statusline, U_ALL_REFRESH);
			wgetch(inputline->inputline);
			resize_occured=0;
			//inst_resize_handler();
		}

		// vprint_all(" A key = %d\n", key);


		/* make sure the current terminal window is at least a usable size */
		if (LINES < MINWINDOWHEIGHT || COLS < MINWINDOWWIDTH){
			refresh();
			clear();
			view_screen_size_warning(E_NONE);	
			key = E_NONE;	
		}		
		else {
			// do all of the screen updates first and process screen events 
			if (currentscreen->type == SERVER) key = process_server_events(key);
			else if (currentscreen->type == CHANNEL) key = process_channel_events(key);
			else if (currentscreen->type == CHAT) key = process_chat_events(key);
			else if (currentscreen->type == DCCCHAT) key = process_dccchat_events(key);
			else if (currentscreen->type == TRANSFER) key = process_transfer_events(key);
			else if (currentscreen->type == LIST) key = process_list_events(key);
			else if (currentscreen->type == HELP) key = process_help_events(key);
		}

		//vprint_all(" B key = %d\n", key);
								
		if (key == E_CLOSE){
			tempscr = currentscreen;					
			currentscreen = select_prev_screen(currentscreen);
			remove_screen(tempscr);
			key = E_CHANGE_SCREEN;
		}	
		else if (key == E_NEXT_WINDOW || key == E_NEXT_WINDOW2){
			currentscreen = select_next_screen(currentscreen);
			key = E_CHANGE_SCREEN;
		}
		else if (key == E_PREV_WINDOW || key == E_PREV_WINDOW2){
			currentscreen = select_prev_screen(currentscreen);
			key = E_CHANGE_SCREEN;
		}
		else if (key >= 0xF000 && key < 0xFF00){
			tempscr = screenlist;
			for (i=0; i < (key - 0xF000); i++) tempscr = tempscr->next;
			currentscreen = select_screen(tempscr);
			key = E_CHANGE_SCREEN;
		}				
		
		// if the user selected to change screens update the new screen as well
		if (key == E_CHANGE_SCREEN){			
			if (currentscreen->type == SERVER) process_server_events(E_NONE);
			else if (currentscreen->type == CHANNEL) process_channel_events(E_NONE);
			else if (currentscreen->type == CHAT) process_chat_events(E_NONE);
			else if (currentscreen->type == DCCCHAT) process_dccchat_events(E_NONE);
			else if (currentscreen->type == TRANSFER) process_transfer_events(E_NONE);
			else if (currentscreen->type == LIST) key = process_list_events(E_NONE);
			else if (currentscreen->type == HELP) key = process_help_events(E_NONE);
		}

		// go through all screens and get all socket descriptors

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(0, &readfds);

		maxsfd=0;
		time(&now);

		/* set all socket descriptors for server and dcc transfers */

		current=screenlist;
		while(current!=NULL){
			if (current->type==SERVER){
				// if connecting to the server, call the connect function
				if (((server *)(current->screen))->connect_status >= 0){
					connect_to_server((server *)(current->screen));
					key = E_NOWAIT;
				}
				else if (((server *)(current->screen))->active){
					sfd = ((server *)(current->screen))->serverfd;
					FD_SET(sfd, &readfds);
					if (maxsfd<sfd) maxsfd=sfd;
				}
			}
			if (current->type==DCCCHAT){
				if (((dcc_chat *)(current->screen))->active){
					sfd = ((dcc_chat *)(current->screen))->dccfd;
					FD_SET(sfd, &readfds);
					if (maxsfd<sfd) maxsfd=sfd;
				}
			}
			if (current->type==TRANSFER){
				currentdcc = ((transfer *)current->screen)->dcclist;
				while(currentdcc!=NULL){
					/* if receiving, worry about the incoming data */
					if (currentdcc->active && currentdcc->type == DCC_RECEIVE){	
						sfd = currentdcc->dccfd;
						FD_SET(sfd, &readfds);
						if (maxsfd<sfd) maxsfd=sfd;
					}
					/* if sending, watch incoming and outgoing sockets */
					else if (currentdcc->active && currentdcc->type == DCC_SEND){
						sfd = currentdcc->dccfd;
						FD_SET(sfd, &readfds);
						FD_SET(sfd, &writefds);
						if (maxsfd<sfd) maxsfd=sfd;
					}
					currentdcc=currentdcc->next;
				}
			}
			current=(current->next);
		}

		/* for NOWAIT perform the next operation right away */
		if (key == E_NOWAIT){
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
		}
		else {
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
		}

		serr = select(maxsfd+1, &readfds, &writefds, NULL, &timeout);
		
		/* read from server and dcc sockets if ready */
		current=screenlist;
		while(current!=NULL){
			// if this is an active (non-disconnected) server and its socket is ready	
			if (current->type==SERVER){				
				if (serr > 0 && ((server *)(current->screen))->active &&
				FD_ISSET(((server*)(current->screen))->serverfd, &readfds)) {		
					serr=recv_line(((server *)(current->screen))->serverfd, buffer, MAXDATASIZE-1);
					if (serr == -1){
						// got disconnected, set server connect flags
						((server *)(current->screen))->active = 0;
						((server *)(current->screen))->connect_status = -1;
						set_statusline_update_status(statusline, U_ALL_REFRESH);
						sprintf (buffer, "%c%d,%dDisconnected from %s\n", 
						3, ERROR_COLOR_F, ERROR_COLOR_B, ((server *)(current->screen))->server);
						print_all(buffer);
					}
					if (serr == 0);					
					else parse_message((server *)(current->screen), buffer);
				}
			}

			/* if this is an active (started, non-disconnected) dcc chat and its socket is ready */

			else if (current->type == DCCCHAT){
				currentdccchat = current->screen;
				if (serr > 0 && currentdccchat->active && FD_ISSET(currentdccchat->dccfd, &readfds)) {	

					bzero(buffer, MAXDATASIZE);	
					serr = recv_all(currentdccchat->dccfd, buffer, MAXDATASIZE-1);
					if (serr == -1) {
						/* error, kill the chat socket */
						print_dcc_chat(currentdccchat, "Remote host ended connection\n");
						remove_dcc_chat(currentdccchat);
					}
					else if (serr == 0){
						// minor error, continue (currently blocking only)				
						// print_dcc_chat(currentdccchat, "Socket error occured");
					}
					else printmsg_dcc_chat(currentdccchat, currentdccchat->nick, buffer);
				}

				/* set up DCC servers and listen for connections  */
				/* for outgoing chats, and start them immediately */
				else if (currentdccchat->active == 0 && currentdccchat->direction == DCC_SEND){ 
					start_outgoing_dcc_chat(currentdccchat);
				}
				/* for incoming chat, permission to start may be required */
				else if (currentdccchat->active == 0 && currentdccchat->direction == DCC_RECEIVE &&
					currentdccchat->serverstatus == 0){ 
					start_incoming_dcc_chat(currentdccchat);
				}
			}
			current=(current->next);
		}

		/* check all the dcc file transfer sockets to see if data is ready */ 
		/* to be received, if so grab it                                   */

		currentdcc=transferscreen->dcclist;
		while(currentdcc != NULL){
			nextdcc = currentdcc->next;


			/* if this is a send in progress, check to see if ack is waiting */
			if (currentdcc != NULL){
				if (serr > 0 && FD_ISSET(currentdcc->dccfd, &readfds)) {
					if (currentdcc->type == DCC_SEND && currentdcc->active){
						get_dccack(currentdcc);
					}
				}
			}


			/* incoming transfer finished, clean up and close socket */
			if (currentdcc->type == DCC_RECEIVE && currentdcc->byte >= currentdcc->size){
                		vprint_all("Filename %s was received successfully\n", currentdcc->filename);
				remove_dcc_file(currentdcc);
				set_transfer_update_status(transferscreen, U_ALL_REFRESH);
				currentdcc = NULL;
			}
			/* do not close the socket until the remote has acked all bytes  */
			else if (currentdcc->type == DCC_SEND && currentdcc->ackbyte >= currentdcc->size){
				vprint_all("Filename %s was sent successfully\n", currentdcc->filename);
				remove_dcc_file(currentdcc);
				set_transfer_update_status(transferscreen, U_ALL_REFRESH);
				currentdcc = NULL;
			}

			/* timeout during transfer, clean up */
			else if (currentdcc->active && (currentdcc->last_activity_at) + TRANSFERTIMEOUT < now ){
        	        	vprint_all ("Timeout receiving %s.\n", currentdcc->filename);
				set_transfer_update_status(transferscreen, U_ALL_REFRESH);
				remove_dcc_file(currentdcc);
				currentdcc = NULL;
        		}
			/* if this is a receive in progress, check if there is more file to transfer */
			else if (serr > 0 && FD_ISSET(currentdcc->dccfd, &readfds)) {
				if (currentdcc->type == DCC_RECEIVE && currentdcc->active){
					if (get_dcc_file(currentdcc) == -1) currentdcc = NULL;
				}
			}
			/* if this is a send in progress, check if there is more file to transfer */
			else if (serr > 0 && FD_ISSET(currentdcc->dccfd, &writefds)) {
				if (currentdcc->type == DCC_SEND && currentdcc->active){
					if (put_dcc_file(currentdcc) == -1) currentdcc = NULL;
				}
			}
			/* if this is a new send, set up a listening server and check for new connections */
			else if (currentdcc->type == DCC_SEND && currentdcc->active == 0){
				start_outgoing_dcc_file(currentdcc);
			}
			/* if this is an incoming file, and config is set to ask, pull up a confirmation form */
			else if (currentdcc->type == DCC_RECEIVE && currentdcc->active == 0){
				if (currentdcc->allowed == 0 && configuration.dccaccept == 0){
					/* bring up form if one isn't up already */
					if (currentform == 0){
						currentform = CF_FILE_ALLOW;
						newdccfile = currentdcc;
					}
				}						
				else{
					if (!start_incoming_dcc_file(currentdcc)){				
						remove_dcc_file(currentdcc);
						currentdcc = NULL;
					}
				}
			}
	
			currentdcc=nextdcc;
		}

		// check stdin if ready
		
		if ((serr > 0 && FD_ISSET(0, &readfds)) || ctrl_c_occured){
			ctrl_c_occured = 0;	
			key = wgetch(inputline->inputline);

			#if (DEBUG & D_STDIN)
				plog("DEC: %d, CHR:'%c'\n", key, key);
			#endif
		}
		else {
			key = E_NONE;
		}

		/* check configuration status and save if changed */
		if (configuration.changed && configuration.autosave){
			if (writeconfig(configfile, &configuration)){
				vprint_all("Configuration saved in %s.\n", configfile);
			}
			else print_all("Error saving configuration.\n");
		}
	}
        end_run();
	return(0);	
}

int process_server_events(int key){
	int selectedmenu;
	int refreshstat;
	int scrollposy, scrollposx;
	int i;
	char wininfo[256];
	char buffer[MAXDATASIZE];
	char inputbuffer[MAXDATASIZE];
	server *currentserver;
	menubar *menus;
	int formcode, formkey;

	menus = servermenus;
	currentserver = (server *)currentscreen->screen;

	// create the window selection menu
	// if forms are showing, pass the key event to forms
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	move_menu(windowmenu, 41, 1);
	ServerMenu[4] = windowmenu;	
	
	key = process_menubar_events(menus, key);
	key = process_screen_events(currentscreen, key);
	key = process_inputline_events(inputline, key);
	selectedmenu = selected_menubar_item_id(menus);
	if (selectedmenu != 0) key = selectedmenu;
	
	if (key==KEY_ENTER || key==10){
		// if a command has been entered pass it to the server as is for now

		strcpy(inputbuffer, inputline->inputbuffer);
		add_inputline_entry(inputline, inputbuffer);
		//inputline->head = add_inputline_entry(inputline->head, inputbuffer);
		//inputline->current = inputline->head;
		set_input_buffer (inputline, "");
			
		i = parse_input(currentserver, inputbuffer);
		if (i == E_CHANGE_SCREEN){
			return(E_CHANGE_SCREEN);
		}
		else if (i != E_NONE) key = i;
		else if (strlen(inputbuffer)>0 && i == 0){
			print_server(currentserver, 
			"Unknown command. Type /help for a list of commands\n");
		}	
	}		

	/* control-k to kill the screen, but you cant do this here so beep */
	if (key == 11){
		beep();
		return(E_NONE);
	}

	// if scrolling the screen set up the proper position
	if (!currentscreen->scrolling){
		getyx(currentserver->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}

	// If the screen has been updated or menu requests a background redraw, redraw screen
	if (server_update_status(currentserver) & U_ALL_REFRESH ||
		menubar_update_status(menus) & U_BG_REDRAW){

		prefresh(currentserver->message, 
			currentscreen->scrollpos, 0, 1, 0, LINES-3, COLS);
		unset_server_update_status(currentserver, U_ALL);
		unset_menubar_update_status(menus, U_BG_REDRAW);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
	}
	
	// update the statusline for this screen if required
	if (statusline_update_status(statusline) & U_ALL_REFRESH){
		// if scrolling print out the line number

		if (currentscreen->scrolling){
			sprintf(wininfo, "(%d/%d) ", currentscreen->scrollpos, LISTLINES);
		}
		else strcpy(wininfo, "");
					
		wattrset(statusline->statusline, COLOR_PAIR(STATUS_COLOR_B*16+STATUS_COLOR_F));
		wattron(statusline->statusline, A_REVERSE);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');

		if (currentserver->active){
			strcat(wininfo, currentserver->server);
		}
		else strcat(wininfo, "Not Connected");

		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
		mvwaddstr(statusline->statusline, 0, 1, currentserver->nick);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}
	
	// update the menu selection screen if required
	draw_menuline_screen(menuline, menus);

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	if (key == E_DISCONNECT){
		if (currentserver->active) disconnect_from_server(currentserver);
		key = E_NOWAIT;				
	}

	else if (key == E_SERVER_ADD_FAVORITE){
		if (currentserver->active){
			if (!config_server_exists(&configuration, currentserver->server, currentserver->port)){
				add_config_server(&configuration, currentserver->server, currentserver->port, LIST_ORDER_FRONT);
				sprintf(buffer, "Server %s:%d added to favorites\n", currentserver->server, 
					currentserver->port);
				print_server(currentserver, buffer);
				key = E_NOWAIT;				
			}
			else {
				sprintf(buffer, "Server %s:%d is already a favorite\n", currentserver->server, 
					currentserver->port);
				print_server(currentserver, buffer);
				key = E_NOWAIT;				
			}
		}
		else {
			print_server(currentserver, "Must be connected to a server to make it a favorite.\n");
		}
	}


	// finally print the current input buffer to the inputline window
	print_inputline(inputline);
	curs_set(1);
	return(key);
}


int process_channel_events(int key){
	int selectedmenu;
	int refreshstat;
	int scrollposy, scrollposx;
	int i, ustartx, ustarty;
	char wininfo[256];
	char buffer[MAXDATASIZE];
	char inputbuffer[MAXDATASIZE];
	channel *currentchannel;
	server *currentserver;
	dcc_chat *new_dccchat;
	menubar *menus;
	chat *newchat;
	int formcode, formkey;
	
	currentchannel = currentscreen->screen;
	currentserver = currentchannel->server;
	menus = channelmenus;

	// create the window selection menu
	move_menu(windowmenu, 12, 1);
	ChannelMenu[1] = windowmenu;	

	//if forms are showing, pass the key event to forms
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	if (currentchannel->selecting == 0){
		// process menu and screen events only when no selecting users
		key = process_menubar_events(menus, key);
		key = process_screen_events(currentscreen, key);
		key = process_inputline_events(inputline, key);
	}
	selectedmenu = selected_menubar_item_id(menus);
	if (selectedmenu != 0) key = selectedmenu;
	
	// ctrl-u for user selection
	else if (key == 0x15){
		currentchannel->selecting = currentchannel->selecting ^ 1;			
		set_channel_update_status(currentchannel, U_ALL_REFRESH);
	}

	else if (key == 27 && currentchannel->selecting == 1){
		currentchannel->selecting = currentchannel->selecting = 0;
		set_channel_update_status(currentchannel, U_ALL_REFRESH);
	}

	// handle input at the keyboard, if menus are not selected
	else if (key == KEY_UP && currentchannel->selecting == 1){
		select_prev_user(currentchannel);
		set_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	else if (key == KEY_DOWN && currentchannel->selecting == 1){
		select_next_user(currentchannel);
		set_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	else if ((key == KEY_PPAGE || key == KEY_A3 || key == 25) && currentchannel->selecting == 1){
		for (i = 0; i < LINES - 4; i++) select_prev_user(currentchannel);
		set_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	else if ((key == KEY_NPAGE || key == KEY_C3 || key == 22) && currentchannel->selecting == 1){
		for (i = 0; i < LINES - 4; i++) select_next_user(currentchannel);
		set_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	else if ((key == KEY_ENTER || key == 10) && currentchannel->selecting < 2){
		if (currentchannel->selecting == 1){
			currentchannel->selecting = 2;
		}
		else {
			// if a command has been entered find out what server it should be sent to 
			// otherwise form a PRIVMSG command and send it to the channel
			// also print it to the channel screen since messages are not echoed

			strcpy(inputbuffer, inputline->inputbuffer);
			add_inputline_entry(inputline, inputbuffer);

			//inputline->head = add_inputline_entry(inputline->head, inputbuffer);
			//inputline->current = inputline->head;

			set_input_buffer (inputline, "");
			
			i = parse_input(currentserver, inputbuffer);
			if (i == E_CHANGE_SCREEN){
				return(E_CHANGE_SCREEN);
			}
			else if (i != E_NONE) key = i;
			else if (strlen(inputbuffer) > 0){
				sendmsg_channel(currentchannel, inputbuffer);
				printmymsg_channel(currentchannel, inputbuffer);
			}
		}
	}
	
	else if (isprint(key) && currentchannel->selecting == 1){
		select_next_user_by_key(currentchannel, key);
		set_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	// handle selected user menu events
	else if (currentchannel->selecting == 2){
		process_menu_events(UserMenu, key);
		if (key == 27 || key == KEY_CANCEL){
			currentchannel->selecting = 0;
			set_channel_update_status(currentchannel, U_ALL_REFRESH);
			set_menuline_update_status(menuline, U_ALL_REFRESH);
		}
		else if (key == 10 || key == KEY_ENTER){
			selectedmenu = selected_menu_item_id(UserMenu);
			if (selectedmenu == E_USER_PASTE){
				append_input_buffer(inputline, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_WHOIS){
				sendcmd_server(currentserver, "WHOIS", selected_channel_nick(currentchannel), "", "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_QUERY){
				if (strcmp(selected_channel_nick(currentchannel), currentserver->nick) == 0){
					print_channel(currentchannel, "You can not chat with yourself.\n");
				}
				else {
					if (chat_by_name(selected_channel_nick(currentchannel)) == NULL){
						newchat = add_chat(selected_channel_nick(currentchannel), currentserver);
						set_channel_update_status(currentchannel, U_ALL_REFRESH);
						set_chat_update_status(newchat, U_ALL_REFRESH);
						set_menuline_update_status(menuline, U_ALL_REFRESH);
						set_statusline_update_status(statusline, U_ALL_REFRESH);
						currentchannel->selecting = 0;
						return(E_CHANGE_SCREEN);
					}
				}
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}

			/* CTCP Menu */
			else if (selectedmenu == E_CTCP_PING){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("PING"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_CLIENTINFO){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("CLIENTINFO"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_USERINFO){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("USERINFO"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_VERSION){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("VERSION"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_FINGER){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("FINGER"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_SOURCE){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("SOURCE"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CTCP_TIME){
				sendcmd_server(currentserver, "PRIVMSG", 
					create_ctcp_message("TIME"), selected_channel_nick(currentchannel), "");
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}

			/* DCC Menu */
			else if (selectedmenu == E_DCC_CHAT){
				currentchannel->selecting = 0;
				if (strcmp(selected_channel_nick(currentchannel), currentserver->nick) == 0){
					print_all("You can not DCC chat with yourself.\n");
					set_channel_update_status(currentchannel, U_ALL_REFRESH);
					set_menuline_update_status(menuline, U_ALL_REFRESH);
				}
				else if (!currentserver->active){
					print_all("Must be connected to a server to DCC chat.\n");
					set_channel_update_status(currentchannel, U_ALL_REFRESH);
					set_menuline_update_status(menuline, U_ALL_REFRESH);
				}
				else {
					new_dccchat = dcc_chat_by_name(selected_channel_nick(currentchannel)); 
					if (new_dccchat == NULL){ 
						new_dccchat = add_outgoing_dcc_chat(selected_channel_nick(currentchannel), 
							currentserver->nick, currentserver);
						start_outgoing_dcc_chat(new_dccchat);
						sendcmd_server(currentserver, "PRIVMSG",
						create_ctcp_command("DCC CHAT chat", "%lu %d", new_dccchat->localip, new_dccchat->localport), 
							selected_channel_nick(currentchannel), "");
					}
					else currentscreen = dcc_chat_screen_by_name(selected_channel_nick(currentchannel));
					set_dccchat_update_status(new_dccchat, U_ALL_REFRESH);
					set_menuline_update_status(menuline, U_ALL_REFRESH);
					set_statusline_update_status(statusline, U_ALL_REFRESH);
					return(E_CHANGE_SCREEN);
				}
			}
			else if (selectedmenu == E_DCC_SEND){
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);

				if (strcmp(selected_channel_nick(currentchannel), currentserver->nick) == 0){
					print_all("You can not DCC send to yourself.\n");
				}
				else if (!currentserver->active){
					print_all("Must be connected to a server to DCC send a file.\n");
				}
				else{
					currentform = CF_DCC_SEND_FILE;
					strcpy(newuser, selected_channel_nick(currentchannel));
					strcpy(newfile, configuration.dcculpath);
				}
			}

			/* Control Menu */
			else if (selectedmenu == E_CONTROL_OP){
				send_server(currentserver, "MODE %s +o %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_DEOP){
				send_server(currentserver, "MODE %s -o %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_VOICE){
				send_server(currentserver, "MODE %s +v %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_DEVOICE){
				send_server(currentserver, "MODE %s -v %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_KICK){
				send_server(currentserver, "KICK %s %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_BAN){
				send_server(currentserver, "MODE %s +b %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_CONTROL_KICKBAN){
				send_server(currentserver, "MODE %s +b %s", currentchannel->channel, selected_channel_nick(currentchannel));
				send_server(currentserver, "KICK %s %s", currentchannel->channel, selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}

			/* List Menu */
			if (selectedmenu == E_USER_ADD_FAVORITE){
				if (!config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, selected_channel_nick(currentchannel))){
					add_config_user(&configuration, CONFIG_FAVORITE_USER_LIST, selected_channel_nick(currentchannel), LIST_ORDER_FRONT);
					vprint_channel(currentchannel, "Nick %s has been added to favorites.\n", selected_channel_nick(currentchannel));
				}
				else vprint_channel(currentchannel, "Nick %s is already a favorite.\n", selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			else if (selectedmenu == E_USER_ADD_IGNORE){
				if (!config_user_exists_exact(&configuration, CONFIG_IGNORED_USER_LIST, selected_channel_nick(currentchannel))){
					add_config_user(&configuration, CONFIG_IGNORED_USER_LIST, selected_channel_nick(currentchannel), LIST_ORDER_FRONT);
					vprint_channel(currentchannel, "Nick %s has been added to ignore list.\n", selected_channel_nick(currentchannel));
				}
				else vprint_channel(currentchannel, "Nick %s is already being ignored.\n", selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			if (selectedmenu == E_USER_REMOVE_FAVORITE){
				if (remove_config_user_by_name(&configuration, CONFIG_FAVORITE_USER_LIST, selected_channel_nick(currentchannel))){
					vprint_channel(currentchannel, "Nick %s has been removed from favorites.\n", selected_channel_nick(currentchannel));
				}
				else vprint_channel(currentchannel, "Nick %s is not a favorite.\n", selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
			if (selectedmenu == E_USER_REMOVE_IGNORED){
				if (remove_config_user_by_name(&configuration, CONFIG_IGNORED_USER_LIST, selected_channel_nick(currentchannel))){
					vprint_channel(currentchannel, "Nick %s is no longer being ignored.\n", selected_channel_nick(currentchannel));
				}
				else vprint_channel(currentchannel, "Nick %s is not currently being ignored.\n", selected_channel_nick(currentchannel));
				currentchannel->selecting = 0;
				set_channel_update_status(currentchannel, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
			}
		}
	}

	if (statusline_update_status(statusline) & U_ALL_REFRESH){
		// if scrolling print out the line number

		if (currentscreen->scrolling){
			sprintf(wininfo, "(%d/%d) ", currentscreen->scrollpos, LISTLINES);
		}
		else strcpy(wininfo, "");
					
		wattrset(statusline->statusline, COLOR_PAIR(STATUS_COLOR_B*16+STATUS_COLOR_F));
		wattron(statusline->statusline, A_REVERSE);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');

		strcat(wininfo, ((channel *)(currentscreen->screen))->channel);
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
		mvwaddstr(statusline->statusline, 0, 1, ((server *)(currentchannel->server))->nick);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}

	refreshstat = channel_update_status(currentchannel);

	if (!currentscreen->scrolling){
		getyx(currentchannel->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}
	if (refreshstat & U_MAIN_REFRESH || menubar_update_status(menus) & U_BG_REDRAW){
		prefresh(currentchannel->message, 
		currentscreen->scrollpos, 0, 1, 0, LINES-3, COLS-USERWINWIDTH-1);
		unset_channel_update_status(currentchannel, U_MAIN_REFRESH);
		unset_menubar_update_status(menus, U_BG_REDRAW);
	}

	if (refreshstat & U_USER_REFRESH || menubar_update_status(menus) & U_BG_REDRAW){
		refresh_user_list(currentchannel);
	        touchwin(currentchannel->message);   
       		touchwin(currentchannel->vline);  
       		wrefresh(currentchannel->vline);
       		wrefresh(currentchannel->user);      
		unset_channel_update_status(currentchannel, U_USER_REFRESH);
	}

	// update the menu selection screen if required
	draw_menuline_screen(menuline, menus);

	if (currentchannel->selecting == 2){
		ustartx = COLS - MAXNICKDISPLEN - UserMenu->width - 4;
		ustarty = user_win_offset(currentchannel);
		if (ustarty > LINES - 4 - UserMenu->height) ustarty = LINES - 4 - UserMenu->height;
		move_menu(UserMenu, ustartx, ustarty);

		move_menu(ControlMenu, ustartx - ControlMenu->width - 1, ustarty + 2);
		move_menu(CtcpMenu, ustartx - CtcpMenu->width - 1, ustarty + 3);
		move_menu(DCCMenu, ustartx - DCCMenu->width - 1, ustarty + 4);
		move_menu(UserListMenu, ustartx - UserListMenu->width - 1, ustarty + 5);

		print_menu(UserMenu);
	}

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	if (key == E_CHANNEL_ADD_FAVORITE){
		if (currentchannel->active){
			if (!config_channel_exists(&configuration, currentchannel->channel)){
				add_config_channel(&configuration, currentchannel->channel, LIST_ORDER_FRONT);
				vprint_channel(currentchannel, "Channel %s added to favorites\n", currentchannel->channel);
				key = E_NOWAIT;				
			}
			else {
				vprint_channel(currentchannel, "Channel %s is already a favorite\n", currentchannel->channel);
				key = E_NOWAIT;				
			}
		}
	}

	else if (key == E_CHANNEL_PART || key == E_CLOSE){
		sendcmd_server(currentchannel->server, "PART", currentchannel->channel, "", currentchannel->server->nick);
	}

	print_inputline(inputline);
	curs_set(1);
	return(key);
}



int process_chat_events(int key){
	int selectedmenu;
	int refreshstat;
	int scrollposy, scrollposx;
	int i;
	char wininfo[256];
	char inputbuffer[MAXDATASIZE];
	menubar *menus;
	chat *currentchat;	
	int formkey, formcode;

	// create the window selection menu
	menus = chatmenus;
	move_menu(windowmenu, 9, 1);
	ChatMenu[1] = windowmenu;	
	currentchat = currentscreen->screen;

	//if forms are showing, pass the key event to forms
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	// process menu events
	key = process_menubar_events(menus, key);
	key = process_screen_events(currentscreen, key);
	key = process_inputline_events(inputline, key);
	selectedmenu = selected_menubar_item_id(menus);
	if (selectedmenu != 0) key = selectedmenu;

	else if (key==KEY_ENTER||key==10){
		// disable scrolling
		set_message_scrolling(currentscreen, 0);

		strcpy(inputbuffer, inputline->inputbuffer);
		add_inputline_entry(inputline, inputbuffer);
		//inputline_head = add_inputline_entry(inputline_head, inputbuffer);
		//inputline_current = inputline_head;

		// send to the appropriate server using PRIVMSG command to the proper nick
		
		i = parse_input(currentchat->server, inputbuffer);
		set_input_buffer (inputline, "");
		if (i == E_CHANGE_SCREEN){
			return(E_CHANGE_SCREEN);
		}
		else if (i != E_NONE) key = i;
		else if (strlen(inputbuffer) > 0){
			sendmsg_chat(currentchat, inputbuffer);
			printmymsg_chat(currentchat, inputbuffer);
		}
		print_inputline(inputline);
	}				

	// update the statusline for this screen if required
	if (statusline_update_status(statusline) & U_ALL_REFRESH){
		// if scrolling print out the line number

		if (currentscreen->scrolling){
			sprintf(wininfo, "(%d/%d) ", currentscreen->scrollpos, LISTLINES);
		}
		else strcpy(wininfo, "");
					
		wattrset(statusline->statusline, COLOR_PAIR(STATUS_COLOR_B*16+STATUS_COLOR_F));
		wattron(statusline->statusline, A_REVERSE);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');

		strcat(wininfo, currentchat->nick);
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
		mvwaddstr(statusline->statusline, 0, 1, currentchat->server->nick);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}

	// strcat (wininfo, currentchat->nick);
	refreshstat = chat_update_status(currentchat);

	if (!currentscreen->scrolling){
		getyx(currentchat->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}
	if (refreshstat & U_ALL_REFRESH || menubar_update_status(menus) & U_BG_REDRAW){
		prefresh(currentchat->message, currentscreen->scrollpos, 0, 1, 0, LINES-3, COLS);
		unset_menubar_update_status(menus, U_BG_REDRAW);
		unset_chat_update_status(currentchat, U_ALL);
	}
		 
	if (key == E_USER_ADD_FAVORITE){
		if (!config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, currentchat->nick)){
			add_config_user(&configuration, CONFIG_FAVORITE_USER_LIST, currentchat->nick, LIST_ORDER_FRONT);
			vprint_chat(currentchat, "Nick %s has been added to favorites.\n", currentchat->nick);
		}
		else vprint_chat(currentchat, "Nick %s is already a favorite.\n", currentchat->nick);
		key = E_NOWAIT;				
	}
	else if (key == E_USER_ADD_IGNORE){
		if (!config_user_exists(&configuration, CONFIG_IGNORED_USER_LIST, currentchat->nick)){
			add_config_user(&configuration, CONFIG_IGNORED_USER_LIST, currentchat->nick, LIST_ORDER_FRONT);
			vprint_chat(currentchat, "Nick %s has been added to ignore list.\n", currentchat->nick);
		}
		else vprint_chat(currentchat, "Nick %s is already being ignored.\n", currentchat->nick);
		key = E_NOWAIT;				
	}

	// update the menu selection screen if required
	draw_menuline_screen(menuline, menus);

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	print_inputline(inputline);
	curs_set(1);
	return(key);
}

int process_dccchat_events(int key){
	int selectedmenu;
	int refreshstat;
	int scrollposy, scrollposx;
	int i;
	char wininfo[256];
	char inputbuffer[MAXDATASIZE];
	int formkey;
	int formcode;
	menubar *menus;
	dcc_chat *currentchat;
	
	menus = dccchatmenus;
	currentchat = currentscreen->screen;

	// create the window selection menu
	move_menu(windowmenu, 13, 1);
	DCCChatMenu[1] = windowmenu;	

	//if forms are showing, pass the key event to forms
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	// process menu events
	key = process_menubar_events(menus, key);
	key = process_screen_events(currentscreen, key);
	key = process_inputline_events(inputline, key);
	selectedmenu = selected_menubar_item_id(menus);
	if (selectedmenu != 0) key = selectedmenu;

	if (key==KEY_ENTER||key==10){

		// disable scrolling
		set_message_scrolling(currentscreen, 0);

		strcpy(inputbuffer, inputline->inputbuffer);
		add_inputline_entry(inputline, inputbuffer);
		//inputline_head = add_inputline_entry(inputline_head, inputbuffer);
		//inputline_current = inputline_head;
		set_input_buffer (inputline, "");

		// send to the appropriate socket, but make sure that the chat is active
		if (parse_input(currentchat->server, inputbuffer)) {}
		else if (strlen(inputbuffer) > 0){
			if (currentchat->active){
				sendmsg_dcc_chat(currentchat, inputbuffer);
				printmymsg_dcc_chat(currentchat, inputbuffer);
			}
			else if (i != E_NONE) key = i;
			else print_dcc_chat(currentchat, "Remote client is not connected.\n");
		}
		print_inputline(inputline);
	}				

	// update the statusline for this screen if required
	if (statusline_update_status(statusline) & U_ALL_REFRESH){
		// if scrolling print out the line number

		if (currentscreen->scrolling){
			sprintf(wininfo, "(%d/%d) ", currentscreen->scrollpos, LISTLINES);
		}
		else strcpy(wininfo, "");
					
		wattrset(statusline->statusline, COLOR_PAIR(STATUS_COLOR_B*16+STATUS_COLOR_F));
		wattron(statusline->statusline, A_REVERSE);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');

		strcat(wininfo, currentchat->nick);
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
		mvwaddstr(statusline->statusline, 0, 1, currentchat->dest);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}

	// strcat (wininfo, currentchat->nick);
	refreshstat = dccchat_update_status(currentchat);

	if (!currentscreen->scrolling){
		getyx(currentchat->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}
	if (refreshstat & U_ALL_REFRESH || menubar_update_status(menus) & U_BG_REDRAW){
		prefresh(currentchat->message, currentscreen->scrollpos, 0, 1, 0, LINES-3, COLS);
		unset_menubar_update_status(menus, U_BG_REDRAW);
		unset_dccchat_update_status(currentchat, U_ALL);
	}

	if (key == E_CLOSE){
		end_dccchat(currentchat);
		return(key);
	}
	else if (key == E_DISCONNECT){
		disconnect_dccchat(currentchat);
	}
	else if (key == E_USER_ADD_FAVORITE){
		if (!config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, currentchat->nick)){
			add_config_user(&configuration, CONFIG_FAVORITE_USER_LIST, currentchat->nick, LIST_ORDER_FRONT);
			vprint_dcc_chat(currentchat, "Nick %s has been added to favorites.\n", currentchat->nick);
		}
		else vprint_dcc_chat(currentchat, "Nick %s is already a favorite.\n", currentchat->nick);
		key = E_NOWAIT;				
	}
	else if (key == E_USER_ADD_IGNORE){
		if (!config_user_exists(&configuration, CONFIG_IGNORED_USER_LIST, currentchat->nick)){
			add_config_user(&configuration, CONFIG_IGNORED_USER_LIST, currentchat->nick, LIST_ORDER_FRONT);
			vprint_dcc_chat(currentchat, "Nick %s has been added to ignore list.\n", currentchat->nick);
		}
		else vprint_dcc_chat(currentchat, "Nick %s is already being ignored.\n", currentchat->nick);
		key = E_NOWAIT;				
	}

	// update the menu selection screen if required
	draw_menuline_screen(menuline, menus);

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	print_inputline(inputline);
	curs_set(1);
	return(key);
}

int process_list_events(int key){
	int selectedmenu;
	int refreshstat;
	int i, scrollposy, scrollposx;
	char wininfo[256];
	char buffer[MAXDATASIZE];
	menubar *menus;
	list *currentlist;
	int formkey;
	int formcode;
	int minuser;
	int maxuser;
	char searchstring[256];
	int searchtype;

	menus = listmenus;
	currentlist = currentscreen->screen;

	// create the window selection menu
	move_menu(windowmenu, 9, 1);
	ListMenu[1] = windowmenu;	

	/* if forms are showing, pass the key event to forms */
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	/* process menu events */
	key = process_menubar_events(menus, key);
	selectedmenu = selected_menubar_item_id(menus);

	/* handle events for selected menu items */
	if (selectedmenu != 0) key = selectedmenu;

	if (key == KEY_UP) select_prev_list_channel(currentlist);
	else if (key == KEY_DOWN) select_next_list_channel(currentlist);
	else if (key == KEY_PPAGE || key == KEY_A3 || key == 25) select_prev_list_channel_page(currentlist);
	else if (key == KEY_NPAGE || key == KEY_C3 || key == 22) select_next_list_channel_page(currentlist);

	else if (key == E_LIST_VIEW && currentform == 0){
		currentform = CF_LIST_OPTIONS;
		key = E_NOWAIT;
		formkey = E_NONE;
	}

	else if (key == E_CHANNEL_ADD_FAVORITE && selected_list_channel(currentlist) != NULL){
		if (!config_channel_exists(&configuration, selected_list_channel(currentlist))){
			add_config_channel(&configuration, selected_list_channel(currentlist), LIST_ORDER_FRONT);
			vprint_all("Channel %s added to favorites\n", selected_list_channel(currentlist));
		}
		else {
			vprint_all("Channel %s is already a favorite\n", selected_list_channel(currentlist));
		}
		set_list_update_status(currentlist, U_ALL_REFRESH);
		key = E_NOWAIT;
	}

	else if (key == E_CHANNEL_JOIN && selected_list_channel(currentlist) != NULL){
		sendcmd_server(server_by_name(currentlist->servername), "JOIN", selected_list_channel(currentlist), "", "");
		set_list_update_status(currentlist, U_ALL_REFRESH);
		key = E_NOWAIT;
	}

	/* entrer brings up the join / add favorites form */
	else if (key==KEY_ENTER || key==10){
		currentform = CF_CHANNEL_SELECT;
		key = E_NOWAIT;
		print_inputline(inputline);
		return(key);
	}		
	else if (isprint(key)) select_next_list_channel_by_key(currentlist, key);

	// update the menu selection screen if required
	curs_set(0);

	// update the statusline for this screen if required
	if (statusline_update_status(statusline) & U_ALL_REFRESH){

		strcpy(wininfo, "");
		strcat(wininfo, currentlist->servername);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
		if (currentlist->active){
			mvwaddstr(statusline->statusline, 0, 0, " Channel List: Fetching...");
		}
	 	else mvwaddstr(statusline->statusline, 0, 0, " Channel List");
		wrefresh(statusline->statusline);
	}

	refreshstat = list_update_status(currentlist);

	if (!currentscreen->scrolling){
		getyx(currentlist->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}

	if ((refreshstat & U_ALL_REFRESH || menubar_update_status(menus) & U_BG_REDRAW)){
		wrefresh(currentlist->message);
		unset_menubar_update_status(menus, U_BG_REDRAW);
		refresh_list_screen(currentlist);
	}		
	unset_list_update_status((list *)(currentscreen->screen), U_ALL);

	draw_menuline_screen(menuline, menus);

	if (currentform == CF_LIST_OPTIONS){
		formcode = get_list_view_options(formkey, searchstring, &minuser, &maxuser, &searchtype);
		//sprintf(buffer, "Search %s, minuser %d, maxuser %d, type %d\n", searchstring, minuser, maxuser, searchtype);
		//print_all(buffer);
		if (formcode == E_OK){
			apply_list_view(currentlist, searchstring, minuser, maxuser, searchtype);
			set_list_update_status(currentlist, U_ALL_REFRESH);
			currentform = 0;
			key = E_NOWAIT;
		}
		if (formcode == E_CANCEL){
			set_list_update_status(currentlist, U_ALL_REFRESH);
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_NOWAIT){
			set_list_update_status(currentlist, U_ALL_REFRESH);
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_CHANNEL_SELECT){
		formcode = get_channel_select_options(formkey);
		if (formcode == E_OK){
			if (selected_list_channel(currentlist) != NULL){
				sendcmd_server(server_by_name(currentlist->servername), "JOIN", 
					selected_list_channel(currentlist), "", "");
			}	
			set_list_update_status(currentlist, U_ALL_REFRESH);
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_CHANNEL_ADD_FAVORITE){
			if (selected_list_channel(currentlist) != NULL){
				if (!config_channel_exists(&configuration, selected_list_channel(currentlist))){
					add_config_channel(&configuration, selected_list_channel(currentlist), LIST_ORDER_FRONT);
                        	}
			}
			set_list_update_status(currentlist, U_ALL_REFRESH);
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			set_list_update_status(currentlist, U_ALL_REFRESH);
			currentform = 0;
			key = E_NOWAIT;
		}
		else key = E_NONE;
	}

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	print_inputline(inputline);

	curs_set(1);
	return(key);
}

int process_transfer_events(int key){
	int selectedmenu;
	int formkey;
	char wininfo[256];
	
	transfer *T;
	static int selected;
	dcc_file *current, *next;

	char filescratch[1024];
	char progressscratch[1024];
	char scratch[1024], rscratch[1024];

	struct in_addr hostipaddr;
	int transfer_num, transfer_pos;
	float ratio;
	int percent;
	int upnum, downnum;
	int i, slen, dlen;
	float kbps, upkbps, downkbps; 
	float ttime, tbytes;
	int displayed;

	time_t now;
	static time_t lastupdate = 0;
	menubar *menus;
	
	curs_set(0);
	menus = transfermenus;

	// create the window selection menu
	move_menu(windowmenu, 13, 1);
	TransferMenu[1] = windowmenu;

	T = ((transfer *)(currentscreen->screen));
	if (T->selectedfile == NULL || !dcc_file_exists(T, T->selectedfile)){
		T->selectedfile = T->dcclist;
	}
	if (T->dcclisttop == NULL || !dcc_file_exists(T, T->dcclisttop)){
		T->dcclisttop = T->dcclist;
	}

	//if forms are showing, pass the key event to forms
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;

	// process menu events
	key = process_menubar_events(menus, key);
	selectedmenu = selected_menubar_item_id(menus);
	if (selectedmenu != 0) key = selectedmenu;

	// handle input at the keyboard
	// control-k to kill the screen, but you cant do this here so beep
	if (key == 11){
		beep();
		return(E_NONE);
	}

	// no scrolling here use these keys to select trasfer
	else if (key==KEY_SF || key==KEY_HOME || key==KEY_A1){} 
	else if (key==KEY_SR || key==KEY_END || key==KEY_C1){} 
	else if (key==KEY_PPAGE || key==KEY_A3){} 
	else if (key==KEY_NPAGE || key==KEY_C3){}
			
	/* bring up the abort / info form */				
	else if (key==KEY_ENTER||key==10){
		currentform = CF_FILE_TRANSFER;
		set_statusline_update_status(statusline, U_ALL_REFRESH); 
		set_screen_update_status(currentscreen, U_ALL_REFRESH); 
	}

	else if (key == KEY_DOWN && T->dcclist != NULL){
		if (T->selectedfile != NULL){
			if (T->selectedfile->next != NULL){
				set_screen_update_status(currentscreen, U_ALL_REFRESH);
				T->selectedfile = T->selectedfile->next;

				/* adjust the top so that it's visible on the screen */
				transfer_num = 0;
				current = T->dcclisttop;
				while(current != NULL){
					transfer_num++;
					if (T->selectedfile == current){
						transfer_num -= ((LINES - 2) / 3);
						for (i = 0; i < transfer_num; i++){
							if (T->dcclisttop->next != NULL) T->dcclisttop = T->dcclisttop->next; 
						}
						break;
					}
					current = current->next;
				}
				if (current == NULL) T->dcclisttop = T->dcclist;			 
			}
		}	
	} 
	else if (key == KEY_UP && T->dcclist != NULL){
		if (T->selectedfile != NULL){
			if (T->selectedfile->prev != NULL){
				set_screen_update_status(currentscreen, U_ALL_REFRESH); 
				T->selectedfile = T->selectedfile->prev;

				/* adjust the top so that it's visible on the screen */
				transfer_num = 0;
				current = T->dcclisttop;
				while(current != NULL){
					if (current == T->selectedfile){
						for (i = 0; i < transfer_num; i++){
							if (T->dcclisttop->prev != NULL) T->dcclisttop = T->dcclisttop->prev; 
						}
						break;
					}
					transfer_num++;
					current = current->prev;
				}
			}
		}
	}

	// update the menu selection screen if required

	time(&now);
	upkbps = 0;
	downkbps = 0;
	upnum = 0;
	downnum = 0;
	transfer_num = 0;
	transfer_pos = 0;

	if (transfer_update_status(T) & U_ALL_REFRESH){	
		werase(T->message);
	}

	if (currentmenusline->current < 0){
		if (now > lastupdate ||	transfer_update_status(T)){	
			displayed = 0;
			lastupdate = now;
			current = T->dcclist;
			while(current != NULL){
				transfer_num++;
				next=current->next;
				// touchline(T->message, transfer_num * 2, 2);	
	
				tbytes = (float)current->byte;
				ttime = (float)(now-current->starttime);
				if (ttime > 0) kbps = tbytes/(ttime*1000);
				else kbps = 0;

				/* start displaying only after the top of the screen */
				if (current == T->dcclisttop) displayed = 1;

				if (displayed){
					transfer_pos++;
					if (current == T->selectedfile) wattrset(T->message, A_REVERSE);
					else wattrset(T->message, A_NORMAL);


					scratch[0] = 0;
					if (current->type == DCC_RECEIVE) sprintf(scratch, "RECEIVING: %s", current->filename);
					else if (current->type == DCC_SEND) sprintf(scratch, "SENDING: %s", current->filename);
					sprintf(rscratch, "%ld/%ld, %.1f KB/s\n", current->byte, current->size, kbps); 

					slen = strlen(scratch);
					dlen = strlen(rscratch);

					/* pad the line with spaces */
					for (i = slen; i < COLS - dlen - 1; i++){
						 scratch[i] = ' ';
					}
					scratch[i] = 0;
					strcat(scratch, rscratch);

					mvwaddstr(T->message, transfer_pos * 3 - 2, 1, scratch);
					ratio = (float)current->byte * 100.0f;
					percent = (int)(ratio / (float)current->size);

					progress_bar(T->message, transfer_pos * 3 - 1, 1, COLS-2, percent);
					current->last_updated_at = now;
					set_statusline_update_status(statusline, U_ALL_REFRESH); 
					set_screen_update_status(currentscreen, U_ALL_REFRESH); 
				}	

				if (current->type == DCC_RECEIVE){
					downnum++;
					downkbps = downkbps + kbps;
				}
				else if (current->type == DCC_SEND){
					upnum++;
					upkbps = upkbps + kbps;
				}
				current = next;
			}
		}
	}
	
	// update the statusline for this screen if required

	if (statusline_update_status(statusline) & U_ALL_REFRESH || transfer_update_status(T) & U_ALL_REFRESH){
		sprintf(wininfo, "UP %d FILES @ %.1fKB/s : DOWN %d FILES @ %.1fKB/s", 
			upnum, upkbps, downnum, downkbps);
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);
	 	mvwaddstr(statusline->statusline, 0, 0, " Transfer Screen");
		touchwin(statusline->statusline);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}

	if (transfer_update_status(T) & U_ALL_REFRESH ||
		menubar_update_status(menus) & U_BG_REDRAW){
		refresh_transfer_screen(T); 
		unset_transfer_update_status(T, U_ALL);
		unset_menubar_update_status(menus, U_BG_REDRAW);
	}

	draw_menuline_screen(menuline, menus);

	if (key == E_TRANSFER_STOP){
		if (dcc_file_exists(T, T->selectedfile)){
			remove_dcc_file(T->selectedfile);
		}
		set_statusline_update_status(statusline, U_ALL_REFRESH); 
		set_screen_update_status(currentscreen, U_ALL_REFRESH); 
		return(E_NOWAIT);
	}
	else if (key == E_TRANSFER_INFO){
		if (dcc_file_exists(T, T->selectedfile)){
			currentform = CF_FILE_TRANSFER_INFO;
		}
		set_statusline_update_status(statusline, U_ALL_REFRESH); 
		set_screen_update_status(currentscreen, U_ALL_REFRESH); 
	}

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	print_inputline(inputline);
	curs_set(1);
	return(key);

}

int process_help_events(int key){
	int selectedmenu;
	int refreshstat;
	int scrollposy, scrollposx;
	int i;
	char wininfo[256];
	char inputbuffer[MAXDATASIZE];
	menubar *menus;
	help *currenthelp;
	int formkey;
	int formcode;
	
	menus = helpmenus;
	currenthelp = currentscreen->screen;

	// create the window selection menu
	move_menu(windowmenu, 9, 1);
	HelpMenu[1] = windowmenu;	

	/* if forms are showing, pass the key event to forms */
	if (currentform){
		formkey = key;
		key = E_NONE;
	}
	else formkey = 0;


	// process menu events
	key = process_menubar_events(menus, key);

	/* convert up and down into scolling events */
	if (key == KEY_DOWN) key = process_screen_events(currentscreen, KEY_SF);
	if (key == KEY_UP) key = process_screen_events(currentscreen, KEY_SR);
	key = process_screen_events(currentscreen, key);
	key = process_inputline_events(inputline, key);
	selectedmenu = selected_menubar_item_id(menus);

	/* handle events for selected menu items */
	if (selectedmenu != 0) key = selectedmenu;

	if (key==KEY_ENTER||key==10){
		// disable scrolling
		// set_message_scrolling(currentscreen, 0);

		//strcpy(inputbuffer, inputline->inputbuffer);
		//inputline_head = add_inputline_entry(inputline_head, inputbuffer);
		//inputline_current = inputline_head;
		//set_input_buffer (inputline, "");
		//print_inputline(inputline);
	}				

	// update the statusline for this screen if required
	if (statusline_update_status(statusline) & U_ALL_REFRESH){

		sprintf(wininfo, "(%d/%d)", currentscreen->scrollpos, LISTLINES);
		wattrset(statusline->statusline, COLOR_PAIR(STATUS_COLOR_B*16+STATUS_COLOR_F));
		wattron(statusline->statusline, A_REVERSE);
		for (i=0; i < COLS; i++) mvwaddch(statusline->statusline, 0, i, ' ');
		mvwaddstr(statusline->statusline, 0, COLS-strlen(wininfo)-1, wininfo);

		sprintf(wininfo, "Help: %s", ((help*)(currentscreen->screen))->name);

		mvwaddstr(statusline->statusline, 0, 1, wininfo);
		wrefresh(statusline->statusline);
		unset_statusline_update_status(statusline, U_ALL);
	}

	refreshstat = help_update_status((help *)(currentscreen->screen));

	if (!currentscreen->scrolling){
		getyx(currenthelp->message, scrollposy, scrollposx);
		if (scrollposy-LINES+3<0) currentscreen->scrollpos=0;
		else currentscreen->scrollpos = scrollposy-LINES+3;
	}
	if (refreshstat & U_ALL_REFRESH || menubar_update_status(menus) & U_BG_REDRAW){
		prefresh(currenthelp->message, 
		currentscreen->scrollpos, 0, 1, 0, LINES-3, COLS);
		unset_menubar_update_status(menus, U_BG_REDRAW);
	}

	/* update the menu selection screen if required */
	draw_menuline_screen(menuline, menus);

	if (currentform) key = formkey;
	key = process_common_form_events(currentscreen, key);

	print_inputline(inputline);
	curs_set(1);
	return(key);
}

int process_common_form_events(screen *inscreen, int key){
	int formcode, formkey;
	int update;
	screen *nextscreen;
	server *currentserver;
	channel *new_channel;
	chat *new_chat;
	dcc_chat *new_dccchat;
	dcc_file *new_dccfile;
	int newport;
	int newform;
	int i;

	/* get the current server for the event */
	if (inscreen->type == SERVER) currentserver = inscreen->screen;
	else if (inscreen->type == CHANNEL) currentserver = ((channel *)inscreen->screen)->server;
	else if (inscreen->type == CHAT) currentserver = ((chat *)inscreen->screen)->server;
	else if (inscreen->type == DCCCHAT) currentserver = ((dcc_chat *)inscreen->screen)->server;

	update = 0;
	if (currentform){
		formkey = key;
		key = E_NONE;
	}

	else formkey = 0;
	//vprint_all_attribs(1,1, "currentform = %d, formkey = %d, key = %d\n", currentform, formkey, key);

	newform = 0;

	if (key == E_EXIT || key == 24) newform = CF_EXIT;

	/* server form spawn events */
	else if (key == E_CONNECT_NEW && currentform == 0) newform = CF_NEW_SERVER_CONNECT;
	else if (key == E_CONNECT_FAVORITE && currentform == 0) newform = CF_FAVORITE_SERVER_CONNECT;
	else if (key == E_SERVER_EDIT_FAVORITE && currentform == 0) newform = CF_EDIT_SERVER_FAVORITES;

	/* channel form spawn events */
	else if (key == E_JOIN_NEW && currentform == 0) newform = CF_JOIN;
	else if (key == E_JOIN_FAVORITE && currentform == 0) newform = CF_JOIN_FAVORITE;
	else if (key == E_CHANNEL_EDIT_FAVORITE && currentform == 0) newform = CF_EDIT_CHANNEL_FAVORITES;

	/* user form spawn events */
	else if (key == E_CHAT_NEW && currentform == 0) newform = CF_NEW_CHAT;
	else if (key == E_CHAT_FAVORITE && currentform == 0) newform = CF_FAVORITE_CHAT;
	else if (key == E_DCC_CHAT_NEW && currentform == 0) newform = CF_NEW_DCC_CHAT;
	else if (key == E_DCC_SEND_NEW && currentform == 0) newform = CF_NEW_DCC_SEND;
	else if (key == E_DCC_CHAT_FAVORITE && currentform == 0) newform = CF_FAVORITE_DCC_CHAT;
	else if (key == E_DCC_SEND_FAVORITE && currentform == 0) newform = CF_FAVORITE_DCC_SEND;
	else if (key == E_USER_EDIT_FAVORITE && currentform == 0) newform = CF_EDIT_USER_FAVORITES;
	else if (key == E_USER_EDIT_IGNORED && currentform == 0) newform = CF_EDIT_USER_IGNORED;

	/* option events */
	else if (key == E_IDENTITY && currentform == 0) newform = CF_IDENTITY;
	else if (key == E_OPTIONS && currentform == 0) newform = CF_CLIENT_OPTIONS;
	else if (key == E_CTCP_OPTIONS && currentform == 0) newform = CF_CTCP_OPTIONS;
	else if (key == E_DCC_OPTIONS && currentform == 0) newform = CF_DCC_OPTIONS;
	else if (key == E_DCC_SEND_OPTIONS && currentform == 0) newform = CF_DCC_SEND_OPTIONS;

	else if (key == E_SAVE_OPTIONS){
		if (writeconfig(configfile, &configuration)){
			vprint_all("Configuration saved in %s.\n", configfile);
		}
		else print_all("Error saving configuration.\n");
		key = E_NOWAIT;
	}

	/* help events */
	else if (key == E_HELP_IRC_COMMANDS){
		add_help("IRC Commands", "", "irccmnds.hlp");
		select_screen(currentscreen);
		key = E_NOWAIT;
	}
	else if (key == E_HELP_COMMANDS){
		add_help("Client Commands", "", "clientcmnds.hlp");
		select_screen(currentscreen);
		key = E_NOWAIT;
	}
	else if (key == E_HELP_KEYS){
		add_help("IRC Commands", "", "keys.hlp");
		select_screen(currentscreen);
		key = E_NOWAIT;
	}
	else if (key == E_HELP_ABOUT && currentform == 0){
		currentform = CF_HELP_ABOUT;
		key = E_NOWAIT;
	}

	else if (key == E_CHANNEL_LIST){
		if (currentserver->active){
			sendcmd_server(currentserver, "LIST", "", "", "");
		}
		else {
			print_server(currentserver, "Must be connected to fetch channel list\n");
			key = E_NOWAIT;
		}
	}
	
	if (newform != 0){
		currentform = newform;
		key = E_NOWAIT;
		formkey = E_NONE;
	}	

	/* form handlers */
	if (currentform == CF_EXIT){
		//vprint_all_attribs(1,1,"%d %d\n", key, formkey);
		formcode = end_run(formkey);
		if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
		}
	}

	/* server forms */
	else if (currentform == CF_NEW_SERVER_CONNECT){
		formcode = get_new_connect_info(formkey, newserver, &newport);
		if (formcode == E_OK){
			if (currentserver->active) disconnect_from_server(currentserver);
			strcpy(currentserver->server, newserver);
			currentserver->port = newport;
			currentserver->connect_status = 0;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
			currentform = 0;
		}
	}
	else if (currentform == CF_FAVORITE_SERVER_CONNECT){
		formcode = get_favorite_connect_info(formkey, newserver, &newport);
		if (formcode == E_OK){
			if (currentserver->active) disconnect_from_server(currentserver);
			strcpy(currentserver->server, newserver);
			currentserver->port = newport;
			currentserver->connect_status = 0;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
			currentform = 0;
		}		
	}
	else if (currentform == CF_EDIT_SERVER_FAVORITES){
		formcode = edit_favorite_servers(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_NOWAIT){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}

	/* channel forms */
	else if (currentform == CF_JOIN){
		formcode = get_new_join_info(formkey, newchannel);
		if (formcode == E_OK){
			currentform = 0;
			if (!currentserver->active) print_server(currentserver, "Must be connected to join.\n");
			else {
				new_channel = channel_by_name(newchannel);			
				if (new_channel == NULL){
					sendcmd_server(currentserver, "JOIN", newchannel, "", "");
					update = U_ALL_REFRESH;
					key = E_NOWAIT;
				}
				else {
					currentscreen = channel_screen_by_name(newchannel);
					set_channel_update_status(new_channel, U_ALL_REFRESH);
					set_menuline_update_status(menuline, U_ALL_REFRESH);
					set_statusline_update_status(statusline, U_ALL_REFRESH);
					return(E_CHANGE_SCREEN);
				}
			}
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_JOIN_FAVORITE){
		formcode = get_favorite_join_info(formkey, newchannel);
		if (formcode == E_OK){
			currentform = 0;
			if (!currentserver->active) print_server(currentserver, "Must be connected to join.\n");
			else {
				new_channel = channel_by_name(newchannel);			
				if (new_channel == NULL){
					sendcmd_server(currentserver, "JOIN", newchannel, "", "");
					update = U_ALL_REFRESH;
					key = E_NOWAIT;
				}
				else {
					currentscreen = channel_screen_by_name(newchannel);
					set_channel_update_status(new_channel, U_ALL_REFRESH);
					set_menuline_update_status(menuline, U_ALL_REFRESH);
					set_statusline_update_status(statusline, U_ALL_REFRESH);
					return(E_CHANGE_SCREEN);
				}
			}
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_EDIT_CHANNEL_FAVORITES){
		formcode = edit_favorite_channels(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_NOWAIT){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}

	/* user forms */
	else if (currentform == CF_NEW_CHAT){
		formcode = get_new_chat_info(formkey, newuser);
		if (formcode == E_OK){
			currentform = 0;
			if (strcmp(newuser, currentserver->nick) == 0) print_all("You can not chat with yourself.\n");
			else if (!currentserver->active) print_all("Must be connected to a server to chat.\n");
			else {
				new_chat = chat_by_name(newuser);
				if (new_chat == NULL) new_chat = add_chat(newuser, currentserver);
				else currentscreen = chat_screen_by_name(newuser);
 
				set_chat_update_status(new_chat, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				return(E_CHANGE_SCREEN);
			}
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			currentform = 0;
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_FAVORITE_CHAT){
		formcode = get_favorite_user_chat_info(formkey, newuser);
		if (formcode == E_OK){
			currentform = 0;
			if (strcmp(newuser, currentserver->nick) == 0) print_all("You can not chat with yourself.\n");
			else if (!currentserver->active) print_all("Must be connected to a server to chat.\n");
			else {
				new_chat = chat_by_name(newuser);
				if (new_chat == NULL) new_chat = add_chat(newuser, currentserver);
				else currentscreen = chat_screen_by_name(newuser);

				set_chat_update_status(new_chat, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				return(E_CHANGE_SCREEN);
			}
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_NEW_DCC_CHAT){
		formcode = get_new_dccchat_info(formkey, newuser);
		if (formcode == E_OK){
			currentform = 0;
			if (strcmp(newuser, currentserver->nick) == 0) print_all("You can not DCC chat with yourself.\n");
			else if (!currentserver->active) print_all("Must be connected to a server to DCC chat.\n");
			else {
				new_dccchat = dcc_chat_by_name(newuser); 
				if (new_dccchat == NULL){ 
					new_dccchat = add_outgoing_dcc_chat(newuser, currentserver->nick, currentserver);
					start_outgoing_dcc_chat(new_dccchat);
					sendcmd_server(currentserver, "PRIVMSG",
					create_ctcp_command("DCC CHAT chat", "%lu %d", new_dccchat->localip, new_dccchat->localport), newuser, "");
				}
				else currentscreen = dcc_chat_screen_by_name(newuser);
				set_dccchat_update_status(new_dccchat, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				return(E_CHANGE_SCREEN);
			}
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_FAVORITE_DCC_CHAT){
		formcode = get_favorite_user_dccchat_info(formkey, newuser);
		if (formcode == E_OK){
			currentform = 0;
			if (strcmp(newuser, currentserver->nick) == 0) print_all("You can not DCC chat with yourself.\n");
			else if (!currentserver->active) print_all("Must be connected to a server to DCC chat.\n");
			else {
				new_dccchat = dcc_chat_by_name(newuser); 
				if (new_dccchat == NULL){ 
					new_dccchat = add_outgoing_dcc_chat(newuser, currentserver->nick, currentserver);
					start_outgoing_dcc_chat(new_dccchat);
					sendcmd_server(currentserver, "PRIVMSG",
					create_ctcp_command("DCC CHAT chat", "%lu %d", new_dccchat->localip, new_dccchat->localport), newuser, "");
				}
				else currentscreen = dcc_chat_screen_by_name(newuser);
				set_dccchat_update_status(new_dccchat, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				return(E_CHANGE_SCREEN);
			}
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_NEW_DCC_SEND){
		formcode = get_new_dccsend_info(formkey, newuser);
		if (formcode == E_OK){
			if (strcmp(newuser, currentserver->nick) == 0){
				print_all("You can not DCC send to yourself.\n");
				currentform = 0;
			}
			else if (!currentserver->active){
				print_all("Must be connected to a server to DCC send a file.\n");
				currentform = 0;
			}					
			else currentform = CF_DCC_SEND_FILE;
			strcpy(newfile, configuration.dcculpath);
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_FAVORITE_DCC_SEND){
		formcode = get_favorite_user_dccsend_info(formkey, newuser);
		if (formcode == E_OK){
			if (strcmp(newuser, currentserver->nick) == 0){
				print_all("You can not DCC send to yourself.\n");
				currentform = 0;
			}
			else if (!currentserver->active){
				print_all("Must be connected to a server to DCC send a file.\n");
				currentform = 0;
			}
			else currentform = CF_DCC_SEND_FILE;
			strcpy(newfile, configuration.dcculpath);
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_DCC_SEND_FILE){
		formcode = select_file(formkey, newfile, 0);
		if (formcode == E_OK){
			new_dccfile = add_outgoing_dcc_file(transferscreen, newuser, newfile);
			if (new_dccfile != NULL){
				start_outgoing_dcc_file(new_dccfile);

				/* truncate the filename by dropping path */		
				for (i = strlen(newfile) - 1; i >= 0; i--){
					if (newfile[i] == '/'){
						i++;
						break;
					}
				}
			
				sendcmd_server(currentserver, "PRIVMSG", create_ctcp_command("DCC SEND", "%s %lu %d %d", 
				newfile + i, new_dccfile->localip, new_dccfile->localport, new_dccfile->size), newuser, "");
				update = U_ALL_REFRESH;
				key = E_NOWAIT;
				currentform = 0;
			}
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_EDIT_USER_FAVORITES){
		formcode = edit_users(formkey, CONFIG_FAVORITE_USER_LIST);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_NOWAIT){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_EDIT_USER_IGNORED){
		formcode = edit_users(formkey, CONFIG_IGNORED_USER_LIST);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_NOWAIT){
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_IDENTITY){
		formcode = get_identity_info(formkey);
		if (formcode == E_OK){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
			vprint_all("Settings will take effect when connecting to a new server.\n");
		}
		else if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_CLIENT_OPTIONS){
		formcode = get_client_options(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_DCC_OPTIONS){
		formcode = get_dcc_info(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_CTCP_OPTIONS){
		formcode = get_ctcp_info(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_DCC_SEND_OPTIONS){
		formcode = get_dcc_send_info(formkey);
		if (formcode == E_OK || formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_HELP_ABOUT){
		formcode = view_about(formkey);
		if (formcode == E_OK){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	/* transfer select options */
	else if (currentform == CF_FILE_TRANSFER){
		formcode = get_transfer_select_options(formkey);
		if (formcode == E_CANCEL){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_TRANSFER_STOP){
			if (dcc_file_exists(inscreen->screen, transferscreen->selectedfile)){
				remove_dcc_file(transferscreen->selectedfile);
			}
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
		else if (formcode == E_TRANSFER_INFO){
			if (dcc_file_exists(inscreen->screen, transferscreen->selectedfile)){
				currentform = CF_FILE_TRANSFER_INFO;
			}
			else currentform = 0;
			update = U_ALL_REFRESH;
			key = E_NOWAIT;
		}
	}
	else if (currentform == CF_FILE_TRANSFER_INFO){
		formcode = get_transfer_info(formkey, transferscreen->selectedfile);
		if (formcode == E_CANCEL || formcode == E_OK){
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	else if (currentform == CF_FILE_ALLOW){
		formcode = allow_transfer(formkey, newdccfile);
		if (formcode == E_CANCEL){
			if (dcc_file_exists(transferscreen, newdccfile)){
				remove_dcc_file(newdccfile);
			}
			set_transfer_update_status(transferscreen, U_ALL_REFRESH);
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}		
		else if (formcode == E_OK){
			newdccfile->allowed = 1;
			update = U_ALL_REFRESH;
			currentform = 0;
			key = E_NOWAIT;
		}
	}

	if (update){
		if (inscreen->type == SERVER) set_server_update_status(inscreen->screen, update);
		else if (inscreen->type == CHANNEL) set_channel_update_status(inscreen->screen, update);
		else if (inscreen->type == CHAT) set_chat_update_status(inscreen->screen, update);
		else if (inscreen->type == DCCCHAT) set_dccchat_update_status(inscreen->screen, update);
		else if (inscreen->type == TRANSFER) set_transfer_update_status(inscreen->screen, update);
		else if (inscreen->type == LIST) set_list_update_status(inscreen->screen, update);
		else if (inscreen->type == HELP) set_help_update_status(inscreen->screen, update);
		set_menuline_update_status(menuline, U_ALL_REFRESH);
		set_statusline_update_status(statusline, U_ALL_REFRESH);
	}
	return(key);
}

menu *build_window_menu(int startx, int starty){
	menu *menu;
	menuitem *item;
	screen *current;
	char menutext[256];
	char menudesc[256];
	int idnum;

	menu = init_menu(startx, starty, "Window", 'W');
	current = screenlist;
	idnum = 0xF000;

	while (current != NULL){
		bzero(menutext, 256);
                if (current->type == SERVER){
			if (((server *)(current->screen))->active){
				sprintf(menutext, " Server: %s port %d ", 
					((server *)(current->screen))->server, ((server *)(current->screen))->port);
				sprintf(menudesc, "%s@%s:%d", ((server *)(current->screen))->nick,
					((server *)(current->screen))->server, ((server *)(current->screen))->port); 
			}
			else{
				sprintf(menutext, " Server Screen (not connected) "); 
				sprintf(menudesc, "Server (not connected)");
			}
		}
                else if (current->type == CHANNEL){
			sprintf(menutext, " Channel: %s on %s ", ((channel *)(current->screen))->channel, 
				((server *)((channel *)(current->screen))->server)->server);
			sprintf(menudesc, "%s@%s#%s", (((channel *)(current->screen))->server)->nick, 
				((channel *)(current->screen))->channel, 
                                (((channel *)(current->screen))->server)->server);
                }
		else if (current->type == TRANSFER){
                        sprintf(menutext, " Transfer Screen ");
                        sprintf(menudesc, " Transfers ");	
                }   
                else if (current->type == CHAT){
			sprintf(menutext, " Chat: %s on %s ", ((chat *)(current->screen))->nick, 
				((server *)((chat *)(current->screen))->server)->server);
			sprintf(menudesc, "%s@%s", ((chat *)(current->screen))->nick, 
                                ((server *)((chat *)(current->screen))->server)->server);
                }
                else if (current->type == DCCCHAT){
			sprintf(menutext, " DCC Chat: %s ", ((dcc_chat *)(current->screen))->nick);
			sprintf(menudesc, "%s", ((dcc_chat *)(current->screen))->nick);
                }
                else if (current->type == LIST){
			sprintf(menutext, " Channel List: %s ", ((list *)(current->screen))->servername); 
			sprintf(menudesc, " ");
                }
                else if (current->type == HELP){
			sprintf(menutext, " Help: %s ", ((help *)(current->screen))->name); 
			sprintf(menudesc, " ");
                }

		item = add_menu_item(menu, menutext, menudesc, 0, M_SELECTABLE, idnum, NULL);
                if (currentscreen==current){
                	menu->selected = item;  
                }
		current = current->next;
		idnum++;
	}
	add_menu_item(menu, "", "", 0, M_DIVIDER, 0, NULL);
	add_menu_item(menu, " Next Window ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(menu, " Previous Window ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	return (menu);
}

int draw_menuline_screen(menuwin *menuline, menubar *menubar){
	screen *current;
	int screennum;
	char repchar;
	int repattr;
	int updatemenu;

	updatemenu = 0;
	curs_set(0);
	print_menubar(menuline, menubar);
	
	// count the number of present screens in the first loop for text alignment
	screennum=0;
	current=screenlist;
	while(current!=NULL){ 
		if (current->hidden == 0) screennum++;
		current=current->next;
	}
	
	// print out screen list on the menuline
	current=screenlist;
	while(current!=NULL){
		repchar='*';
		repattr = A_NORMAL;
		if (current->type==SERVER){
			repchar='S';
			if (server_update_status(current->screen)) repattr = A_BOLD;
		}	
		else if (current->type==CHANNEL){
			repchar='C';
			if (channel_update_status(current->screen)) repattr = A_BOLD;

		}	
		else if (current->type==TRANSFER){
			repchar='T';
			if (transfer_update_status(current->screen)) repattr = A_BOLD;
		}	
		else if (current->type==CHAT){
			repchar='c';
			if (chat_update_status(current->screen)) repattr = A_BOLD;
		}	
		else if (current->type==DCCCHAT){
			repchar='d';
			if (dccchat_update_status(current->screen)) repattr = A_BOLD;
		}	
		else if (current->type==LIST){
			repchar='L';
			// if (list_update_status(current->screen)) repattr = A_BOLD;
		}	
		else if (current->type==HELP){
			repchar='H';
			// if (list_update_status(current->screen)) repattr = A_BOLD;
		}	
		
		// if some screen has updated build the new menu selection window	
		if (repattr == A_BOLD) updatemenu = 1;

		if (currentscreen==current) repattr = A_REVERSE | A_BOLD;
		wattrset(menuline->menuline, COLOR_PAIR(LIST_COLOR_B*16+LIST_COLOR_F));
		wattron(menuline->menuline, repattr);

		if (current->hidden == 0){
			mvwaddch(menuline->menuline, 0, COLS-screennum-1, repchar);
			screennum--;
		}
		current=current->next;
	}

	/* make the window menu when a screen changes */
	if (screenupdated){
		windowmenu = build_window_menu(22,1);
		screenupdated = 0;
	}
	wrefresh(menuline->menuline);
	return(1);
}

int parse_input(server *currentserver, char *buffer){
	char command[MAXDATASIZE];
	char parameters[MAXDATASIZE];
	char nick[MAXDATASIZE];
	char message[MAXDATASIZE];
	char server[MAXDATASIZE];
	char channelname[MAXDATASIZE];
	char subcommand[MAXDATASIZE];
	char subparameters[MAXDATASIZE];
	char file[MAXDATASIZE];

	int port;

	screen *S;
	int pnum;

	strcpy(command, "");
	strcpy(parameters, "");

	if (!sscanf(buffer, "/%s %[^\n]", command, parameters)){
		return(E_NONE);
	}
	
	if (strcasecmp(command, "quit") == 0){
		return(E_EXIT);
	}

	else if (strcasecmp(command, "close") == 0){
		return(E_CLOSE);
	}

	else if (strcasecmp(command, "disconnect") == 0){
		return(E_DISCONNECT);
	}


	if (strcasecmp(command, "connect") == 0 || strcasecmp(command, "server") == 0){
		pnum = sscanf(parameters, "%s %d", server, &port);
		if (pnum < 1){
			vprint_all("Usage: /%s <server> [port]\n", command);
			return(E_OTHER);
		}
		else {
			if (pnum == 1) port = 6667;
			if (currentserver->active) disconnect_from_server(currentserver);
			strcpy(currentserver->server, server);
			currentserver->port = port;
			currentserver->connect_status = 0;
			return(E_OTHER);
		}
	}

	else if (strcasecmp(command, "msg") == 0 || strcasecmp(command, "message") == 0 || 
		strcasecmp(command, "chat") == 0){
		chat *C;
		
		pnum = sscanf(parameters, "%s %[^\n]", nick, message);
		if (pnum < 1){
			vprint_all("Usage: /%s <nick> <message>\n", command);
			return(E_OTHER);
		}
		else if (strcmp(nick, currentserver->nick) == 0) print_all("You can not chat with yourself.\n");
		else if (!currentserver->active) print_all("Must be connected to a server to chat.\n");
		else {

			S = chat_screen_by_name(nick);
			if (S == NULL){
				C = add_chat(nick, currentserver);
				set_chat_update_status(C, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				if (pnum == 2){
					printmsg_chat(C, nick, message);
					sendmsg_chat(C, message);
				}
				return(E_CHANGE_SCREEN);
			}
			else {
				currentscreen = S;
				C = S->screen;
				set_chat_update_status(C, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				if (pnum == 2){
					printmsg_chat(C, nick, message);
					sendmsg_chat(C, message);
				}
				return(E_CHANGE_SCREEN);
			}				
		}	
	}

	else if (strcasecmp(command, "ctcp") == 0){
		if (sscanf(parameters, "%s %[^\n]", nick, message) == 2){
			sendcmd_server(currentserver, "PRIVMSG", create_ctcp_message(message), nick, currentserver->nick);
		}
		else vprint_all("Usage: /ctcp <nick> <message>|<command>\n");
		return(E_OTHER);
	}

	else if (strcasecmp(command, "dcc") == 0){
		dcc_chat *D;
		dcc_file *S;
		int i;
		
		strcpy(subcommand, "");
		strcpy(subparameters, "");
		pnum = sscanf(parameters, "%s %[^\n]", subcommand, subparameters);
		if (pnum < 1){
			vprint_all("Usage: /dcc chat|send <nick> ...\n");
			return(E_OTHER);
		}
		else {
			if (strcasecmp(subcommand, "chat") == 0){
				pnum = sscanf(subparameters, "%s %[^\n]", nick, message);
				if (pnum < 1){
					vprint_all("Usage: /dcc chat <nick>\n");
					return(E_OTHER);
				}
				else {
					if (strcmp(nick, currentserver->nick) == 0) print_all("You can not DCC chat with yourself.\n");
					else if (!currentserver->active) print_all("Must be connected to a server to DCC chat.\n");

					else {
						D = dcc_chat_by_name(nick); 
						if (D == NULL){ 
							D = add_outgoing_dcc_chat(nick, currentserver->nick, currentserver);
							start_outgoing_dcc_chat(D);
							sendcmd_server(currentserver, "PRIVMSG",
							create_ctcp_command("DCC CHAT chat", "%lu %d", D->localip, D->localport), nick, "");
						}
						else currentscreen = dcc_chat_screen_by_name(nick);
						set_dccchat_update_status(D, U_ALL_REFRESH);
						set_menuline_update_status(menuline, U_ALL_REFRESH);
						set_statusline_update_status(statusline, U_ALL_REFRESH);
						return(E_CHANGE_SCREEN);
					}
				}
			}

			if (strcasecmp(subcommand, "send") == 0){
				pnum = sscanf(subparameters, "%s %[^\n]", nick, file);
				if (pnum < 2){
					vprint_all("Usage: /dcc send <nick> <filename>\n");
					return(E_OTHER);
				}
				else{
					if (strcmp(nick, currentserver->nick) == 0) print_all("You can not DCC send to yourself.\n");
					else if (!currentserver->active) print_all("Must be connected to a server to DCC send a file.\n");
					else {
						vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Attempting to DCC Send file %s to %s.\n",
							file, nick);

						/* truncate the filename by dropping path */		
						for (i = strlen(file) - 1; i >= 0; i--){
							if (file[i] == '/'){
								break;
							}
						}
						i++;
			
						S = add_outgoing_dcc_file(transferscreen, nick, file);
						if (S != NULL){
							start_outgoing_dcc_file(S);

							sendcmd_server(currentserver, "PRIVMSG", create_ctcp_command("DCC SEND", "%s %lu %d %d", 
							file + i, S->localip, S->localport, S->size), nick, "");
						}
					}
				}
			}
		}
	}

	else if (strcasecmp(command, "part") == 0){
		channel *C;
		
		if (get_next_word(parameters, channelname)){
			send_current_server(&buffer[1]);
			return(E_OTHER);
		}
		else {
			if (currentscreen->type == CHANNEL){
				C = currentscreen->screen;
				sendcmd_server(currentserver, "PART", C->channel, "", currentserver->nick);
				return(E_OTHER);
			}
		}
	}

	else if (strcasecmp(command, "me") == 0){
		char message[MAXDATASIZE];
		channel *C;
		
		if (currentscreen->type == CHANNEL){
			C = currentscreen->screen;
			sendmsg_channel(C, create_ctcp_command("ACTION", parameters));
			sprintf(message, "%c%d,%d* %s %s%c%d,%d *\n", 
			3, CTCP_COLOR_F, CTCP_COLOR_B, C->server->nick, parameters, 3, CTCP_COLOR_F, CTCP_COLOR_B);
			print_channel(C, message);
			return(E_OTHER);
		}
	}
	
	/* intercept /nick to record old name, if server refuses we rename back */
	else if (strcasecmp(command, "nick") == 0){
		pnum = sscanf(parameters, "%s", nick);
		if (pnum < 1){
			vprint_all("Usage: /nick <nick>\n");
		}
		else{
			sendcmd_server(currentserver, "NICK", nick, "", currentserver->nick);
			strcpy(currentserver->lastnick, currentserver->nick);
			strcpy(currentserver->nick, nick);

		}
		return(E_OTHER);
	}

	else{
		// send non empty buffer
		if (buffer[0] != 0){
			if (!currentserver->active) print_all("Connect to a server first.\n");
			else send_current_server(&buffer[1]);
		}
	}
	return(1);
} 
		

void parse_message(server *currentserver, char *buffer){ 	
	char scratch[MAXDATASIZE];
	char command[MAXDATASIZE];
	char cmdnick[MAXDATASIZE];
	char cmduser[MAXDATASIZE];
	char cmdhost[MAXDATASIZE];
	char cmdparam[MAXDATASIZE];
	char message[MAXDATASIZE];
	char ctcpmessage[MAXDATASIZE];
	char fromchannel[MAXDATASIZE];
	char dest[MAXDATASIZE];

	char touser[555];
	char userstatus;
	int op, voice;
	screen *current;
	
	command_parse(buffer, command, cmdparam, cmdnick, cmduser, cmdhost);

	// respond to a ping
	if (strcmp(command,"PING")==0){
		sendcmd_server(currentserver, "PONG", cmdparam, "", "");
	}

	else if (strcasecmp(command,"PRIVMSG")==0){
		char rawmessage[MAXDATASIZE];
		channel *sourcechannel;
		chat *sourcechat;
		int ctcp;

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, rawmessage); 

		// check what channel the destination message goes to
		// or what whose chat screen this is coming from
		sourcechannel = channel_by_name(dest);
 		sourcechat = chat_by_name(cmdnick);

		// check to see if this is a ctcp encoded message
		// if so check and respond to ctcp request if required otherwise display the decoded message
		// notice tells the user what was done, message is the decoded ctcp message, and
		// reply is what goes back to the irc server

		ctcp = parse_ctcp(message, rawmessage);
		if (ctcp){
			if (translate_ctcp_message(message, cmdnick, cmduser, cmdhost, ctcpmessage)){
				if (sourcechannel != NULL ) print_channel(sourcechannel, ctcpmessage);
				else if (sourcechat != NULL ) print_channel(sourcechannel, ctcpmessage);
			}
			else if (!execute_ctcp(currentserver, message, cmdnick, cmduser, cmdhost, dest)){
				if (sourcechannel != NULL ) print_channel(sourcechannel, message);
				else if (sourcechat != NULL ) print_channel(sourcechannel, message);
			}
		}

		// If sent from a channel, show message in the correct channel screen. If from a 
		// open user chat print it to that screen, otherwise open a user chat and print
		// there

		else if (sourcechannel != NULL){ 			
			printmsg_channel(sourcechannel, cmdnick, message);
		}			
		else if (sourcechat != NULL){
	 		printmsg_chat(sourcechat, cmdnick, message);
		}

		
		else if (strncasecmp(dest, currentserver->nick, MAXNICKLEN)==0){
			if (!config_user_exists(&configuration, CONFIG_IGNORED_USER_LIST, cmdnick) ||
				config_user_exists(&configuration, CONFIG_FAVORITE_USER_LIST, cmdnick)){
				sourcechat = add_chat(cmdnick, currentserver);
				set_chat_update_status(sourcechat, U_ALL_REFRESH);
				set_menuline_update_status(menuline, U_ALL_REFRESH);
				set_statusline_update_status(statusline, U_ALL_REFRESH);
				printmsg_chat(sourcechat, cmdnick, message);
			}
			else vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Ignoring message from %s.\n", cmdnick); 
		}
	}
	
	else if (strcasecmp(command,"JOIN")==0){
		channel *C;

		get_next_param(cmdparam, dest);
		sprintf(touser, "%c%d,%d%s has joined %s\n", 3, JOIN_COLOR_F, JOIN_COLOR_B, cmdnick, dest);
		
		// if i'm joining, note what channel i'm about to enter 
		// replace this code with something that will auto create missing channels
		if (strcmp(cmdnick, currentserver->nick) == 0){
			C = add_channel(dest, currentserver);
			C->active = 1;
			set_channel_update_status(C, U_ALL_REFRESH);
			set_menuline_update_status(menuline, U_ALL_REFRESH);
                        set_statusline_update_status(statusline, U_ALL_REFRESH);
		}
		else {
			C = channel_by_name(dest);
			if (C != NULL) {
				add_user(C, cmdnick, 0, 0);
				set_channel_update_status(C, U_USER_REFRESH);
			}
		}
		print_channel(C, touser);
	}
			
	else if (strcasecmp(command,"PART")==0){
		channel *C;

		get_next_param(cmdparam, dest);
		sprintf(touser, "%c%d,%d%s has left %s\n", 3, PART_COLOR_F, PART_COLOR_B, cmdnick, dest);
		
		// if i'm leaving, turn off the active flag in this channel
		C=channel_by_name(dest);
		if (C != NULL){
			print_channel(C, touser);
			remove_user(C, cmdnick);
			if (strcmp(cmdnick, currentserver->nick)==0) C->active = 0;
		}
	}	
	
	// if a user quits, nick must be removed from all joined channels  
	else if (strcasecmp(command,"QUIT")==0){
		screen *current;

		get_next_param(cmdparam, message);
		sprintf(touser, "%c%d,%d%s has quit IRC, %s\n", 3, PART_COLOR_F, PART_COLOR_B, cmdnick, message);

		current=screenlist; 
		while(current!=NULL){
			if (current->type==CHANNEL){
				if (remove_user (current->screen, cmdnick)){
					print_channel(current->screen, touser);		
				}
			}
			current=current->next;
		}
	}               
	
	else if (strcasecmp(command,"NOTICE")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(NOTICE_COLOR_F, NOTICE_COLOR_B, "%s\n", message);
	}
		
	// :user!host@domain KICK #channel nick :message
	else if (strcasecmp(command,"KICK")==0){
		channel *C;

		get_next_param(cmdparam, fromchannel); 				
		get_next_param(cmdparam, dest); 				
		get_next_param(cmdparam, message); 					
		sprintf(touser,"%c%d,%d%s kicks %s, %s\n", 3, KICK_COLOR_F, KICK_COLOR_B, cmdnick, dest, message );

		C = channel_by_name(fromchannel);				
		print_channel(C, touser);
		remove_user(C, dest);				
	}

	// if a nick is changed go through all channels and rename nicks
	// add chat nick changes later
	// :nick!host@domain NICK :newnick
	else if (strcasecmp(command,"NICK")==0){

		get_next_param(cmdparam, dest); 				
		sprintf(touser,"%c%d,%d%s is now known as %s\n", 3, RENAME_COLOR_F, RENAME_COLOR_B, cmdnick, dest);

		current=screenlist; 
		while(current!=NULL){
			if (current->type==CHANNEL){
				userstatus = get_user_status(current->screen, cmdnick, &op, &voice);

				if (remove_user (current->screen, cmdnick)){
					print_channel(current->screen, touser);

					/* preserve user status (op, voice) */
					add_user(current->screen, dest, op, voice);		
					set_channel_update_status(current->screen, U_ALL_REFRESH);
				}
			}
			// if my nick is changed update the server nick as well
			else if (current->type==SERVER){
				if (strcmp(((server*)(current->screen))->nick, cmdnick) == 0){
					print_server(current->screen, touser);
					strcpy(((server*)(current->screen))->nick, dest);
					set_statusline_update_status(statusline, U_ALL_REFRESH);		
				}
			}
			current=current->next;
		}
	}

	// :nick!host@domain MODE #channel/user +/-attribute [match] 
	else if (strcasecmp(command,"MODE")==0){
		channel *C;
		get_next_param(cmdparam, dest); 				
		get_next_param(cmdparam, message);
		if (cmdparam[0]!=0){ 					
			sprintf(touser,"%c%d,%d%s sets mode %s %s for %s\n", 3, 
				MODE_COLOR_F, MODE_COLOR_B, cmdnick, message, cmdparam, dest);
		}
		else { 					
			sprintf(touser,"%c%d,%d%s sets mode %s for %s\n", 3, 
				MODE_COLOR_F, MODE_COLOR_B, cmdnick, message, dest);
		}
		
		C = channel_by_name(dest);
		if (C == NULL) print_server(currentserver, touser);
		else{
			print_channel(C, touser);
			change_user_status(C, cmdparam, message);
		}
	}		

	// :nick!host@domain INVITE nick :#channel
	else if (strcasecmp(command,"INVITE")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		sprintf(touser,"%c%d,%d%s has been invited to join %s\n", 3, 
			INVITE_COLOR_F, INVITE_COLOR_B, dest, message);
		print_server(currentserver, touser);
	}
	
	else if (strcasecmp(command,"ERROR")==0){
		// get_next_param(cmdparam, message); 				
		sprintf(touser,"%c%d,%d%s\n", 3, ERROR_COLOR_F, ERROR_COLOR_B, cmdparam );
		print_all(touser);				
	}

/* numeric commands *************************************************************************************/

	/* 001 - welcome message */
	else if (strcmp(command,"001")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 002 - hostinfo */
	else if (strcmp(command,"002")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 003 - creation time */
	else if (strcmp(command,"003")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 004 - version info and user and channel modes (weird one) */
	else if (strcmp(command,"004")==0){
		char usermodes[MAXDATASIZE];
		char chanmodes[MAXDATASIZE];
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		get_next_param(cmdparam, message);
		get_next_param(cmdparam, usermodes);
		get_next_param(cmdparam, chanmodes);
		vprint_server(currentserver, "Server supports user modes %s and channel modes %s\n", usermodes, chanmodes);
	}		

	/* commands availble (weird one), rfc says bounce message (skipping it for now) */
	else if (strcmp(command,"005")==0);

	/* 250 - number of connections */
	else if (strcmp(command, "250")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 251 - userinfo */
	else if (strcmp(command, "251")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 252 - number of ops present */
	else if (strcmp(command, "252")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, scratch);
		vprint_server(currentserver, "There are %s IRC ops online\n", scratch);
		print_server(currentserver, touser);

	}

	/* 253 - number of unknown connections present */
	else if (strcmp(command, "253")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, scratch);
		vprint_server(currentserver, "There are %s unknown connections\n", scratch); 
	}

	/* 254 - number of channels present */
	else if (strcmp(command, "254")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, scratch);
		vprint_server(currentserver, "There is a total of %s channels formed\n", scratch);
	}

	/* 255 - client and server numbers */
	else if (strcmp(command, "255")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 263 - server too busy */
	else if (strcmp(command, "263")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 265 - local users present */
	else if (strcmp(command, "265")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* 266 - global users present */
	else if (strcmp(command, "266")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	// 302 RPL_USERHOST
	// 303 RPL_ISON

	// 301 RPL_AWAY
	else if (strcmp(command,"301")==0){
		char usernick[MAXDATASIZE];
                get_next_param(cmdparam, dest);
                get_next_param(cmdparam, usernick);
                get_next_param(cmdparam, message);
                sprintf(touser,"User %s is away, %s\n", usernick, message);
                print_server(currentserver, touser);
        }

	// 305 RPL_UNAWAY
	else if (strcmp(command,"305")==0){
                get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		sprintf(touser,"%s\n", message);
		print_server(currentserver, touser);
	}
	
	// 306 RPL_NOAWAY
	else if (strcmp(command,"306")==0){
                get_next_param(cmdparam, dest);
                get_next_param(cmdparam, message);
                sprintf(touser,"%s\n", message);
                print_server(currentserver, touser);
        }

	// 307 ??????? not in rfc 
	// :astro.ga.us.dal.net 307 _sn00p_ alex :has identified for this nick


	// 311 RPL_WHOISUSER
	// first line of /whois info 
	// :server 311 mynick usernick username host.domain * :name
	else if (strcmp(command,"311")==0){
		char usernick[MAXDATASIZE];
		char host[MAXDATASIZE];
		char username[MAXDATASIZE];
		char name[MAXDATASIZE];

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, usernick);
		get_next_param(cmdparam, username);
		get_next_param(cmdparam, host);
		get_next_param(cmdparam, message);
		get_next_param(cmdparam, name);
		sprintf(touser,"%s is %s, %s@%s\n", usernick, name, username, host);
		//print_server(currentserver, touser);
		print_all(touser);
	}

	// 312 RPL_WHOISSERVER
	// second line of /whois info 
	//:server 312 mynick usernick servername :message
	else if (strcmp(command,"312")==0){
		char usernick[MAXDATASIZE];
		char servername[MAXDATASIZE];

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, usernick);
		get_next_param(cmdparam, servername);
		get_next_param(cmdparam, message);
		sprintf(touser,"%s is using %s (%s)\n", usernick, servername, message);
		//print_server(currentserver, touser);
		print_all(touser);

	}

	// 313 RPL_WHOISOPERATOR
        // <nick> :is an IRC operator
	else if (strcmp(command,"313")==0){
		char usernick[MAXDATASIZE];
                get_next_param(cmdparam, dest);
                get_next_param(cmdparam, usernick);
                get_next_param(cmdparam, message); 
                sprintf(touser,"%s %s\n", usernick, message);
                //print_server(currentserver, touser);
		print_all(touser);

        }
	
	// 314 RPL_WHOWASUSER
        // "<nick> <user> <host> * :<real name>"	
	// reply to whowas
	else if (strcmp(command,"314")==0){
		char usernick[MAXDATASIZE];
		char host[MAXDATASIZE];
		char username[MAXDATASIZE];
		char name[MAXDATASIZE];

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, usernick);
		get_next_param(cmdparam, username);
		get_next_param(cmdparam, host);
		get_next_param(cmdparam, message);
		get_next_param(cmdparam, name);
		sprintf(touser,"%s was previously being used by %s,%s@%s\n", usernick, name, username, host);
		//print_server(currentserver, touser);
		print_all(touser);

	}

	// 317 RPL_WHOISIDLE *********
        // "<nick> <integer> :seconds idle"
	// idle time line of /whois reply  
	else if (strcmp(command,"317")==0){
		char usernick[MAXDATASIZE];
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, usernick);
		get_next_param(cmdparam, message);
		sprintf(touser,"%s has been idle for %s seconds\n", usernick, message);
		//print_server(currentserver, touser);
		print_all(touser);

	}

	// 318 RPL_ENDOFWHOIS
	else if (strcmp(command,"318")==0);

	// 319 RPL_WHOISCHANNELS
	// line of /whois info that says which channels the user is in 
	//:server 319 mynick usernick :channels
	else if (strcmp(command,"319")==0){
		char usernick[MAXDATASIZE];

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, usernick);
		get_next_param(cmdparam, message);
		sprintf(touser,"%s jas joined %s\n", usernick, message);
		//print_server(currentserver, touser);
		print_all(touser);

	}
	
	// 369 RPL_ENDOFWHOWAS
	

	// channel list
	// 321 start of new list
	else if (strcmp(command,"321")==0){
		list *L;
		if (list_by_name(cmdnick) == NULL){
			L = add_list(currentserver);
			L->active = 1;
			set_menuline_update_status(menuline, U_ALL_REFRESH);
                        set_statusline_update_status(statusline, U_ALL_REFRESH);
		}
		else{
		
		}
	}
			
	else if (strcmp(command,"322")==0){
		char channelname[MAXDATASIZE];
		char people[MAXDATASIZE];
		char topic[MAXDATASIZE];
		char topic1l[MAXDATASIZE];		
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, channelname);
		get_next_param(cmdparam, people);
		get_next_param(cmdparam, topic);
			
		strncpy(topic1l, topic, COLS - 34);
		add_list_channel(active_list_by_server(currentserver), channelname, atoi(people), topic1l, 1);
	}
	
	else if (strcmp(command,"323")==0){
		list *L;
		L = active_list_by_server(currentserver);
		if (L != NULL) L->active = 0;
	}

	// Channel Website add
	// :<server> 328 <user> <channel> :http
	else if (strcmp(command,"328")==0){
		char channelname[MAXDATASIZE];
		char url[MAXDATASIZE];

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, channelname);
		get_next_param(cmdparam, url);
					
		sprintf(touser, "Channel %s website can be found at %s\n", channelname, url);
		print_channel(channel_by_name(channelname), touser);
	}
	
	// :lineone.uk.eu.dal.net 332 _sn00p_ #dc-isoz :ab0,1[Welcome to #DC-iSOZ]
	// channel welcome message
	else if (strcmp(command,"332")==0){
		channel *C;
		
		strcpy(fromchannel, "");
		strcpy(message, "");

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, fromchannel);
		get_next_param(cmdparam, message);

		C = channel_by_name(fromchannel);
		sprintf(touser, "%s\n", message);
		print_channel(C, touser);	
	}

	//channel started by message
	// :lineone.uk.eu.dal.net 333 _sn00p_ #dc-isoz roife 992889330
	else if (strcmp(command,"333")==0){
		char startnick[MAXDATASIZE];
		char starttime[MAXDATASIZE];
		time_t starttime_t;
		char *startdate;
		
		channel *C;
		
		strcpy(fromchannel, "");
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, fromchannel);
		get_next_param(cmdparam, startnick);
		get_next_param(cmdparam, starttime);
		
		starttime_t = atol(starttime);
		startdate = ctime(&starttime_t);
		C = channel_by_name(fromchannel);
		sprintf(touser, "Channel topic set by %s on %s\n", startnick, startdate);
		print_channel(C, touser);	
	}

	
	//user list
	else if (strcmp(command,"353")==0){
		char prefix[64];
		char channelname[64];
		char userstring[512];
		char user[64];
		channel *C;
		
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, prefix);
		get_next_param(cmdparam, channelname);
		get_next_param(cmdparam, userstring);

		C = channel_by_name(channelname);
		//sprintf(scratch, "Nick:%s Channel:%s OP:%s Users:%s\n", dest, channelname, prefix, userstring);
		//printf("%s\n", command);	
		while(get_next_word(userstring, user)){
			bzero(scratch, 63);
			sprintf(scratch, " %-.9s\n", user);
			add_user(C, user, 0, 0);
		}
	}
	/* if user list is completely sent, refresh the user list */
	/* also place top and selected at top of the list         */
	else if (strcmp(command,"366")==0){
		channel *C;

		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, fromchannel); 
		C = channel_by_name(fromchannel);
		if (C != NULL){
			C->top = C->userlist;
			C->selected = C->userlist;
			set_channel_update_status(C, U_USER_REFRESH);
			// refresh_user_list(C);
		}
	}

	/* message of the day */
	else if (strcmp(command,"372")==0||strcmp(command,"375")==0||strcmp(command,"376")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message);
		vprint_server(currentserver, "%s\n", message);
	}

	/* no such nick/channel */
	else if (strcmp(command,"401")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s: %s.\n", message, dest); 
	}

	/* no such channel */
	else if (strcmp(command,"403")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s: %s.\n", message, dest); 
	}

	/* Cannot send to channel */
	else if (strcmp(command,"404")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s: %s.\n", message, dest); 
	}

	/* List output too large */
	else if (strcmp(command,"416")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Error processing command %s, %s.\n", message, cmdparam); 
	}

	/* Unknown command */
	else if (strcmp(command, "421")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Unknown command: %s.\n", dest); 
	}

	/* Nick held */
	else if (strcmp(command, "432")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s.\n", dest); 
	}

	/* nick already in use */
	else if (strcmp(command, "433")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s\n", message); 

		/* if first configuration nick is taken try the alternate */
		if (strcmp(currentserver->nick, configuration.nick) == 0 && currentserver->nickinuse != 2){
			currentserver->nickinuse = 2;
			vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Primary nick taken, trying alternate nick.\n", dest); 
			sendcmd_server(currentserver, "NICK", configuration.alt_nick, "", "");
			sendcmd_server(currentserver, "MODE", configuration.mode, configuration.alt_nick, "");
			strcpy(currentserver->lastnick, currentserver->nick);
			strcpy(currentserver->nick, configuration.alt_nick);
		}
		else{
			vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Use /nick to select a different nickname.\n", dest); 
			strcpy(currentserver->nick, currentserver->lastnick);
		}
		set_statusline_update_status(statusline, U_ALL);
	}

	/* not in channel */
	else if (strcmp(command, "442")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "You are not in channel %s.\n", dest); 
	}
	
	/* register first */
	else if (strcmp(command, "451")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s\n", message); 
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Make sure you have logged in correctly and used the proper nickname.\n"); 
		print_server(currentserver, touser);
	}

	/* Cannot join channel (+i) */
	else if (strcmp(command,"473")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s %s\n", message, dest); 
	}

	/* Cannot join channel (+b) */
	else if (strcmp(command,"474")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "%s %s\n", message, dest); 
	}
	
	/* identify to a registered nick */
	else if (strcmp(command,"477")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "%s\n", message); 
	}

	/* not an op */
	else if (strcmp(command,"482")==0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Channel %s: %s\n", dest, message); 
	}
	
	/* if this is an unknown numeric command, just print it to the screen */
	else if (atoi(command)>0){
		get_next_param(cmdparam, dest);
		get_next_param(cmdparam, message); 
		vprint_server(currentserver, "(%s -> %s) %s %s\n", command, dest, message, cmdparam);
	}
}

int ctrl_c_handler(void){
	ctrl_c_occured=1;
	ungetch(3);
	return(0);
}

int alarm_handler(void){
	alarm_occured=1;
	return(0);
}

int resize_handler(void){
	// remove_resize_handler();
	resize_occured=1;
	return(0);
}

int int_handler(void){
	curs_set(1);
        endwin();
	printf("SIGINT caught. Exiting...\n");

	exit(0);
}

int abrt_handler(void){
	curs_set(1);
        endwin();
	printf("SIGABRT caught. Exiting...\n");
	exit(0);
}

int pipe_handler(void){
	/* don't do anything at this point */
	return(0);
}

 
int inst_ctrlc_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)ctrl_c_handler;
	err=sigaction(SIGINT, &act, NULL);
 
	return(err);
}

int inst_abrt_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)abrt_handler;
	err=sigaction(SIGABRT, &act, NULL);
 
	return(err);
}

int inst_term_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)int_handler;
	err=sigaction(SIGTERM, &act, NULL);
 
	return(err);
}

int inst_alarm_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)alarm_handler;
	err=sigaction(SIGALRM, &act, NULL);
 
	return(err);
}

int inst_resize_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)resize_handler;
	err=sigaction(SIGWINCH, &act, NULL);
 
	return(err);
}

int inst_pipe_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = (void *)pipe_handler;
	err=sigaction(SIGPIPE, &act, NULL);
 
	return(err);
}

int remove_resize_handler(){
	int err;
	struct sigaction act;
 
	bzero(&act, sizeof(struct sigaction));
	act.sa_handler = SIG_DFL;
	err=sigaction(SIGWINCH, &act, NULL);
 
	return(err);
}

/* communication functions ******************************************************/


int disconnect_from_server (server *S){
	char buffer[MAXDATASIZE];

	sprintf(buffer,"QUIT\n");
	send_all(S->serverfd, buffer, strlen(buffer));
	S->active = 0;
	close(S->serverfd);
	vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B,"Disconnected from %s\n", S->server);
	set_statusline_update_status(statusline, U_ALL_REFRESH);
	screenupdated = 1;
	return(1);
}	

int connect_to_server (server *S){
	char buffer[MAXDATASIZE];
	static struct sockaddr_in their_addr;
	static struct hostent *host;

	// get the name of the host 

	S->active=0;
	alarm_occured=0;
	alarm(CONNECTTIMEOUT);

	// connect to a server one step at a time returning to report messages after each step

	if (S->connect_status == 0){
		strcpy(S->user, configuration.user);
		strcpy(S->host, configuration.hostname);
		strcpy(S->domain, configuration.domain);
		strcpy(S->name, configuration.userdesc);
		strcpy(S->nick, configuration.nick);
		strcpy(S->lastnick, configuration.nick);

		if ((S->serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Error: Cannot create a socket\n");
			S->connect_status = -1;
			return(0);
		}
		else{
			vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Looking up %s ...\n", S->server);
			S->connect_status = 1;
			return(1);
		}
	}

	if (S->connect_status == 1){
		if ((host = gethostbyname(S->server)) == NULL) {
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Error: Cannot find host %s\n", S->server);
			S->connect_status = -1;
			return(0);
		}
		else {
			vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B,"Connecting to %s (%s) on port %d ...\n", 
				host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), S->port);
			S->connect_status = 2;
			return(1);
		}
	}

	if (S->connect_status == 2){
		their_addr.sin_family = AF_INET;                                            
		their_addr.sin_port = htons(S->port);                              
		their_addr.sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(their_addr.sin_zero), 8);
	        if (connect (S->serverfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
			vprint_all_attribs(ERROR_COLOR_F, ERROR_COLOR_B, "Failed to connect to host %s (%s) on port %d ...\n", 
				host->h_name, inet_ntoa(*((struct in_addr *)host->h_addr)), S->port);
			S->connect_status = -1;
			return(0);
		}
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B, "Connected to %s (%s) on port %d ...\n", 
			host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), S->port);
		S->connect_status = 3;
		return(1);
	}

	if (S->connect_status == 3){
		vprint_all_attribs(MESSAGE_COLOR_F, MESSAGE_COLOR_B,"Logging on as %s ...\n", S->nick);

		S->nickinuse = 1;
		sprintf(buffer,"USER %s %s %s :%s\n",S->user, S->host, S->domain, S->name);
		send_all(S->serverfd, buffer, strlen(buffer));
		sprintf(buffer,"NICK :%s\n",S->nick);
		send_all(S->serverfd, buffer, strlen(buffer));
		sprintf(buffer,"MODE %s +%s\n",S->nick, configuration.mode);
		send_all(S->serverfd, buffer, strlen(buffer));
		S->connect_status = -1;
        	if (alarm_occured) return (0);
		else {
			fcntl(S->serverfd, F_SETFL, O_NONBLOCK);
			S->active=1;
			set_statusline_update_status(statusline, U_ALL_REFRESH);
			screenupdated = 1;
			return(1);
		}
	}	
	return(0);
}	

void sendcmd_server(server *S, char *command, char *args, char *dest, char *nick){
	char scratch[MAXDATASIZE];
	
	if (strlen(nick)==0 && strlen(dest)==0 && strlen(args)==0){
		sprintf(scratch, "%s\n", command); 
	}
	else if (strlen(nick)==0 && strlen(dest)==0){
		sprintf(scratch, "%s :%s\n", command, args); 
	}
	else if (strlen(args)==0 && strlen(dest)>0){
		sprintf(scratch, ":%s %s %s\n", nick, command, dest); 
	}
	else {
		sprintf(scratch, ":%s %s %s :%s\n", nick, command, dest, args); 
	}
	// print_server((char *)S, scratch);
	send_all(S->serverfd, scratch, strlen(scratch));

}

void sendmsg_channel(channel *C, char *message){
	if (C!=NULL && message!=NULL){
		 sendcmd_server(C->server, "PRIVMSG", message, C->channel, (C->server)->nick);
	}
}

void sendmsg_chat(chat *C, char *message){
	if (C!=NULL && message!=NULL){
		 sendcmd_server(C->server, "PRIVMSG", message, C->nick, (C->server)->nick);
	}
}

void sendmsg_dcc_chat(dcc_chat *C, char *message){
	char scratch[MAXDATASIZE];
	
	if (C!=NULL && message!=NULL){
		sprintf(scratch, "%s\n", message); 
		send_all(C->dccfd, scratch, strlen(scratch));
	}
}

void send_current_server(char *message){
	char scratch[MAXDATASIZE];
	bzero(scratch, MAXDATASIZE);

	//if (message[strlen(message)-1]!='\n') strcat(message, "\n");
	sprintf(scratch, "%s\n", message);
	send_all(get_serverfd(currentscreen), scratch, strlen(scratch));
}

void send_server(server *S, char *template, ...){
	char message[MAXDATASIZE];
	char scratch[MAXDATASIZE];
        va_list ap;

        va_start(ap, template);
        vsprintf(message, template, ap);
        va_end(ap);
	sprintf(scratch, "%s\n", message);
	send_all(S->serverfd, scratch, strlen(scratch));
}

int get_serverfd(screen *screen){
	if (screen->type==SERVER) return (((server *)(screen->screen))->serverfd);
	else if (screen->type==CHANNEL) return (((server *)((channel *)(screen->screen))->server)->serverfd);
	else if (screen->type==CHAT) return (((server *)((chat *)(screen->screen))->server)->serverfd);
	else return(-1);
}

server *get_server(screen *screen){
	if (screen->type==SERVER) return ((server *)(screen->screen));
	else if (screen->type==CHANNEL) return ((server *)((channel *)(screen->screen))->server);
	else return (NULL);
}
 

