//
// Epson GT-9300 scanner button daemon
// Copyleft )c( 2004 by Bernhard Stiftner
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
// along with this program;a if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
//
//

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <usb.h>

#define SCRIPT		"/root/scanner/buttonpressed.sh %d"
#define	ESC        	0x1B		/* ASCII value for ESC */
#define TIMEOUT	   	30 * 1000	/* 30 seconds */
#define POLL_DELAY	500*1000	/* poll twice per second */

#define NUM_SUPPORTED_DEVICES 1
static int supported_devices[NUM_SUPPORTED_DEVICES][2] = {
	{ 0x04B8, 0x011B }	// Epson GT-9300 (aka Perfection 2400)
};

struct usb_device* scanner_device;
struct usb_dev_handle* scanner_handle;
int usb_interface = 0;
int usb_in_ep = 0;
int usb_out_ep = 0;
int killed = 0;


// Returns the scanner device
struct usb_device* get_scanner_device(void) {
  int i;
  struct usb_bus *bus;
  struct usb_device *device;
  struct usb_interface_descriptor *interface;
  
  bus = usb_busses;
  while (bus != NULL) {
    device = usb_busses->devices;
    while (device != NULL) {
      for (i=0; i<NUM_SUPPORTED_DEVICES; i++) {
        if (device->descriptor.idVendor == supported_devices[i][0] &&
          device->descriptor.idProduct == supported_devices[i][1]) {
          
          interface = &device->config[0].interface->altsetting[0];

          /* Now we look for usable endpoints */
          int num;
          for (num = 0; num < interface->bNumEndpoints; num++) {
            struct usb_endpoint_descriptor *endpoint;
            int address, direction, transfer_type;
            
            endpoint = &interface->endpoint[num];
            address = endpoint->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
            direction = endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK;
            transfer_type = endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK;
          
            /* save the endpoints we need later */
            if (transfer_type == USB_ENDPOINT_TYPE_BULK) {
              if (direction) {	/* in */
                if (!usb_in_ep)
                  usb_in_ep = endpoint->bEndpointAddress;
              } else {	/* out */
                if (!usb_out_ep)
                  usb_out_ep = endpoint->bEndpointAddress;
              }
            }
          }
          
          /* look for suitable interfaces */
          int found = 0;
          for (usb_interface = 0; usb_interface < device->config[0].bNumInterfaces && !found; usb_interface++) {
            switch (device->descriptor.bDeviceClass) {
              case USB_CLASS_VENDOR_SPEC:
                found = 1;
                break;
              case USB_CLASS_PER_INTERFACE:
                switch (device->config[0].interface[usb_interface].altsetting[0].bInterfaceClass) {
                  case USB_CLASS_VENDOR_SPEC:
                  case USB_CLASS_PER_INTERFACE:
                  case 16: /* data? */
                  found = 1;
                  break;
                }
                break;
            }
          }
          usb_interface--;

          syslog(LOG_DEBUG, "using device: vendor: %x, product: %x, in-ep: %x, out-ep: %x, interface: %x",
          	device->descriptor.idVendor, device->descriptor.idProduct,
          	usb_in_ep, usb_out_ep, usb_interface);
          	
          if (!found) return NULL;
          
          return device;
      	}
      }
      device = device->next;
    }
    bus = bus->next;
  }
  return NULL;
}


// Returns the scanner button number
int get_scanner_button(usb_dev_handle* scanner_handle) {
  char bytes[255];
  int rcv_len;
  int i;    
  int button;

  bytes[0] = ESC;
  bytes[1] = '!';
  bytes[2] = '\0';
  
  //syslog(LOG_DEBUG, "usb comm, stage 1");
  i = usb_bulk_write(scanner_handle, usb_out_ep, bytes, 2, TIMEOUT);
  if (i<0) {
    usb_clear_halt(scanner_handle, usb_out_ep); 
    return -1;
  }
  
  //syslog(LOG_DEBUG, "usb comm, stage 2");    
  i = usb_bulk_read(scanner_handle, usb_in_ep, bytes, 4, TIMEOUT);  
  if (i<0) {
    usb_clear_halt(scanner_handle, usb_in_ep);
    return -1;
  }
    
  //syslog(LOG_DEBUG, "usb comm, stage 3");
  rcv_len = bytes[3] << 8 | bytes[2];
  i = usb_bulk_read(scanner_handle, usb_in_ep, bytes, rcv_len, TIMEOUT);
  if (i<0) {
    usb_clear_halt(scanner_handle, usb_in_ep);
    return -1;
  }
  button = bytes[0];
  return button;
}


// Ensures a graceful exit on SIGHUP/SIGTERM/SIGINT
void sighandler(int i) {
  killed = 1;
  usb_close(scanner_handle);
  syslog(LOG_INFO, "quit");
  closelog();
  exit(0);
}
	

// Executes an auxiliary program
void execute(const char* program) {
  system(program);
}


int main(void) {
  int i;
  int button;
  pid_t pid;
  
  openlog(NULL, 0, LOG_DAEMON);
  
  pid = fork();
  if (pid < 0) { 
    printf("Can't fork!\n");
    exit(1);
  } else if (pid != 0) {
    return 0;
  }
    
  usb_init();
  usb_find_busses();
  usb_find_devices();
  
  scanner_device = get_scanner_device();
  if (scanner_device == NULL) {
    printf("No scanner found.\n");
    return 1;
  }

  signal(SIGTERM, &sighandler);
  signal(SIGHUP, &sighandler);
  signal(SIGINT, &sighandler);
  
  syslog(LOG_INFO, "scanbuttond started");

  while (killed == 0) {
    if (scanner_device == NULL) {
      usleep(POLL_DELAY);
      continue;
    }
    scanner_handle = usb_open(scanner_device);
    if (scanner_handle == NULL) {
      usb_close(scanner_handle);
      usleep(POLL_DELAY);
      scanner_device = get_scanner_device();
      continue;
    }
    if (usb_claim_interface(scanner_handle, usb_interface) != 0) {
      usb_close(scanner_handle);
      usleep(POLL_DELAY);
      continue;
    }
    
    button = get_scanner_button(scanner_handle);
    usb_release_interface(scanner_handle, usb_interface); 
    usb_close(scanner_handle);
    
    if (button > 0) { 
    	syslog(LOG_INFO, "button %d has been pressed.", button);
    	char cmd[256];
    	sprintf(cmd, SCRIPT, button);
    	execute(cmd);
    }
    
    usleep(POLL_DELAY);
  }
  
  syslog(LOG_INFO, "exited main loop"); 
      
  return 0;

}

