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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

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
#include "log.h"
#include "cmenu.h"

extern menu *ServerMenu[6];
extern menu *ChannelMenu[4];
extern menu *TransferMenu[4];
extern menu *ListMenu[4];
extern menu *ChatMenu[4];
extern menu *DCCChatMenu[4];
extern menu *HelpMenu[2];
extern menu *UserMenu;
extern menu *UserListMenu;
extern menu *CtcpMenu;
extern menu *DCCMenu;
extern menu *ControlMenu;

void move_menu(menu *menuptr, int startx, int starty){
	if (menuptr != NULL){
		menuptr->startx = startx;
		menuptr->starty = starty;
		if (menuptr->window != NULL) mvwin(menuptr->window, starty, startx);
	}
}

menu *init_menu(int startx, int starty, char *name, int key){
	menu *menuptr;

	menuptr = malloc(sizeof(menu));
	if (menuptr != NULL){
		menuptr->startx = startx;
		menuptr->starty = starty;
		menuptr->item = NULL;
		menuptr->selected = NULL;
		menuptr->chosen = NULL;
		menuptr->width = 2;
		menuptr->height = 2;
		menuptr->entries = 0;
		strncpy(menuptr->name, name, MENUNAMELEN);
		menuptr->key = key;
		menuptr->window = NULL;
	}
	else {
		print_all("Couldn't allocate menu memory\n");
		exit (-1);
	}	
	return(menuptr);
}

menubar *init_menubar(int entries, int textstart, int textspace, menu **menu){
	menubar *menubarptr;

	menubarptr = malloc(sizeof(menubar));
	if (menubarptr != NULL){
		menubarptr->entries = entries;
		menubarptr->textstart = textstart;
		menubarptr->textspace = textspace;
		menubarptr->changed = 1;
		menubarptr->current = -1;
		menubarptr->menu = menu;
	}
	else {
		plog("Couldn't allocate menu memory in init_menubar\n");
		exit (-1);
	}	
	return(menubarptr);
}

int delete_menu(menu *menu){
	menuitem *current;

	if (menu != NULL){
		current = menu->item;
		while (current != NULL){
			// free all space dedicated to items
			current = current->next;
			free(current);
		}
		free(menu);
	}
	return(1);
}

int selected_menu_item_id(menu *menu){
	int id;

	if (menu == NULL) return(0);
	if (menu->chosen != NULL){
	
		id = 0;
		// if at the last submenu return selected id and reset
		// the chosen flag

		if ((menu->chosen)->child == NULL){
			id = (menu->chosen)->id;
			menu->chosen = NULL;
			return(id);
		}	
		else {
			// if a menu item was chosen, reset all chosen
			// flags on the way back
			id = selected_menu_item_id((menu->chosen)->child);
			if (id != 0) menu->chosen = NULL;
			return(id);
		}
	}
	return(0);
}
	
int select_next_item(menu *menu){
	menuitem *current, *next;

	if (menu != NULL){
		current = menu->item;
		while (current != NULL){
			// if this is the currently selected item, find next selectable item
			if (current == menu->selected){
				next = current->next;
				while (next != NULL){
					if ((next->option)&M_SELECTABLE){
						menu->selected = next;
						break;
					}
					next = next->next;
				}
				// if reached the end, no selectable items found, start from 1st
				// if no selectables exist, exit after entire menu is checked
				if (next == NULL){
					next = menu->item;
					while (next != NULL){
						if ((next->option)&M_SELECTABLE){
                                                	menu->selected = next;
                                                	break;
						}
                                        	next = next->next;
					}
                                }
				break;
			}		
			current = current->next;
		}
	}
	return(1);
}

int select_prev_item(menu *menu){
	menuitem *current, *prev, *last;

	if (menu != NULL){
		current = menu->item;
		while (current != NULL){
			// if this is the currently selected item, find previous selectable item
			if (current == menu->selected){
				prev = current->prev;
				while (prev != NULL){
					if ((prev->option)&M_SELECTABLE){
						menu->selected = prev;
						break;
					}
					prev = prev->prev;
				}
				// if reached the beginning, no selectable items found, find last
				// start at last and check the entire menu backwards
				if (prev == NULL){
					current = menu->item;
					last = current;
					while (current != NULL){
						last = current;
						current = current->next;
					}	
					prev = last;
					while (prev != NULL){
						if ((prev->option)&M_SELECTABLE){
                                                	menu->selected = prev;
                                                	break;
						}
                                        	prev = prev->prev;
					}
                                }
				break;
			}		
			current = current->next;
		}
	}
	return(1);
}

menuitem *add_menu_item(menu *menuptr, char *text, char *desc, int key, int option, int id, menu *child){
	menuitem *current, *last, *new;

	if (menuptr != NULL){
		new = malloc (sizeof(menuitem));
		if (new == NULL){
			print_all("Couldn't allocate menu item memory\n");
			exit (-1);
		}
		strcpy(new->text, text);
		strcpy(new->description, desc);
		new->key = key;
		new->option = option;
		new->id = id;
		new->child = child;

		if (menuptr->item == NULL){
			new->next = NULL;
			new->prev = NULL;
			menuptr->item = new;
			menuptr->selected = new;
		}
		else {
			last = NULL;
			current = menuptr->item;
			while(current != NULL){
				last = current;
				current = current->next;
			}
			last->next = new;
			new->prev = last;
			new->next = NULL;
		}
		
		if (menuptr->width < strlen(text) + 2){
			 menuptr->width = strlen(text) + 2;
		}
		menuptr->entries++;
		menuptr->height++;	
		return(new);
	}
	return(NULL);
}



void init_all_menus(){
	menu *serverconn;
	menu *channeljoin;


	/** SERVER MENU ***********************************************************************************************/

	ServerMenu[0] = init_menu(1, 1, "Server", 'S');
	add_menu_item(ServerMenu[0], " Connect New ...              ", "", 0, M_SELECTABLE, E_CONNECT_NEW, NULL);
	add_menu_item(ServerMenu[0], " Connect Favorite ...         ", "", 0, M_SELECTABLE, E_CONNECT_FAVORITE, NULL);
	add_menu_item(ServerMenu[0], " Disconnect                   ", "", 0, M_SELECTABLE, E_DISCONNECT, NULL);
	add_menu_item(ServerMenu[0], "                              ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[0], " Add to Favorites             ", "", 0, M_SELECTABLE, E_SERVER_ADD_FAVORITE, NULL);
	add_menu_item(ServerMenu[0], " Edit Favorites ...           ", "", 0, M_SELECTABLE, E_SERVER_EDIT_FAVORITE, NULL);
	add_menu_item(ServerMenu[0], "                              ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[0], " Exit                  Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	ServerMenu[1] = init_menu(11, 1,"Channel", 'N');
	add_menu_item(ServerMenu[1], " Join New ...             ", "", 0, M_SELECTABLE, E_JOIN_NEW, NULL);
	add_menu_item(ServerMenu[1], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[1], " Join Favorite ...        ", "", 0, M_SELECTABLE, E_JOIN_FAVORITE, NULL);
	add_menu_item(ServerMenu[1], " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_CHANNEL_EDIT_FAVORITE, NULL);
	add_menu_item(ServerMenu[1], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[1], " Channel List ...         ", "", 0, M_SELECTABLE, E_CHANNEL_LIST, NULL);

	ServerMenu[2] = init_menu(22, 1,"User", 'U');
	add_menu_item(ServerMenu[2], " Chat New ...             ", "", 0, M_SELECTABLE, E_CHAT_NEW, NULL);
	add_menu_item(ServerMenu[2], " Chat Favorite ...        ", "", 0, M_SELECTABLE, E_CHAT_FAVORITE, NULL);
	add_menu_item(ServerMenu[2], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[2], " DCC Chat New ...         ", "", 0, M_SELECTABLE, E_DCC_CHAT_NEW, NULL);
	add_menu_item(ServerMenu[2], " DCC Chat Favorite ...    ", "", 0, M_SELECTABLE, E_DCC_CHAT_FAVORITE, NULL);
	add_menu_item(ServerMenu[2], " DCC Send New ...         ", "", 0, M_SELECTABLE, E_DCC_SEND_NEW, NULL);
	add_menu_item(ServerMenu[2], " DCC Send Favorite ...    ", "", 0, M_SELECTABLE, E_DCC_SEND_FAVORITE, NULL);
	add_menu_item(ServerMenu[2], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[2], " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(ServerMenu[2], " Edit Ignored ...         ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);

	ServerMenu[3] = init_menu(30, 1, "Options", 'T');
	add_menu_item(ServerMenu[3], " Identity ...           ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(ServerMenu[3], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[3], " Client Options ...     ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(ServerMenu[3], " CTCP Options ...       ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(ServerMenu[3], " DCC Options ...        ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(ServerMenu[3], " DCC Send Options ...        ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(ServerMenu[3], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[3], " Save Configuration     ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);

	ServerMenu[4] = init_menu(41, 1, "Window", 'W');
	add_menu_item(ServerMenu[4], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(ServerMenu[4], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	ServerMenu[5] = init_menu(51, 1, "Help", 'P');
	add_menu_item(ServerMenu[5], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(ServerMenu[5], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(ServerMenu[5], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(ServerMenu[5], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ServerMenu[5], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);


	/** CHANNEL MENU ***********************************************************************************************/


	ChannelMenu[0] = init_menu(1, 1,"Channel", 'N');
	add_menu_item(ChannelMenu[0], " Join New ...             ", "", 0, M_SELECTABLE, E_JOIN_NEW, NULL);
	add_menu_item(ChannelMenu[0], " Join Favorite ...        ", "", 0, M_SELECTABLE, E_JOIN_FAVORITE, NULL);
	add_menu_item(ChannelMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChannelMenu[0], " Add to Favorites         ", "", 0, M_SELECTABLE, E_CHANNEL_ADD_FAVORITE, NULL);
	add_menu_item(ChannelMenu[0], " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_CHANNEL_EDIT_FAVORITE, NULL);
	add_menu_item(ChannelMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChannelMenu[0], " Leave                    ", "", 0, M_SELECTABLE, E_CHANNEL_PART, NULL);
	add_menu_item(ChannelMenu[0], " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(ChannelMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChannelMenu[0], " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	ChannelMenu[1] = init_menu(12, 1, "Window", 'W');
	add_menu_item(ChannelMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(ChannelMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	ChannelMenu[2] = init_menu(22, 1, "Help", 'P');
	add_menu_item(ChannelMenu[2], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(ChannelMenu[2], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(ChannelMenu[2], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(ChannelMenu[2], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChannelMenu[2], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);

	CtcpMenu = init_menu(1, 1 ,"Ctcp", 0);
	add_menu_item(CtcpMenu, " Ping         ", "", 0, M_SELECTABLE, E_CTCP_PING, NULL);
	add_menu_item(CtcpMenu, " Client Info  ", "", 0, M_SELECTABLE, E_CTCP_CLIENTINFO, NULL);
	add_menu_item(CtcpMenu, " User Info    ", "", 0, M_SELECTABLE, E_CTCP_USERINFO, NULL);
	add_menu_item(CtcpMenu, " Version      ", "", 0, M_SELECTABLE, E_CTCP_VERSION, NULL);
	add_menu_item(CtcpMenu, " Finger       ", "", 0, M_SELECTABLE, E_CTCP_FINGER, NULL);
	add_menu_item(CtcpMenu, " Source       ", "", 0, M_SELECTABLE, E_CTCP_SOURCE, NULL);
	add_menu_item(CtcpMenu, " Time         ", "", 0, M_SELECTABLE, E_CTCP_TIME, NULL);

	DCCMenu = init_menu(1, 1 ,"DCC", 0);
	add_menu_item(DCCMenu, " Chat     ", "", 0, M_SELECTABLE, E_DCC_CHAT, NULL);
	add_menu_item(DCCMenu, " Send ... ", "", 0, M_SELECTABLE, E_DCC_SEND, NULL);

	ControlMenu = init_menu(1, 1 ,"Control", 0);
	add_menu_item(ControlMenu, " Op (+o)       ", "", 0, M_SELECTABLE, E_CONTROL_OP, NULL);
	add_menu_item(ControlMenu, " DeOP (-o)     ", "", 0, M_SELECTABLE, E_CONTROL_DEOP, NULL);
	add_menu_item(ControlMenu, " Voice (+v)    ", "", 0, M_SELECTABLE, E_CONTROL_VOICE, NULL);
	add_menu_item(ControlMenu, " DeVoice (-v)  ", "", 0, M_SELECTABLE, E_CONTROL_DEVOICE, NULL);
	add_menu_item(ControlMenu, "               ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ControlMenu, " Kick          ", "", 0, M_SELECTABLE, E_CONTROL_KICK, NULL);
	add_menu_item(ControlMenu, " Ban           ", "", 0, M_SELECTABLE, E_CONTROL_BAN, NULL);
	add_menu_item(ControlMenu, " Kick and Ban  ", "", 0, M_SELECTABLE, E_CONTROL_KICKBAN, NULL);

	UserListMenu = init_menu(1, 1 ,"User List", 0);
	add_menu_item(UserListMenu, " Add to Favorites      ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(UserListMenu, " Remove from Favorites ", "", 0, M_SELECTABLE, E_USER_REMOVE_FAVORITE, NULL);
	add_menu_item(UserListMenu, " Ignore                ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(UserListMenu, " Unignore              ", "", 0, M_SELECTABLE, E_USER_REMOVE_IGNORED, NULL);

	UserMenu = init_menu(1, 1,"User", 0);
	add_menu_item(UserMenu, " Copy and Paste    ", "", 0, M_SELECTABLE, E_USER_PASTE, NULL);
	add_menu_item(UserMenu, "                   ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(UserMenu, " Control ...       ", "", 0, M_SELECTABLE, -1, ControlMenu);
	add_menu_item(UserMenu, " Ctcp ...          ", "", 0, M_SELECTABLE, -1, CtcpMenu);
	add_menu_item(UserMenu, " DCC ...           ", "", 0, M_SELECTABLE, -1, DCCMenu);
	add_menu_item(UserMenu, " User List ...     ", "", 0, M_SELECTABLE, -1, UserListMenu);
	add_menu_item(UserMenu, "                   ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(UserMenu, " Chat              ", "", 0, M_SELECTABLE, E_QUERY, NULL);
	add_menu_item(UserMenu, " Whois             ", "", 0, M_SELECTABLE, E_WHOIS, NULL);

	// add_menu_item(UserMenu, " Dcc ...           ", "", 0, M_SELECTABLE, -1, NULL);
	// add_menu_item(UserMenu, " Info ...          ", "", 0, M_SELECTABLE, -1, InfoMenu);

	/** CHAT MENU ***********************************************************************************************/


	ChatMenu[0] = init_menu(1, 1, "Chat", 'T');
	add_menu_item(ChatMenu[0], " Chat New ...             ", "", 0, M_SELECTABLE, E_CHAT_NEW, NULL);
	add_menu_item(ChatMenu[0], " Chat Favorite ...        ", "", 0, M_SELECTABLE, E_CHAT_FAVORITE, NULL);
	add_menu_item(ChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChatMenu[0], " Ignore                   ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(ChatMenu[0], " Edit Ignore List ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menu_item(ChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChatMenu[0], " Add to Favorites         ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(ChatMenu[0], " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(ChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChatMenu[0], " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(ChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChatMenu[0], " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	ChatMenu[1] = init_menu(9, 1, "Window", 'W');
	add_menu_item(ChatMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(ChatMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	ChatMenu[2] = init_menu(20, 1, "Help", 'P');
	add_menu_item(ChatMenu[2], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(ChatMenu[2], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(ChatMenu[2], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(ChatMenu[2], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ChatMenu[2], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);


	/** DCCCHAT MENU ***********************************************************************************************/


	DCCChatMenu[0] = init_menu(1, 1, "DCC Chat", 'T');

	add_menu_item(DCCChatMenu[0], " DCC Chat New ...         ", "", 0, M_SELECTABLE, E_DCC_CHAT_NEW, NULL);
	add_menu_item(DCCChatMenu[0], " DCC Chat Favorite ...    ", "", 0, M_SELECTABLE, E_DCC_CHAT_FAVORITE, NULL);
	add_menu_item(DCCChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(DCCChatMenu[0], " Ignore                   ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(DCCChatMenu[0], " Edit Ignore List ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menu_item(DCCChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(DCCChatMenu[0], " Add to Favorites         ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(DCCChatMenu[0], " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(DCCChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(DCCChatMenu[0], " Disconnect               ", "", 0, M_SELECTABLE, E_DISCONNECT, NULL);
	add_menu_item(DCCChatMenu[0], " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(DCCChatMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(DCCChatMenu[0], " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	DCCChatMenu[1] = init_menu(13, 1, "Window", 'W');
	add_menu_item(DCCChatMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(DCCChatMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	DCCChatMenu[2] = init_menu(24, 1, "Help",'P');
	add_menu_item(DCCChatMenu[2], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(DCCChatMenu[2], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(DCCChatMenu[2], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(DCCChatMenu[2], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(DCCChatMenu[2], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);


	/** TRANSFER MENU ***********************************************************************************************/


	TransferMenu[0] = init_menu(1, 1,"Transfer", 'T');
	add_menu_item(TransferMenu[0], " Abort Selected           ", "", 0, M_SELECTABLE, E_TRANSFER_STOP, NULL);
	add_menu_item(TransferMenu[0], " Info                     ", "", 0, M_SELECTABLE, E_TRANSFER_INFO, NULL);
	add_menu_item(TransferMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(TransferMenu[0], " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	TransferMenu[1] = init_menu(13, 1, "Window", 'W');
	add_menu_item(TransferMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(TransferMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

	TransferMenu[2] = init_menu(23, 1, "Help", 'P');
	add_menu_item(TransferMenu[2], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(TransferMenu[2], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(TransferMenu[2], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(TransferMenu[2], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(TransferMenu[2], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);


	/** LIST MENU ***************************************************************************************************/

	ListMenu[0] = init_menu(1, 1,"List", 'T');
	add_menu_item(ListMenu[0], " View ...                 ", "", 0, M_SELECTABLE, E_LIST_VIEW, NULL);
	add_menu_item(ListMenu[0], " Add to Favorites         ", "", 0, M_SELECTABLE, E_CHANNEL_ADD_FAVORITE, NULL);
	add_menu_item(ListMenu[0], " Join                     ", "", 0, M_SELECTABLE, E_CHANNEL_JOIN, NULL);	
	add_menu_item(ListMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ListMenu[0], " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(ListMenu[0], "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ListMenu[0], " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	ListMenu[1] = init_menu(9, 1, "Window", 'W');
	add_menu_item(ListMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(ListMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_DISCONNECT, NULL);

	ListMenu[2] = init_menu(20, 1, "Help", 'P');
	add_menu_item(ListMenu[2], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(ListMenu[2], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(ListMenu[2], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(ListMenu[2], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ListMenu[2], " About                  ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);


	/** HELP MENU **************************************************************************************************/

	HelpMenu[0] = init_menu(1, 1, "Help",'P');
	add_menu_item(HelpMenu[0], " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(HelpMenu[0], " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(HelpMenu[0], " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(HelpMenu[0], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(HelpMenu[0], " Close           Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(HelpMenu[0], "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(HelpMenu[0], " Exit            Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);

	HelpMenu[1] = init_menu(9, 1, "Window", 'W');
	add_menu_item(HelpMenu[1], " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(HelpMenu[1], " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);

}	

void print_menu(menu *menu){
	menuitem *current;
	int ypos, i, mheight, mwidth, mstarty, mstartx, strabslen;
	char text[256];

	if (menu == NULL) return;
	if (menu->starty >= LINES || menu->startx >= COLS){
		print_all("Menu cannot be displayed properly, please resize the screen.\n");
	}
	else {
/*
		if (menu->startx + menu->width > COLS) mwidth = COLS - menu->startx;
		else mwidth = menu->width;

		if (menu->starty + menu->height > LINES) mheight = LINES - menu->starty;
		else mheight = menu->height;

		// set this correctly to fit on screen (fix)
		mstartx = menu->startx;
		mstarty = menu->starty;
*/

		if (menu->startx + menu->width > COLS) mstartx = COLS - menu->width;
		else mstartx = menu->startx;

		// set this correctly to fit on screen (fix)
		mstarty = menu->starty;
		mwidth = menu->width;
		mheight = menu->height;


		if (menu->window == NULL){
			menu->window = newwin(mheight, mwidth, mstarty, mstartx);
		}
		else mvwin(menu->window, mstarty, mstartx);
		if (menu->window == NULL) return;

		ypos = 0;
		current = menu->item;	
		curs_set(0);
	
		if (current != NULL && strlen(current->text) > mwidth - 2) strabslen = mwidth - 2;
		else strabslen = 255;

		wattrset(menu->window, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
		wattron(menu->window, A_REVERSE);
		box(menu->window,0,0);
		while (current != NULL){

			text[0] = 0;
			strncat(text, current->text, strabslen);
			for (i=strlen(text); i < (mwidth-2) && i < COLS; i++){
				text[i] = ' ';
			}
			text[i] = 0;	

			if ((current->option) & M_DIVIDER){
				wattrset(menu->window, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
				wattron(menu->window, A_REVERSE);
	                	mvwhline(menu->window, ypos+1, 1, 0, mwidth-2);
			}
			else {
				if (current == menu->selected){
					wattrset(menu->window, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
					wattron(menu->window, A_NORMAL);
					mvwaddstr(menu->window, ypos+1, 1, text);
				} 
				else if ((current->option) & M_SELECTABLE){
					wattrset(menu->window, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
					wattron(menu->window, A_REVERSE);
       		                 	mvwaddstr(menu->window, ypos+1, 1, text);
       		         	}
				else{
					wattrset(menu->window, COLOR_PAIR(MENU_COLOR_DIM_B*16+MENU_COLOR_DIM_F));
					wattron(menu->window, A_REVERSE);
					wattron(menu->window, A_DIM);
               		         	mvwaddstr(menu->window, ypos+1, 1, text);
					wattrset(menu->window, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
					wattron(menu->window, A_REVERSE);
                		}
			}

			ypos++;
			if (ypos >= LINES) break;
			current = current->next;
		}
		wrefresh(menu->window);
		if (menu->chosen != NULL){
			if ((menu->chosen)->child != NULL) print_menu ((menu->chosen)->child);
		}
	}
}

WINDOW *print_sub_menu(WINDOW *screen, menu *menu){
	WINDOW *menuwin;
	menuitem *current;
	int ypos, i, mheight, mwidth, mstarty, mstartx, strabslen;
	char text[256];

	if (menu->starty >= LINES || menu->startx >= COLS){
		print_all("Menu cannot be displayed, please resize the screen.\n");
		return(NULL);
	}

	if (menu->startx + menu->width > COLS) mwidth = COLS - menu->startx;
	else mwidth = menu->width;

	if (menu->starty + menu->height > LINES) mheight = LINES - menu->starty;
	else mheight = menu->height;

	// set this correctly to fit on screen (fix)
	mstartx = menu->startx;
	mstarty = menu->starty;

	menuwin = subwin(screen, mheight, mwidth, mstarty, mstartx);
	wattrset(menuwin, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
	wattron(menuwin, A_REVERSE);
	box(menuwin,0,0);

	ypos = 0;
	current = menu->item;	
	curs_set(0);
	
	if (strlen(current->text) > mwidth - 2) strabslen = mwidth - 2;
	else strabslen = 255;

	while (current != NULL){

		text[0] = 0;
		strncat(text, current->text, strabslen);
		for (i=strlen(text); i < (mwidth-2); i++){
			text[i] = ' ';
		}
		text[i] = 0;

		if ((current->option) & M_DIVIDER){
			wattrset(menuwin, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
			wattron(menuwin, A_REVERSE);
                	mvwhline(menuwin, ypos+1, 1, 0, mwidth-2);
		}
		else {
			if (current == menu->selected){
				wattrset(menuwin, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
				wattron(menuwin, A_NORMAL);
				mvwaddstr(menuwin, ypos+1, 1, text);
			} 
			else if ((current->option) & M_SELECTABLE){
				wattrset(menuwin, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
				wattron(menuwin, A_REVERSE);
                        	mvwaddstr(menuwin, ypos+1, 1, text);
                	}
			else{
				wattrset(menuwin, COLOR_PAIR(MENU_COLOR_DIM_B*16+MENU_COLOR_DIM_F));
				wattron(menuwin, A_REVERSE);
				wattron(menuwin, A_DIM);
                        	mvwaddstr(menuwin, ypos+1, 1, text);
				wattrset(menuwin, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
				wattron(menuwin, A_REVERSE);
                	}
		}

		ypos++;
		current = current->next;
        }
	return(menuwin);
}

void print_menubar(menuwin *menuline, menubar *menubar){
        int posx, i, j, k;              
                
        int menuentries;
        char *cmenutitle;
        menu *cmenu;
                
        curs_set(0);
                        
        // sprintf (buffer, "update %x:%x\n", menuline->update, menuline_update_status(menuline));
        // print_all(buffer);
                        
        // print out the menu selection on the menuline if refresh changes are required
        if (menuline_update_status(menuline) & U_ALL_REFRESH){
                                        
                // clear the menuline
                wattrset(menuline->menuline, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
                wattron(menuline->menuline, A_REVERSE);
                                        
                for (i=0; i<COLS; i++) mvwaddch(menuline->menuline, 0, i, ' ');
                                        
                menuentries = menubar->entries;
                posx = menubar->textstart;
                        
                // print all of the menu choices highlighting selectable key
                for (i=0; i < menuentries; i++){
                        cmenu = (menubar->menu)[i];
                        cmenutitle = cmenu->name;
                        wattrset(menuline->menuline, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
                        wattron(menuline->menuline, A_REVERSE);
                        k = 0;
                        for (j = 0; j < strlen(cmenutitle); j++){
                                if (toupper(cmenutitle[j]) == toupper ((unsigned char)(cmenu->key)) && k == 0){
                                        wattrset(menuline->menuline, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
                                        wattron(menuline->menuline, A_BOLD);
                                        wattron(menuline->menuline, A_STANDOUT);
                                        wattron(menuline->menuline, A_REVERSE);
                                        mvwaddch(menuline->menuline, 0, posx, cmenutitle[j]);
                                        wattrset(menuline->menuline, COLOR_PAIR(MENU_COLOR_B*16+MENU_COLOR_F));
                                        wattron(menuline->menuline, A_REVERSE);
                                        k = 1;
                                }
                                else {
                                        mvwaddch(menuline->menuline, 0, posx, cmenutitle[j]);
                                }
                                posx++;
                        }
                        posx = posx + menubar->textspace;
                }                
                wrefresh(menuline->menuline);
	        unset_menuline_update_status(menuline, U_ALL_REFRESH);
		touchwin(menuline->menuline);		
	}
	if (menubar->current != -1){
		print_menu((menubar->menu)[menubar->current]);
	}

}

int process_menubar_events(menubar *line, int event){
        int i;
	int returnevent;
	menu *menu;

	menu = (line->menu)[line->current];

	if (line == NULL) return (event);
	
	// if no menus are showing test for keys that may open a menu
	// if true set the current menu and remove event
	else if (line->current == -1){
                for (i = 0; i < (line->entries); i++){
                        if (((line->menu)[i])->key == (event + 0x40)){
                                line->current = i;
				line->update = U_ALL_REDRAW;
                                return(E_NONE);
                        }
                }
		// otherwise return the event back to the previous handler
		return(event);
	}

	// handle menu events such as menuline selection and menuline wrapping                 
	// since left and right keys are used to select a menu, intercept these keys
	// and don't pass them to menu event handlers

        else if (event == KEY_LEFT){
		line->current--;
		line->update = U_BG_REDRAW;
		if (line->current < 0){
			line->current = line->entries - 1;
		}
		return (E_NONE);
	}
        else if (event == KEY_RIGHT){
		line->current++;
		line->update = U_BG_REDRAW;
		if (line->current > line->entries - 1){
			line->current = 0;
		}
		return (E_NONE);
	}

	// however pass everything else to the menu handler
	// esc is an exception which closes some menu somewhere down
	// the line and requires a backround redraw

	else { 
                if (event == 0x01B || event == KEY_CANCEL) line->update = U_BG_REDRAW;
		returnevent = process_menu_events(menu, event);
                if (returnevent == 0x01B || returnevent == KEY_CANCEL){
			line->current = -1;
			return (E_NONE);	
		}
	}
	return(E_NONE);
}

int process_menu_events(menu *menu, int event){
	int returnevent;
	
	// handle menu events such as menu selection and menu wrapping
        if (menu == NULL) return (event);
                         
	// if key is cancel or escape remove chosen at the second last menu
	if (event == 0x01B || event == KEY_CANCEL){
		if (menu->chosen == NULL) return (event);
		else if ((menu->chosen)->child != NULL){
			returnevent = process_menu_events((menu->chosen)->child, event);
			if (returnevent == 0x01B || returnevent == KEY_CANCEL){ 
				menu->chosen = NULL;
				menu->selected = menu->item;
				return (E_NONE);
			}
		}
		else return (returnevent);
	} 
                                
	else if (event == 0x0A || event == 0x0D || event == KEY_ENTER){
		if (menu->chosen == NULL){
			menu->chosen = menu->selected;
		}
		else if ((menu->chosen)->child != NULL) {
			returnevent = process_menu_events((menu->chosen)->child, event);
			return (returnevent);	
		}
	}

	// if key is up or down and this menu has a child pass those to the children
        else if (event == KEY_DOWN){
                if (menu->chosen != NULL){
			if ((menu->chosen)->child != NULL){
				process_menu_events((menu->chosen)->child, event);
				return (E_NONE);
			}
		}
		select_next_item(menu);
		return (E_NONE);
       	}
        else if (event == KEY_UP){
                if (menu->chosen != NULL){
			if ((menu->chosen)->child != NULL){
				process_menu_events((menu->chosen)->child, event);
				return (E_NONE);
			}
		}
                select_prev_item(menu);
		return (E_NONE);
        }
	return (event);
}

int selected_menubar_item_id(menubar *line){
	int id = 0;

	if (line == NULL || line->current < 0 || line->current > line->entries) return(0);
	if (line->menu == NULL) return(0);
	id = selected_menu_item_id((line->menu)[line->current]);
        
	// if something was selected close the menu since 
	// it isn't needed anymore 
	if (id != 0){
		line->current = -1;
		line->update = U_BG_REDRAW;
	}
	return(id);  
}

void close_menubar(menubar *menubar){
	menubar->current = -1;
	menubar->update = U_BG_REDRAW;
}

inline int menubar_update_status(menubar *menubar){
	return(menubar->update);
}

inline void set_menubar_update_status(menubar *menubar, int update){
	menubar->update = ((menubar->update) | (update));
}

void unset_menubar_update_status(menubar *menubar, int update){
	menubar->update = ((menubar->update) & (0xffff ^ update));
}


