// frontend.h: interface between frontends and scanbuttond server
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

#ifndef __SCANBTND_FRONTEND_H_INCLUDED
#define __SCANBTND_FRONTEND_H_INCLUDED

#include <time.h>
#include <errno.h>

// custom errno values
#define SCANBTND_EBASE		1024
#define SCANBTND_EGENERIC	SCANBTND_EBASE + 1
#define SCANBTND_ESOCKET	SCANBTND_EBASE + 2
#define SCANBTND_ENOHOST	SCANBTND_EBASE + 3
#define SCANBTND_ECONNECT	SCANBTND_EBASE + 4
#define SCANBTND_ENOMEM		SCANBTND_EBASE + 5
#define SCANBTND_ESEND		SCANBTND_EBASE + 6
#define SCANBTND_ERECV		SCANBTND_EBASE + 7
#define SCANBTND_ECONNREFUSED	SCANBTND_EBASE + 8


struct scanbtnd_link;
typedef struct scanbtnd_link scanbtnd_link_t;

struct scanbtnd_event;
typedef struct scanbtnd_event scanbtnd_event_t;

struct scanbtnd_device;
typedef struct scanbtnd_device scanbtnd_device_t;

struct scanbtnd_device_list_item;
typedef struct scanbtnd_device_list_item scanbtnd_device_list_item_t;

struct scanbtnd_lock_info;
typedef struct scanbtnd_lock_info scanbtnd_lock_info_t;


struct scanbtnd_link 
{
	int socket;
	char* servername;
	int port;
	int flags;
	scanbtnd_device_list_item_t* devices;
	int filter_policy;
	scanbtnd_device_list_item_t* filter_exceptions;
	time_t last_device_update_time;
};

#define SCANBTND_EVENT_BUTTON_PRESSED 		1
#define SCANBTND_EVENT_BUTTON_RELEASED		2
#define SCANBTND_EVENT_DEVICE_ADDED		3
#define SCANBTND_EVENT_DEVICE_REMOVED		4
#define SCANBTND_EVENT_LOCK_ACQUIRED		5
#define SCANBTND_EVENT_LOCK_LOST		6
#define SCANBTND_EVENT_CRITICAL_OVERFLOW	7

struct scanbtnd_event
{
	int type;
	time_t time;
	scanbtnd_device *device;
	int button;
};

struct scanbtnd_device
{
	char* manufacturer;
	char* model;
	char* sane_identifier;
	int interface;
};

struct scanbtnd_device_list_item
{
	scanbtnd_device_t* device;
	scanbtnd_device_list_item_t* next;
};

struct scanbtnd_lock_info
{
	scanbtnd_device_t* device;
	time_t acquire_time;
	time_t expiration_time;
	char* client_location;
	char* client_username;
};

// --------------------------
// Connecting / Disconnecting
// --------------------------

/**
 * Connects to a scanbuttond server.
 * @return the link descriptor, or NULL in case of failure
 */
scanbtnd_link_t* scanbtnd_connect(const char* servername, int port, int flags);

/**
 * Disconnects from a scanbuttond server.
 * @return 0 in case of success, -1 in case of failure
 */
void scanbtnd_disconnect(scanbtnd_link_t* link);

// -------------------------
// Listing available devices
// -------------------------

/**
 * Lists all devices known to the scanbtnd server.
 * @return a linked list of devices known by the server.
 * The list may not be up to date due to buffering.
 * You must not free this pointer, it's also used internally.
 */
const scanbtnd_device_list_item_t* scanbtnd_get_devices(scanbtnd_link_t* link);

/**
 * Forces an update of the device list.
 * @return the number of devices which have been added or removed
 */
int scanbtnd_update_devices(scanbtnd_link_t* link);

// ---------------
// Event filtering
// ---------------

#define SCANBTND_FILTER_POLICY_DEFAULT	0
#define SCANBTND_FILTER_POLICY_ALLOW 	0
#define SCANBTND_FILTER_POLICY_REJECT 	1

/**
 * Sets the event filtering policy.
 * @param policy either SCANBTND_FILTER_POLICY_ALLOW or
 *               or SCANBTND_FILTER_POLICY_REJECT
 */
void scanbtnd_set_filter_policy(scanbtnd_link_t* link, int policy);

/**
 * Gets the current filtering policy.
 * @return the filtering policy
 */
int scanbtnd_get_filter_policy(scanbtnd_link_t* link);

/**
 * Adds an exception to the filter policy.
 * If the filter policy is ACCEPT, events from the given device will be ignored.
 * If the filter policy is REJECT, events from the given device will be reported.
 */
void scanbtnd_add_filter_exception(scanbtnd_link_t* link, scanbtnd_device_t* device);

/**
 * Removes an exception from the filter policy.
 */
void scanbtnd_remove_filter_exception(scanbtnd_link_t* link, scanbtnd_device_t* device);

/**
 * Clears all filter policy exceptions.
 */
void scanbtnd_reset_filter_exceptions(scanbtnd_link_t* link);

/**
 * Lists all exceptions.
 * @return a linked list of all devices which are excluded from the filter policy.
 * You must not free this pointer, it's also used internally!
 */
const scanbtnd_device_list_item_t* scanbtnd_get_filter_exceptions(scanbtnd_link_t* link);

// --------------
// Event Handling
// --------------

/**
 * Gets the number of pending (=queued) events.
 */
int scanbtnd_event_get_num_pending(scanbtnd_link_t* link);

/**
 * Gets the next event from the queue.
 * @return the next queued event, or NULL if there is no queued event
 */
scanbtnd_event_t* scanbtnd_event_get(scanbtnd_link_t* link);

// --------------
// Device Locking
// --------------

/**
 * Tries to lock the scanner device for a maximum of millis milliseconds.
 * The scanbuttond server grants you exclusive access to a locked device, however
 * this does not mean that other applications (e.g. SANE) are also blocked from
 * using the device.
 * @return 0 in case of success, -1 in case of error
 */
int scanbtnd_lock_device(scanbtnd_link_t* link, scanbtnd_device_t* device, unsigned long millis);

/**
 * Unlocks the given scanner.
 * Does nothing if the scanner is already unlocked.
 */
void scanbtnd_unlock_device(scanbtnd_link_t* link, scanbtnd_device_t* device);

/**
 * Gets locking information from this scanner.
 */
scanbtnd_lock_info_t* scanbtnd_get_locking_info(scanbtnd_link_t* link, scanbtnd_device_t* device);


#endif
