// protocol.h: scanbuttond network protocol definition
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

#ifndef __SCANBTND_PROTOCOL_H_INCLUDED
#define __SCANBTND_PROTOCOL_H_INCLUDED

// --------
// Requests
// --------

//#define SCANBTND_PROTOCOL_CONNECT		1
//#define SCANBTND_PROTOCOL_DISCONNECT		2
#define SCANBTND_PROTOCOL_GET_DEVICES		3
#define SCANBTND_PROTOCOL_SET_FILTER_POLICY	4
#define SCANBTND_PROTOCOL_ADD_FILTER_EXC	5
#define SCANBTND_PROTOCOL_REMOVE_FILTER_EXC	6
#define SCANBTND_PROTOCOL_RESET_FILTER_EXC	7
#define SCANBTND_PROTOCOL_GET_NUM_QD_EVENTS	8
#define SCANBTND_PROTOCOL_GET_EVENT		9
#define SCANBTND_PROTOCOL_LOCK_DEVICE		10
#define SCANBTND_PROTOCOL_UNLOCK_DEVICE		11
#define SCANBTND_PROTOCOL_GET_LOCK_STATUS	12


// ---------
// Responses
// ---------

#define SCANBTND_PROTOCOL_ACK			128
#define SCANBTND_PROTOCOL_NAK			129

#endif
