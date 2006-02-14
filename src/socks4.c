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
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "autodefs.h"
#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "autodefs.h"
#include "defines.h"

#include "log.h"
#include "ncolor.h"
#include "common.h"
#include "events.h"
#include "cevents.h"
#include "screen.h"
#include "network.h"
#include "parser.h"
#include "ctcp.h"
#include "dcc.h"
#include "main.h"
#include "cmenu.h"
#include "forms.h"
#include "config.h"
#include "option.h"
#include "socks4.h"

char *socks4_error_message(int code){
	if (code == 90){
		return("Received request granted message from proxy.\n");
	}
	else if (code == 91){
		return("Received request rejected message from proxy... closing connection.\n");
	}
	else if (code == 92){
                return("Received request rejected message from proxy, ident connection failure... closing connection.\n");
	}
	else if (code == 93){
		return("Received request rejected message from proxy, ident authentication failure... closing connection.\n");
	}
        else{
		return("Received unknown message code from proxy... closing connection.\n");
      	}
}	

int process_sock4_server_message(server *S, unsigned char *message, int len){
	int version;
	int code;
	int destport;
	struct in_addr destip;

	version = message[0];
	code = message[1];

	// vprint_all("Got sock4 message ver %d code %d\n", version, code);
	if (version == 0){
		/* request granted, swap descriptors */
		if (code == 90){
			destport = (message[2] << 8 ) | message[3];
			memcpy(&destip, &message[4], 4);  
			vprint_server_attrib(S, MESSAGE_COLOR, socks4_error_message(code));
			vprint_server_attrib(S, MESSAGE_COLOR, "Connected via proxy to %s port %d ...\n",
				inet_ntoa(destip), destport);
			S->serverfd = S->proxyfd;
			S->proxyfd = -1;
			S->proxyactive = 0;
			S->active = 0;

			/* resume normal connection process */
			S->connect_status = 5;
			return(1);
		}

		/* request rejected */
		vprint_server_attrib(S, ERROR_COLOR, socks4_error_message(code));
		S->active = 0;
		S->proxyactive = 0;
		close(S->proxyfd);
		S->connect_status = -1;
		return(0);
	}
	else {
		vprint_server_attrib(S, ERROR_COLOR, "Unknown proxy version number %d\n", version);
		S->active = 0;
		S->proxyactive = 0;
		close(S->proxyfd);
		S->connect_status = -1;
		return(0);
	}
	return(1);
}

int process_sock4_dccchat_message(dcc_chat *D, unsigned char *message, int len){
	int version;
	int code;
	int destport;
	struct in_addr destip;

	version = message[0];
	code = message[1];

	if (len == 0) remove_dcc_chat(D);

	//vprint_all("Got sock4 dcc_chat message ver %d code %d, conn %d serv %d\n", 
	//	version, code, D->connect_status, D->server_status);

	if (version == 0){
		/* request granted, swap descriptors */
		if (code == 90){
			destport = (message[2] << 8 ) | message[3];
			memcpy(&destip, &message[4], 4);

			if (D->direction == DCC_RECEIVE && D->connect_status == 4){
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Connected via proxy to %s port %d ...\n",
					inet_ntoa(destip), destport);
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
			}

			/* bind error message */
			else if (D->direction == DCC_SEND && D->server_status == 5){
				D->server_status = 6;
				D->connectip = htonl(destip.s_addr);
				D->connectport = destport;
				
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Proxy socket waiting at %s port %d ...\n",
					inet_ntoa(destip), D->connectport);
			}

			/* client connect message */ 
			else if (D->direction == DCC_SEND && D->server_status == 7){
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Remote client connected to proxy ...\n");
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				D->server_status = -1;
			}
			else {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, 
					"Received unexpected request granted message from proxy ...\n");
				remove_dcc_chat(D);
			}
			return(1);
		}
		/* request rejected */
		vprint_dcc_chat_attrib(D, ERROR_COLOR, socks4_error_message(code));
		remove_dcc_chat(D);
		return(0);
	}
	else {
		vprint_dcc_chat_attrib(D, ERROR_COLOR, "Unknown proxy version number\n");
		remove_dcc_chat(D);
		return(0);
	}

	vprint_dcc_chat_attrib(D, ERROR_COLOR, "Received Unknown SOCKS4 message\n");
	remove_dcc_chat(D);
	return(0);
}

int process_sock4_dccfile_message(dcc_file *D, unsigned char *message, int len){
	int version;
	int code;
	int destport;
	struct in_addr destip;

	version = message[0];
	code = message[1];

	// vprint_all("Got sock4 dcc_chat message ver %d code %d\n", version, code);
	if (version == 0){
		/* request granted, swap descriptors */
		if (code == 90){
			destport = (message[2] << 8 ) | message[3];
			memcpy(&destip, &message[4], 4);

			if (D->type == DCC_RECEIVE && D->connect_status == 4){
				vprint_all_attrib(DCC_COLOR, "Connected via proxy to %s port %d ...\n",
					inet_ntoa(destip), destport);
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				D->connect_status = -1;
			}

			/* bind error message */
			else if (D->type == DCC_SEND && D->server_status == 5){
				D->server_status = 6;
				D->connectip = htonl(destip.s_addr);
				D->connectport = destport;
				
				vprint_all_attrib(DCC_COLOR, "Proxy socket waiting at %s port %d ...\n",
					inet_ntoa(destip), D->connectport);
			}

			/* client connect message */ 
			else if (D->type == DCC_SEND && D->server_status == 7){
				vprint_all_attrib(DCC_COLOR, "Remote client connected to proxy ...\n");
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				D->server_status = -1;
			}
			else {
				vprint_all_attrib(ERROR_COLOR, 
					"Received unexpected request granted message from proxy ...\n");
			}
			return(1);
		}

		/* request rejected */
		vprint_all_attrib(ERROR_COLOR, socks4_error_message(code)); 
		D->active = 0;
		D->proxyactive = 0;
		return(0);
	}
	else {
		vprint_all_attrib(ERROR_COLOR, "Unknown proxy version number\n");
		D->active = 0;
		D->proxyactive = 0;
		return(0);
	}
	return(1);
}

