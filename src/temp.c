	else if (S->proxy == PROXY_SOCKS5){
	        // use the SOCKS5 proxy rather than a direct connection 

		if (S->connect_status == 0){
			S->active = 0;
			S->proxyactive = 0;
			S->serverfd = -1;
			S->proxyfd = -1;

			strcpy(S->user, configuration.user);
			strcpy(S->host, configuration.hostname);
			strcpy(S->domain, configuration.domain);
			strcpy(S->name, configuration.userdesc);
			strcpy(S->nick, configuration.nick);
			strcpy(S->lastnick, configuration.nick);

			if ((S->proxyfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				vprint_server_attrib(S, ERROR_COLOR, "Error: Cannot create a socket\n");
				S->connect_status = -1;
				return(0);
			}
			else{
				vprint_server_attrib(S, MESSAGE_COLOR, "Looking up SOCKS5 proxy %s ...\n", 
					configuration.proxyhostname);
				S->connect_status = 1;
				return(1);
			}
		}

		else if (S->connect_status == 1){
			if ((host = gethostbyname(configuration.proxyhostname)) == NULL) {
				vprint_server_attrib(S, ERROR_COLOR, "Error: Cannot find SOCKS5 proxy %s\n", 
					configuration.proxyhostname);
				S->connect_status = -1;
				return(0);
			}
			else {
				vprint_server_attrib(S, MESSAGE_COLOR, "Connecting to SOCKS5 proxy %s (%s) port %d ...\n", 
					host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), 
					configuration.proxyport);
				S->connect_status = 2;
				return(1);
			}
		}

		else if (S->connect_status == 2){
			their_addr.sin_family = AF_INET;                                            
			their_addr.sin_port = htons(configuration.proxyport); 
			their_addr.sin_addr = *((struct in_addr *)host->h_addr);
			bzero(&(their_addr.sin_zero), 8);
		        if (connect (S->proxyfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
				vprint_server_attrib(S, ERROR_COLOR, "Failed to connect to proxy %s (%s) on port %d ...\n", 
					host->h_name, inet_ntoa(*((struct in_addr *)host->h_addr)), 
					configuration.proxyport);
				S->connect_status = -1;
				return(0);
			}
			vprint_server_attrib(S, MESSAGE_COLOR, "Connected to proxy %s (%s) on port %d ...\n", 
				host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), configuration.proxyport);
			vprint_server_attrib(S, MESSAGE_COLOR, "Looking up IRC server %s ...\n", S->server);
			S->connect_status = 3;
			S->proxyactive = 1;
			return(1);
		}

		else if (S->connect_status == 3){
			if ((host = gethostbyname(S->server)) == NULL) {
				vprint_server_attrib(S, ERROR_COLOR, "Error: Cannot find server %s\n", S->server);
				S->connect_status = -1;
				return(0);
			}
			else {
				vprint_server_attrib(S, MESSAGE_COLOR, "Negotiating authentication methods ...\n");
				/* create SOCK5 connect message */
				buffer[0] = 5;
				buffer[1] = 2; /* NUMBER OF METHODS */
				buffer[2] = 0; /* NO AUTHENTICATION */
				buffer[3] = 2; /* USERNAME / PASSWORD */
				buffer[4] = 1; /* GSS API */
				send_ball(S->proxyfd, buffer, 4);

				/* at this point, wait until the proxy replies and advances status */
				S->connect_status = 4;
				return(1);
			}
		}

		else if (S->connect_status == 8){
			vprint_server_attrib(S, MESSAGE_COLOR, "Sending username and password ...\n");

			/* create username / password message */
			userlen = (unsigned char) strlen(configuration.proxyusername);
			passlen = (unsigned char) strlen(configuration.proxypassword);

			buffer[0] = 1; /* METHOD VERSION */
			buffer[1] = userlen; /* USERLEN */
			buffer[2 + userlen] = passlen; /* PASSLEN */
			memcpy(&buffer[2], configuration.proxyusername, userlen); /* USERNAME */
			memcpy(&buffer[3 + userlen], configuration.proxypassword, passlen); /* PASSWORD */
			send_ball(S->proxyfd, buffer, 3 + userlen + passlen);

			/* at this point, wait until the proxy replies and advances status */
			S->connect_status = 9;
			return(1);
		}

		else if (S->connect_status == 10){
			if ((host = gethostbyname(S->server)) == NULL) {
				vprint_server_attrib(S, ERROR_COLOR, "Error: Cannot find server %s\n", S->server);
				S->connect_status = -1;
				return(0);
			}
			else {
				vprint_server_attrib(S, MESSAGE_COLOR, "Connecting via proxy to %s (%s) port %d ...\n", 
					host->h_name, inet_ntoa(*((struct in_addr*)host->h_addr)), S->port);
				S->connect_status = 11;
				return(1);
			}
		}

		else if (S->connect_status == 11){
			vprint_server_attrib(S, MESSAGE_COLOR, "Requesting connection to %s from proxy ...\n", S->server);

			/* create SOCK5 connect message */
			buffer[0] = 5;
			buffer[1] = 1; /* CONNECT */
			buffer[2] = 0; /* RESERVED */
			buffer[3] = 1; /* ADDRESS TYPE - IPv4 */
			memcpy(&buffer[4], (struct in_addr*)host->h_addr, 4);
			buffer[8] = (S->port & 0xff00) >> 8;
			buffer[9] = (S->port & 0xff);
			send_ball(S->proxyfd, buffer, 10);

			/* at this point, wait until the proxy replies */
			S->connect_status = 12;
		}

		if (S->connect_status == 13){
			vprint_server_attrib(S, MESSAGE_COLOR, "Logging on as %s ...\n", S->nick);

			S->nickinuse = 1;
			sprintf(buffer,"USER %s %s %s :%s\n",S->user, S->host, S->domain, S->name);
			send_all(S->serverfd, buffer, strlen(buffer));
			if (strlen(S->password)){
				sprintf(buffer,"PASS %s\n", S->password);
				send_all(S->serverfd, buffer, strlen(buffer));
			}
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
	return(0);
}	

