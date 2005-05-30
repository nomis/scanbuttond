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


#include <sys/stat.h>
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
#define BUTTONPRESSED_SCRIPT		"/etc/scanbuttond/buttonpressed.sh %d %s"
#define INITSCANNER_SCRIPT		"/etc/scanbuttond/initscanner.sh"
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
  syslog(LOG_INFO, "received signal %d", i);
  scanbtnd_exit();
  syslog(LOG_INFO, "quit");
  closelog();
  exit(0);
}


// Executes an auxiliary program
void execute_as_child(const char* program) {
  if (!program) return;
  int pid = fork();
  if (pid == 0) {
    system(program);
    exit(EXIT_SUCCESS);
  } else if (pid < 0) {
    syslog(LOG_ERR, "fork() failed, cannot execute external program!");
  }
}


void execute_and_wait(const char* program) {
  if (!program) return;
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
  pid_t pid, sid;
  scanner_device* dev;

  // daemonize
  pid = fork();
  if (pid < 0) { 
    printf("Can't fork!\n");
    exit(1);
  } else if (pid > 0) {
    exit(0);
  } 
  
  // change the file mode mask
  umask(0); 
 
  openlog(NULL, 0, LOG_DAEMON);
  
  // create a new session for the child process
  sid = setsid();
  if (sid < 0) {
    syslog(LOG_ERR, "Could not create a new SID! Terminating.");
    exit(1);
  }

  // Change the current working directory
  if ((chdir("/")) < 0) {
    syslog(LOG_WARNING, "Could not chdir to /. Hmmm, strange... "\
      "Trying to continue.");
  }
  
  // close standard file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  
  // setup the environment
  char* oldpath = getenv("PATH");
  char* dir = dirname(argv[0]);
  path = (char*)malloc(strlen(oldpath) + strlen(dir) + 1);
  strcpy(path, oldpath);
  strcat(path, ":");
  strcat(path, dir);
  setenv("PATH", path, 1);
  free(path);
  
  // prepare daemon operation
  
  syslog(LOG_DEBUG, "running scanner initialization script...");
  execute_and_wait(INITSCANNER_SCRIPT);
  syslog(LOG_DEBUG, "initialization script executed.");
  
  if (scanbtnd_init() != 0) {
    syslog(LOG_ERR, "Error initializing backend. Terminating.");
    exit(1);
  }
  
  scanner_device* devices = scanbtnd_get_supported_devices();
  
  if (devices == NULL) {
    syslog(LOG_WARNING, "no known scanner found yet, " \
    	"waiting for device to be attached");
  }
  
  list_devices(devices);

  signal(SIGTERM, &sighandler);
  signal(SIGHUP, &sighandler);
  signal(SIGINT, &sighandler);
  signal(SIGSEGV, &sighandler);
  signal(SIGCLD, SIG_IGN);
  
  syslog(LOG_INFO, "scanbuttond started");

  // main loop
  while (killed == 0) {
  
    if (devices == NULL) {
      syslog(LOG_DEBUG, "rescanning devices...");
      scanbtnd_rescan();
      devices = scanbtnd_get_supported_devices();
      if (devices == NULL) {
        syslog(LOG_DEBUG, "no supported devices found. rescanning in a few seconds...");
        usleep(RETRY_DELAY);
        continue;
      }
      syslog(LOG_DEBUG, "found supported devices. running scanner initialization script...");
      execute_and_wait(INITSCANNER_SCRIPT);
      syslog(LOG_DEBUG, "initialization script executed.");
    }
    
    dev = devices;    
    while (dev != NULL) {
      result = scanbtnd_open(dev);
      if (result != 0) {
        syslog(LOG_WARNING, "scanbtnd_open failed, error code: %d", result);
        if (result == -ENODEV) {
          // device has been disconnected, force re-scan
          syslog(LOG_INFO, "scanbtnd_open returned -ENODEV, device rescan will be performed");
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
	sprintf(cmd, BUTTONPRESSED_SCRIPT, button, scanbtnd_get_sane_device_descriptor(dev));
        execute_as_child(cmd);
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
      
  exit(0);

}

