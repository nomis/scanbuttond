// libfrontend.c: frontend library
// This file is part of scanbuttond.
// Copyleft )c( 2006 by Bernhard Stiftner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 

#include "frontend.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define MAX_DEV_BUFFER_TIME	30
#define DEFAULT_SOCKET_TIMEOUT		2000

// -------
// helpers
// -------

void scanbtnd_free_device(scanbtnd_device_t* device) 
{
	// TODO: there's something missing...
	free(device);
}

void scanbtnd_free_device_list(scanbtnd_device_list_item_t* root)
{
	scanbtnd_device_list_item_t* dev_item = root;
	scanbtnd_device_list_item_t* next_item;

	while (dev_item != NULL) {
		next_item = dev_item->next;
		free_device(dev_item->device);
		free(dev_item);
		dev_item = next_item;
	}
}

int scanbtnd_perform_request(scanbtnd_link_t* link, char* send_buf, int send_len, char* recv_buf, int recv_len)
{
	int num_bytes;
	int corresponding_packet_received = 0;

	while (!corresponding_packet_received) {
		num_bytes = send(link->socket, (void*)send_buf, send_len, 0);
		if (num_bytes != send_len) {
			errno = SCANBTND_ESEND;
			return -1;
		}
		num_bytes = recv(link->socket, (void*)recv_buf, recv_len, 0);
		if (num_bytes >= 2) {
			if (recv_buf[1] == send_buf[0] &&
			     (recv_buf[0] == SCANBTND_PROTOCOL_ACK || 
                             recv_buf[0] == SCANBTND_PROTOCOL_NAK))
				corresponding_packet_received = 1;
		}
	}
	return num_bytes;
}

// -------------
// API functions
// -------------

scanbtnd_link_t* scanbtnd_connect(const char* servername, int port, int flags)
{
	int s;
	struct hostent* host;
	struct sockaddr_in srv;
	scanbtnd_link_t* link;
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		errno = SCANBTND_ESOCKET;
		return NULL;
	}
	
	host = gethostbyname(servername);
	if (host->h_addr_list[0] == 0) {
		errno = SCANBTND_ENOHOST;
		return NULL;
	}
	
	srv.sin_addr = *(struct in_addr*) host->h_addr_list[0];
	srv.sin_port = htons( (unsigned short int) atol(port));
	srv.sin_family = AF_INET;
	
	if (connect(s, (struct sockaddr*) &srv, sizeof(srv)) < 0) {
		errno = SCANBTND_ECONNECT;
		return NULL;
	}
	
	scanbtnd_set_socket_timeout(s, DEFAULT_SOCKET_TIMEOUT);
	
	link = (scanbtnd_link_t*)malloc(sizeof(scanbtnd_link_t));
	if (link == NULL) {
		errno = SCANBTND_ENOMEM;
		close(s);
		return NULL;
	}
	link->socket = s;
	link->servername = (char*)malloc(strlen(servername)+1);
	if (link->servername == NULL) {
		errno = SCANBTND_ENOMEM;
		close(s);
		free(link);
		return NULL;
	}
	strcpy(link->servername, servername);
	link->port = port;
	link->flags = flags;
	link->devices = NULL;
	link->filter_policy = SCANBTND_FILTER_POLICY_DEFAULT;
	link->filter_exceptions = NULL;
	link->last_device_update_time = 0;
	
	return link;
}

void scanbtnd_disconnect(scanbtnd_link_t* link) 
{
	close(link->socket);
	scanbtnd_free_device_list(link->devices);
	scanbtnd_free_device_list(link->filter_exceptions);
	free(link->servername);
	free(link);
}

const scanbtnd_device_list_item_t* scanbtnd_get_devices(scanbtnd_link_t* link) 
{
	if (difftime(time(NULL), link->last_device_update_time) > MAX_DEV_BUFFER_TIME) {
		scanbtnd_update_devices(link);
	}
	return devices;
}

int scanbtnd_update_devices(scanbtnd_link_t* link)
{
	scanbtnd_free_device_list(link->devices);
	link->devices = NULL;

	// TODO: something's missing here...

	last_device_update_time = time(NULL);
}

void scanbtnd_set_filter_policy(scanbtnd_link_t* link, int policy);

int scanbtnd_get_filter_policy(scanbtnd_link_t* link);

void scanbtnd_add_filter_exception(scanbtnd_link_t* link, scanbtnd_device_t* device);

void scanbtnd_remove_filter_exception(scanbtnd_link_t* link, scanbtnd_device_t* device);

void scanbtnd_reset_filter_exceptions(scanbtnd_link_t* link);

const scanbtnd_device_list_item_t* scanbtnd_get_filter_exceptions(scanbtnd_link_t* link);

int scanbtnd_event_get_num_pending(scanbtnd_link_t* link);

scanbtnd_event_t* scanbtnd_event_get(scanbtnd_link_t* link);

int scanbtnd_lock_device(scanbtnd_link_t* link, scanbtnd_device_t* device, unsigned long millis);

void scanbtnd_unlock_device(scanbtnd_link_t* link, scanbtnd_device_t* device);

scanbtnd_lock_info_t scanbtnd_get_locking_info(scanbtnd_link_t* link, scanbtnd_device_t* device);
