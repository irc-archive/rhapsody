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

#define LARGEFILES

/* for Tru64 (OSF1) extended errno.h definitions */
#define _LIBC_POLLUTION_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>


#include "defines.h"
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0100000
#endif

#include "log.h"
#include "ncolor.h"
#include "common.h"
#include "network.h"
#include "screen.h"
#include "dcc.h"
#include "comm.h"
#include "ctcp.h"


/* start incoming dcc chat, connect to server */
int start_incoming_dcc_chat(dcc_chat *D){
	char buffer[MAXDATASIZE];
	unsigned long hostiph;
	struct sockaddr_in initiator;       
	struct in_addr hostaddr;
	static struct hostent *host;
	int alarm_occured;
	int userlen, passlen;

	if (D == NULL) return (0);
	if (D->server_status == 0) D->proxy = configuration.proxy;

	if (D->proxy == PROXY_DIRECT){
		D->active = 0;
		D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
		if (D->dccfd == -1) {
			vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Chat: Cannot create a socket\n");
			return(0);
		}

		hostiph = ntohl(D->remoteip); 
		hostaddr.s_addr = hostiph; 		
	        initiator.sin_family = AF_INET;                          
	        initiator.sin_port = htons(D->remoteport);         
	        initiator.sin_addr.s_addr = hostiph;
	        bzero(&(initiator.sin_zero), 8);     

		D->server_status=1;

		alarm_occured=0;
		alarm(CONNECTTIMEOUT);	
		if (connect (D->dccfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 || alarm_occured){
			alarm_occured=0;	
			vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
				inet_ntoa(hostaddr), strerror(errno));
			D->server_status = -1;
			return(0);
	        }
		fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
		D->active = 1;
		return(1);
	}

	else if (D->proxy == PROXY_SOCKS4){
		D->active=0;

		if (D->connect_status == 0){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Chat: Cannot create a socket\n");
				return(0);
			}
			else{
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Looking up SOCKS4 proxy %s ...\n", 
					configuration.proxyhostname);
				D->connect_status = 1;
				return(1);
			}
                }
		else if (D->connect_status == 1){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_dcc_chat_attrib(D, ERROR_COLOR, "Error: Cannot find SOCKS4 proxy %s\n", 
					configuration.proxyhostname);
                        	D->connect_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connecting to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 2;
                        	return(1);
                	}
		}
		else if (D->connect_status == 2){
		        initiator.sin_family = AF_INET;     
		        initiator.sin_port = htons(configuration.proxyport);
		        initiator.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(initiator.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);	
			if (connect (D->proxyfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connected to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 3;
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
				return(1);
			}
		}

		else if (D->connect_status == 3){
			hostiph = ntohl(D->remoteip);
			hostaddr.s_addr = hostiph;

			vprint_dcc_chat_attrib(D, DCC_COLOR, "Requesting connection to %s from proxy ...\n", 
				inet_ntoa(hostaddr));

			/* create SOCK4 connect message */
			buffer[0] = 4;
			buffer[1] = 1; /* CONNECT */
			buffer[2] = (D->remoteport & 0xff00) >> 8;
			buffer[3] = (D->remoteport & 0xff);
			memcpy(&buffer[4], &hostiph, 4);
			strcpy(&buffer[8], configuration.proxyusername);
			send_ball(D->proxyfd, buffer, 8 + strlen(buffer + 8) + 1);

			/* at this point, wait until the proxy replies */
			D->connect_status = 4;			
			D->proxyactive = 1;
			return(1);
		}
	}

	else if (D->proxy == PROXY_SOCKS5){
	        // use the SOCKS5 proxy rather than a direct connection 

		D->active = 0;
		if (D->connect_status == 0){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Chat: Cannot create a socket\n");
				return(0);
			}
			else{
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Looking up SOCKS5 proxy %s ...\n", 
					configuration.proxyhostname);
				D->connect_status = 1;
				return(1);
			}
		}

		else if (D->connect_status == 1){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "Error: Cannot find SOCKS5 proxy %s\n", 
					configuration.proxyhostname);
				D->connect_status = -1;
				return(0);
			}
			else {
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Connecting to SOCKS5 proxy %s (%s) port %d ...\n", 
					host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
				D->connect_status = 2;
				return(1);
			}
		}

		else if (D->connect_status == 2){
			initiator.sin_family = AF_INET;     
			initiator.sin_port = htons(configuration.proxyport);
			initiator.sin_addr = *((struct in_addr *)host->h_addr);
			bzero(&(initiator.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);	
			if (connect (D->proxyfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connected to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 3;
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
				return(1);
			}
		}

		else if (D->connect_status == 3){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Negotiating authentication methods ...\n");
			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* NUMBER OF METHODS */
			buffer[2] = 0; /* NO AUTHENTICATION */
			buffer[3] = 2; /* USERNAME / PASSWORD */
			buffer[4] = 1; /* GSS API */
			send_ball(D->proxyfd, buffer, 4);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 5;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns username authentication advance connect_status to 8 */
		else if (D->connect_status == 8){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Sending username and password ...\n");

			/* create username / password message */
			userlen = (unsigned char) strlen(configuration.proxyusername);
			passlen = (unsigned char) strlen(configuration.proxypassword);

			buffer[0] = 1; /* METHOD VERSION */
			buffer[1] = userlen; /* USERLEN */
			buffer[2 + userlen] = passlen; /* PASSLEN */
			memcpy(&buffer[2], configuration.proxyusername, userlen); /* USERNAME */
			memcpy(&buffer[3 + userlen], configuration.proxypassword, passlen); /* PASSWORD */
			send_ball(D->proxyfd, buffer, 3 + userlen + passlen);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 9;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns no authentication skip to connect_status 10 */
		else if (D->connect_status == 10){

			hostiph = ntohl(D->remoteip);
			hostaddr.s_addr = hostiph;

			vprint_dcc_chat_attrib(D, DCC_COLOR, "Requesting connection to %s from proxy ...\n", 
				inet_ntoa(hostaddr));

			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 1; /* CONNECT */
			buffer[2] = 0; /* RESERVED */
			buffer[3] = 1; /* ADDRESS TYPE - IPv4 */
			memcpy(&buffer[4], &hostiph, 4);
			buffer[8] = (D->remoteport & 0xff00) >> 8;
			buffer[9] = (D->remoteport & 0xff);
			send_ball(D->proxyfd, buffer, 10);

			/* at this point, wait until the proxy replies */
			D->connect_status = 11;
			D->proxyactive = 1;
			return(1);
		}
	}	
	D->connect_status = -1;
	D->active = 0;
	return(0);
}


int start_outgoing_dcc_chat(dcc_chat *D){
        int length;
        char buffer[MAXDATASIZE];
        fd_set readfds;
        struct timeval timeout;
        int serr, berr;
	int size;
	int fd, new_fd;
	int portstartrange, portendrange, port;
	struct sockaddr_in serveraddr, clientaddr, proxyaddr;       
	struct hostent *host;
	int alarm_occured;
	unsigned long hostiph;
	struct in_addr hostaddr;
	int userlen, passlen;

	portstartrange = configuration.dccstartport;
	portendrange = configuration.dccendport;

	if (D == NULL) return(0);
	if (D->server_status == 0) D->proxy = configuration.proxy;
	
	if (D->proxy == PROXY_DIRECT){
		if (D->server_status == 0){
			/* if not yet done, configure and bind the socket to listen */
			D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->dccfd == -1) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, 
					"Error (%d %s) opening DCC chat server socket.\n", errno, strerror(errno));
				D->server_status = -1;
				return(-1);
			}

			/* find the next available free port */
			for (port = portstartrange; port <= portendrange; port++){
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_port = htons(port);
				serveraddr.sin_addr.s_addr = INADDR_ANY;

				memset(&(serveraddr.sin_zero), 0, 8);
	
				berr = bind(D->dccfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
			
				/* if port in already in use, try the next one in the range */
				if (berr == -1 && errno == EADDRINUSE){}
				else if (berr == -1){
					vprint_dcc_chat_attrib(D, ERROR_COLOR, 
						"Error (%d %s) binding DCC chat server socket.\n", 
					errno, strerror(errno));
				}
				else{
					berr = 0;
					break;		
				}
			}
			if (berr == -1){
				D->server_status = -1;
				return(-1);
			}
			fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
		        listen(D->dccfd, 1);

			D->server_status = 1;
			D->localport = ntohs(serveraddr.sin_port);
			D->connectport = D->localport;

			host = gethostbyname(configuration.dcchostname);
			if (host == NULL){
				vprint_dcc_chat_attrib(D, ERROR_COLOR, 
					"Error starting DCC chat: lookup for \"%s\" failed\n", configuration.dcchostname);
				return(0);
			}
	
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Using %s (%s) for DCC source host\n", 
				configuration.dcchostname, inet_ntoa(*((struct in_addr*)host->h_addr)));

			D->localip = htonl(((struct in_addr *)(host->h_addr))->s_addr);
			D->connectip = D->localip;

			vprint_dcc_chat_attrib(D, DCC_COLOR, 
				"DCC chat server listening on port %d\n", ntohs(serveraddr.sin_port));
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);

			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC CHAT chat", "%lu %d", 
				D->localip, D->localport), D->nick, "");
			return(1);
		}

		/* once server initialization is complete, wait for new connection */
		else if (D->server_status == 1){
		        size = sizeof(struct sockaddr_in);
		        new_fd = accept(D->dccfd, (struct sockaddr *)&clientaddr, &size);
			if (new_fd != -1){	
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Accepting DCC connection from %s port %d\n", 
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				/* at this point close the listening socket and replace it with the active one */
				close(D->dccfd);
				D->dccfd = new_fd;
				D->active = 1;
				return(1);
			}		
		}
		return(0);
	}

	else if (D->proxy == PROXY_SOCKS4){
		if (D->server_status == 0){

			host = gethostbyname(configuration.dcchostname);
			if (host == NULL){
				vprint_dcc_chat_attrib(D, ERROR_COLOR, 
					"Error starting DCC chat: lookup for \"%s\" failed\n", configuration.dcchostname);
				return(0);
			}
	
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Using %s (%s) for DCC source host\n", 
				configuration.dcchostname, inet_ntoa(*((struct in_addr*)host->h_addr)));

			D->localport = ntohs(serveraddr.sin_port);
			D->localip = htonl(((struct in_addr *)(host->h_addr))->s_addr);

			D->server_status = 1;
			return(1);

		}
		/* obtain proxy permission to connect to bind */
		if (D->server_status == 1){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Chat: Cannot create a socket to proxy\n");
				return(0);
			}
			else{
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Looking up SOCKS4 proxy %s ...\n", 
					configuration.proxyhostname);
				D->server_status = 2;
				return(1);
			}
                }
		else if (D->server_status == 2){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_dcc_chat_attrib(D, ERROR_COLOR, "Error: Cannot find SOCKS4 proxy %s\n", 
					configuration.proxyhostname);
                        	D->server_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connecting to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 3;
                        	return(1);
                	}
		}
		else if (D->server_status == 3){
		        proxyaddr.sin_family = AF_INET;     
		        proxyaddr.sin_port = htons(configuration.proxyport);
		        proxyaddr.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(proxyaddr.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);
			if (connect (D->proxyfd, (struct sockaddr *)&proxyaddr, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connected to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 4;
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
				return(1);
			}
		}

		else if (D->server_status == 4){
			hostiph = ntohl(D->localip);
			hostaddr.s_addr = hostiph;

			vprint_dcc_chat_attrib(D, DCC_COLOR, "Requesting bind to %s port %d from proxy ...\n", 
				inet_ntoa(hostaddr), D->localport);

			/* create SOCK4 connect message */
			buffer[0] = 4;
			buffer[1] = 2; /* BIND */
			buffer[2] = (D->localport & 0xff00) >> 8;
			buffer[3] = (D->localport & 0xff);
			memcpy(&buffer[4], &hostiph, 4);
			strcpy(&buffer[8], configuration.proxyusername);
			send_ball(D->proxyfd, buffer, 8 + strlen(buffer + 8) + 1);

			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 5;
			return(1);
		}

		else if (D->server_status == 6){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);
			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC CHAT chat", "%lu %d", 
				D->connectip, D->connectport), D->nick, "");

			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 7;

			return(1);
		}
		return(0);
	}

	else if (D->proxy == PROXY_SOCKS5){
		if (D->server_status == 0){

			host = gethostbyname(configuration.dcchostname);
			if (host == NULL){
				vprint_dcc_chat_attrib(D, ERROR_COLOR, 
					"Error starting DCC chat: lookup for \"%s\" failed\n", configuration.dcchostname);
				return(0);
			}
	
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Using %s (%s) for DCC source host\n", 
				configuration.dcchostname, inet_ntoa(*((struct in_addr*)host->h_addr)));

			D->localport = ntohs(serveraddr.sin_port);
			D->localip = htonl(((struct in_addr *)(host->h_addr))->s_addr);

			D->server_status = 1;
			return(1);

		}
		/* obtain proxy permission to connect to bind */
		if (D->server_status == 1){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Chat: Cannot create a socket to proxy\n");
				return(0);
			}
			else{
				vprint_dcc_chat_attrib(D, DCC_COLOR, "Looking up SOCKS5 proxy %s ...\n", 
					configuration.proxyhostname);
				D->server_status = 2;
				return(1);
			}
                }
		else if (D->server_status == 2){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_dcc_chat_attrib(D, ERROR_COLOR, "Error: Cannot find SOCKS5 proxy %s\n", 
					configuration.proxyhostname);
                        	D->server_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connecting to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 3;
                        	return(1);
                	}
		}
		else if (D->server_status == 3){
		        proxyaddr.sin_family = AF_INET;     
		        proxyaddr.sin_port = htons(configuration.proxyport);
		        proxyaddr.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(proxyaddr.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);
			if (connect (D->proxyfd, (struct sockaddr *)&proxyaddr, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_dcc_chat_attrib(D, ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_dcc_chat_attrib(D, DCC_COLOR, "Connected to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 4;
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
				return(1);
			}
		}

		else if (D->server_status == 4){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Negotiating authentication methods ...\n");
			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* NUMBER OF METHODS */
			buffer[2] = 0; /* NO AUTHENTICATION */
			buffer[3] = 2; /* USERNAME / PASSWORD */
			buffer[4] = 1; /* GSS API */
			send_ball(D->proxyfd, buffer, 4);

			/* at this point, wait until the proxy replies and advances status */
			D->server_status = 5;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns username authentication advance connect_status to 8 */
		else if (D->connect_status == 8){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Sending username and password ...\n");

			/* create username / password message */
			userlen = (unsigned char) strlen(configuration.proxyusername);
			passlen = (unsigned char) strlen(configuration.proxypassword);

			buffer[0] = 1; /* METHOD VERSION */
			buffer[1] = userlen; /* USERLEN */
			buffer[2 + userlen] = passlen; /* PASSLEN */
			memcpy(&buffer[2], configuration.proxyusername, userlen); /* USERNAME */
			memcpy(&buffer[3 + userlen], configuration.proxypassword, passlen); /* PASSWORD */
			send_ball(D->proxyfd, buffer, 3 + userlen + passlen);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 9;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns no authentication skip to connect_status 10 */
		else if (D->server_status == 10){
			hostiph = ntohl(D->localip);
			hostaddr.s_addr = hostiph;

			vprint_dcc_chat_attrib(D, DCC_COLOR, "Requesting bind to %s port %d from proxy ...\n", 
				inet_ntoa(hostaddr), D->localport);

			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* BIND */
			buffer[2] = 0; /* RESERVED */
			buffer[3] = 1; /* ADDRESS TYPE - IPv4 */
			memcpy(&buffer[4], &hostiph, 4);
			buffer[8] = (D->localport & 0xff00) >> 8;
			buffer[9] = (D->localport & 0xff);
			send_ball(D->proxyfd, buffer, 10);

			/* at this point, wait until the proxy replies */
			D->server_status = 11;
			D->proxyactive = 1;
			return(1);
		}
		else if (D->server_status == 12){
			vprint_dcc_chat_attrib(D, DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);
			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC CHAT chat", "%lu %d", 
				D->connectip, D->connectport), D->nick, "");

			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 13;

			return(1);
		}
		return(0);
	}
	return(-1);
}


/* add incoming file data to the list */
dcc_file *add_incoming_dcc_file(transfer *transfer, server *server, char *nick, char *filename, unsigned long hostip, 
	unsigned int port, unsigned long size){
        dcc_file *firstdcc;
        dcc_file *new;
	char filenamex[1024];
	char filepath[1024];
	char filestamp[1024];
	struct tm *t;
	time_t ct;
	FILE *fp;
	int fd;

	sprintf(filepath, "%s/%s", configuration.dccdlpath, filename);

	/* check if the file exists, and if it does, append a timestamp extension */
	fp = fopen(filepath, "rb");
	
	if (fp != NULL && configuration.dccduplicates == 1){
		ct = time(NULL);
		t = localtime(&ct);
		sprintf(filestamp, "%s.%04d%02d%02d%02d%02d%02d", filename, t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		vprint_all_attrib(DCC_COLOR, "DCC file %s exists, saving as %s\n", filename, filestamp);
		sprintf(filepath, "%s/%s", configuration.dccdlpath, filestamp);
		fclose(fp);
		strcpy(filenamex, filestamp);
	}
	else strcpy(filenamex, filename);

	//fp = fopen(filepath, "wb");
	//if (fp == NULL){

	fd = open(filepath, O_RDWR|O_CREAT|O_TRUNC|O_LARGEFILE, 0666);
	if (fd < 0) {
		vprint_all_attrib(ERROR_COLOR, "DCC File: Couldn't open file %s for writing\n", filepath);
		return(NULL);
	}

        firstdcc = transfer->dcclist;
        new = calloc(sizeof(dcc_file), 1);
        if (new==NULL){
		plog ("Cannot allocate memory for DCC file transfer in add_incoming_dcc_file");
		exit (-1);
	}
        if (firstdcc != NULL){
        	firstdcc->prev = new;
                new->next = firstdcc;
                new->prev = NULL;
                transfer->dcclist = new;
	}
        else {
		new->next = NULL;
                new->prev = NULL;
                transfer->dcclist = new;
	}
	transfer->dcclisttop = new;
	transfer->selectedfile = new;
        strcpy(new->filename, filenamex);
        strcpy(new->nick, nick);
        new->remoteip = hostip;
        new->remoteport = port;
        new->size = size; 
        new->type = DCC_RECEIVE; 
	new->transfer = transfer;
	new->byte=0;
	new->server=server;
	//new->dccfp = fp;
	new->filefd = fd;
	new->allowed = 0;
        return (new);
}

/* add outgoing file data to the list */
dcc_file *add_outgoing_dcc_file(transfer *transfer, server *server, char *nick, char *filename){
	int i;
	int fd;
	FILE *fp;
        dcc_file *firstdcc;
        dcc_file *new;
	struct stat buf;
	int err;

#ifdef LARGEFILES
	fd = open(filename, O_RDONLY | O_LARGEFILE);
	if (fd < 0){
		vprint_all_attrib(ERROR_COLOR, "DCC File: Couldn't open file %s for reading\n", filename);
		return(NULL);
	}	
#else
	fp = fopen(filename, "rb");
	if (fp == NULL){
		vprint_all_attrib(ERROR_COLOR, "DCC File: Couldn't open file %s for reading\n", filename);
		return(NULL);
	}

#endif

		
	firstdcc = transfer->dcclist;
	new = calloc(sizeof(dcc_file), 1);
	if (new==NULL){
		plog ("Cannot allocate memory for DCC file transfer in add_outgoing_dcc_file");
		exit (-1);
	}

	if (firstdcc != NULL){
		firstdcc->prev = new;

		new->next = firstdcc;
		new->prev = NULL;
		transfer->dcclist = new;
	}
        else {
		new->next = NULL;
                new->prev = NULL;
                transfer->dcclist = new;
	}

	/* truncate the filename by dropping path */
	for (i = strlen(filename) - 1; i >= 0; i--){
		if (filename[i] == '/'){
			i++;
			break;
		}
	}

	strcpy(new->shortfilename, filename + i);
	transfer->dcclisttop = new;
	transfer->selectedfile = new;
        strcpy(new->filename, filename);
        strcpy(new->nick, nick);
        new->type = DCC_SEND; 
	new->transfer = transfer;
	new->active = 0;
	new->byte = 0;
	new->server = server;

	/* find the length of the outgoing file */

#ifdef LARGEFILES
	err = fstat(fd, &buf);
	new->size = buf.st_size;
	
	if (err == -1 && errno != EOVERFLOW){ 
		vprint_all_attrib(ERROR_COLOR, "Error (%d %s) getting file size for filename %s.\n", 
			errno, strerror(errno), filename);
	}
	new->filefd = fd;
#else
	fseek(fp, 0, SEEK_END);
	new->size = ftell(fp);
	if (new->size == -1){ 
		vprint_all_attrib(ERROR_COLOR, "Error (%d %s) getting file size for filename %s.\n", 
			errno, strerror(errno), filename);
	}
	fseek(fp, 0, SEEK_SET);
	new->dccfp = fp;
#endif

	//vprint_all("filesize = %lu\n", new->size);

	time(&(new->last_activity_at));
	time(&(new->last_updated_at));

        return (new);
}


/* start incoming file transfer, connect to remote server */
int start_incoming_dcc_file(dcc_file *D){  	
	char buffer[MAXDATASIZE];
	static struct hostent *host;
	unsigned long hostiph;
	struct sockaddr_in initiator;       
	struct in_addr hostaddr;
	int alarm_occured;
	int userlen, passlen;
			
	if (D == NULL) return (0);

	if (D->server_status == 0) D->proxy = configuration.proxy;
	if (D->proxy == PROXY_DIRECT){

		D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
		if (D->dccfd == -1) {
			vprint_all_attrib(ERROR_COLOR, "DCC File: Cannot create a socket for %s\n", D->filename);
			remove_dcc_file(D);		
			return(0);
		}

		hostiph = ntohl(D->remoteip); 
		hostaddr.s_addr = hostiph; 		
	        initiator.sin_family = AF_INET;                          
	        initiator.sin_port = htons(D->remoteport);         
	        initiator.sin_addr.s_addr = hostiph;
	        bzero(&(initiator.sin_zero), 8);     

		alarm_occured=0;
		alarm(CONNECTTIMEOUT);	
		if (connect (D->dccfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 || alarm_occured){
			if (alarm_occured){
				alarm_occured = 0;
				vprint_all_attrib(ERROR_COLOR, "Timeout while connecting to %s\n", inet_ntoa(hostaddr));
			}
			else{
				vprint_all_attrib(ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(hostaddr), strerror(errno));
			}
			return(0);
	        }
		fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
		time(&(D->starttime));
		time(&(D->last_activity_at));
		time(&(D->last_updated_at));
		D->active=1;
		return(1);
	}

	else if (D->proxy == PROXY_SOCKS4){
		D->active=0;

		if (D->connect_status == 0){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_all_attrib(ERROR_COLOR, "DCC File: Cannot create a socket\n");
				return(0);
			}
			else{
				vprint_all_attrib(DCC_COLOR, "Looking up SOCKS4 proxy %s ...\n", 
					configuration.proxyhostname);
				D->connect_status = 1;
				return(1);
			}
                }
		else if (D->connect_status == 1){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_all_attrib(ERROR_COLOR, "Error: Cannot find SOCKS4 proxy %s\n", 
					configuration.proxyhostname);
                        	D->connect_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_all_attrib(DCC_COLOR, "Connecting to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 2;
                        	return(1);
                	}
		}
		else if (D->connect_status == 2){
		        initiator.sin_family = AF_INET;                          
		        initiator.sin_port = htons(configuration.proxyport);         
		        initiator.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(initiator.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);	
			if (connect (D->proxyfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;
				vprint_all_attrib(ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_all_attrib(DCC_COLOR, "Connected to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 3;
				return(1);
			}
		}
		else if (D->connect_status == 3){
			hostiph = ntohl(D->remoteip);
			hostaddr.s_addr = hostiph;

			vprint_all_attrib(DCC_COLOR, "Requesting connection to %s from proxy ...\n", 
				inet_ntoa(hostaddr));

			/* create SOCK4 connect message */
			buffer[0] = 4;
			buffer[1] = 1; /* CONNECT */
			buffer[2] = (D->remoteport & 0xff00) >> 8;
			buffer[3] = (D->remoteport & 0xff);
			memcpy(&buffer[4], &hostiph, 4);
			strcpy(&buffer[8], configuration.proxyusername);
			send_ball(D->proxyfd, buffer, 8 + strlen(buffer + 8) + 1);

			/* at this point, wait until the proxy replies */
			D->connect_status = 4;			
			D->proxyactive = 1;
			D->server_status = 1;

			fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
			time(&(D->starttime));
			time(&(D->last_activity_at));
			time(&(D->last_updated_at));

			return(1);
		}
	}
	else if (D->proxy == PROXY_SOCKS5){
	        // use the SOCKS5 proxy rather than a direct connection 

		D->active = 0;
		if (D->connect_status == 0){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_all_attrib(ERROR_COLOR, "DCC Chat: Cannot create a socket\n");
				return(0);
			}
			else{
				vprint_all_attrib(DCC_COLOR, "Looking up SOCKS5 proxy %s ...\n", 
					configuration.proxyhostname);
				D->connect_status = 1;
				return(1);
			}
		}

		else if (D->connect_status == 1){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
				vprint_all_attrib(ERROR_COLOR, "Error: Cannot find SOCKS5 proxy %s\n", 
					configuration.proxyhostname);
				D->connect_status = -1;
				return(0);
			}
			else {
				vprint_all_attrib(DCC_COLOR, "Connecting to SOCKS5 proxy %s (%s) port %d ...\n", 
					host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
				D->connect_status = 2;
				return(1);
			}
		}

		else if (D->connect_status == 2){
			initiator.sin_family = AF_INET;     
			initiator.sin_port = htons(configuration.proxyport);
			initiator.sin_addr = *((struct in_addr *)host->h_addr);
			bzero(&(initiator.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);	
			if (connect (D->proxyfd, (struct sockaddr *)&initiator, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_all_attrib(ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_all_attrib(DCC_COLOR, "Connected to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->connect_status = 3;
				return(1);
			}
		}

		else if (D->connect_status == 3){
			vprint_all_attrib(DCC_COLOR, "Negotiating authentication methods ...\n");
			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* NUMBER OF METHODS */
			buffer[2] = 0; /* NO AUTHENTICATION */
			buffer[3] = 2; /* USERNAME / PASSWORD */
			buffer[4] = 1; /* GSS API */
			send_ball(D->proxyfd, buffer, 4);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 5;
			D->proxyactive = 1;
			fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
			return(1);
		}

		/* if proxy returns username authentication advance connect_status to 8 */
		else if (D->connect_status == 8){
			vprint_all_attrib(DCC_COLOR, "Sending username and password ...\n");

			/* create username / password message */
			userlen = (unsigned char) strlen(configuration.proxyusername);
			passlen = (unsigned char) strlen(configuration.proxypassword);

			buffer[0] = 1; /* METHOD VERSION */
			buffer[1] = userlen; /* USERLEN */
			buffer[2 + userlen] = passlen; /* PASSLEN */
			memcpy(&buffer[2], configuration.proxyusername, userlen); /* USERNAME */
			memcpy(&buffer[3 + userlen], configuration.proxypassword, passlen); /* PASSWORD */
			send_ball(D->proxyfd, buffer, 3 + userlen + passlen);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 9;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns no authentication skip to connect_status 10 */
		else if (D->connect_status == 10){

			hostiph = ntohl(D->remoteip);
			hostaddr.s_addr = hostiph;

			vprint_all_attrib(DCC_COLOR, "Requesting connection to %s from proxy ...\n", 
				inet_ntoa(hostaddr));

			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 1; /* CONNECT */
			buffer[2] = 0; /* RESERVED */
			buffer[3] = 1; /* ADDRESS TYPE - IPv4 */
			memcpy(&buffer[4], &hostiph, 4);
			buffer[8] = (D->remoteport & 0xff00) >> 8;
			buffer[9] = (D->remoteport & 0xff);
			send_ball(D->proxyfd, buffer, 10);

			/* at this point, wait until the proxy replies */
			D->connect_status = 11;
			D->proxyactive = 1;

			time(&(D->starttime));
			time(&(D->last_activity_at));
			time(&(D->last_updated_at));
			return(1);
		}
	}	
	return(0);
}

/* start outgoing file transfer, create a local server */
int start_outgoing_dcc_file(dcc_file *D){
	//char message[MAXDATASIZE];
	//struct in_addr hostaddr;
	char buffer[MAXDATASIZE];
	static struct hostent *host;
	unsigned long hostiph;
        int serr, berr;
	int size;
	int fd, new_fd;
	int portstartrange, portendrange, port;
	int alarm_occured;
	struct sockaddr_in serveraddr, clientaddr, proxyaddr;
	struct hostent *localhost;
	struct in_addr hostaddr;
	int userlen, passlen;

	portstartrange = configuration.dccstartport;
	portendrange = configuration.dccendport;
			
	if (D == NULL) return (0);
	if (D->server_status == 0) D->proxy = configuration.proxy;

	if (D->proxy == PROXY_DIRECT){

		/* if not yet done, configure and bind the socket to listen */
		if (D->server_status == 0){
			D->dccfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->dccfd == -1) {
				vprint_all_attrib(ERROR_COLOR, 
					"Error (%d %s) opening DCC send server socket for filename %s.\n", 
					errno, strerror(errno), D->filename);
				D->server_status = -1;
				return(0);
        		}

			/* find the next available free port */
			for (port = portstartrange; port <= portendrange; port++){
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_port = htons(port);
				serveraddr.sin_addr.s_addr = INADDR_ANY;

				memset(&(serveraddr.sin_zero), 0, 8);

				berr = bind(D->dccfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
			
				/* if port in already in use, try the next one in the range */
				if (berr == -1 && errno == EADDRINUSE){}
				else if (berr == -1){
					vprint_all_attrib(ERROR_COLOR, "Error (%d %s) binding DCC send server socket.\n", 
						errno, strerror(errno));
				}
				else{
					berr = 0;
					break;		
				}
			}
			if (berr == -1){
				D->server_status = -1;
				return(-1);
			}

			serr = fcntl(D->dccfd, F_SETFL, O_NONBLOCK);
		        listen(D->dccfd, 1);

			D->server_status = 1;
			D->localport = ntohs(serveraddr.sin_port);
			D->connectport = D->localport;

			localhost = gethostbyname(configuration.dcchostname);
			if (localhost == NULL){
				vprint_all_attrib(ERROR_COLOR, "Error starting DCC send: lookup for \"%s\" failed\n", 
					configuration.dcchostname);
				return(0);
			}

			vprint_all_attrib(DCC_COLOR, "Using %s (%s) for DCC source host\n", configuration.dcchostname,
				inet_ntoa(*((struct in_addr*)localhost->h_addr)));

			D->localip = htonl(((struct in_addr *)(localhost->h_addr))->s_addr);
			D->connectip = D->localip;

			vprint_all_attrib(DCC_COLOR, "DCC send server listening on port %d\n", ntohs(serveraddr.sin_port));
			vprint_all_attrib(DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);
			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC SEND", "%s %lu %d %d",
                        D->shortfilename, D->connectip, D->connectport, D->size), D->nick, "");

			return(1);
		}

		/* once server initialization is complete, wait for new connection */
		else if (D->server_status == 1){
		        size = sizeof(struct sockaddr_in);
		        new_fd = accept(D->dccfd, (struct sockaddr *)&clientaddr, &size);
			if (new_fd != -1){	
				vprint_all_attrib(DCC_COLOR, "Accepting DCC connection from %s port %d\n", 
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				/* at this point close the listening socket and replace it with the active one */
				close(D->dccfd);
				D->dccfd = new_fd;
				D->active = 1;
				time(&(D->starttime));
				time(&(D->last_activity_at));
				time(&(D->last_updated_at));
				return(1);
			}		
		}
	}
	else if (D->proxy == PROXY_SOCKS4){
		if (D->server_status == 0){
			host = gethostbyname(configuration.dcchostname);
			if (host == NULL){
				vprint_all_attrib(ERROR_COLOR, 
					"Error starting DCC chat: lookup for \"%s\" failed\n", configuration.dcchostname);
				return(0);
			}
	
			vprint_all_attrib(DCC_COLOR, "Using %s (%s) for DCC source host\n", 
				configuration.dcchostname, inet_ntoa(*((struct in_addr*)host->h_addr)));

			D->localport = ntohs(serveraddr.sin_port);
			D->localip = htonl(((struct in_addr *)(host->h_addr))->s_addr);

			D->server_status = 1;
			return(1);

		}
		/* obtain proxy permission to connect to bind */
		if (D->server_status == 1){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_all_attrib(ERROR_COLOR, "DCC Chat: Cannot create a socket to proxy\n");
				return(0);
			}
			else{
				vprint_all_attrib(DCC_COLOR, "Looking up SOCKS4 proxy %s ...\n", 
					configuration.proxyhostname);
				D->server_status = 2;
				return(1);
			}
                }
		else if (D->server_status == 2){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_all_attrib(ERROR_COLOR, "Error: Cannot find SOCKS4 proxy %s\n", 
					configuration.proxyhostname);
                        	D->server_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_all_attrib(DCC_COLOR, "Connecting to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 3;
                        	return(1);
                	}
		}
		else if (D->server_status == 3){
		        proxyaddr.sin_family = AF_INET;     
		        proxyaddr.sin_port = htons(configuration.proxyport);
		        proxyaddr.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(proxyaddr.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);
			if (connect (D->proxyfd, (struct sockaddr *)&proxyaddr, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_all_attrib(ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
                        	vprint_all_attrib(DCC_COLOR, "Connected to SOCKS4 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 4;
				return(1);
			}
		}

		else if (D->server_status == 4){
			hostiph = ntohl(D->localip);
			hostaddr.s_addr = hostiph;

			vprint_all_attrib(DCC_COLOR, "Requesting bind to %s port %d from proxy ...\n", 
				inet_ntoa(hostaddr), D->localport);

			/* create SOCK4 connect message */
			buffer[0] = 4;
			buffer[1] = 2; /* BIND */
			buffer[2] = (D->localport & 0xff00) >> 8;
			buffer[3] = (D->localport & 0xff);
			memcpy(&buffer[4], &hostiph, 4);
			strcpy(&buffer[8], configuration.proxyusername);
			send_ball(D->proxyfd, buffer, 8 + strlen(buffer + 8) + 1);

			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 5;
			return(1);
		}

		else if (D->server_status == 6){
			vprint_all_attrib(DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);

			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC SEND", "%s %lu %d %d",
                        D->shortfilename, D->connectip, D->connectport, D->size), D->nick, "");


			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 7;
			time(&(D->starttime));
			time(&(D->last_activity_at));
			time(&(D->last_updated_at));

			return(1);
		}
		return(0);
	}

	else if (D->proxy == PROXY_SOCKS5){
		if (D->server_status == 0){

			host = gethostbyname(configuration.dcchostname);
			if (host == NULL){
				vprint_all_attrib(ERROR_COLOR, 
					"Error starting DCC chat: lookup for \"%s\" failed\n", configuration.dcchostname);
				return(0);
			}
	
			vprint_all_attrib(DCC_COLOR, "Using %s (%s) for DCC source host\n", 
				configuration.dcchostname, inet_ntoa(*((struct in_addr*)host->h_addr)));

			D->localport = ntohs(serveraddr.sin_port);
			D->localip = htonl(((struct in_addr *)(host->h_addr))->s_addr);

			D->server_status = 1;
			return(1);

		}
		/* obtain proxy permission to connect to bind */
		if (D->server_status == 1){
			D->proxyfd = socket(AF_INET, SOCK_STREAM, 0);
			if (D->proxyfd == -1) {
				vprint_all_attrib(ERROR_COLOR, "DCC Chat: Cannot create a socket to proxy\n");
				return(0);
			}
			else{
				vprint_all_attrib(DCC_COLOR, "Looking up SOCKS5 proxy %s ...\n", 
					configuration.proxyhostname);
				D->server_status = 2;
				return(1);
			}
                }
		else if (D->server_status == 2){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
                        	vprint_all_attrib(ERROR_COLOR, "Error: Cannot find SOCKS5 proxy %s\n", 
					configuration.proxyhostname);
                        	D->server_status = -1;
                        	return(0);
                	}
                	else {
                        	vprint_all_attrib(DCC_COLOR, "Connecting to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 3;
                        	return(1);
                	}
		}
		else if (D->server_status == 3){
		        proxyaddr.sin_family = AF_INET;     
		        proxyaddr.sin_port = htons(configuration.proxyport);
		        proxyaddr.sin_addr = *((struct in_addr *)host->h_addr);
		        bzero(&(proxyaddr.sin_zero), 8);     

			alarm_occured=0;
			alarm(CONNECTTIMEOUT);
			if (connect (D->proxyfd, (struct sockaddr *)&proxyaddr, sizeof(struct sockaddr)) < 0 
				|| alarm_occured){
				alarm_occured=0;	
				vprint_all_attrib(ERROR_COLOR, "DCC Error while connecting to %s: %s\n", 
					inet_ntoa(*((struct in_addr*)host->h_addr)), strerror(errno));
                        	D->connect_status = -1;
				D->server_status = -1;
				return(0);
		        }
			else {
                        	vprint_all_attrib(DCC_COLOR, "Connected to SOCKS5 proxy %s (%s) port %d ...\n",
                                	host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
                        	D->server_status = 4;
				fcntl(D->proxyfd, F_SETFL, O_NONBLOCK);
				return(1);
			}
		}

		else if (D->server_status == 4){
			vprint_all_attrib(DCC_COLOR, "Negotiating authentication methods ...\n");
			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* NUMBER OF METHODS */
			buffer[2] = 0; /* NO AUTHENTICATION */
			buffer[3] = 2; /* USERNAME / PASSWORD */
			buffer[4] = 1; /* GSS API */
			send_ball(D->proxyfd, buffer, 4);

			/* at this point, wait until the proxy replies and advances status */
			D->server_status = 5;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns username authentication advance connect_status to 8 */
		else if (D->connect_status == 8){
			vprint_all_attrib(DCC_COLOR, "Sending username and password ...\n");

			/* create username / password message */
			userlen = (unsigned char) strlen(configuration.proxyusername);
			passlen = (unsigned char) strlen(configuration.proxypassword);

			buffer[0] = 1; /* METHOD VERSION */
			buffer[1] = userlen; /* USERLEN */
			buffer[2 + userlen] = passlen; /* PASSLEN */
			memcpy(&buffer[2], configuration.proxyusername, userlen); /* USERNAME */
			memcpy(&buffer[3 + userlen], configuration.proxypassword, passlen); /* PASSWORD */
			send_ball(D->proxyfd, buffer, 3 + userlen + passlen);

			/* at this point, wait until the proxy replies and advances status */
			D->connect_status = 9;
			D->proxyactive = 1;
			return(1);
		}

		/* if proxy returns no authentication skip to connect_status 10 */
		else if (D->server_status == 10){
			hostiph = ntohl(D->localip);
			hostaddr.s_addr = hostiph;

			vprint_all_attrib(DCC_COLOR, "Requesting bind to %s port %d from proxy ...\n", 
				inet_ntoa(hostaddr), D->localport);

			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 2; /* BIND */
			buffer[2] = 0; /* RESERVED */
			buffer[3] = 1; /* ADDRESS TYPE - IPv4 */
			memcpy(&buffer[4], &hostiph, 4);
			buffer[8] = (D->localport & 0xff00) >> 8;
			buffer[9] = (D->localport & 0xff);
			send_ball(D->proxyfd, buffer, 10);

			/* at this point, wait until the proxy replies */
			D->server_status = 11;
			D->proxyactive = 1;
			return(1);
		}
		else if (D->server_status == 12){
			vprint_all_attrib(DCC_COLOR, "Awaiting connection from %s ...\n", D->nick);
			sendcmd_server(D->server, "PRIVMSG", create_ctcp_command("DCC SEND", "%s %lu %d %d",
                        D->shortfilename, D->connectip, D->connectport, D->size), D->nick, "");

			/* at this point, wait until the proxy replies */
			D->proxyactive = 1;
			D->server_status = 13;
			time(&(D->starttime));
			time(&(D->last_activity_at));
			time(&(D->last_updated_at));

			return(1);
		}
		return(0);
	}
	return(0);
}

int get_dcc_file(dcc_file *D){
	char buffer[MAXDCCPACKET];
	int len;
	unsigned char ack[5];

	if (D->type == DCC_RECEIVE){
		len = recv(D->dccfd, buffer, MAXDCCPACKET, 0);
		if (len > 0){
			//vprint_all("DCC %d byte(s) recvd\n", len);
			D->byte += len;

#ifdef LARGEFILES
			if(write(D->filefd, buffer, len) != len)
			{
				vprint_all_attrib(ERROR_COLOR, "Error %d, %s writing to file %s. Closing connection...\n", 
					errno, strerror(errno), D->filename);
				remove_dcc_file(D);		
				return(-1);
            		}
#else
			fwrite(buffer, len, 1, D->dccfp);
			if (ferror(D->dccfp)){
				vprint_all_attrib(ERROR_COLOR, "Error %d, %s writing to file %s. Closing connection...\n", 
					errno, strerror(errno), D->filename);
				remove_dcc_file(D);		
				return(-1);
			}		
#endif			
			time(&(D->last_activity_at));
			gen_dccack(D->byte, ack);
			send(D->dccfd, ack, 4, 0);
			
		}
		else if (len == 0){
			if (D->byte == D->size){
				gen_dccack(D->byte, ack);
				send(D->dccfd, ack, 4, 0);
			}
			else{
				vprint_all_attrib(ERROR_COLOR, "Error %d, %s while receiving %s. Closing connection...\n", 
				errno, strerror(errno), D->filename);
				remove_dcc_file(D);		
				return(-1);
			}
		}
		else if (len == -1){
			//fprintf(stderr, "DCC read would block\r\n");
			gen_dccack(D->byte, ack);
			send(D->dccfd, ack, 4, 0);
			return(0);			
		}
	}		
	if (D->byte >= D->size) D->active=0;
	return(len);
}

int put_dcc_file(dcc_file *D){
	struct timeval timeout;      	
	fd_set writefds;
	char buffer[MAXDCCPACKET];
	long numbytes;
	int serr, len;
	
	len=0;

	if (D->type == DCC_SEND){

#ifdef LARGEFILES
		numbytes = read(D->filefd, buffer, MAXDCCPACKET);
		if (numbytes < 0){
			vprint_all_attrib(ERROR_COLOR, "Error %d, %s reading file %s. Closing connection...\n", 
				 errno, strerror(errno), D->filename);
			remove_dcc_file(D);		
			return(-1);
		}			
#else
		numbytes = fread(buffer, 1, MAXDCCPACKET, D->dccfp);				
		if (feof(D->dccfp)){
			// return(0);
		}
		else if (ferror(D->dccfp)){
			vprint_all_attrib(ERROR_COLOR, "Error %d, %s reading file %s. Closing connection...\n", 
				 errno, strerror(errno), D->filename);
			remove_dcc_file(D);		
			return(-1);
		}			

#endif
		if (numbytes > 0){
			len = send(D->dccfd, buffer, numbytes, 0);
			// vprint_all("Sent %d bytes %s\n", len, D->filename);
		
			/* if the entire packet was written, nothing else needs to be done for now */
			if (len == numbytes) D->byte += len;

			/* nothing was sent */
			else if (len == -1){
				vprint_all_attrib(ERROR_COLOR, "DCC error sending %s: %d %s\n", D->filename, errno, strerror(errno));
				remove_dcc_file(D);		
				return(-1);			
			}
			/* if the packet wasn't completed, rewind to the last sent byte */
			else{
				D->byte += len;
#ifdef LARGEFILES
				fseek(D->dccfp, D->byte, SEEK_SET);
#else
				lseek(D->filefd, D->byte, SEEK_SET);
#endif
			}
			time(&(D->last_activity_at));
		}
	}

	//if (D->byte >= D->size) D->active=0;
	return(len);
}

void gen_dccack(unsigned long byte, unsigned char *ack){
	bzero(ack, 5);
	ack[3]=byte&0xff;	
	ack[2]=(byte&0xff00)>>8;
	ack[1]=(byte&0xff0000)>>16;
	ack[0]=(byte&0xff000000)>>24;
	//vprint_all("Sending DCC ack %d, %d, %d, %d: value %lu\n", ack[3], ack[2], ack[1], ack[0], byte);
}

int get_dccack(dcc_file *D){
	unsigned char ackbuffer[5];
	int len;
	unsigned long ack;

	// vprint_all("looking for ack\n", ack);

	if (D->type == DCC_SEND){
		/* get the 4 ack bytes */
		len = recv(D->dccfd, ackbuffer, 4, 0);
		if (len == 4){

			ack = ackbuffer[3];
			ack |= ackbuffer[2] << 8;
			ack |= ackbuffer[1] << 16;
			ack |= ackbuffer[0] << 24;
			
			D->ackbyte = ack;
			//vprint_all("Received DCC ack %d, %d, %d, %d: value %lu\n", ackbuffer[3], ackbuffer[2], ackbuffer[1], ackbuffer[0], ack);			
			return(1);

		}
		else if (len > 0){
			vprint_all_attrib(ERROR_COLOR, "Received %d byte DCC ack for %s. Closing connection...\n", len, D->filename);
			remove_dcc_file(D);		
			return(-1);
		}			
		else{
			vprint_all_attrib(ERROR_COLOR, "Error %d, %s receiving DCC ack for %s. Closing connection...\n", 
			errno, strerror(errno), D->filename);
			remove_dcc_file(D);		
			return(-1);
		}
	}
	return(-1);

}

int dcc_file_exists(transfer *T, dcc_file *D){
	dcc_file *current;

	if (D == NULL || T == NULL) return(0);

	current = T->dcclist;
	while(current != NULL){
		if (current == D) return(1);
		current = current->next;
	}
	return(0);
}

void remove_dcc_file(dcc_file *D){
	if (D == NULL) return;

	if (D->dccfp != NULL) fclose (D->dccfp);
	if (D->filefd != 0)
	{
        	if (D->type == DCC_RECEIVE && fsync(D->filefd) < 0) 
		    	vprint_all_attrib(ERROR_COLOR, "Error fsync'ing %s: %s\n", D->filename, strerror(errno));
      		close(D->filefd);     
	}
     
	if (D->dccfd != 0) close (D->dccfd);
	if (D->proxyfd != 0) close (D->proxyfd);

	if (D->next == NULL && D->prev == NULL){
		((transfer *)D->transfer)->dcclist = NULL;
	}
	else if (D->prev == NULL){
		((transfer *)D->transfer)->dcclist = D->next;
		((dcc_file *)D->next)->prev = NULL;
	}
	else if (D->next == NULL){
		((dcc_file *)D->prev)->next = NULL;
	}
	else{
		((dcc_file *)D->prev)->next = D->next;
		((dcc_file *)D->next)->prev = D->prev;
	}
	(D->transfer)->dcclisttop = (D->transfer)->dcclist;
	(D->transfer)->selectedfile = (D->transfer)->dcclist;

	free(D);
}

void remove_dcc_chat(dcc_chat *D){
	D->direction=-1;
	D->active=0;
	D->proxyactive = 0;
	D->connect_status = -1;
	D->server_status = -1;
	if (D->dccfd != 0) close (D->dccfd);
	if (D->proxyfd != 0) close (D->proxyfd);
	D->dccfd = -1;
	D->proxyfd = -1;
}

