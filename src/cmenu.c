
/*****************************************************************************/
/*                                                                           */
/*  Copyright (C) 2006 Adrian Gonera                                         */
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

#include "log.h"
#include "ncolor.h"
#include "common.h"
#include "screen.h"
#include "events.h"
#include "cevents.h"
#include "cmenu.h"

menubar *servermenus;
menubar *channelmenus;
menubar *chatmenus;
menubar *dccchatmenus;
menubar *transfermenus;
menubar *listmenus;
menubar *helpmenus;
menubar *currentmenusline;

menu *UserMenu;
menu *UserListMenu;
menu *CtcpMenu;
menu *DCCMenu;
menu *ControlMenu;
menu *WindowMenu;

menubar *init_menubar(int textstart, int textspacing, int id){
	menubar *menubarptr;

	menubarptr = malloc(sizeof(menubar));
	
	if (menubarptr != NULL){
		menubarptr->entries = 0;
		menubarptr->textstart = textstart;
		menubarptr->textspace = textspacing;
		menubarptr->changed = 1;
		menubarptr->selected = NULL;
		menubarptr->menuhead = NULL;
		menubarptr->menutail = NULL;
	}
	else {
		plog("Couldn't allocate menu memory in init_menubar\n");
		exit (-1);
	}	
	return(menubarptr);
}

menu *init_menu(int startx, int starty, char *name, int key, int id){
	menu *menuptr;

	menuptr = malloc(sizeof(menu));
	if (menuptr != NULL){
		strncpy(menuptr->name, name, MENUNAMELEN);
		menuptr->startx = startx;
		menuptr->starty = starty;
		menuptr->itemhead = NULL;
		menuptr->itemtail = NULL;
		menuptr->selected = NULL;
		menuptr->chosen = NULL;
		menuptr->width = 2;
		menuptr->height = 2;
		menuptr->entries = 0;
		menuptr->id = id;
		menuptr->key = key;
		menuptr->window = NULL;
	}
	else {
		print_all("Couldn't allocate menu memory\n");
		exit (-1);
	}	
	return(menuptr);
}


int delete_menu(menu *menuptr){
	menuitem *current;

	if (menuptr != NULL){
		delete_menu_items(menuptr);
		free(menuptr);
		return(1);
	}

	/* fix bar linked list */ 
	return(0);
}

int delete_menu_items(menu *menuptr){
	menuitem *current, *next;

	if (menuptr != NULL){
		current = menuptr->itemhead;
		while (current != NULL){
			// free all space dedicated to items
			next = current->next;
			free(current);
			menuptr->entries--;
			menuptr->height--;			
			current = next;
		}
		menuptr->itemhead = NULL;
		menuptr->itemtail = NULL;
		menuptr->selected = NULL;
		menuptr->chosen = NULL;
		
		/* remove the menu window */
		delwin(menuptr->window);
		menuptr->window = NULL;		
		return(1);
	}
	return(0);
}

int add_menubar_menu(menubar *menubarptr, menu *menuptr, int option){
	menu *current, *last, *new;

	if (menubarptr != NULL && menuptr != NULL){
		menuptr->bar = menubarptr;

		if (option & M_ADD_FIRST){
			menuptr->prev = NULL;
			menuptr->next = menubarptr->menuhead;
			//menubarptr->selected = menuptr;		

			if (menubarptr->menuhead != NULL) (menubarptr->menuhead)->prev = menuptr;
			menubarptr->menuhead = menuptr;
			if (menubarptr->menutail == NULL) menubarptr->menutail = menuptr;
		}
		else{
			menuptr->next = NULL;
			menuptr->prev = menubarptr->menutail;

			if (menubarptr->menutail != NULL) (menubarptr->menutail)->next = menuptr;
			menubarptr->menutail = menuptr;
			if (menubarptr->menuhead == NULL){
				//menubarptr->selected = menuptr;		
				menubarptr->menuhead = menuptr;
			}
		}

		menubarptr->entries++;
		menubarptr->changed = 1;
	}
	return(1);
}

menu *get_menubar_menu(menubar *menubarptr, int id){
	menu *current;

	if (menubarptr != NULL){
		current = menubarptr->menuhead;
		while (current != NULL){
			if (current->id == id) return(current);
			current = current->next;
		}
	}
	return(NULL);
}

void set_menu_position(menu *menuptr, int startx, int starty){
	if (menuptr != NULL){
		menuptr->startx = startx;
		menuptr->starty = starty;
		if (menuptr->window != NULL) mvwin(menuptr->window, starty, startx);
	}
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
	menuitem *next;

	if (menu != NULL){
		if (menu->selected != NULL){
			next = (menu->selected)->next;
			while (next != NULL){
				if ((next->option) & M_SELECTABLE){
					menu->selected = next;
					break;
				}
				next = next->next;
			}
			// if reached the end, no selectable items found, start from 1st
			// if no selectables exist, exit after entire menu is checked
			if (next == NULL){
				next = menu->itemhead;
				while (next != NULL){
					if ((next->option) & M_SELECTABLE){
						menu->selected = next;
						break;
					}
					next = next->next;
				}
			}
		}
		else menu->selected = menu->itemhead;
	}
	return(1);
}

int select_prev_item(menu *menu){
	menuitem *prev;

	if (menu != NULL){
		if (menu->selected != NULL){
			prev = (menu->selected)->prev;
			while (prev != NULL){
				if ((prev->option) & M_SELECTABLE){
					menu->selected = prev;
					break;
				}
				prev = prev->prev;
			}

			// if reached the beginning, no selectable items found
			// start at last and check the entire menu backwards
			if (prev == NULL){
				prev = menu->itemtail;
				while (prev != NULL){
					if ((prev->option) & M_SELECTABLE){
                                                menu->selected = prev;
                                                break;
					}
                                        prev = prev->prev;
				}
			}		
		}
		else menu->selected = menu->itemhead;
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

		if (option & M_ADD_FIRST){
			new->prev = NULL;
			new->next = menuptr->itemhead;
			menuptr->selected = new;		

			if (menuptr->itemhead != NULL) (menuptr->itemhead)->prev = new;
			menuptr->itemhead = new;
			if (menuptr->itemtail == NULL) menuptr->itemtail = new;
		}
		else{
			new->next = NULL;
			new->prev = menuptr->itemtail;

			if (menuptr->itemtail != NULL) (menuptr->itemtail)->next = new;
			menuptr->itemtail = new;
			if (menuptr->itemhead == NULL){
				menuptr->selected = new;		
				menuptr->itemhead = new;
			}
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

	menu *newmenu;

	/** SERVER MENU ***********************************************************************************************/

	servermenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "Server", 'S', MENU_NORMAL);
	add_menu_item(newmenu, " Connect New ...              ", "", 0, M_SELECTABLE, E_CONNECT_NEW, NULL);
	add_menu_item(newmenu, " Connect Favorite ...         ", "", 0, M_SELECTABLE, E_CONNECT_FAVORITE, NULL);
	add_menu_item(newmenu, "                              ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Add to Favorites             ", "", 0, M_SELECTABLE, E_SERVER_ADD_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Favorites ...           ", "", 0, M_SELECTABLE, E_SERVER_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, "                              ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Disconnect                   ", "", 0, M_SELECTABLE, E_DISCONNECT, NULL);
	add_menu_item(newmenu, " Close                 Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit                  Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Channel", 'N', MENU_NORMAL);
	add_menu_item(newmenu, " Join New ...        ", "", 0, M_SELECTABLE, E_JOIN_NEW, NULL);
	add_menu_item(newmenu, "                     ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Join Favorite ...   ", "", 0, M_SELECTABLE, E_JOIN_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Favorites ...  ", "", 0, M_SELECTABLE, E_CHANNEL_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, "                     ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Channel List ...    ", "", 0, M_SELECTABLE, E_CHANNEL_LIST, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "User", 'U', MENU_NORMAL);
	add_menu_item(newmenu, " Chat New ...           ", "", 0, M_SELECTABLE, E_CHAT_NEW, NULL);
	add_menu_item(newmenu, " Chat Favorite ...      ", "", 0, M_SELECTABLE, E_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " DCC Chat New ...       ", "", 0, M_SELECTABLE, E_DCC_CHAT_NEW, NULL);
	add_menu_item(newmenu, " DCC Chat Favorite ...  ", "", 0, M_SELECTABLE, E_DCC_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, " DCC Send New ...       ", "", 0, M_SELECTABLE, E_DCC_SEND_NEW, NULL);
	add_menu_item(newmenu, " DCC Send Favorite ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_FAVORITE, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Edit Favorites ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Ignored ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(servermenus, newmenu, 0);	


	/** CHANNEL MENU ***********************************************************************************************/


	channelmenus = init_menubar(3, 4, 0);
	
	newmenu = init_menu(M_AUTO, M_AUTO, "Channel", 'N', MENU_NORMAL);
	add_menu_item(newmenu, " Join New ...             ", "", 0, M_SELECTABLE, E_JOIN_NEW, NULL);
	add_menu_item(newmenu, " Join Favorite ...        ", "", 0, M_SELECTABLE, E_JOIN_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Add to Favorites         ", "", 0, M_SELECTABLE, E_CHANNEL_ADD_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_CHANNEL_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Leave                    ", "", 0, M_SELECTABLE, E_CHANNEL_PART, NULL);
	add_menu_item(newmenu, " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(channelmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "User", 'S', MENU_NORMAL);
	add_menu_item(newmenu, " Select ...      Ctlr-u ", "", 0, M_SELECTABLE, E_USER_SELECT, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Chat New ...           ", "", 0, M_SELECTABLE, E_CHAT_NEW, NULL);
	add_menu_item(newmenu, " Chat Favorite ...      ", "", 0, M_SELECTABLE, E_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " DCC Chat New ...       ", "", 0, M_SELECTABLE, E_DCC_CHAT_NEW, NULL);
	add_menu_item(newmenu, " DCC Chat Favorite ...  ", "", 0, M_SELECTABLE, E_DCC_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, " DCC Send New ...       ", "", 0, M_SELECTABLE, E_DCC_SEND_NEW, NULL);
	add_menu_item(newmenu, " DCC Send Favorite ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_FAVORITE, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Edit Favorites ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Ignored ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menubar_menu(channelmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(channelmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(channelmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(channelmenus, newmenu, 0);	

	CtcpMenu = init_menu(1, 1, "Ctcp", 0, MENU_NORMAL);
	add_menu_item(CtcpMenu, " Ping         ", "", 0, M_SELECTABLE, E_CTCP_PING, NULL);
	add_menu_item(CtcpMenu, " Client Info  ", "", 0, M_SELECTABLE, E_CTCP_CLIENTINFO, NULL);
	add_menu_item(CtcpMenu, " User Info    ", "", 0, M_SELECTABLE, E_CTCP_USERINFO, NULL);
	add_menu_item(CtcpMenu, " Version      ", "", 0, M_SELECTABLE, E_CTCP_VERSION, NULL);
	add_menu_item(CtcpMenu, " Finger       ", "", 0, M_SELECTABLE, E_CTCP_FINGER, NULL);
	add_menu_item(CtcpMenu, " Source       ", "", 0, M_SELECTABLE, E_CTCP_SOURCE, NULL);
	add_menu_item(CtcpMenu, " Time         ", "", 0, M_SELECTABLE, E_CTCP_TIME, NULL);

	DCCMenu = init_menu(1, 1 ,"DCC", 0, MENU_NORMAL);
	add_menu_item(DCCMenu, " Chat     ", "", 0, M_SELECTABLE, E_DCC_CHAT, NULL);
	add_menu_item(DCCMenu, " Send ... ", "", 0, M_SELECTABLE, E_DCC_SEND, NULL);

	ControlMenu = init_menu(1, 1 ,"Control", 0, MENU_NORMAL);
	add_menu_item(ControlMenu, " Op (+o)       ", "", 0, M_SELECTABLE, E_CONTROL_OP, NULL);
	add_menu_item(ControlMenu, " DeOP (-o)     ", "", 0, M_SELECTABLE, E_CONTROL_DEOP, NULL);
	add_menu_item(ControlMenu, " Voice (+v)    ", "", 0, M_SELECTABLE, E_CONTROL_VOICE, NULL);
	add_menu_item(ControlMenu, " DeVoice (-v)  ", "", 0, M_SELECTABLE, E_CONTROL_DEVOICE, NULL);
	add_menu_item(ControlMenu, "               ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(ControlMenu, " Kick          ", "", 0, M_SELECTABLE, E_CONTROL_KICK, NULL);
	add_menu_item(ControlMenu, " Ban           ", "", 0, M_SELECTABLE, E_CONTROL_BAN, NULL);
	add_menu_item(ControlMenu, " Kick and Ban  ", "", 0, M_SELECTABLE, E_CONTROL_KICKBAN, NULL);

	UserListMenu = init_menu(1, 1 ,"User List", 0, MENU_NORMAL);
	add_menu_item(UserListMenu, " Add to Favorites      ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(UserListMenu, " Remove from Favorites ", "", 0, M_SELECTABLE, E_USER_REMOVE_FAVORITE, NULL);
	add_menu_item(UserListMenu, " Ignore                ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(UserListMenu, " Unignore              ", "", 0, M_SELECTABLE, E_USER_REMOVE_IGNORED, NULL);

	UserMenu = init_menu(1, 1, "User", 0, MENU_NORMAL);
	add_menu_item(UserMenu, " Copy and Paste  ", "", 0, M_SELECTABLE, E_USER_PASTE, NULL);
	add_menu_item(UserMenu, "                 ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(UserMenu, " Control ...     ", "", 0, M_SELECTABLE, -1, ControlMenu);
	add_menu_item(UserMenu, " Ctcp ...        ", "", 0, M_SELECTABLE, -1, CtcpMenu);
	add_menu_item(UserMenu, " DCC ...         ", "", 0, M_SELECTABLE, -1, DCCMenu);
	add_menu_item(UserMenu, " User List ...   ", "", 0, M_SELECTABLE, -1, UserListMenu);
	add_menu_item(UserMenu, "                 ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(UserMenu, " Chat            ", "", 0, M_SELECTABLE, E_QUERY, NULL);
	add_menu_item(UserMenu, " Whois           ", "", 0, M_SELECTABLE, E_WHOIS, NULL);


	/** CHAT MENU ***********************************************************************************************/

	chatmenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "Chat", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " Chat New ...             ", "", 0, M_SELECTABLE, E_CHAT_NEW, NULL);
	add_menu_item(newmenu, " Chat Favorite ...        ", "", 0, M_SELECTABLE, E_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Ignore                   ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(newmenu, " Edit Ignore List ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menu_item(newmenu, " Add to Favorites         ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(chatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'N', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(chatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(chatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(chatmenus, newmenu, 0);	


	/** DCCCHAT MENU ***********************************************************************************************/


	dccchatmenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "DCC Chat", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " DCC Chat New ...         ", "", 0, M_SELECTABLE, E_DCC_CHAT_NEW, NULL);
	add_menu_item(newmenu, " DCC Chat Favorite ...    ", "", 0, M_SELECTABLE, E_DCC_CHAT_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Ignore                   ", "", 0, M_SELECTABLE, E_USER_ADD_IGNORE, NULL);
	add_menu_item(newmenu, " Edit Ignore List ...     ", "", 0, M_SELECTABLE, E_USER_EDIT_IGNORED, NULL);
	add_menu_item(newmenu, " Add to Favorites         ", "", 0, M_SELECTABLE, E_USER_ADD_FAVORITE, NULL);
	add_menu_item(newmenu, " Edit Favorites ...       ", "", 0, M_SELECTABLE, E_USER_EDIT_FAVORITE, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Disconnect               ", "", 0, M_SELECTABLE, E_DISCONNECT, NULL);
	add_menu_item(newmenu, " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(dccchatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'N', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(dccchatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(dccchatmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help",'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(dccchatmenus, newmenu, 0);	


	/** TRANSFER MENU ***********************************************************************************************/

	transfermenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "Transfer", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " Abort Selected           ", "", 0, M_SELECTABLE, E_TRANSFER_STOP, NULL);
	add_menu_item(newmenu, " Info                     ", "", 0, M_SELECTABLE, E_TRANSFER_INFO, NULL);
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(transfermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'O', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(transfermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(transfermenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(transfermenus, newmenu, 0);	


	/** LIST MENU ***************************************************************************************************/

	listmenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "List", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " View ...                 ", "", 0, M_SELECTABLE, E_LIST_VIEW, NULL);
	add_menu_item(newmenu, " Add to Favorites         ", "", 0, M_SELECTABLE, E_CHANNEL_ADD_FAVORITE, NULL);
	add_menu_item(newmenu, " Join                     ", "", 0, M_SELECTABLE, E_CHANNEL_JOIN, NULL);	
	add_menu_item(newmenu, "                          ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Close             Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit              Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(listmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'O', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(listmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(listmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys      ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands  ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands     ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                  ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " About            ", "", 0, M_SELECTABLE, E_HELP_ABOUT, NULL);
	add_menubar_menu(listmenus, newmenu, 0);	


	/** HELP MENU **************************************************************************************************/

	helpmenus = init_menubar(3, 4, 0);

	newmenu = init_menu(M_AUTO, M_AUTO, "Help", 'P', MENU_NORMAL);
	add_menu_item(newmenu, " Client Keys            ", "", 0, M_SELECTABLE, E_HELP_KEYS, NULL);
	add_menu_item(newmenu, " Client Commands        ", "", 0, M_SELECTABLE, E_HELP_COMMANDS, NULL);
	add_menu_item(newmenu, " IRC Commands           ", "", 0, M_SELECTABLE, E_HELP_IRC_COMMANDS, NULL);
	add_menu_item(newmenu, "                        ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Close           Ctrl-k ", "", 0, M_SELECTABLE, E_CLOSE, NULL);
	add_menu_item(newmenu, " Exit            Ctrl-x ", "", 0, M_SELECTABLE, E_EXIT, NULL);
	add_menubar_menu(helpmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Options", 'T', MENU_NORMAL);
	add_menu_item(newmenu, " Identity ...          ", "", 0, M_SELECTABLE, E_IDENTITY, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Client Options ...    ", "", 0, M_SELECTABLE, E_OPTIONS, NULL);
	add_menu_item(newmenu, " CTCP Options ...      ", "", 0, M_SELECTABLE, E_CTCP_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Options ...       ", "", 0, M_SELECTABLE, E_DCC_OPTIONS, NULL);
	add_menu_item(newmenu, " DCC Send Options ...  ", "", 0, M_SELECTABLE, E_DCC_SEND_OPTIONS, NULL);
	add_menu_item(newmenu, " Network Settings ...  ", "", 0, M_SELECTABLE, E_NETWORK_OPTIONS, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Colors ...            ", "", 0, M_SELECTABLE, E_COLOR_OPTIONS, NULL);
	add_menu_item(newmenu, " Terminal Info ...     ", "", 0, M_SELECTABLE, E_TERM_INFO, NULL);
	add_menu_item(newmenu, "                       ", "", 0, M_DIVIDER, -1, NULL);
	add_menu_item(newmenu, " Save Configuration    ", "", 0, M_NONE, E_SAVE_OPTIONS, NULL);
	add_menubar_menu(helpmenus, newmenu, 0);	

	newmenu = init_menu(M_AUTO, M_AUTO, "Window", 'W', MENU_WINDOWLIST);
	add_menu_item(newmenu, " Previous Window          ", "", 0, M_SELECTABLE, E_NEXT_WINDOW, NULL);
	add_menu_item(newmenu, " Next Window              ", "", 0, M_SELECTABLE, E_PREV_WINDOW, NULL);
	add_menubar_menu(helpmenus, newmenu, 0);	
}	

void print_menu(menu *menuptr){
	menu *cmenu;
	menuitem *current;
	int ypos, xpos, i, mheight, mwidth, mstarty, mstartx, strabslen;
	char text[256];

	if (menuptr == NULL) return;
	if (menuptr->starty >= LINES || menuptr->startx >= COLS){
		print_all("Menu cannot be displayed properly, please resize the screen.\n");
	}
	else {

		/* X position... find it if set to auto */
		if (menuptr->startx == M_AUTO && menuptr->bar != NULL){
	                mstartx = menuptr->bar->textstart - 2;
			cmenu = menuptr->bar->menuhead;
			while(cmenu != NULL){
				if (cmenu == menuptr) break;
				mstartx = mstartx + menuptr->bar->textspace + strlen(cmenu->name);
				cmenu = cmenu->next;
			}
		}
		else mstartx = menuptr->startx;
		if (mstartx + menuptr->width > COLS) mstartx = COLS - menuptr->width;

		/* Y position... set this correctly to fit on screen (fix) */
		if (menuptr->starty == M_AUTO && menuptr->bar != NULL){
			mstarty = 1;
		}
		else mstarty = menuptr->starty;


		mwidth = menuptr->width;
		mheight = menuptr->height;

		if (menuptr->window == NULL){
			menuptr->window = newwin(mheight, mwidth, mstarty, mstartx);
		}
		else mvwin(menuptr->window, mstarty, mstartx);
		if (menuptr->window == NULL) return;

		ypos = 0;
		current = menuptr->itemhead;	
		curs_set(0);
	
		if (current != NULL && strlen(current->text) > mwidth - 2) strabslen = mwidth - 2;
		else strabslen = 255;

		wattrset(menuptr->window, MENU_COLOR);
		wattron(menuptr->window, A_REVERSE);
		box(menuptr->window,0,0);
		while (current != NULL){

			text[0] = 0;
			strncat(text, current->text, strabslen);
			for (i=strlen(text); i < (mwidth-2) && i < COLS; i++){
				text[i] = ' ';
			}
			text[i] = 0;	

			if ((current->option) & M_DIVIDER){
				wattrset(menuptr->window, MENU_COLOR);
				wattron(menuptr->window, A_REVERSE);
	                	mvwhline(menuptr->window, ypos+1, 1, 0, mwidth-2);
			}
			else {
				if (current == menuptr->selected){
					wattrset(menuptr->window, MENU_COLOR);
					wattron(menuptr->window, A_NORMAL);
					mvwaddstr(menuptr->window, ypos+1, 1, text);
				} 
				else if ((current->option) & M_SELECTABLE){
					wattrset(menuptr->window, MENU_COLOR);
					wattron(menuptr->window, A_REVERSE);
       		                 	mvwaddstr(menuptr->window, ypos+1, 1, text);
       		         	}
				else{
					wattrset(menuptr->window, MENU_COLOR);
					wattron(menuptr->window, A_REVERSE);
					wattron(menuptr->window, A_DIM);
               		         	mvwaddstr(menuptr->window, ypos+1, 1, text);
					wattrset(menuptr->window, MENU_COLOR);
					wattron(menuptr->window, A_REVERSE);
                		}
			}

			ypos++;
			if (ypos >= LINES) break;
			current = current->next;
		}
		wrefresh(menuptr->window);
		if (menuptr->chosen != NULL){
			if ((menuptr->chosen)->child != NULL) print_menu ((menuptr->chosen)->child);
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
	wattrset(menuwin, MENU_COLOR);
	wattron(menuwin, A_REVERSE);
	box(menuwin,0,0);

	ypos = 0;
	current = menu->itemhead;	
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
			wattrset(menuwin, MENU_COLOR);
			wattron(menuwin, A_REVERSE);
                	mvwhline(menuwin, ypos+1, 1, 0, mwidth-2);
		}
		else {
			if (current == menu->selected){
				wattrset(menuwin, MENU_COLOR);
				wattron(menuwin, A_NORMAL);
				mvwaddstr(menuwin, ypos+1, 1, text);
			} 
			else if ((current->option) & M_SELECTABLE){
				wattrset(menuwin, MENU_COLOR);
				wattron(menuwin, A_REVERSE);
                        	mvwaddstr(menuwin, ypos+1, 1, text);
                	}
			else{
				wattrset(menuwin, MENU_COLOR);
				wattron(menuwin, A_REVERSE);
				wattron(menuwin, A_DIM);
                        	mvwaddstr(menuwin, ypos+1, 1, text);
				wattrset(menuwin, MENU_COLOR);
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
                
	if (menuline == NULL || menubar == NULL) return;
        curs_set(0);
                        
        // sprintf (buffer, "update %x:%x\n", menuline->update, menuline_update_status(menuline));
        // print_all(buffer);
                        
        // print out the menu selection on the menuline if refresh changes are required
        if (menuline_update_status(menuline) & U_ALL_REFRESH){
                                        
                // clear the menuline
                wattrset(menuline->menuline, MENU_COLOR);
                wattron(menuline->menuline, A_REVERSE);
                                        
                for (i = 0; i < COLS; i++) mvwaddch(menuline->menuline, 0, i, ' ');
                                        
                // menuentries = menubar->entries;
                posx = menubar->textstart;
                        
                // print all of the menu choices highlighting selectable key
		
		cmenu = menubar->menuhead;
		while(cmenu != NULL){
			cmenutitle = cmenu->name;
			wattrset(menuline->menuline, MENU_COLOR);
			wattron(menuline->menuline, A_REVERSE);
			k = 0;

			for (j = 0; j < strlen(cmenutitle); j++){
				if (toupper(cmenutitle[j]) == toupper ((unsigned char)(cmenu->key)) && k == 0){
					wattrset(menuline->menuline, MENU_COLOR);
					wattron(menuline->menuline, A_BOLD);
					wattron(menuline->menuline, A_STANDOUT);
					wattron(menuline->menuline, A_REVERSE);
					mvwaddch(menuline->menuline, 0, posx, cmenutitle[j]);
					wattrset(menuline->menuline, MENU_COLOR);
					wattron(menuline->menuline, A_REVERSE);
					k = 1;
                                }
                                else {
                                        mvwaddch(menuline->menuline, 0, posx, cmenutitle[j]);
                                }
				posx++;
			}
			posx = posx + menubar->textspace;
			cmenu = cmenu->next;
		}                
		wrefresh(menuline->menuline);
		unset_menuline_update_status(menuline, U_ALL_REFRESH);
		touchwin(menuline->menuline);		
	}
	if (menubar->selected != NULL){
		print_menu(menubar->selected);
	}

}

int process_menubar_events(menubar *line, int event){
        int i;
	int returnevent;
	menu *menu;

	if (line == NULL) return (event);

	// if no menus are showing test for keys that may open a menu
	// if true set the current menu and remove event
	else if (line->selected == NULL){
		menu = line->menuhead;
		while(menu != NULL){
                        if (menu->key == (event + 0x40)){
                                line->selected = menu;
				line->update = U_ALL_REDRAW;
                                return(E_NONE);
                        }
			menu = menu->next;
                }
		// otherwise return the event back to the previous handler
		return(event);
	}

	// handle menu events such as menuline selection and menuline wrapping                 
	// since left and right keys are used to select a menu, intercept these keys
	// and don't pass them to menu event handlers

        else if (event == KEY_LEFT){
		line->selected = (line->selected)->prev;
		line->update = U_BG_REDRAW;
		if (line->selected == NULL){
			line->selected = line->menutail;
		}
		return (E_NONE);
	}
        else if (event == KEY_RIGHT){
		line->selected = (line->selected)->next;
		line->update = U_BG_REDRAW;
		if (line->selected == NULL){
			line->selected = line->menuhead;
		}
		return (E_NONE);
	}

	// however pass everything else to the menu handler
	// esc is an exception which closes some menu somewhere down
	// the line and requires a backround redraw

	else {
                if (event == 0x01B || event == KEY_CANCEL) line->update = U_BG_REDRAW;
		returnevent = process_menu_events(line->selected, event);
                if (returnevent == 0x01B || returnevent == KEY_CANCEL){
			line->selected = NULL;
			return (E_NONE);	
		}
	}
	return(E_NONE);
}

int process_menu_events(menu *menu, int event){
	int returnevent = 0;
	
	// handle menu events such as menu selection and menu wrapping
        if (menu == NULL) return (event);
                         
	// if key is cancel or escape remove chosen at the second last menu
	if (event == 0x01B || event == KEY_CANCEL){
		if (menu->chosen == NULL) return (event);
		else if ((menu->chosen)->child != NULL){
			returnevent = process_menu_events((menu->chosen)->child, event);
			if (returnevent == 0x01B || returnevent == KEY_CANCEL){ 
				menu->chosen = NULL;
				menu->selected = menu->itemhead;
				return (E_NONE);
			}
		}
		return (event);
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

	if (line == NULL || line->selected == NULL) return(0);
	if (line->menuhead == NULL) return(0);
	id = selected_menu_item_id(line->selected);
        
	// if something was selected close the menu since 
	// it isn't needed anymore 
	if (id != 0){
		line->selected = NULL;
		line->update = U_BG_REDRAW;
	}
	return(id);  
}

void close_menubar(menubar *menubar){
	if (menubar != NULL){
		menubar->selected = NULL;
		menubar->update = U_BG_REDRAW;
	}
}

int menubar_update_status(menubar *menubar){
	if (menubar != NULL){
		return(menubar->update);
	}
	return(0);
}

void set_menubar_update_status(menubar *menubar, int update){
	if (menubar != NULL){
		menubar->update = ((menubar->update) | (update));
	}
}

void unset_menubar_update_status(menubar *menubar, int update){
	if (menubar != NULL){
		menubar->update = ((menubar->update) & (0xffff ^ update));
	}
}


