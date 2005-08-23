/*****************************************************************************/
/*                                                                           */
/*  Copyright (C) 2005 Adrian Gonera                                         */
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

#include "ncolor.h"
#include "common.h"
#include "screen.h"
#include "events.h"
#include "cevents.h"
#include "forms.h"
#include "option.h"
#include "config.h"

static form *mainform = NULL;

#define FORM_COLOR_MAIN make_color(configuration.form_color_fg, configuration.form_color_bg)
#define FORM_COLOR_TEXTLINE make_color(configuration.formfield_color_fg, configuration.formfield_color_bg)
#define FORM_COLOR_LIST make_color(configuration.formfield_color_fg, configuration.formfield_color_bg)
#define FORM_COLOR_BUTTON make_color(configuration.formbutton_color_fg, configuration.formbutton_color_bg)
#define FORM_COLOR_CHECKBOX make_color(configuration.form_color_fg, configuration.form_color_bg)


/** form remove ****************************************************************************/

int close_all_forms(){
	mainform = remove_form(mainform);
	return(1);
}

/** new server connect *********************************************************************/


int get_new_connect_info(int key, char *server, int *port, char *password){
	Fcomponent *comp;
	Ftextline *textline;
	static char portstr[32] = "6667";
	static char serverstr[256] = "";
	int event;

	if (mainform == NULL) mainform = create_new_server_connect_form(serverstr, atol(portstr), password);

	event = process_form_events(mainform, key);
		
	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}
	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_SERVER);
		textline = comp->component;
		strcpy(server, Ftextline_buffer_contents(textline));

		comp = form_component_by_id(mainform, FORM_ID_PORT);
		textline = comp->component;
		strcpy(portstr, Ftextline_buffer_contents(textline));
		*port = atoi(portstr);

		comp = form_component_by_id(mainform, FORM_ID_PASSWORD);
		textline = comp->component;
		strcpy(password, Ftextline_buffer_contents(textline));

		mainform = remove_form(mainform);
		return(E_OK);
	}
	print_form(mainform);
	return(key);
}

form *create_new_server_connect_form(char *server, int port, char *password){
	void *comp;
	form *form;
	char portstr[32];

	form = add_form("Connect to Server", 0x000, -1, -1, 38, 10, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, 11, 4, 1024, 25, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);        
	set_Ftextline_buffer(comp, server);
	add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port", 2, 5, 11, 5, 5, 5, FORM_COLOR_TEXTLINE, 0, IN_NUM);
	sprintf(portstr, "%d", port);
	set_Ftextline_buffer(comp, portstr);
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	
	comp = (void *) add_Ftextline("Password", 2, 6, 11, 6, 1024, 25, FORM_COLOR_TEXTLINE, STYLE_MASK_TEXT, IN_LETTERS|IN_PUNCT);        
	set_Ftextline_buffer(comp, password);
	add_form_component(form, comp, FORM_ID_PASSWORD, F_TEXTLINE);

	comp = (void *) add_Fbutton("Connect", 18, 8, 9, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 8, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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
	form = add_form("Server Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_SERVER, F_LIST);

	server = configuration.serverfavorite;
	while (server != NULL){
		strncpy(listline, server->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
		sprintf(portstr, "%d", server->port);
		strncat(listline, portstr, 255);
 
		add_Flistline(comp, 0, listline, server, FORMLIST_LAST);
		server = server->next;
	}

	comp = (void *) add_Fbutton("Connect", 20, 18, 9, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	print_form(form);
	return(form);
}


/** favorite server edit ********************************************************************/

int edit_favorite_servers(int key){
	int event;
	Fcomponent *comp;
	Flist *listline;
	Ftextline *textline;
	config_server *current;
	static config_server *edited;
	static int activeform = 0;

	char serverstr[256];
	char passstr[256];
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
				mainform = create_change_favorite_server_form(edited->name, edited->port, edited->password);
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

			comp = form_component_by_id(mainform, FORM_ID_PASSWORD);
			textline = comp->component;
			strcpy(passstr, Ftextline_buffer_contents(textline));

			if (!config_server_exists(&configuration, serverstr, port)){
				add_config_server(&configuration, serverstr, port, passstr, LIST_ORDER_FRONT);
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

			comp = form_component_by_id(mainform, FORM_ID_PASSWORD);
			textline = comp->component;
			strcpy(passstr, Ftextline_buffer_contents(textline));

			/* now copy the changes into the config struct */
			if (edited != NULL){
				strcpy(edited->name, serverstr);
				strcpy(edited->password, passstr);
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

	form = add_form("Edit Server Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
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

	comp = (void *) add_Fbutton("Add", 13, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_SERVER_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_favorite_server_form(){
	void *comp;
	form *form;

	form = add_form("Add Server Favorite", 0x000, -1, -1, 38, 10, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, 11, 4, 1024, 25, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
        add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port", 2, 5, 11, 5, 5, 5, FORM_COLOR_TEXTLINE, 0, IN_NUM);
       	set_Ftextline_buffer(comp, "6667");
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	
	comp = (void *) add_Ftextline("Password", 2, 6, 11, 6, 1024, 25, FORM_COLOR_TEXTLINE, STYLE_MASK_TEXT, IN_LETTERS|IN_PUNCT);        
	add_form_component(form, comp, FORM_ID_PASSWORD, F_TEXTLINE);


	comp = (void *) add_Fbutton("Add", 22, 8, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 8, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_favorite_server_form(char *server, int port, char *password){
	void *comp;
	form *form;
	char portstr[32];

	form = add_form("Edit Server Favorite", 0x000, -1, -1, 38, 10, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Server", 2, 4, 11, 4, 1024, 25, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_SERVER, F_TEXTLINE);
	set_Ftextline_buffer(comp, server);

	comp = (void *) add_Ftextline("Port", 2, 5, 11, 5, 5, 5, FORM_COLOR_TEXTLINE, 0, IN_NUM);
        sprintf(portstr, "%d", port);
	add_form_component(form, comp, FORM_ID_PORT, F_TEXTLINE);
	set_Ftextline_buffer(comp, portstr);
	
	comp = (void *) add_Ftextline("Password", 2, 6, 11, 6, 1024, 25, FORM_COLOR_TEXTLINE, STYLE_MASK_TEXT, IN_LETTERS|IN_PUNCT);        
	add_form_component(form, comp, FORM_ID_PASSWORD, F_TEXTLINE);
	set_Ftextline_buffer(comp, password);


	comp = (void *) add_Fbutton("OK", 21, 8, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 8, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("New Channel", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, 10, 4, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
        
	set_Ftextline_buffer(comp, "#");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Join", 21, 6, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("Channel Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_CHANNEL, F_LIST);

	channel = configuration.channelfavorite;
	while (channel != NULL){
		strncpy(listline, channel->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, 0, listline, channel, FORMLIST_LAST);
		channel = channel->next;
	}

	comp = (void *) add_Fbutton("Join", 23, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	print_form(form);
	return(form);
}

/** favorite channel edit ***********************************************************************/


int edit_favorite_channels(int key){
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

	form = add_form("Edit Channel Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_CHANNEL, F_LIST);

	channel = configuration.channelfavorite;
	while (channel != NULL){
		strncpy(listline, channel->name, 255);
		add_Flistline(comp, 0, listline, channel, FORMLIST_LAST);
		channel = channel->next;
	}

	comp = (void *) add_Fbutton("Add", 13, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_favorite_channel_form(){
	void *comp;
	form *form;

	form = add_form("Add Channel Favorite", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, 10, 4, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	set_Ftextline_buffer(comp, "#");        
	add_form_component(form, comp, FORM_ID_CHANNEL, F_TEXTLINE);

	comp = (void *) add_Fbutton("Add", 22, 6, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_favorite_channel_form(char *channel){
	void *comp;
	form *form;

	form = add_form("Edit Channel Favorite", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Channel", 2, 4, 10, 4, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
        
	add_form_component(form, comp, FORM_ID_CHANNEL, F_TEXTLINE);
	set_Ftextline_buffer(comp, channel);
	
	comp = (void *) add_Fbutton("OK", 21, 6, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("New Chat", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 7, 4, 1024, 29, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Chat", 21, 6, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	user = configuration.userfavorite;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, 0, listline, user, FORMLIST_LAST);
		user = user->next;
	}

	comp = (void *) add_Fbutton("Chat", 23, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("New DCC Chat", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 7, 4, 1024, 29, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
        set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Chat", 21, 6, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** favorite user dcc_chat  ****************************************************************/

int get_favorite_user_dccchat_info(int key, char *user){
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

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	user = configuration.userfavorite;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		i = strlen(listline);
		if (i > 29) i = 29;
		for (; i < 31; i++) listline[i] = ' ';
		listline[31] = 0;
 
		add_Flistline(comp, 0, listline, user, FORMLIST_LAST);
		user = user->next;
	}

	comp = (void *) add_Fbutton("Chat", 23, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	print_form(form);
	return(form);
}

/** new dcc send *********************************************************************/

int get_new_dccsend_info(int key, char *user){
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

	form = add_form("Send DCC File", 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 7, 4, 1024, 29, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
        set_Ftextline_buffer(comp, "");
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);

	comp = (void *) add_Fbutton("Select File", 14, 6, 13, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** favorite user dcc send  ****************************************************************/

int get_favorite_user_dccsend_info(int key, char *user){
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

	form = add_form("User Favorites", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
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


	comp = (void *) add_Fbutton("Select File", 16, 18, 13, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	print_form(form);
	return(form);
}

/** edit favorite / banned users edit ***********************************************************************/

int edit_users(int key, int listnum){
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

	form = add_form(title, 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_USER, F_LIST);

	id = FORM_ID_USERLIST_START;
	user = userlist;
	while (user != NULL){
		strncpy(listline, user->name, 255);
		add_Flistline(comp, id, listline, user, FORMLIST_LAST);
		user = user->next;
		id++;
	}

	comp = (void *) add_Fbutton("Add", 13, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Del", 19, 18, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_DELETE, F_BUTTON);
	comp = (void *) add_Fbutton("Edit", 25, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_USER_EDIT, F_BUTTON);
	comp = (void *) add_Fbutton("Done", 32, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);

	return(form);
}

form *create_new_user_form(char *title){
	void *comp;
	form *form;

	form = add_form(title, 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 7, 4, 1024, 29, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, "");

	comp = (void *) add_Fbutton("Add", 22, 6, 5, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

form *create_change_user_form(char *title, char *user){
	void *comp;
	form *form;

	form = add_form(title , 0x000, -1, -1, 38, 8, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 10, 4, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, user);
	
	comp = (void *) add_Fbutton("OK", 21, 6, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 6, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/**  list view options  ***************************************************************************/

int get_list_view_options(int key, char *searchstr, int *minuser, int *maxuser, int *searchtype){
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

	form = add_form("List View Options", 0x000, -1, -1, 40, 12, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Search for", 2, 4, 14, 4, 1024, 24, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_SEARCH, F_TEXTLINE);

	comp = (void *) add_Ftextline("Users >", 2, 5, 14, 5, 8, 7, FORM_COLOR_TEXTLINE, 0, IN_NUM); 
	add_form_component(form, comp, FORM_ID_USERSMORE, F_TEXTLINE);

	comp = (void *) add_Ftextline("Users <", 2, 6, 14, 6, 8, 7, FORM_COLOR_TEXTLINE, 0, IN_NUM);
       	add_form_component(form, comp, FORM_ID_USERSLESS, F_TEXTLINE);


	comp = (void *) add_Fcheckbox_array("Sort by:", 2, 8, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_SORT, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Channel", 23, 8, 14, 8, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, FORM_ID_SORTCHANNEL, box);

	box = add_Fcheckbox("Users", 34, 8, 27, 8, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, FORM_ID_SORTUSERS, box);

	comp = (void *) add_Fbutton("OK", 23, 10, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 10, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/**  channel select options  ***************************************************************************/

int get_channel_select_options(int key){
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

	form = add_form("Channel", 0x000, -1, -1, 38, 7, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Fbutton("Join", 2, 4, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Add to Favorites", 9, 4, 18, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CHANNEL_ADD, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 4, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}


/** transfer select options  ***************************************************************************/

int get_transfer_select_options(int key){
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;
	int event;
	int ret;

	if (mainform == NULL){
		mainform = add_form("Transfer", 0x000, -1, -1, 34, 7, FORM_COLOR_MAIN, STYLE_TITLE);        
		comp = (void *) add_Fbutton("Information", 2, 4, 13, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_TRANSFERINFO, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 16, 4, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, FORM_ID_CANCEL, F_BUTTON);
		comp = (void *) add_Fbutton("Abort", 25, 4, 7, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

		mainform = add_form("Transfer Info", 0x000, -1, -1, 38, 12, FORM_COLOR_MAIN, STYLE_TITLE);        
		comp = (void *) add_Fbutton("OK", 16, 10, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

		sprintf(scratch, "Nick       : %s\nProgress   : %lu/%lu\nRemote IP  : %s\nRemote Port: %u\nElapsed    : %02d:%02d:%02d\nRemaining  : %02d:%02d:%02d\n", 
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

		mainform = add_form("Incoming File", 0x000, -1, -1, 32, 9, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("Allow", 6, 7, 9, FORM_COLOR_BUTTON, E_OK, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_OK, F_BUTTON);
		comp = (void *) add_Fbutton("Deny", 18, 7, 8, FORM_COLOR_BUTTON, E_CANCEL, STYLE_CENTER_JUSTIFY);
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
		mainform = add_form("About", 0x000, -1, -1, 40, 9, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("OK", 16, 7, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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
		mainform = add_form("Window Size ", 0x000, -1, -1, 32, 7, FORM_COLOR_MAIN, STYLE_TITLE);
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
		mainform = add_form("Exit and Save Settings", 0x000, -1, -1, 39, 6, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("Exit&Save", 2, 4, 11, FORM_COLOR_BUTTON, E_EXIT_AND_SAVE, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT_AND_SAVE, F_BUTTON);
		comp = (void *) add_Fbutton("Exit&Discard", 14, 4, 14, FORM_COLOR_BUTTON, E_EXIT, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 29, 4, 8, FORM_COLOR_BUTTON, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);
	}

	else if (mainform == NULL){
		mainform = add_form("Exit", 0x000, -1, -1, 22, 6, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("Exit", 2, 4, 8, FORM_COLOR_BUTTON, E_EXIT, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 12, 4, 8, FORM_COLOR_BUTTON, E_CANCEL, STYLE_CENTER_JUSTIFY);
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

/** exit last server  *******************************************************************************************/

int end_last_server(int key){
	void *comp;
        int revent;
               
	if (mainform == NULL){
		mainform = add_form("Close Last Server and Exit", 0x000, -1, -1, 30, 6, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("Close and Exit", 2, 4, 16, FORM_COLOR_BUTTON, E_EXIT, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_EXIT, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 20, 4, 8, FORM_COLOR_BUTTON, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);
	}

	revent = process_form_events(mainform, key);      

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

/** exit last server  *******************************************************************************************/

int end_server_screens(int key){
	void *comp;
        int revent;
               
	if (mainform == NULL){
		mainform = add_form("Close All Server Windows", 0x000, -1, -1, 33, 6, FORM_COLOR_MAIN, STYLE_TITLE);
		comp = (void *) add_Fbutton("Close All Windows", 2, 4, 19, FORM_COLOR_BUTTON, E_CLOSE, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CLOSE, F_BUTTON);
		comp = (void *) add_Fbutton("Cancel", 23, 4, 8, FORM_COLOR_BUTTON, E_CANCEL, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_CANCEL, F_BUTTON);
	}

	revent = process_form_events(mainform, key);      

	if (revent == E_CLOSE){
		mainform = remove_form(mainform);
		return(E_CLOSE);
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

	form = add_form("Identity", 0x000, -1, -1, 40, 17, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Nick", 2, 4, 12, 4, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_NICK, F_TEXTLINE);
	set_Ftextline_buffer(comp, nick);

	comp = (void *) add_Ftextline("Alt Nick", 2, 5, 12, 5, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_ALTNICK, F_TEXTLINE);
	set_Ftextline_buffer(comp, altnick);

	comp = (void *) add_Ftextline("Name", 2, 6, 12, 6, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT|IN_BLANK);
       	add_form_component(form, comp, FORM_ID_NAME, F_TEXTLINE);
	set_Ftextline_buffer(comp, name);

	comp = (void *) add_Ftextline("User", 2, 7, 12, 7, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_USER, F_TEXTLINE);
	set_Ftextline_buffer(comp, user);

	comp = (void *) add_Ftextline("Host", 2, 8, 12, 8, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_HOST, F_TEXTLINE);
	set_Ftextline_buffer(comp, host);

	comp = (void *) add_Ftextline("Domain", 2, 9, 12, 9, 1024, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
       	add_form_component(form, comp, FORM_ID_DOMAIN, F_TEXTLINE);
	set_Ftextline_buffer(comp, domain);

	for (i = 0; i < strlen(mode); i++){
		if (mode[i] == 'i') modei = 1;
		else if (mode[i] == 'w') modew = 1;
		else if (mode[i] == 's') modes = 1;
	}

	comp = (void *) add_Fcheckbox("Invisible Mode (+i)", 27, 11, 2, 11, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_I, F_CHECKBOX);
	set_Fcheckbox_value(comp, modei);

	comp = (void *) add_Fcheckbox("Server Notices (+s)", 27, 12, 2, 12, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_S, F_CHECKBOX);
	set_Fcheckbox_value(comp, modes);

	comp = (void *) add_Fcheckbox("Wallops (+w)", 27, 13, 2, 13, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_MODE_W, F_CHECKBOX);
	set_Fcheckbox_value(comp, modew);

	comp = (void *) add_Fbutton("OK", 23, 15, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 15, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** ctcp options ***************************************************************************/

int get_ctcp_info(int key){
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

	form = add_form("CTCP Options", 0x000, -1, -1, 40, 12, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Fcheckbox("Respond to CTCP", 36, 4, 2, 4, FORM_COLOR_CHECKBOX, 
		STYLE_CHECKBOX_STAR);
	add_form_component(form, comp, FORM_ID_CTCP_ON, F_CHECKBOX);
	set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Ftextline("Throttle", 2, 5, 34, 5, 3, 4, FORM_COLOR_TEXTLINE, 0, IN_NUM);
	add_form_component(form, comp, FORM_ID_CTCP_THROTTLE, F_TEXTLINE);
	sprintf(scratch, "%d", throttle);
	set_Ftextline_buffer(comp, scratch);

	comp = (void *) add_Ftextline("Finger", 2, 7, 12, 7, MAXDESCLEN, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT|IN_BLANK);
	add_form_component(form, comp, FORM_ID_CTCP_FINGER, F_TEXTLINE);
	set_Ftextline_buffer(comp, finger);

	comp = (void *) add_Ftextline("UserInfo", 2, 8, 12, 8, MAXDESCLEN, 26, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT|IN_BLANK); 
	add_form_component(form, comp, FORM_ID_CTCP_USERINFO, F_TEXTLINE);
	set_Ftextline_buffer(comp, userinfo);

	comp = (void *) add_Fbutton("OK", 23, 10, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 10, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("DCC Options", 0x000, -1, -1, 38, 10, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Ftextline("Hostname", 2, 4, 13, 4, 1024, 23, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
       	set_Ftextline_buffer(comp, hostname);
	add_form_component(form, comp, FORM_ID_DCCHOST, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port Start", 2, 5, 30, 5, 5, 6, FORM_COLOR_TEXTLINE, 0, IN_NUM);
	sprintf(portstrbuf, "%5d", startport);
	set_Ftextline_buffer(comp, portstrbuf);
	add_form_component(form, comp, FORM_ID_DCCPORTSTART, F_TEXTLINE);

	comp = (void *) add_Ftextline("Port End", 2, 6, 30, 6, 5, 6, FORM_COLOR_TEXTLINE, 0, IN_NUM);
	sprintf(portstrbuf, "%5d", endport);
	set_Ftextline_buffer(comp, portstrbuf);
	add_form_component(form, comp, FORM_ID_DCCPORTEND, F_TEXTLINE);

	comp = (void *) add_Fbutton("OK", 21, 8, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 28, 8, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
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

	form = add_form("DCC Send Options", 0x000, -1, -1, 40, 21, FORM_COLOR_MAIN, STYLE_TITLE);

	comp = (void *) add_Ftextline("DL Path", 2, 4, 11, 4, 1024, 27, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_DLPATH, F_TEXTLINE);
	set_Ftextline_buffer(comp, dlpath);

	comp = (void *) add_Ftextline("UL Path", 2, 5, 11, 5, 1024, 27, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT); 
	add_form_component(form, comp, FORM_ID_ULPATH, F_TEXTLINE);
	set_Ftextline_buffer(comp, ulpath);


	comp = (void *) add_Fcheckbox_array("DCC Block Size", 2, 7, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_BLOCKSIZE, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("256", 36, 7, 27, 7, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 256, box);

	box = add_Fcheckbox("512", 36, 8, 27, 8, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 512, box);

	box = add_Fcheckbox("1024", 36, 9, 27, 9, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1024, box);

	box = add_Fcheckbox("2048", 36, 10, 27, 10, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 2048, box);

	box = add_Fcheckbox("4096", 36, 11, 27, 11, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 4096, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, blocksize);


	comp = (void *) add_Fcheckbox_array("Accept Downloads", 2, 13, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_DLACCEPT, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Auto", 36, 13, 27, 13, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1, box);

	box = add_Fcheckbox("Ask", 36, 14, 27, 14, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 0, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, accept);


	comp = (void *) add_Fcheckbox_array("Duplicates", 2, 16, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_DLDUPLICATES, F_CHECKBOX_ARRAY);

	box = add_Fcheckbox("Rename", 36, 16, 27, 16, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 1, box);

	box = add_Fcheckbox("Replace", 36, 17, 27, 17, FORM_COLOR_CHECKBOX, STYLE_CHECKBOX_STAR);
	add_Fcheckbox_to_array((Fcheckbox_array *)comp, 0, box);

	Fcheckbox_array_select_by_id((Fcheckbox_array *)comp, duplicates);


	comp = (void *) add_Fbutton("OK", 23, 19, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 19, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);
         
	return(form);
}

/** client options ***************************************************************************/

int get_client_options(int key){
	int event;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;

	if (mainform == NULL){
		mainform = create_client_options_form();
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

		comp = form_component_by_id(mainform, FORM_ID_TIMESTAMP_FORMAT);
		textline = comp->component;
 		strcpy(configuration.timestampformat, Ftextline_buffer_contents(textline));


		comp = form_component_by_id(mainform, FORM_ID_TIMESTAMP_CHANNEL);
		checkbox = comp->component;
 		configuration.channeltimestamps = Fcheckbox_value(checkbox);

		comp = form_component_by_id(mainform, FORM_ID_TIMESTAMP_CHAT);
		checkbox = comp->component;
 		configuration.chattimestamps = Fcheckbox_value(checkbox);

		comp = form_component_by_id(mainform, FORM_ID_TIMESTAMP_DCC);
		checkbox = comp->component;
 		configuration.dcctimestamps = Fcheckbox_value(checkbox);


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


form *create_client_options_form(){
	void *comp;
	form *form;
	Fcheckbox *box;
	int i;
	char temp[256];

	form = add_form("Client Options", 0x000, -1, -1, 40, 15, FORM_COLOR_MAIN, STYLE_TITLE);

	sprintf(temp, "%d", configuration.connecttimeout);        
	comp = (void *) add_Ftextline("Server Connect Timeout", 2, 4, 35, 4, 2, 3, FORM_COLOR_TEXTLINE, 0, IN_LETTERS|IN_PUNCT);
	add_form_component(form, comp, FORM_ID_CONNECT_TIMEOUT, F_TEXTLINE);
	set_Ftextline_buffer(comp, temp);

	comp = (void *) add_Ftextline("Timestamp Format", 2, 6, 25, 6, 40, 13, FORM_COLOR_TEXTLINE, 0, IN_ALL);
	add_form_component(form, comp, FORM_ID_TIMESTAMP_FORMAT, F_TEXTLINE);
	set_Ftextline_buffer(comp, configuration.timestampformat);

	comp = (void *) add_Fcheckbox("Timestamps in Channels", 36, 7, 2, 7, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_TIMESTAMP_CHANNEL, F_CHECKBOX);
	if (configuration.channeltimestamps) set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Fcheckbox("Timestamps in Chats", 36, 8, 2, 8, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_TIMESTAMP_CHAT, F_CHECKBOX);
	if (configuration.chattimestamps) set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Fcheckbox("Timestamps in DCCs", 36, 9, 2, 9, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_TIMESTAMP_DCC, F_CHECKBOX);
	if (configuration.dcctimestamps) set_Fcheckbox_value(comp, 1);


	comp = (void *) add_Fcheckbox("Autosave Configuration", 36, 11, 2, 11, FORM_COLOR_CHECKBOX, 0);
	add_form_component(form, comp, FORM_ID_AUTOSAVE, F_CHECKBOX);
	if (configuration.autosave) set_Fcheckbox_value(comp, 1);

	comp = (void *) add_Fbutton("OK", 23, 13, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 13, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

         
	return(form);
}

/** color options ***************************************************************************/

int get_color_info(int key){
	int event, id;
	Fcomponent *comp;
	Ftextline *textline;
	Fcheckbox *checkbox;

	if (mainform == NULL){
		mainform = create_color_options_form();
	}

	event = process_form_events(mainform, key);

	if (event == FORM_ID_CANCEL){
		mainform = remove_form(mainform);
		return(E_CANCEL);
	}   

	if (event == FORM_ID_RESET){
		configuration.menu_color_fg = DEFAULT_MENU_COLOR_F;
		configuration.menu_color_bg = DEFAULT_MENU_COLOR_B;
		configuration.form_color_fg = DEFAULT_FORM_COLOR_F;
		configuration.form_color_bg = DEFAULT_FORM_COLOR_B;
		configuration.formfield_color_fg = DEFAULT_FORMFIELD_COLOR_F;
		configuration.formfield_color_bg = DEFAULT_FORMFIELD_COLOR_B;
		configuration.formbutton_color_fg = DEFAULT_FORMBUTTON_COLOR_F;
		configuration.formbutton_color_bg = DEFAULT_FORMBUTTON_COLOR_B;

		configuration.win_color_fg = DEFAULT_WIN_COLOR_F;
		configuration.win_color_bg = DEFAULT_WIN_COLOR_B;

		configuration.message_color = DEFAULT_MESSAGE_COLOR;
		configuration.error_color = DEFAULT_ERROR_COLOR;
		configuration.notice_color = DEFAULT_NOTICE_COLOR;
		configuration.ctcp_color = DEFAULT_CTCP_COLOR;
		configuration.dcc_color = DEFAULT_DCC_COLOR;
		configuration.join_color = DEFAULT_JOIN_COLOR;
		configuration.rename_color = DEFAULT_RENAME_COLOR;
		configuration.kick_color = DEFAULT_KICK_COLOR;
		configuration.mode_color = DEFAULT_MODE_COLOR;
		configuration.invite_color = DEFAULT_INVITE_COLOR;

		mainform = remove_form(mainform);
		return(E_NONE);
	}   

	else if (event == FORM_ID_OK){
		comp = form_component_by_id(mainform, FORM_ID_WINCOLOR_F);
		configuration.win_color_fg = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_WINCOLOR_B);
		configuration.win_color_bg = active_list_line_id(comp->component);

		comp = form_component_by_id(mainform, FORM_ID_MENUCOLOR_F);
		configuration.menu_color_fg = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_MENUCOLOR_B);
		configuration.menu_color_bg = active_list_line_id(comp->component);

		comp = form_component_by_id(mainform, FORM_ID_FORMCOLOR_F);
		configuration.form_color_fg = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_FORMCOLOR_B);
		configuration.form_color_bg = active_list_line_id(comp->component);

		comp = form_component_by_id(mainform, FORM_ID_FIELDCOLOR_F);
		configuration.formfield_color_fg = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_FIELDCOLOR_B);
		configuration.formfield_color_bg = active_list_line_id(comp->component);

		comp = form_component_by_id(mainform, FORM_ID_BUTTONCOLOR_F);
		configuration.formbutton_color_fg = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_BUTTONCOLOR_B);
		configuration.formbutton_color_bg = active_list_line_id(comp->component);

		/* message and text colors */
		comp = form_component_by_id(mainform, FORM_ID_MESSAGECOLOR);
		configuration.message_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_ERRORCOLOR);
		configuration.error_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_NOTICECOLOR);
		configuration.notice_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_CTCPCOLOR);
		configuration.ctcp_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_DCCCOLOR);
		configuration.dcc_color = active_list_line_id(comp->component);

		comp = form_component_by_id(mainform, FORM_ID_JOINCOLOR);
		configuration.join_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_RENAMECOLOR);
		configuration.rename_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_KICKCOLOR);
		configuration.kick_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_MODECOLOR);
		configuration.mode_color = active_list_line_id(comp->component);
		comp = form_component_by_id(mainform, FORM_ID_INVITECOLOR);
		configuration.invite_color = active_list_line_id(comp->component);


		/* set the default foreground and backround colors */
		assume_default_colors(configuration.win_color_fg, configuration.win_color_bg);

		mainform = remove_form(mainform);
		configuration.changed = 1;
		return(E_OK);
	}
	else if (event == KEY_ENTER || event == 10) process_form_events(mainform, E_NEXT);

	print_form(mainform);
	return(key);
}


form *create_color_options_form(){
	void *comp;
	form *form;
	Fcheckbox *box;
	Ftextarea *text;
	int i;
	char temp[256];

	form = add_form("Color Settings", 0x000, -1, -1, 60, 18, FORM_COLOR_MAIN, STYLE_TITLE);

	text = add_Ftextarea("Screen", 2, 4, 20, 1, STYLE_LEFT_JUSTIFY, -1, "Screen");
	add_form_textarea (form, text);
	comp = (void *) add_Flist("Foreground", 22, 4, 33, 4, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_WINCOLOR_F, F_LIST);
	create_color_list(comp, configuration.win_color_fg);
	comp = (void *) add_Flist("Background", 41, 4, 52, 4, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_WINCOLOR_B, F_LIST);
	create_color_list(comp, configuration.win_color_bg);

	text = add_Ftextarea("Menu", 2, 5, 20, 1, STYLE_LEFT_JUSTIFY, -1, "Menus");
	add_form_textarea (form, text);
	comp = (void *) add_Flist("Foreground", 22, 5, 33, 5, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_MENUCOLOR_B, F_LIST);
	create_color_list(comp, configuration.menu_color_bg);
	comp = (void *) add_Flist("Background", 41, 5, 52, 5, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_MENUCOLOR_F, F_LIST);
	create_color_list(comp, configuration.menu_color_fg);
	
	text = add_Ftextarea("Form", 2, 6, 20, 1, STYLE_LEFT_JUSTIFY, -1, "Forms");
	add_form_textarea (form, text);
	comp = (void *) add_Flist("Foreground", 22, 6, 33, 6, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_FORMCOLOR_B, F_LIST);
	create_color_list(comp, configuration.form_color_bg);
	comp = (void *) add_Flist("Background", 41, 6, 52, 6, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_FORMCOLOR_F, F_LIST);
	create_color_list(comp, configuration.form_color_fg);

	text = add_Ftextarea("Fields", 2, 7, 20, 1, STYLE_LEFT_JUSTIFY, -1, "Form Fields");
	add_form_textarea (form, text);
	comp = (void *) add_Flist("Foreground", 22, 7, 33, 7, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_FIELDCOLOR_F, F_LIST);
	create_color_list(comp, configuration.formfield_color_fg);
	comp = (void *) add_Flist("Background", 41, 7, 52, 7, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_FIELDCOLOR_B, F_LIST);
	create_color_list(comp, configuration.formfield_color_bg);

	text = add_Ftextarea("Buttons", 2, 8, 20, 1, STYLE_LEFT_JUSTIFY, -1, "Form Buttons");
	add_form_textarea (form, text);
	comp = (void *) add_Flist("Foreground", 22, 8, 33, 8, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_BUTTONCOLOR_F, F_LIST);
	create_color_list(comp, configuration.formbutton_color_fg);
	comp = (void *) add_Flist("Background", 41, 8, 52, 8, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_BUTTONCOLOR_B, F_LIST);
	create_color_list(comp, configuration.formbutton_color_bg);


	/* first column */
	comp = (void *) add_Flist("Message Color", 2, 10, 22, 10, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_MESSAGECOLOR, F_LIST);
	create_color_list(comp, configuration.message_color);

	comp = (void *) add_Flist("Error Color", 2, 11, 22, 11, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_ERRORCOLOR, F_LIST);
	create_color_list(comp, configuration.error_color);

	comp = (void *) add_Flist("Notice Color", 2, 12, 22, 12, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_NOTICECOLOR, F_LIST);
	create_color_list(comp, configuration.notice_color);

	comp = (void *) add_Flist("CTCP Color", 2, 13, 22, 13, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_CTCPCOLOR, F_LIST);
	create_color_list(comp, configuration.ctcp_color);

	comp = (void *) add_Flist("DCC Color", 2, 14, 22, 14, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_DCCCOLOR, F_LIST);
	create_color_list(comp, configuration.dcc_color);

	/* second column */
	comp = (void *) add_Flist("Join Color", 32, 10, 52, 10, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_JOINCOLOR, F_LIST);
	create_color_list(comp, configuration.join_color);

	comp = (void *) add_Flist("Rename Color", 32, 11, 52, 11, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_RENAMECOLOR, F_LIST);
	create_color_list(comp, configuration.rename_color);

	comp = (void *) add_Flist("Kick Color", 32, 12, 52, 12, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_KICKCOLOR, F_LIST);
	create_color_list(comp, configuration.kick_color);

	comp = (void *) add_Flist("Mode Color", 32, 13, 52, 13, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_MODECOLOR, F_LIST);
	create_color_list(comp, configuration.mode_color);

	comp = (void *) add_Flist("Invite Color", 32, 14, 52, 14, 6, 1, FORM_COLOR_LIST, STYLE_TITLE | STYLE_NO_HIGHLIGHT);
	add_form_component(form, comp, FORM_ID_INVITECOLOR, F_LIST);
	create_color_list(comp, configuration.invite_color);

	comp = (void *) add_Fbutton("Reset to Defaults", 23, 16, 19, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_RESET, F_BUTTON);
	comp = (void *) add_Fbutton("OK", 43, 16, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 50, 16, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

         
	return(form);
}

void create_color_list(Flist *comp, int curcolor){
	add_Flistline(comp, C_BLACK, "Black", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_WHITE, "White", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_RED, "Red", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_GREEN, "Green", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_BLUE, "Blue", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_YELLOW, "Yellow", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_MAGENTA, "Purple", NULL, FORMLIST_LAST);
	add_Flistline(comp, C_CYAN, "Cyan", NULL, FORMLIST_LAST);
	set_active_list_line_by_id(comp, curcolor);
}

/** terminal info *******************************************************************************************/

int get_term_info(int key){
        void *comp;
	Ftextarea *text;
        int revent;

	if (mainform == NULL){

		mainform = add_form("Terminal Information", 0x000, -1, -1, 32, 13, FORM_COLOR_MAIN, STYLE_TITLE);

		text = add_Ftextarea("Terminal", 2, 3, 36, 1, STYLE_CENTER_JUSTIFY, -1, "Terminal         %.11s", termname());
		add_form_textarea (mainform, text);

		text = add_Ftextarea("Screen", 2, 4, 36, 1, STYLE_CENTER_JUSTIFY, -1,   "Screen Size      %dx%d", COLS, LINES);
		add_form_textarea (mainform, text);

		#ifdef NCURSES_VERSION
		text = add_Ftextarea("Interface", 2, 5, 36, 1, STYLE_CENTER_JUSTIFY, -1, "Curses Version   %s", NCURSES_VERSION);
		add_form_textarea (mainform, text);
		#else
		// text = add_Ftextarea("Interface", 2, 5, 36, 1, STYLE_CENTER_JUSTIFY, -1, "Curses Version    Unknown");
		// add_form_textarea (mainform, text);
		#endif


		text = add_Ftextarea("Color", 2, 6, 36, 1, STYLE_CENTER_JUSTIFY, -1,     "Color Support    %c", has_colors()?'Y':'N');
		add_form_textarea (mainform, text);

		text = add_Ftextarea("Colors", 2, 7, 36, 1, STYLE_CENTER_JUSTIFY, -1,    "Colors           %d", COLORS);
		add_form_textarea (mainform, text);

		text = add_Ftextarea("Pairs", 2, 8, 36, 1, STYLE_CENTER_JUSTIFY, -1,     "Color Palette    %d", COLOR_PAIRS);
		add_form_textarea (mainform, text);

		text = add_Ftextarea("Pairs", 2, 9, 36, 1, STYLE_CENTER_JUSTIFY, -1,     "Custom Colors    %c", can_change_color()?'Y':'N');
		add_form_textarea (mainform, text);

		comp = (void *) add_Fbutton("OK", 12, 11, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
		add_form_component(mainform, comp, E_OK, F_BUTTON);
	}

	print_form(mainform);
	if (key == 10 || key == KEY_ENTER){
		mainform = remove_form(mainform);
		return(E_OK);
	}
	else return(E_NONE);
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


	form = add_form("Select File", 0x000, -1, -1, 40, 20, FORM_COLOR_MAIN, STYLE_TITLE);
        
	comp = (void *) add_Flist("", 2, 3, 2, 4, 36, 13, FORM_COLOR_LIST, STYLE_TITLE);
	add_form_component(form, comp, FORM_ID_FILE, F_LIST);

	//vprint_all("Reading directory %s.\n", path);

	/* open the current directory */ 
	dirp = opendir(path);

	/* if cannot read directory, provide a way to step out of it */
	if (dirp == NULL){
		vprint_all("Cannot open directory %s.\n", path);
		strcpy(listline, "       DIR ..");
		add_Flistline(comp, 0, listline, NULL, FORMLIST_LAST);
	}
	else{
		while((direntp = readdir(dirp)) != NULL){
			sprintf(pathbuffer, "%s/%s", path, direntp->d_name);
			if (stat(pathbuffer, &filestat) == -1){
				/* large file support still complains about filesize */
				if (errno != EOVERFLOW){
					vprint_all("Error %d, %s getting stats on file %s\n",
						errno, strerror(errno), direntp->d_name);
					continue;
				}
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

	comp = (void *) add_Fbutton("OK", 23, 18, 6, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_OK, F_BUTTON);
	comp = (void *) add_Fbutton("Cancel", 30, 18, 8, FORM_COLOR_BUTTON, E_COMPONENT_ID, STYLE_CENTER_JUSTIFY);
	add_form_component(form, comp, FORM_ID_CANCEL, F_BUTTON);

	return(form);
}

