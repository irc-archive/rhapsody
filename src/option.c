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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "autodefs.h"
#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "common.h"
#include "screen.h"
#include "events.h"
#include "cevents.h"
#include "forms.h"
#include "option.h"
#include "config.h"

#define FORM_ID_OK 1
#define FORM_ID_CANCEL 27
#define FORM_ID_YES 1
#define FORM_ID_NO 0
#define FORM_ID_SERVER 10
#define FORM_ID_PORT 11
#define FORM_ID_SERVER_ADD 12
#define FORM_ID_SERVER_DELETE 13
#define FORM_ID_SERVER_EDIT 14
#define FORM_ID_NICK 32 
#define FORM_ID_ALTNICK 33 
#define FORM_ID_NAME 34 
#define FORM_ID_USER 35
#define FORM_ID_HOST 36
#define FORM_ID_DOMAIN 37
#define FORM_ID_MODE_I 38
#define FORM_ID_MODE_W 39
#define FORM_ID_MODE_S 40

#define FORM_ID_CONNECT_TIMEOUT 51 
#define FORM_ID_AUTOSAVE 52

#define FORM_ID_OTHER 255
#define FORM_ID_SERVERLIST_START 1024

#define FORM_ID_CHANNEL 50
#define FORM_ID_CHANNEL_ADD 51
#define FORM_ID_CHANNEL_DELETE 52
#define FORM_ID_CHANNEL_EDIT 53
#define FORM_ID_CHANNELLIST_START 2048

#define FORM_ID_SEARCH 60
#define FORM_ID_USERSMORE 61
#define FORM_ID_USERSLESS 62
#define FORM_ID_SORT 63
#define FORM_ID_SORTCHANNEL LIST_SORT_CHANNEL
#define FORM_ID_SORTDESC LIST_SORT_DESCRIPTION
#define FORM_ID_SORTUSERS LIST_SORT_USERS

#define FORM_ID_USERS 70
#define FORM_ID_USER_ADD 71
#define FORM_ID_USER_DELETE 72
#define FORM_ID_USER_EDIT 73
#define FORM_ID_USERLIST_START 2048

#define FORM_ID_FILE 80
#define FORM_ID_FILELIST_START 2048

#define FORM_ID_BLOCKSIZE 90
#define FORM_ID_DLPATH 91
#define FORM_ID_ULPATH 92
#define FORM_ID_DLACCEPT 93
#define FORM_ID_DLDUPLICATES 94

#define FORM_ID_DCCHOST 95
#define FORM_ID_DCCPORTSTART 96
#define FORM_ID_DCCPORTEND 97

#define FORM_ID_TRANSFERSTOP 98
#define FORM_ID_TRANSFERINFO 99
#define FORM_ID_TRANSFERALLOW 100

#define FORM_ID_CTCP_ON 110
#define FORM_ID_CTCP_THROTTLE 111
#define FORM_ID_CTCP_FINGER 112
#define FORM_ID_CTCP_USERINFO 113

static form *mainform = NULL;
//static form *addform = NULL;
//static form *editform = NULL;
//static form *mainfavform = NULL;
//static form *mainbanform = NULL;

/** form remove ****************************************************************************/

int close_all_forms(){
	//form = remove_form(form);
	mainform = remove_form(mainform);
	//addform = remove_form(addform);
	//editform = remove_form(editform);
	//mainfavform = remove_form(mainfavform);
	//mainbanform = remove_form(mainbanform);
	return(1);
}

/** new server connect *********************************************************************/


int get_new_connect_info(int key, char *server, int *port){
	Fcomponent *comp;
	Ftextline *textline;
	static char portstr[32] = "6667";
	static char serverstr[256] = "";
	int event;

	if (mainform == NULL) mainform = create_new_server_connect_form(serverstr, atol(portstr));

	event = process_form_events(mainform, key);
		
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_SERVER);
		textline = comp->component;
		strcpy(serverstr, Ftextline_buffer_contents(textline));
		strcpy(server, serverstr);

		comp = form_component_by_id(mainform, FORM_ID_PORT);
		textline = comp->component;
		strcpy(portstr, Ftextline_buffer_contents(textline));
		*port = atoi(portstr);
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_server_connect_form(char *server, int port){
	void *comp;
	form *form;
	char portstr[32];

	form = add_form("Connect to Server", 0x000, -1, -1, 38, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 4, 1024, 27, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);        
	set_Ftextline_buffer(comp, server);
	add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 5, 5, 5, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
	sprintf(portstr, "%d", port);
	set_Ftextline_buffer(comp, portstr);
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	
	comp = (void *) add_Fbutton("Connect", 18, 7, 9, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 7, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** favorite server connect ****************************************************************/

int get_favorite_connect_info(int key, char *server, int *port){
	int id, event;
	Fcomponent *comp;
	Flist *listline;
	config_server *current;

	if (mainform == NULL) mainform = create_favorite_server_connect_form();

	event = process_form_events(mainform, key);

	/* escape or cancel button to quit the form of course */
	if (event == 27 || event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_SERVER);
		listline = comp->component;

		/* find out which server has been chosen */
		current = active_list_line_ptrid(listline);
				
		mainform = remove_form(mainform);
		if (current != NULL){
			strcpy(server, current->name);
			*port = current->port;
			return(E_OK);
		}
		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);	
	print_form(mainform);
	return(key);
}

form *create_favorite_server_connect_form(){
	void *comp;
	form *form;
	int i, id;
	char listline[256];
	char portstr[64];
	config_server *server;

	/* Server New Connect Form */
	form = add_form("Server Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	//comp = (void *) add_Flist("Server                         Port", 2, 3, 2, 4, 36, 12, 1, 0, STYLE_TITLE);
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_SERVER, F_LIST);

	// id is incremented every server line starting from FORM_ID_SERVERLIST_START
	id = FORM_ID_SERVERLIST_START;
	server = configuration.serverfavorite;
	while (server != NULL){
		strncpy(listline, server->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
		sprintf(portstr, "%d", server->port);
		strncat(listline, portstr, 255);
 
		add_Flistline(comp, id, listline, server, FORMLIST_LAST);
		server = server->next;
		id++;
	}

	comp = (void *) add_Fbutton("Connect", 20, 18, 9, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	print_form(form);
	return(form);
}


/** favorite server edit ********************************************************************/

int edit_favorite_servers(int key){
	//static form *mainform = NULL;
	//static form *addform = NULL;
	//static form *editform = NULL;
	static int activeformA = 0;
	int event;
	Fcomponent *comp;
	Flist *listline;
	Ftextline *textline;
	config_server *current;
	static config_server *edited;
	static int activeform = 0;

	char serverstr[256];
	char portstr[32];
	int port;


	if (activeform == 0){
		if (mainform == NULL) mainform = create_edit_favorite_server_form();

		event = process_form_events(mainform, key);
		
		if (event == 27 || event == FORM_ID_CANCEL){
			mainform = remove_form(mainform);
			return(E_CANCEL);
		}
		else if (event == FORM_ID_OK){
			mainform = remove_form(mainform);
			configuration.changed = 1;
			return(E_OK);
		}
		else if (event == FORM_ID_SERVER_ADD){
			activeform = 1;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_SERVER_DELETE){
			comp = form_component_by_id(mainform, FORM_ID_SERVER);
			listline = comp->component;

			/* find out which server has been chosen */
			current = active_list_line_ptrid(listline);
					
			mainform = remove_form(mainform);
			if (current != NULL) remove_config_server(&configuration, current);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_SERVER_EDIT){
			comp = form_component_by_id(mainform, FORM_ID_SERVER);
			listline = comp->component;

			edited = active_list_line_ptrid(listline);
			mainform = remove_form(mainform);
			if (edited != NULL){
				mainform = create_change_favorite_server_form(edited->name, edited->port);
				activeform = 2;
			}
			return(E_NOWAIT);
		}
		else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);
		print_form(mainform);
		return(key);
	}

	/* add form */
	else if (activeform == 1){
		if (mainform == NULL) mainform = create_new_favorite_server_form();

		event = process_form_events(mainform, key);
		
		if (event == 27 || event == FORM_ID_CANCEL){
			activeform = 0;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_SERVER);
			textline = comp->component;
			strcpy(serverstr, Ftextline_buffer_contents(textline));

			comp = form_component_by_id(mainform, FORM_ID_PORT);
			textline = comp->component;
			strcpy(portstr, Ftextline_buffer_contents(textline));
			port = atoi(portstr);

			if (!config_server_exists(&configuration, serverstr, port)){
				add_config_server(&configuration, serverstr, port, LIST_ORDER_FRONT);
			}
			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		// else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);	
		print_form(mainform);
		return(key);
	}

	/* edit form */
	else if (activeform == 2){
	
		event = process_form_events(mainform, key);
		
		if (event == 27 || event == FORM_ID_CANCEL){
			activeform = 0;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_SERVER);
			textline = comp->component;
			strcpy(serverstr, Ftextline_buffer_contents(textline));

			comp = form_component_by_id(mainform, FORM_ID_PORT);
			textline = comp->component;
			strcpy(portstr, Ftextline_buffer_contents(textline));
			port = atoi(portstr);

			/* now copy the changes into the config struct */
			if (edited != NULL){
				strcpy(edited->name, serverstr);
				edited->port = port;
			}

			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}

		//else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);	
		print_form(mainform);
		return(key);
	}
	return(key);
}

form *create_edit_favorite_server_form(){
	void *comp;
	form *form;
	int id, i;
	char listline[256];
	char portstr[64];
	config_server *server;

	form = add_form("Edit Server Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_SERVER, F_LIST);

	id = FORM_ID_SERVERLIST_START;
	server = configuration.serverfavorite;
	while (server != NULL){
		strncpy(listline, server->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
		sprintf(portstr, "%d", server->port);
		strncat(listline, portstr, 255);
 
		add_Flistline(comp, id, listline, server, FORMLIST_LAST);
		server = server->next;
		id++;
	}

	comp = (void *) add_Fbutton("Add", 13, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_favorite_server_form(){
	void *comp;
	form *form;

	form = add_form("Add Server Favorite", 0x000, -1, -1, 38, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 4, 1024, 27, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 5, 5, 5, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
        
	set_Ftextline_buffer(comp, "6667");
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	
	comp = (void *) add_Fbutton("Add", 22, 7, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 7, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_favorite_server_form(char *server, int port){
	void *comp;
	form *form;
	char portstr[32];

	form = add_form("Edit Server Favorite", 0x000, -1, -1, 38, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 4, 1024, 27, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);
	set_Ftextline_buffer(comp, server);

	comp = (void *) add_Ftextline("Port", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		9, 5, 5, 5, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
        
	sprintf(portstr, "%d", port);
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	set_Ftextline_buffer(comp, portstr);
	
	comp = (void *) add_Fbutton("OK", 21, 7, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 7, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** new channel join *********************************************************************/

int get_new_join_info(int key, char *channel){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	int event;

	if (mainform == NULL) mainform = create_new_channel_join_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		textline = comp->component;
		strcpy(channel, Ftextline_buffer_contents(textline));
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_channel_join_form(){
	void *comp;
	form *form;

	form = add_form("New Channel", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		10, 4, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	set_Ftextline_buffer(comp, "#");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Join", 21, 6, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** favorite channel join  ****************************************************************/

int get_favorite_join_info(int key, char *channel){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Flist *listline;
	config_channel *current;

	if (mainform == NULL) mainform = create_favorite_channel_join_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_CHANNEL);
		listline = comp->component;

		/* find out which channel has been chosen */
		current = active_list_line_ptrid(listline);
				
		mainform = remove_form(mainform);
		if (current != NULL){
			strcpy(channel, current->name);
			return(E_OK);
		}
		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);		
	print_form(mainform);
	return(key);
}

form *create_favorite_channel_join_form(){
	void *comp;
	form *form;
	int i, id;
	char listline[256];
	config_channel *channel;

	form = add_form("Channel Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	//comp = (void *) add_Flist("Channel                            ", 2, 3, 2, 4, 36, 12, 1, 0, STYLE_TITLE);
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_CHANNEL, F_LIST);

	// id is incremented every server line starting from FORM_ID_SERVERLIST_START
	id = FORM_ID_CHANNELLIST_START;
	channel = configuration.channelfavorite;
	while (channel != NULL){
		strncpy(listline, channel->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, id, listline, channel, FORMLIST_LAST);
		channel = channel->next;
		id++;
	}

	comp = (void *) add_Fbutton("Join", 23, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	print_form(form);
	return(form);
}

/** favorite channel edit ***********************************************************************/


int edit_favorite_channels(int key){
	//static form *mainform = NULL;
	//static form *addform = NULL;
	//static form *editform = NULL;
	static int activeform = 0;
	int event;
	Fcomponent *comp;
	Flist *listline;
	Ftextline *textline;
	config_channel *current;
	static config_channel *edited;
	char channelstr[256];


	/* main edit form */ 
	if (activeform == 0){
		if (mainform == NULL) mainform = create_edit_favorite_channel_form();
		event = process_form_events(mainform, key);
		
		if (event == FORM_ID_CANCEL){
			mainform = remove_form(mainform);
			return(E_CANCEL);
		}
		else if (event == FORM_ID_OK){
			mainform = remove_form(mainform);
			configuration.changed = 1;
			return(E_OK);
		}
		else if (event == FORM_ID_CHANNEL_ADD){
			activeform = 1;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_CHANNEL_DELETE){
			comp = form_component_by_id(mainform, FORM_ID_CHANNEL);
			listline = comp->component;

			/* find out which channel has been chosen */
			current = active_list_line_ptrid(listline);
				
			mainform = remove_form(mainform);
			if (current != NULL) remove_config_channel(&configuration, current);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_CHANNEL_EDIT){
			comp = form_component_by_id(mainform, FORM_ID_CHANNEL);
			listline = comp->component;

			edited = active_list_line_ptrid(listline);					
			if (edited != NULL){
				mainform = remove_form(mainform);
				mainform = create_change_favorite_channel_form(edited->name);
				activeform = 2;
				return(E_NOWAIT);
			}
		}
		else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);
		print_form(mainform);
		return(key);
	}

	/* add form */
	else if (activeform == 1){
		if (mainform == NULL) mainform = create_new_favorite_channel_form();

		event = process_form_events(mainform, key);
		
		if (event == FORM_ID_CANCEL){
			activeform = 0;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_CHANNEL);
			textline = comp->component;
			strcpy(channelstr, Ftextline_buffer_contents(textline));

			if (!config_channel_exists(&configuration, channelstr)){
				add_config_channel(&configuration, channelstr, LIST_ORDER_FRONT);
			}

			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		//else if (event == KEY_ENTER || event == 10) process_form_events(addform, E_NEXT);
	
		print_form(mainform);
		return(key);
	}

	/* edit form */
	else if (activeform == 2){
	
		event = process_form_events(mainform, key);

		if (event == FORM_ID_CANCEL){
			activeform = 0;
			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_CHANNEL);
			textline = comp->component;
			/* now copy the changes into the config struct */
			if (edited != NULL) strcpy(edited->name, Ftextline_buffer_contents(textline));

			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		//else if (event == KEY_ENTER || event == 10) process_form_events(editform, E_NEXT);
		print_form(mainform);
		return(key);
	}
	return(key);
}


form *create_edit_favorite_channel_form(){
	void *comp;
	form *form;
	int id;
	char listline[256];
	config_channel *channel;

	form = add_form("Edit Channel Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_CHANNEL, F_LIST);

	id = FORM_ID_CHANNELLIST_START;
	channel = configuration.channelfavorite;
	while (channel != NULL){
		strncpy(listline, channel->name, 255);
		add_Flistline(comp, id, listline, channel, FORMLIST_LAST);
		channel = channel->next;
		id++;
	}

	comp = (void *) add_Fbutton("Add", 13, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_favorite_channel_form(){
	void *comp;
	form *form;

	form = add_form("Add Channel Favorite", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		10, 4, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
	set_Ftextline_buffer(comp, "#");        
	add_form_component(form, comp, FORM_ID_CHANNEL, F_TEXTLINE);

	comp = (void *) add_Fbutton("Add", 22, 6, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_favorite_channel_form(char *channel){
	void *comp;
	form *form;

	form = add_form("Edit Channel Favorite", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		10, 4, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	add_form_component(form, comp, FORM_ID_CHANNEL, F_TEXTLINE);
	set_Ftextline_buffer(comp, channel);
	
	comp = (void *) add_Fbutton("OK", 21, 6, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** new chat *********************************************************************/

int get_new_chat_info(int key, char *user){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	int event;

	if (mainform == NULL) mainform = create_new_chat_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		textline = comp->component;
		strcpy(user, Ftextline_buffer_contents(textline));
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_chat_form(){
	void *comp;
	form *form;

	form = add_form("New Chat", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		7, 4, 1024, 29, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Chat", 21, 6, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** favorite user chat  ****************************************************************/

int get_favorite_user_chat_info(int key, char *user){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Flist *listline;
	config_user *current;

	if (mainform == NULL) mainform = create_favorite_chat_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		listline = comp->component;

		/* find out which user has been chosen */
		current = active_list_line_ptrid(listline);
				
		mainform = remove_form(mainform);
		if (current != NULL){
			strcpy(user, current->name);
			return(E_OK);
		}
		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);		
	print_form(mainform);
	return(key);
}

form *create_favorite_chat_form(){
	void *comp;
	form *form;
	int i, id;
	char listline[256];
	config_user *user;

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	// id is incremented every line starting from FORM_ID_USERLIST_START
	id = FORM_ID_USERLIST_START;
	user = configuration.userfavorite;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, id, listline, user, FORMLIST_LAST);
		user = user->next;
		id++;
	}

	comp = (void *) add_Fbutton("Chat", 23, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	print_form(form);
	return(form);
}

/** new dcc chat *********************************************************************/

int get_new_dccchat_info(int key, char *user){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	int event;

	if (mainform == NULL) mainform = create_new_dccchat_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		textline = comp->component;
		strcpy(user, Ftextline_buffer_contents(textline));
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_dccchat_form(){
	void *comp;
	form *form;

	form = add_form("New DCC Chat", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		7, 4, 1024, 29, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Chat", 21, 6, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** favorite user dcc_chat  ****************************************************************/

int get_favorite_user_dccchat_info(int key, char *user){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Flist *listline;
	config_user *current;

	if (mainform == NULL) mainform = create_favorite_dccchat_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		listline = comp->component;

		/* find out which user has been chosen */
		current = active_list_line_ptrid(listline);
				
		mainform = remove_form(mainform);
		if (current != NULL){
			strcpy(user, current->name);
			return(E_OK);
		}
		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);		
	print_form(mainform);
	return(key);
}

form *create_favorite_dccchat_form(){
	void *comp;
	form *form;
	int i, id;
	char listline[256];
	config_user *user;

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	// id is incremented every line starting from FORM_ID_USERLIST_START
	id = FORM_ID_USERLIST_START;
	user = configuration.userfavorite;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, id, listline, user, FORMLIST_LAST);
		user = user->next;
		id++;
	}

	comp = (void *) add_Fbutton("Chat", 23, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	print_form(form);
	return(form);
}

/** new dcc send *********************************************************************/

int get_new_dccsend_info(int key, char *user){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	int event;

	if (mainform == NULL) mainform = create_new_dccsend_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		textline = comp->component;
		strcpy(user, Ftextline_buffer_contents(textline));
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_dccsend_form(){
	void *comp;
	form *form;

	form = add_form("Send DCC File", 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		7, 4, 1024, 29, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
        
	set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Select File", 14, 6, 13, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** favorite user dcc send  ****************************************************************/

int get_favorite_user_dccsend_info(int key, char *user){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Flist *listline;
	config_user *current;

	if (mainform == NULL) mainform = create_favorite_dccsend_form();

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_USER);
		listline = comp->component;

		/* find out which user has been chosen */
		current = active_list_line_ptrid(listline);
				
		mainform = remove_form(mainform);
		if (current != NULL){
			strcpy(user, current->name);
			return(E_OK);
		}
		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);		
	print_form(mainform);
	return(key);
}

form *create_favorite_dccsend_form(){
	void *comp;
	form *form;
	int i, id;
	char listline[256];
	config_user *user;

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	// id is incremented every line starting from FORM_ID_USERLIST_START
	id = FORM_ID_USERLIST_START;
	user = configuration.userfavorite;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, id, listline, user, FORMLIST_LAST);
		user = user->next;
		id++;
	}


	comp = (void *) add_Fbutton("Select File", 16, 18, 13, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	print_form(form);
	return(form);
}

/** edit favorite / banned users edit ***********************************************************************/

int edit_users(int key, int listnum){
	//static form *mainform = NULL;
	//static form *mainfavform = NULL;
	//static form *mainbanform = NULL;
	//static form *addform = NULL;
	//static form *editform = NULL;
	static int activeform = 0;
	int id, event;
	Fcomponent *comp;
	Flist *listline;
	Ftextline *textline;
	config_user *current;
	static config_user *edited;

	char userstr[256];


	/* in main user select list form */
	if (activeform == 0){
		if (mainform == NULL){
			if (listnum == CONFIG_FAVORITE_USER_LIST) 
				mainform = create_edit_user_form("Favorite Users", configuration.userfavorite);
			else mainform = create_edit_user_form("Ignored Users", configuration.userignored);
		}

		event = process_form_events(mainform, key);
		
		if (event == 27 || event == FORM_ID_CANCEL){
			mainform = remove_form(mainform);
			return(E_CANCEL);
		}
		else if (event == FORM_ID_OK){
			mainform = remove_form(mainform);
			configuration.changed = 1;
			return(E_OK);
		}
		else if (event == FORM_ID_USER_ADD){
			mainform = remove_form(mainform);
			activeform = 1;
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_USER_DELETE){
			comp = form_component_by_id(mainform, FORM_ID_USER);
			listline = comp->component;
			current = active_list_line_ptrid(listline);

			if (current != NULL) remove_config_user(&configuration, listnum, current);

			mainform = remove_form(mainform);
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_USER_EDIT){
			comp = form_component_by_id(mainform, FORM_ID_USER);
			listline = comp->component;
			edited = active_list_line_ptrid(listline);

			if (edited != NULL){
				mainform = remove_form(mainform);
				activeform = 2;
				return(E_NOWAIT);
			}
			else{
				mainform = remove_form(mainform);
				return(E_NOWAIT);
			}
		}
		else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);
		print_form(mainform);
		return(key);
	}

	/* add form */
	else if (activeform == 1){
		if (mainform == NULL){
			if (listnum == CONFIG_FAVORITE_USER_LIST) mainform = create_new_user_form("Add Favorite User");
			else mainform = create_new_user_form("Add Ignored User");
		}
		event = process_form_events(mainform, key);

		if (event == FORM_ID_CANCEL || event == 27){
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_USER);
			textline = comp->component;
			strcpy(userstr, Ftextline_buffer_contents(textline));

			if (!config_user_exists_exact(&configuration, listnum, userstr)) 
				add_config_user(&configuration, listnum, userstr, LIST_ORDER_FRONT);

			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		print_form(mainform);
		return(key);
	}

	/* edit form */
	else if (activeform == 2){
		if (mainform == NULL){
			if (listnum == CONFIG_FAVORITE_USER_LIST) 
				mainform = create_change_user_form("Edit Favorite User", edited->name);
			else mainform = create_change_user_form("Edit Ignored User", edited->name);
		}
		event = process_form_events(mainform, key);

		if (event == FORM_ID_CANCEL || event == 27){
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		else if (event == FORM_ID_OK){
			comp = form_component_by_id(mainform, FORM_ID_USER);
			textline = comp->component;

			/* now copy the changes into the config struct */
			if (edited != NULL) strcpy(edited->name, Ftextline_buffer_contents(textline));

			/* destroy the main form so that it can be rebuilt and updated */
			mainform = remove_form(mainform);
			activeform = 0;
			return(E_NOWAIT);
		}
		print_form(mainform);
		return(key);
	}
	return(key);
}


form *create_edit_user_form(char *title, config_user *userlist){
	void *comp;
	form *form;
	int id;
	char listline[256];
	config_user *user;

	form = add_form(title, 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	id = FORM_ID_USERLIST_START;
	user = userlist;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		add_Flistline(comp, id, listline, user, FORMLIST_LAST);
		user = user->next;
		id++;
	}

	comp = (void *) add_Fbutton("Add", 13, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_user_form(char *title){
	void *comp;
	form *form;

	form = add_form(title, 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		7, 4, 1024, 29, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, "");

	comp = (void *) add_Fbutton("Add", 22, 6, 5, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_user_form(char *title, char *user){
	void *comp;
	form *form;

	form = add_form(title , 0x000, -1, -1, 38, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		10, 4, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, user);
	
	comp = (void *) add_Fbutton("OK", 21, 6, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/**  list view options  ***************************************************************************/

int get_list_view_options(int key, char *searchstr, int *minuser, int *maxuser, int *searchtype){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox_array *array;
	int event;

	if (mainform == NULL) mainform = create_list_view_form();

	event = process_form_events(mainform, key);

	if (event == 27 || event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_SEARCH);
		textline = comp->component;
		strcpy(searchstr, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_USERSMORE);
		textline = comp->component;
		*minuser = atoi(Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_USERSLESS);
		textline = comp->component;
		*maxuser = atoi(Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_SORT);
		array = comp->component;
		*searchtype = Fcheckbox_array_selected_id(array);

		mainform = remove_form(mainform);
		return(E_OK);
	}

	print_form(mainform);
	return(key);
}


form *create_list_view_form(){
	void *comp;
	form *form;
	Fcheckbox *box;
	int i;

	form = add_form("List View Options", 0x000, -1, -1, 40, 12, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Search for", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		14, 4, 1024, 24, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_SEARCH, F_TEXTLINE);

	comp = (void *) add_Ftextline("Users >", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		14, 5, 8, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM); 
	add_form_component(form, comp, FORM_ID_USERSMORE, F_TEXTLINE);

	comp = (void *) add_Ftextline("Users <", 2, 6, COLOR_PAIR(F_BLUE*16+F_WHITE),
		14, 6, 8, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
       	add_form_component(form, comp, FORM_ID_USERSLESS, F_TEXTLINE);


	comp = (void *) add_Fcheckbox_array("Sort by:", 2, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), 0);
	add_form_component(form, comp, FORM_ID_SORT, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Channel", 23, 8, 14, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, FORM_ID_SORTCHANNEL, box);

	box = add_Fcheckbox("Users", 34, 8, 27, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, FORM_ID_SORTUSERS, box);

	comp = (void *) add_Fbutton("OK", 23, 10, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 10, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/**  channel select options  ***************************************************************************/

int get_channel_select_options(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
	int event;
	int ret;

	if (mainform == NULL) mainform = create_channel_select_form();

	event = process_form_events(mainform, key);

	if (event == 27 || event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		mainform = remove_form(mainform);
		return(E_OK);
	}
	else if (event == FORM_ID_CHANNEL_ADD){
		mainform = remove_form(mainform);
		return(E_CHANNEL_ADD_FAVORITE);
	}
	print_form(mainform);
	return(key);
}


form *create_channel_select_form(){
	void *comp;
	form *form;
	int i;

	form = add_form("Channel", 0x000, -1, -1, 38, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Fbutton("Join", 2, 4, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Add to Favorites", 9, 4, 18, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 4, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** transfer select options  ***************************************************************************/

int get_transfer_select_options(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
	int event;
	int ret;

	if (mainform == NULL){
		mainform = add_form("Transfer", 0x000, -1, -1, 34, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);        
		comp = (void *) add_Fbutton("Information", 2, 4, 13, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_TRANSFERINFO, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 16, 4, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_CANCEL, F_BUTTON);
		comp = (void *) add_Fbutton("Abort", 25, 4, 7, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_TRANSFERSTOP, F_BUTTON);
	}	

	event = process_form_events(mainform, key);

	if (event == 27 || event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_TRANSFERINFO){
		mainform = remove_form(mainform);
		return(E_TRANSFER_INFO);
	}
	else if (event == FORM_ID_TRANSFERSTOP){
		mainform = remove_form(mainform);
		return(E_TRANSFER_STOP);
	}
	print_form(mainform);
	return(key);
}

int get_transfer_info(int key, dcc_file *F){
	//static form *form = NULL;
	char scratch[2048];
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
	int event;
	int ret;
	long etime, rtime;
	int eh, em, es, rh, rm, rs; 
	Ftextarea *text;
	struct in_addr addr;

	if (F == NULL) return (E_CANCEL);

	if (mainform == NULL){
		addr.s_addr = htonl(F->hostip);

		mainform = add_form("Transfer Info", 0x000, -1, -1, 38, 12, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);        
		comp = (void *) add_Fbutton("OK", 16, 10, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_OK, F_BUTTON);

		/* precalculate the timestamps */		
		etime = time(NULL) - F->starttime;
		if (etime > 10000000) etime = 0;
		eh = etime / 3600;
		em = (etime % 3600) / 60;
		es = (etime % 60);
		
		if (F->byte > 0 && etime > 0) rtime = (F->size - F->byte) / (F->byte / etime);
		else rtime = 0;
		rh = rtime / 3600;
		rm = (rtime % 3600) / 60;
		rs = (rtime % 60);

		sprintf(scratch, "Nick       : %s\nProgress   : %ld/%ld\nRemote IP  : %s\nRemote Port: %u\nElapsed    : %02d:%02d:%02d\nRemaining  : %02d:%02d:%02d\n", 
			F->nick, F->byte, F->size, inet_ntoa(addr), F->port, eh, em, es, rh, rm, rs); 

		text = add_Ftextarea("info", 2, 3, 36, 12, 0, -1, scratch);
		add_form_textarea (mainform, text);
	}	

	event = process_form_events(mainform, key);

	if (event == 27 || event == FORM_ID_CANCEL || event == FORM_ID_OK){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_TRANSFERINFO){
		mainform = remove_form(mainform);
		return(E_TRANSFER_INFO);
	}
	else if (event == FORM_ID_TRANSFERSTOP){
		mainform = remove_form(mainform);
		return(E_TRANSFER_STOP);
	}
	print_form(mainform);
	mainform = remove_form(mainform);
	return(key);
}

/** allow transfer form **************************************************************************************/

int allow_transfer(int key, dcc_file *F){
	Ftextarea *text;
	char scratch[2048];
	void *comp;
        int revent;
	struct in_addr addr;
               
	if (mainform == NULL){
		addr.s_addr = htonl(F->hostip);

		mainform = add_form("Incoming File", 0x000, -1, -1, 32, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
		comp = (void *) add_Fbutton("Allow", 6, 7, 9, 1, 0, E_OK, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_OK, F_BUTTON);
		comp = (void *) add_Fbutton("Deny", 18, 7, 8, 1, 0, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);

		sprintf(scratch, "From  : %s\nFile  : %s\nIP    : %s\n", F->nick, F->filename, inet_ntoa(addr));
		text = add_Ftextarea("Info", 2, 3, 30, 4, STYLE_CENTER_JUSTIFY, -1, scratch);

		add_form_textarea (mainform, text);
	}

	revent = process_form_events(mainform, key);      

	if (revent == E_OK){
		mainform = remove_form(mainform);
		return(revent);
	}
	if (revent == E_CANCEL){
		mainform = remove_form(mainform);
		return(revent);
	}                 
	print_form(mainform);
	return(E_NONE);
}

/** help about *******************************************************************************************/

int view_about(int key){
        void *comp;
	Ftextarea *text;
        int revent;
                
	if (mainform == NULL){
		mainform = add_form("About", 0x000, -1, -1, 40, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
		comp = (void *) add_Fbutton("OK", 16, 7, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_OK, F_BUTTON);
		text = add_Ftextarea("Version", 2, 3, 36, 4, STYLE_CENTER_JUSTIFY, -1, 
			CODE_NAME"\nVersion "CODE_VERSION" "CODE_STATE" "CODE_BUILD"\nCopyright (C) "CODE_COPYRIGHT", "CODE_DEVELOPER);
		add_form_textarea (mainform, text);
	}

	print_form(mainform);
	if (key == 10 || key == KEY_ENTER){
		mainform = remove_form(mainform);
		return(E_OK);
	}
	else return(E_NONE);
}

/** screen size warning  *********************************************************************************/

int view_screen_size_warning(int key){
        //static form *form = NULL;
	char buffer[256];
        void *comp;
	Ftextarea *text;
        int revent;
                
	if (mainform == NULL){
		sprintf(buffer, "This program requires\na window larger than %dx%d.\nPlease resize the window.", MINWINDOWWIDTH, MINWINDOWHEIGHT);
		mainform = add_form("Window Size ", 0x000, -1, -1, 32, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
		text = add_Ftextarea("Warning", 2, 3, 36, 4, STYLE_CENTER_JUSTIFY, -1, buffer);  
		add_form_textarea (mainform, text);
	}

	print_form(mainform);
	mainform = remove_form(mainform);
	if (key == 10 || key == KEY_ENTER) return(E_OK);
	else return(E_NONE);
}


/** exit form *******************************************************************************************/

int end_run(int key){
	//static form *form = NULL;     
	void *comp;
        int revent;
               
	if (mainform == NULL && configuration.autosave == 0 && configuration.changed == 1){
		mainform = add_form("Exit and Save Settings", 0x000, -1, -1, 39, 6, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
		comp = (void *) add_Fbutton("Exit&Save", 2, 4, 11, 1, 0, E_EXIT_AND_SAVE, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT_AND_SAVE, F_BUTTON);
		comp = (void *) add_Fbutton("Exit&Discard", 14, 4, 14, 1, 0, E_EXIT, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 29, 4, 8, 1, 0, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);
	}

	else if (mainform == NULL){
		mainform = add_form("Exit", 0x000, -1, -1, 22, 6, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
		comp = (void *) add_Fbutton("Exit", 2, 4, 8, 1, 0, E_EXIT, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 12, 4, 8, 1, 0, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);
	}

	revent = process_form_events(mainform, key);      

	if (revent == E_EXIT_AND_SAVE){
		writeconfig(configfile, &configuration);
		curs_set(1);
		endwin();
		printf("Configuration saved in %s\n", configfile);
		exit(0);
	}
	if (revent == E_EXIT){
		curs_set(1);
		endwin();
		printf("\n");
		exit(0);
	}                 
	else if (revent == E_CANCEL || revent == 27){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	print_form(mainform);
	return(E_NONE);
}

/** client identity ***************************************************************************/

int get_identity_info(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
        int event;

	if (mainform == NULL){
		mainform = create_identity_form(configuration.nick, configuration.alt_nick, configuration.userdesc,
			configuration.user, configuration.hostname, configuration.domain, configuration.mode);
	}

	event = process_form_events(mainform, key);
		
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}

	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_NICK);
		textline = comp->component;
 		strcpy(configuration.nick, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_ALTNICK);
		textline = comp->component;
 		strcpy(configuration.alt_nick, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_NAME);
		textline = comp->component;
 		strcpy(configuration.userdesc, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_USER);
		textline = comp->component;
 		strcpy(configuration.user, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_HOST);
		textline = comp->component;
		strcpy(configuration.hostname, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_DOMAIN);
		textline = comp->component;
 		strcpy(configuration.domain, Ftextline_buffer_contents(textline));

		strcpy(configuration.mode, "");
			
		comp = form_component_by_id(mainform, FORM_ID_MODE_I);
		checkbox = comp->component;
		if (Fcheckbox_value(checkbox)) strcat(configuration.mode, "i");

		comp = form_component_by_id(mainform, FORM_ID_MODE_W);
		checkbox = comp->component;
		if (Fcheckbox_value(checkbox)) strcat(configuration.mode, "w");

		comp = form_component_by_id(mainform, FORM_ID_MODE_S);
		checkbox = comp->component;
		if (Fcheckbox_value(checkbox)) strcat(configuration.mode, "s");

	 	configuration.changed = 1;
		mainform = remove_form(mainform);
		return(E_OK);
	}
	//else if (key == 10 || key == KEY_ENTER) process_form_events(form, E_NEXT);
	
	print_form(mainform);
	return(key);
}


form *create_identity_form(char *nick, char *altnick, char *name, char *user, char *host, char *domain, char *mode){
	void *comp;
	form *form;
	int modei = 0;
	int modew = 0;
	int modes = 0;
	int i;

	form = add_form("Identity", 0x000, -1, -1, 40, 17, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 4, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_NICK, F_TEXTLINE);
	set_Ftextline_buffer(comp, nick);

	comp = (void *) add_Ftextline("Alt Nick", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 5, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_ALTNICK, F_TEXTLINE);
	set_Ftextline_buffer(comp, altnick);

	comp = (void *) add_Ftextline("Name", 2, 6, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 6, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT|IN_BLANK);
       	add_form_component(form, comp, FORM_ID_NAME, F_TEXTLINE);
	set_Ftextline_buffer(comp, name);

	comp = (void *) add_Ftextline("User", 2, 7, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 7, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, user);

	comp = (void *) add_Ftextline("Host", 2, 8, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 8, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_HOST, F_TEXTLINE);
	set_Ftextline_buffer(comp, host);

	comp = (void *) add_Ftextline("Domain", 2, 9, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 9, 1024, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_DOMAIN, F_TEXTLINE);
	set_Ftextline_buffer(comp, domain);

	for (i = 0; i < strlen(mode); i++){
		if (mode[i] == 'i') modei = 1;
		else if (mode[i] == 'w') modew = 1;
		else if (mode[i] == 's') modes = 1;
	}

	comp = (void *) add_Fcheckbox("Invisible Mode (+i)", 27, 11, 2, 11, COLOR_PAIR(F_BLUE*16+F_WHITE), 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_I, F_CHECKBOX);
	set_Fcheckbox_value(comp, modei);

	comp = (void *) add_Fcheckbox("Server Notices (+s)", 27, 12, 2, 12, COLOR_PAIR(F_BLUE*16+F_WHITE), 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_S, F_CHECKBOX);
	set_Fcheckbox_value(comp, modes);

	comp = (void *) add_Fcheckbox("Wallops (+w)", 27, 13, 2, 13, COLOR_PAIR(F_BLUE*16+F_WHITE), 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_W, F_CHECKBOX);
	set_Fcheckbox_value(comp, modew);

	/*
	comp = (void *) add_Fcheckbox("Deaf in Channels (+d)", 27, 14, 2, 14, COLOR_PAIR(F_BLUE*16+F_WHITE), 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_D, F_CHECKBOX);
	*/

	comp = (void *) add_Fbutton("OK", 23, 15, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 15, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** ctcp options ***************************************************************************/

int get_ctcp_info(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
        int event;

	if (mainform == NULL){
		mainform = create_ctcp_form(configuration.ctcpreply, configuration.ctcpthrottle,
			configuration.ctcpfinger, configuration.ctcpuserinfo);
	}

	event = process_form_events(mainform, key);
		
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}

	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_CTCP_ON);
		checkbox = comp->component;
		if (Fcheckbox_value(checkbox)) configuration.ctcpreply = 1;
		else configuration.ctcpreply = 0;

		comp = form_component_by_id(mainform, FORM_ID_CTCP_THROTTLE);
		textline = comp->component;
 		configuration.ctcpthrottle = atoi(Ftextline_buffer_contents(textline));
			
		comp = form_component_by_id(mainform, FORM_ID_CTCP_FINGER);
		textline = comp->component;
 		strcpy(configuration.ctcpfinger, Ftextline_buffer_contents(textline));
			
		comp = form_component_by_id(mainform, FORM_ID_CTCP_USERINFO);
		textline = comp->component;
 		strcpy(configuration.ctcpuserinfo, Ftextline_buffer_contents(textline));
			
	 	configuration.changed = 1;
		mainform = remove_form(mainform);
		return(E_OK);
	}
	//else if (key == 10 || key == KEY_ENTER) process_form_events(form, E_NEXT);
	
	print_form(mainform);
	return(key);
}


form *create_ctcp_form(int status, int throttle, char *finger, char *userinfo){
	void *comp;
	form *form;
	char scratch[32];

	form = add_form("CTCP Options", 0x000, -1, -1, 40, 12, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	comp = (void *) add_Fcheckbox("Respond to CTCP", 36, 4, 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE), 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_CTCP_ON, F_CHECKBOX);
	set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Ftextline("Throttle", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		34, 5, 3, 4, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
	add_form_component(form, comp, FORM_ID_CTCP_THROTTLE, F_TEXTLINE);
	sprintf(scratch, "%d", throttle);
	set_Ftextline_buffer(comp, scratch);

	comp = (void *) add_Ftextline("Finger", 2, 7, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 7, MAXDESCLEN, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT|IN_BLANK);
	add_form_component(form, comp, FORM_ID_CTCP_FINGER, F_TEXTLINE);
	set_Ftextline_buffer(comp, finger);

	comp = (void *) add_Ftextline("UserInfo", 2, 8, COLOR_PAIR(F_BLUE*16+F_WHITE),
		12, 8, MAXDESCLEN, 26, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT|IN_BLANK); 
	add_form_component(form, comp, FORM_ID_CTCP_USERINFO, F_TEXTLINE);
	set_Ftextline_buffer(comp, userinfo);

	comp = (void *) add_Fbutton("OK", 23, 10, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 10, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** dcc options ***************************************************************************/

int get_dcc_info(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	int event;

	if (mainform == NULL) mainform = create_dcc_form(configuration.dcchostname, 
		configuration.dccstartport, configuration.dccendport);

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_DCCHOST);
		textline = comp->component;
		strcpy(configuration.dcchostname, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_DCCPORTSTART);
		textline = comp->component;
		configuration.dccstartport = atoi(Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_DCCPORTEND);
		textline = comp->component;
		configuration.dccendport = atoi(Ftextline_buffer_contents(textline));

 		configuration.changed = 1;
		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_dcc_form(char *hostname, int startport, int endport){
	void *comp;
	form *form;
	char portstrbuf[16];

	form = add_form("DCC Options", 0x000, -1, -1, 38, 10, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Hostname", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		13, 4, 1024, 23, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
       	set_Ftextline_buffer(comp, hostname);
	add_form_component(form, comp, FORM_ID_DCCHOST, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port Start", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		30, 5, 5, 6, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
	sprintf(portstrbuf, "%5d", startport);
	set_Ftextline_buffer(comp, portstrbuf);
	add_form_component(form, comp, FORM_ID_DCCPORTSTART, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port End", 2, 6, COLOR_PAIR(F_BLUE*16+F_WHITE),
		30, 6, 5, 6, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_NUM);
	sprintf(portstrbuf, "%5d", endport);
	set_Ftextline_buffer(comp, portstrbuf);
	add_form_component(form, comp, FORM_ID_DCCPORTEND, F_TEXTLINE);

	comp = (void *) add_Fbutton("OK", 21, 8, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 8, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** dcc send options ***************************************************************/


int get_dcc_send_info(int key){
	//static form *form = NULL;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
	Fcheckbox_array *checkboxarray;

	int event;

	if (mainform == NULL){
		mainform = create_dcc_send_form(configuration.dccdlpath, configuration.dcculpath, configuration.dccblocksize,
			configuration.dccaccept, configuration.dccduplicates);
	}

	event = process_form_events(mainform, key);

	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   

	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_DLPATH);
		textline = comp->component;
 		strcpy(configuration.dccdlpath, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_ULPATH);
		textline = comp->component;
		strcpy(configuration.dcculpath, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_BLOCKSIZE);
		checkboxarray = comp->component;
		configuration.dccblocksize = Fcheckbox_array_selected_id(checkboxarray);

		comp = form_component_by_id(mainform, FORM_ID_DLACCEPT);
		checkboxarray = comp->component;
		configuration.dccaccept = Fcheckbox_array_selected_id(checkboxarray);

		comp = form_component_by_id(mainform, FORM_ID_DLDUPLICATES);
		checkboxarray = comp->component;
		configuration.dccduplicates = Fcheckbox_array_selected_id(checkboxarray);

 		configuration.changed = 1;
		mainform = remove_form(mainform);
		return(E_OK);
	}

	print_form(mainform);
	return(key);
}


form *create_dcc_send_form(char *dlpath, char *ulpath, int blocksize, int accept, int duplicates){
	void *comp;
	form *form;
	Fcheckbox *box;
	int i;

	form = add_form("DCC Send Options", 0x000, -1, -1, 40, 21, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	comp = (void *) add_Ftextline("DL Path", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		11, 4, 1024, 27, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_DLPATH, F_TEXTLINE);
	set_Ftextline_buffer(comp, dlpath);

	comp = (void *) add_Ftextline("UL Path", 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE),
		11, 5, 1024, 27, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_ULPATH, F_TEXTLINE);
	set_Ftextline_buffer(comp, ulpath);


	comp = (void *) add_Fcheckbox_array("DCC Block Size", 2, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), 0);
	add_form_component(form, comp, FORM_ID_BLOCKSIZE, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("256", 36, 7, 27, 7, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 256, box);

	box = add_Fcheckbox("512", 36, 8, 27, 8, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 512, box);

	box = add_Fcheckbox("1024", 36, 9, 27, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1024, box);

	box = add_Fcheckbox("2048", 36, 10, 27, 10, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 2048, box);

	box = add_Fcheckbox("4096", 36, 11, 27, 11, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 4096, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, blocksize);


	comp = (void *) add_Fcheckbox_array("Accept Downloads", 2, 13, COLOR_PAIR(F_BLUE*16+F_WHITE), 0);
	add_form_component(form, comp, FORM_ID_DLACCEPT, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Auto", 36, 13, 27, 13, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1, box);

	box = add_Fcheckbox("Ask", 36, 14, 27, 14, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 0, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, accept);


	comp = (void *) add_Fcheckbox_array("Duplicates", 2, 16, COLOR_PAIR(F_BLUE*16+F_WHITE), 0);
	add_form_component(form, comp, FORM_ID_DLDUPLICATES, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Rename", 36, 16, 27, 16, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1, box);

	box = add_Fcheckbox("Replace", 36, 17, 27, 17, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 0, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, duplicates);


	comp = (void *) add_Fbutton("OK", 23, 19, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 19, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** client options ***************************************************************************/

int get_client_options(int key){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;

	if (mainform == NULL){
		mainform = create_client_options_form(configuration.connecttimeout, configuration.autosave);
	}

	event = process_form_events(mainform, key);

	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   

	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_CONNECT_TIMEOUT);
		textline = comp->component;
 		configuration.connecttimeout = atoi(Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_AUTOSAVE);
		checkbox = comp->component;
 		configuration.autosave = Fcheckbox_value(checkbox);

 		configuration.changed = 1;
		mainform = remove_form(mainform);
		return(E_OK);
	}

	print_form(mainform);
	return(key);
}


form *create_client_options_form(int timeout, int autosave){
	void *comp;
	form *form;
	Fcheckbox *box;
	int i;
	char temp[256];

	form = add_form("Client Options", 0x000, -1, -1, 40, 9, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);

	sprintf(temp, "%d", timeout);        
	comp = (void *) add_Ftextline("Server Connect Timeout", 2, 4, COLOR_PAIR(F_BLUE*16+F_WHITE),
		35, 4, 2, 3, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_CURSOR_DARK, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_CONNECT_TIMEOUT, F_TEXTLINE);
	set_Ftextline_buffer(comp, temp);

	comp = (void *) add_Fcheckbox("Autosave Configuration", 36, 5, 2, 5, COLOR_PAIR(F_BLUE*16+F_WHITE), 0);
	add_form_component(form, comp, FORM_ID_AUTOSAVE, F_CHECKBOX);
	if (autosave) set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Fbutton("OK", 23, 7, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 7, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** file select form  ***************************************************************************/


int select_file(int key, char *path, int options){
	//static form *form = NULL;
	int event;
	Fcomponent *comp;
	Ftextline *textline;
	Flist *list;
	Fcheckbox *checkbox;
	static char pathbuffer[1024];


	if (mainform == NULL){
		strcpy(pathbuffer, path);
		chdir(pathbuffer);
		mainform = create_directory_listing_form(pathbuffer);
	}

	event = process_form_events(mainform, key);
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   

	/* use enter to go to buttons if file, or change directory if directory */
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_FILE);
		list = comp->component;

		/* find out which file has been chosen */
		if (active_list_line_id(list) == 1){
			getcwd(path, 1023);
			strncat(path, "/", 1023);
			strncat(path, active_list_line_text(list) + 11, 1023);
			mainform = remove_form(mainform);
			//vprint_all("selected %s\n", pathbuffer);
			return(E_OK);
		}
		
		/* or if this is a directory, change to it */
		else if (active_list_line_id(list) == 0){
			getcwd(pathbuffer, 1023);
			strncat(pathbuffer, "/", 1023);
			strncat(pathbuffer, active_list_line_text(list) + 11, 1023);
			chdir(pathbuffer);
			mainform = remove_form(mainform);
			mainform = create_directory_listing_form(pathbuffer);
		}

		else return(E_CANCEL);
	}
	else if (event == KEY_ENTER || event == 10){
		comp = form_component_by_id(mainform, FORM_ID_FILE);
		list = comp->component;

		/* if this is a directory, change to it immediately */
		if (active_list_line_id(list) == 0){
			getcwd(pathbuffer, 1023);
			strncat(pathbuffer, "/", 1023);
			strncat(pathbuffer, active_list_line_text(list) + 11, 1023);
			chdir(pathbuffer);
			mainform = remove_form(mainform);
			mainform = create_directory_listing_form(pathbuffer);
		}
		else process_form_events(mainform, E_NEXT);
	}

	print_form(mainform);
	return(key);
}	


form *create_directory_listing_form(char *path){
	void *comp;
	form *form;
	int id;
	char listline[256];
	char pathbuffer[1024];
	config_user *user;
	DIR *dirp;
	struct dirent *direntp;
	struct stat filestat;


	form = add_form("Select File", 0x000, -1, -1, 40, 20, COLOR_PAIR(F_BLUE*16+F_WHITE), STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, 1, 0, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_FILE, F_LIST);

	//vprint_all("Reading directory %s.\n", path);

	/* open the current directory */ 
	dirp=opendir(path);

	/* if cannot read directory, provide a way to step out of it */
	if (dirp == NULL){
		vprint_all("Cannot open directory %s.\n", path);
		strcpy(listline, "       DIR ..");
		add_Flistline(comp, 0, listline, NULL, FORMLIST_LAST);
	}
	else{
		while((direntp=readdir(dirp)) != NULL){
			sprintf(pathbuffer, "%s/%s", path, direntp->d_name);
			if (stat(pathbuffer, &filestat) == -1){
				vprint_all("Error %d, %s getting stats on file %s\n",
					errno, strerror(errno), direntp->d_name);
				continue;
			}

			/* if entry is a file */ 
			if (S_ISREG(filestat.st_mode)){
				sprintf(listline, "%10lu %s", (unsigned long)filestat.st_size, direntp->d_name);
				add_Flistline(comp, 1, listline, NULL, FORMLIST_LAST);
			}

			/* if entry is a directory */
			else if (S_ISDIR(filestat.st_mode)){
				sprintf(listline, "       DIR %s", direntp->d_name);
				add_Flistline(comp, 0, listline, NULL, FORMLIST_LAST);
			}
		}
	        closedir(dirp);
	}

	comp = (void *) add_Fbutton("OK", 23, 18, 6, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, 1, 0, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	return(form);
}

