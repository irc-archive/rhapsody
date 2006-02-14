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
#include "misc.h"

int screenupdated;

void progress_bar(WINDOW *win, int posy, int posx, int size, int percent){
	int to_fill; 
	int i;

	to_fill = (size * percent) / 100;
	wattrset(win, COLOR_PAIR(PROGRESS_COLOR_B));
	wattron(win, A_REVERSE);
	wattron(win, A_BOLD);
	for (i=0; i<size; i++){
		if (i == to_fill+1){
			if (has_colors()){
				wattrset(win, COLOR_PAIR(PROGRESS_COLOR_F));
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
	if (current != NULL) return (current->info);
	return (NULL);
}

screen *server_screen_by_name(char *servername){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type==SERVER){
			if (strcmp(((server *)(current->info))->server, servername)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}

int get_server_count(){
	screen *current;
	int count;

	count = 0;
	current=screenlist;
	while(current!=NULL){
		if (current->type == SERVER) count++;			
		current=current->next;
	}
	return(count);
}

/*
int close_all_server_screens(server *S){
	screen *current, *remserver;

	current = screenlist;
	while(current != NULL){
		if (current->info == S) remserver = current;

		if (current->type == CHANNEL){
			if (((channel *)(current->info))->server == S) remove_screen(current);
		}
		else if (current->type == CHAT){
			if (((chat *)(current->info))->server == S) remove_screen(current);
		}
		else if (current->type == LIST){
			if (((list *)(current->info))->server == S) remove_screen(current);
		}
		current=current->next;
	}
	currentscreen = select_prev_screen(remserver);	
	remove_screen(remserver);
	return(1);
}
*/

int close_screen_and_children(screen *scr){
	screen *current;

	current = screenlist;
	while(current != NULL){
		if (current->parent == scr) remove_screen(current);
		current=current->next;
	}
	currentscreen = select_prev_screen(scr);	
	remove_screen(scr);
	return(1);
}

int get_child_count(screen *scr){
	screen *current;
	int count;

	count = 0;
	current = screenlist;
	while(current != NULL){
		if (current->parent == scr) count++;
		current=current->next;
	}
	return(count);
}

/*
int get_server_screen_count(server *S){
	screen *current;
	int count;
	
	count = 0;
	current=screenlist;
	while(current != NULL){
		if (current->type == CHANNEL){
			if (((channel *)(current->info))->server == S) ;
		}
		else if (current->type == CHAT){
			if (((chat *)(current->info))->server == S) count++;
		}
		else if (current->type == LIST){
			if (((list *)(current->info))->server == S) count++;
		}
		current=current->next;
	}
	return(count);
}

*/

channel *channel_by_name(char *channelname, server *S){
	screen *current;

	current = channel_screen_by_name(channelname, S);
	if (current != NULL) return (current->info);
	return (NULL);
}

screen *channel_screen_by_name(char *channelname, server *S){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == CHANNEL){
			if (strcasecmp(((channel *)(current->info))->channel, channelname) == 0){
				if (((channel *)(current->info))->server == S){
					break;
				}
			}
		}
		current=current->next;
	}
	return(current);
}

chat *chat_by_name(char *chatname, server *S){
	screen *current;

	current = chat_screen_by_name(chatname, S);
	if (current != NULL) return (current->info);
	return (NULL);
}

screen *chat_screen_by_name(char *chatname, server *S){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == CHAT){
			if (strcasecmp(((chat *)(current->info))->nick, chatname) == 0){
				if (((chat *)(current->info))->server == S){
					break;
				}
			}
		}
		current=current->next;
	}
	return(current);
}

dcc_chat *dcc_chat_by_name(char *chatname, server *S){
	screen *current;

	current = dcc_chat_screen_by_name(chatname, S);
	if (current != NULL) return (current->info);
	return (NULL);
}

screen *dcc_chat_screen_by_name(char *chatname, server *S){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == DCCCHAT){
			if (strcasecmp(((dcc_chat *)(current->info))->nick, chatname)==0){
				if (((dcc_chat *)(current->info))->server == S){
					break;
				}
			}
		}
		current=current->next;
	}
	return(current);
}

list *list_by_name(char *listname){
	screen *current;

	current = list_screen_by_name(listname);
	if (current != NULL) return (current->info);
	return (NULL);
}

list *active_list_by_name(char *listname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == LIST){
			if (((list *)(current->info))->active){
				if (strcasecmp(((list *)(current->info))->server->server, listname)==0){
					break;
				}
			}
		}
		current=current->next;
	}
	return(current->info);
}

screen *list_screen_by_name(char *listname){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == LIST){
			if (strcasecmp(((list *)(current->info))->server->server, listname)==0){
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

	// find which server this list belongs to
	current=screenlist;
	while(current!=NULL){
		if (current->type == LIST){
			if (((list *)(current->info))->server == server){
				currentlist = (list *)(current->info);
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

	// find which server this list belongs to
	current=screenlist;
	while(current!=NULL){
		if (current->type == LIST){
			if (((list *)(current->info))->active){
				if (((list *)(current->info))->server == server){
					currentlist = (list *)(current->info);
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
	if (current != NULL) return (current->info);
	return (NULL);
}

screen *transfer_screen_by_name(char *name){
	screen *current;

	current=screenlist;
	while(current!=NULL){
		if (current->type == TRANSFER){
			if (strcmp(((transfer *)(current->info))->name, name)==0){
				break;
			}
		}
		current=current->next;
	}
	return(current);
}


