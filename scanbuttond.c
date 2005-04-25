// scanbuttond
// Copyleft )c( 2004-2005 by Bernhard Stiftner
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


#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include "backends/backend.h"

// the button number and the SANE device name are passed to the script as arguments
#define SCRIPT		"/etc/scanbuttond/buttonpressed.sh %d %s"
#define POLL_DELAY	333*1000	/* poll three times a second */
#define RETRY_DELAY	2000*1000	/* if the device is currently not available,
					   wait 2 seconds and try again */

static char* connection_names[CONNECTIONS_COUNT] = 
    { "none", "libusb" };

int killed = 0;
char* path;


char* scanbtnd_get_connection_name(int connection) {
  return connection_names[connection];
}

// Ensures a graceful exit on SIGHUP/SIGTERM/SIGINT/SIGSEGV
void sighandler(int i) {
  killed = 1;
  scanbtnd_exit();
  syslog(LOG_INFO, "quit");
  closelog();
  exit(0);
}


// Executes an auxiliary program
void execute(const char* program) {
  system(program);
}


void list_devices(scanner_device* devices) {
  scanner_device* dev = devices;
  while (dev != NULL) {
    syslog(LOG_INFO, "found scanner: vendor=\"%s\", product=\"%s\", connection=\"%s\", sane_name=\"%s\"",
      dev->vendor, dev->product, scanbtnd_get_connection_name(dev->connection),
      scanbtnd_get_sane_device_descriptor(dev));
    dev = dev->next;
  }
}


int main(int argc, char** argv) {
  int button;
  int result;
  pid_t pid;
  scanner_device* dev;
 
  openlog(NULL, 0, LOG_DAEMON);
  
  char* oldpath = getenv("PATH");
  char* dir = dirname(argv[0]);
  path = (char*)malloc(strlen(oldpath) + strlen(dir) + 1);
  strcpy(path, oldpath);
  strcat(path, ":");
  strcat(path, dir);
  setenv("PATH", path, 1);
  free(path);
  
  pid = fork();
  if (pid < 0) { 
    printf("Can't fork!\n");
    exit(1);
  } else if (pid != 0) {
    sleep(1);
    return 0;
  }  
  
  scanbtnd_init();
  
  scanner_device* devices = scanbtnd_get_supported_devices();
  
  if (devices == NULL) {
    scanbtnd_exit();
    closelog();
    printf("No scanner found.\n");
    return 1;
  }
  
  list_devices(devices);

  signal(SIGTERM, &sighandler);
  signal(SIGHUP, &sighandler);
  signal(SIGINT, &sighandler);
  signal(SIGSEGV, &sighandler);
  
  syslog(LOG_INFO, "scanbuttond started");

  // main loop
  while (killed == 0) {
  
    if (devices == NULL) {
      scanbtnd_rescan();
      devices = scanbtnd_get_supported_devices();
    }
    
    dev = devices;    
    while (dev != NULL) {
      result = scanbtnd_open(dev);
      if (result != 0) {
        if (result == -ENODEV) {
          // device has been disconnected, force re-scan
          syslog(LOG_INFO, "device rescan forced.");
          devices = NULL;
        }                  
        usleep(RETRY_DELAY);
        break;
      }
      
      button = scanbtnd_get_button(dev);
      scanbtnd_close(dev);
       
      if ((button > 0) && (button != dev->lastbutton)) { 
        syslog(LOG_INFO, "button %d has been pressed.", button);
        dev->lastbutton = button;
        char cmd[256];
	sprintf(cmd, SCRIPT, button, scanbtnd_get_sane_device_descriptor(dev));
        execute(cmd);
      }
      if ((button == 0) && (dev->lastbutton > 0)) {
        syslog(LOG_INFO, "button %d has been released.", dev->lastbutton);
        dev->lastbutton = button;
      } 
      dev = dev->next;
    }
    
    usleep(POLL_DELAY);
    
  }
  
  syslog(LOG_INFO, "exited main loop"); 
      
  return 0;

}

