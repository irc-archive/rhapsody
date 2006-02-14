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
#include "socks5.h"

char *socks5_error_message(int code){
	if (code == 0){
		return("Received request granted message from proxy.\n");
	}
	else if (code == 1){
		return("SOCKS general server failure... closing connection.\n");
	}
	else if (code == 2){
		return("Received request not allowed message from proxy... closing connection.\n");
	}
	else if (code == 3){
		return("Received network unreachable message from proxy... closing connection.\n");
	}
	else if (code == 4){
		return("Received host unreachable message from proxy... closing connection.\n");
	}
	else if (code == 5){
		return("Received connection refused message from proxy... closing connection.\n");
	}
	else{
		return("Received unknown message code from proxy... closing connection.\n");
	}
}

char *socks5_auth_message(int code){
	if (code == 0){
		return("Proxy selects no authentication.\n");
	}
	else if (code == 1){
		return("Proxy selects GSSAPI authentication.\n");
	}
	else if (code == 2){
		return("Proxy selects username / password authentication.\n");
	}
	else if (code >= 3 && code <= 127){
		return("Proxy selects unsupported IANA authentication method... closing connection.\n");
	}
	else if (code == 255){
		return("Proxy rejects all supported authentication types... closing connection.\n");
	}
	else{
		return("Proxy selects Unknown authentication type... closing connection\n");
	}
}

int get_socks5_host_and_port(unsigned char *message, char *hostname, int *port){
	int addrtype;
	int hostnamelen;
	struct in_addr hostip;

	addrtype = message[3];			

	/* ipv4 address */
	if (addrtype == 1){
		memcpy(&hostip, &message[4], 4);  
		strcpy(hostname, inet_ntoa(hostip));
		*port = (message[8] << 8 ) | message[9];
		return(1);
	}

	/* hostname and domain */
	else if (addrtype == 3){		
		strcpy(hostname, message + 4);
		hostnamelen = strlen(hostname);
		*port = (message[5 + hostnamelen] << 8 ) | message[6 + hostnamelen];
		return(1);
	}
	return(0);
}

int process_sock5_server_message(server *S, unsigned char *message, int len){
	int version;
	int code;
	int port;
	char hostname[MAXHOSTLEN];

	version = message[0];
	code = message[1];

	//vprint_all("Got sock5 message ver %d code %d\n", version, code);

	/* connect portion is marked by version 5 */
	if (version == 5){
		if (S->connect_status == 4){
			if (code >= 0 && code <= 2){ 
				vprint_server_attrib(S, MESSAGE_COLOR, socks5_auth_message(code));

				if (code == 0) S->connect_status = 10;
				else if (code == 1) S->connect_status = 5;
				else if (code == 2) S->connect_status = 8;
				return(1);
			}
			vprint_server_attrib(S, ERROR_COLOR, socks5_auth_message(code));
			S->active = 0;
			S->proxyactive = 0;
			S->connect_status = -1;
			close(S->proxyfd);
			return(0);
		}
		else if (S->connect_status == 13){ 
			/* request granted, swap descriptors */
			if (code == 0){
				// vprint_server_attrib(S, MESSAGE_COLOR, socks5_error_message(code));
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_server_attrib(S, MESSAGE_COLOR, 
					"Connected via proxy to %s port %d ...\n", hostname, port);

				S->serverfd = S->proxyfd;
				S->proxyfd = -1;
				S->proxyactive = 0;
				S->active = 0;

				/* resume normal connection process */
				S->connect_status = 15;
				return(1);
			}
	
			/* request rejected */
			vprint_server_attrib(S, ERROR_COLOR, socks5_error_message(code));
			S->active = 0;
			S->proxyactive = 0;
			S->connect_status = -1;
			close(S->proxyfd);
			return(0);
		}
	}

	/* username/password authentication is marked by sub negotiation version 1 */
	else if (version == 1){
		if (S->connect_status == 9){
			if (code == 0){
				vprint_server_attrib(S, MESSAGE_COLOR, "Authentication was successful.\n");
				S->connect_status = 10;
				return(1);
			}
			vprint_server_attrib(S, ERROR_COLOR, "Authentication failure... closing connection.\n");
			S->active = 0;
			S->proxyactive = 0;
			S->connect_status = -1;
			close(S->proxyfd);
 			return(0);
		}
	} 
	else {
		vprint_server_attrib(S, ERROR_COLOR, "Unknown proxy version number %d.\n", version);
		S->active = 0;
		S->proxyactive = 0;
		S->connect_status = -1;
		close(S->proxyfd);
		return(0);
	}
	return(1);
}

int process_sock5_dccchat_message(dcc_chat *D, unsigned char *message, int len){
	int version;
	int code;
	int port;
	char hostname[MAXHOSTLEN];
	struct hostent *host;

	version = message[0];
	code = message[1];

	if (len == 0) remove_dcc_chat(D);

	//vprint_all("Got sock5 dcc_chat message ver %d code %d conn %d serv %d\n", 
	//	version, code, D->connect_status, D->server_status);

	/* connect / authentication portion is marked by version 5 */
	if (version == 5){
		if (D->connect_status == 5){
			if (code >= 0 && code <= 2){
				vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_auth_message(code));
				if (code == 0) D->connect_status = 10;
				else if (code == 1) D->connect_status = 5;
				else if (code == 2) D->connect_status = 8;
				return(1);
			}
			vprint_dcc_chat_attrib(D, ERROR_COLOR, socks5_auth_message(code));
			remove_dcc_chat(D);
			return(0);
		}
		else if (D->server_status == 5){
			if (code >= 0 && code <= 2){
				vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_auth_message(code));
				if (code == 0) D->server_status = 10;
				else if (code == 1) D->server_status = 5;
				else if (code == 2) D->server_status = 8;
				return(1);
			}
			vprint_dcc_chat_attrib(D, ERROR_COLOR, socks5_auth_message(code));	
			remove_dcc_chat(D);
			return(0);
		}

		else if (D->connect_status == 11){ 
		/* request granted, swap descriptors */
			if (code == 0){
				//vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_error_message(code)); 
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_dcc_chat_attrib(D, DCC_COLOR, 
					"Connected via proxy to %s port %d ...\n", hostname, port);
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				return(1);
			}
	
			/* request rejected */
			vprint_dcc_chat_attrib(D, ERROR_COLOR, socks5_error_message(code)); 
			remove_dcc_chat(D);
			return(0);
		}

		else if (D->server_status == 11){
			if (code == 0){
				//vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_error_message(code)); 
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Proxy socket waiting at %s port %d ...\n",
					hostname, port);

				if ((host = gethostbyname(hostname)) == NULL) {
					vprint_dcc_chat_attrib(D, ERROR_COLOR, 
						"Error: Cannot lookup originating DCC hostname %s.\n", hostname);
					remove_dcc_chat(D);
					return(0);
				}
				else {
					D->server_status = 12;
					D->connectport = port;
					D->connectip = htonl(((struct in_addr *)(host->h_addr))->s_addr);
					return(1);
				}
			}
			/* request rejected */
			vprint_dcc_chat_attrib(D, ERROR_COLOR, socks5_error_message(code)); 
			remove_dcc_chat(D);
			return(0);
		}

		else if (D->server_status == 13){
			if (code == 0){
				//vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_error_message(code));
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_dcc_chat_attrib(D, DCC_COLOR, 
					"Connected via proxy to %s port %d ...\n", hostname, port);

				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				D->server_status = -1;
				return(1);
			}
	
			/* request rejected */
			vprint_dcc_chat_attrib(D, ERROR_COLOR, socks5_error_message(code));
			remove_dcc_chat(D);
			return(0);
		}
	}

	/* username/password authentication is marked by sub negotiation version 1 */
	else if (version == 1){
		if (D->connect_status == 9){
			if (code == 0){
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Authentication was successful.\n");
				D->connect_status = 10;
				return(1);
			}
			vprint_dcc_chat_attrib(D, ERROR_COLOR, "Authentication failure... closing connection.\n");
			remove_dcc_chat(D);
 			return(0);
		}
		if (D->server_status == 9){
			if (code == 0){
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Authentication was successful.\n");
				D->server_status = 10;
				return(1);
			}
			vprint_dcc_chat_attrib(D, ERROR_COLOR, "Authentication failure... closing connection.\n");
			remove_dcc_chat(D);
 			return(0);
		}
	} 
	else {
		vprint_dcc_chat_attrib(D, ERROR_COLOR, "Unknown proxy version number\n");
		remove_dcc_chat(D);
		return(0);
	}

	vprint_dcc_chat_attrib(D, ERROR_COLOR, "Received Unknown SOCKS5 message\n");
	remove_dcc_chat(D);
	return(0);
}

int process_sock5_dccfile_message(dcc_file *D, unsigned char *message, int len){
	int version;
	int code;
	int port;
	char hostname[MAXHOSTLEN];
	struct hostent *host;

	version = message[0];
	code = message[1];

	if (len == 0) remove_dcc_file(D);

	//vprint_all("Got sock5 dcc_file message ver %d code %d conn %d serv %d\n", 
	//	version, code, D->connect_status, D->server_status);

	/* connect portion is marked by version 5 */
	if (version == 5){
		if (D->connect_status == 5){ 
			if (code >= 0 && code <= 2){ 
				vprint_all_attrib(DCC_COLOR, socks5_auth_message(code));
				if (code == 0) D->connect_status = 10;
				else if (code == 1) D->connect_status = 5;
				else if (code == 2) D->connect_status = 8;
				return(1);
			}
			vprint_all_attrib(ERROR_COLOR, socks5_auth_message(code));
			remove_dcc_file(D);
			return(0);
		}
		else if (D->server_status == 5){ 
			if (code >= 0 && code <= 2){ 
				vprint_all_attrib(DCC_COLOR, socks5_auth_message(code));
				if (code == 0) D->server_status = 10;
				else if (code == 1) D->server_status = 5;
				else if (code == 2) D->server_status = 8;
				return(1);
			}
			vprint_all_attrib(ERROR_COLOR, socks5_auth_message(code));
			remove_dcc_file(D);
			return(0);
		}

		else if (D->server_status == 5){
			if (code >= 0 && code <= 2){
				vprint_all_attrib(DCC_COLOR, socks5_auth_message(code));
				if (code == 0) D->server_status = 10;
				else if (code == 1) D->server_status = 5;
				else if (code == 2) D->server_status = 8;
				return(1);
			}
			vprint_all_attrib(ERROR_COLOR, socks5_auth_message(code));	
			remove_dcc_file(D);
			return(0);
		}

		else if (D->connect_status == 11){ 
			/* request granted, swap descriptors */
			if (code == 0){
				//vprint_all_attrib(DCC_COLOR, socks5_error_message(code)); 
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_all_attrib(DCC_COLOR, 
					"Connected via proxy to %s port %d ...\n", hostname, port);
				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				return(1);
			}
	
			/* request rejected */
			vprint_all_attrib(ERROR_COLOR, socks5_error_message(code)); 
			remove_dcc_file(D);
			return(0);
		}
		else if (D->server_status == 11){
			if (code == 0){
				//vprint_dcc_chat_attrib(D, DCC_COLOR, socks5_error_message(code)); 
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_all_attrib(DCC_COLOR, "Proxy socket waiting at %s port %d ...\n",
					hostname, port);

				if ((host = gethostbyname(hostname)) == NULL) {
					vprint_all_attrib(ERROR_COLOR, 
						"Error: Cannot lookup originating DCC hostname %s.\n", hostname);
					remove_dcc_file(D);
					return(0);
				}
				else {
					D->server_status = 12;
					D->connectport = port;
					D->connectip = htonl(((struct in_addr *)(host->h_addr))->s_addr);
					return(1);
				}
			}
			/* request rejected */
			vprint_all_attrib(ERROR_COLOR, socks5_error_message(code)); 
			remove_dcc_file(D);
			return(0);
		}
		else if (D->server_status == 13){
			if (code == 0){
				//vprint_all_attrib(DCC_COLOR, socks5_error_message(code));
				get_socks5_host_and_port(message, hostname, &port); 
				vprint_all_attrib(DCC_COLOR, 
					"Connected via proxy to %s port %d ...\n", hostname, port);

				D->dccfd = D->proxyfd;
				D->proxyfd = -1;
				D->proxyactive = 0;
				D->active = 1;
				D->server_status = -1;
				return(1);
			}
	
			/* request rejected */
			vprint_all_attrib(ERROR_COLOR, socks5_error_message(code));
			remove_dcc_file(D);
			return(0);
		}
	}

	/* username/password authentication is marked by sub negotiation version 1 */
	else if (version == 1){
		if (D->connect_status == 9){
			if (code == 0){
				vprint_all_attrib(DCC_COLOR, "Authentication was successful.\n");
				D->connect_status = 10;
				return(1);
			}
			vprint_all_attrib(ERROR_COLOR, "Authentication failure... closing connection.\n");
			remove_dcc_file(D);
 			return(0);
		}
		if (D->server_status == 9){
			if (code == 0){
				vprint_all_attrib(DCC_COLOR, "Authentication was successful.\n");
				D->server_status = 10;
				return(1);
			}
			vprint_all_attrib(ERROR_COLOR, "Authentication failure... closing connection.\n");
			remove_dcc_file(D);
 			return(0);
		}
	} 
	else {
		vprint_all_attrib(ERROR_COLOR, "Unknown proxy version number\n");
		remove_dcc_file(D);
		return(0);
	}
	return(1);
}

