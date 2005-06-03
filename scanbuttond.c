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
#include <getopt.h>
#include "backends/backend.h"

#define VERSION				"0.2.2-pre"
#define DEF_BUTTONPRESSED_SCRIPT	"/etc/scanbuttond/buttonpressed.sh"
#define DEF_INITSCANNER_SCRIPT		"/etc/scanbuttond/initscanner.sh"
#define DEF_POLL_DELAY			333000L
#define MIN_POLL_DELAY			1000L
#define DEF_RETRY_DELAY			2000000L
#define MIN_RETRY_DELAY			10000L
#define BUF_SIZE			256


char* buttonpressed_script;
char* initscanner_script;
long poll_delay;
long retry_delay;
int daemonize;


static char* connection_names[CONNECTIONS_COUNT] = 
    { "none", "libusb" };

int killed = 0;
char* path;


char* scanbtnd_get_connection_name(int connection) {
  return connection_names[connection];
}


void shutdown(void) {
  syslog(LOG_INFO, "shutting down...");
  scanbtnd_exit();
  syslog(LOG_INFO, "shutdown complete");
  closelog();
}


// Ensures a graceful exit on SIGHUP/SIGTERM/SIGINT/SIGSEGV
void sighandler(int i) {
  killed = 1;
  syslog(LOG_INFO, "received signal %d", i);
  shutdown();
  exit(i == SIGTERM ? EXIT_SUCCESS : EXIT_FAILURE);
}


// Executes an external program as an independent child
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


// Executes an external program and wait until it terminates
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


void show_version(void) {
  printf("This is scanbuttond, version %s\n", VERSION);
  printf("Copyleft )c( 2004-2005 by Bernhard Stiftner and contributors.\n");
  printf("Scanbuttond comes with ABSOLUTELY NO WARRANTY!\n");
  printf("This is free software, and you are welcome to redistribute it\n");
  printf("under certain conditions; see the file COPYING for details.\n");
}


void show_usage(void) {
  printf("Usage: scanbuttond [OPTION]...\n\n");
  printf("Options:\n");
  printf("  -f, --foreground            Run in foreground instead of background\n");
  printf("  -s, --buttonscript=SCRIPT   The name of the script to be run when a button has been pressed\n");
  printf("                              default: %s\n", DEF_BUTTONPRESSED_SCRIPT);
  printf("  -S, --initscript=SCRIPT     The name of the script to be run to initialize the scanners\n");
  printf("                              default: %s\n", DEF_INITSCANNER_SCRIPT);
  printf("  -p, --pollingdelay=DELAY    The polling delay (ms), default: %ld\n", DEF_POLL_DELAY);
  printf("  -r, --retrydelay=DELAY      The retry delay (ms), default: %ld\n", DEF_RETRY_DELAY);
  printf("  -h, --help                  Shows this screen\n");
  printf("  -v, --version               Shows the version\n");
}


static struct option const long_opts[] = {
  {"foreground", no_argument, NULL, 'f'},
  {"buttonscript", required_argument, NULL, 's'},
  {"initscript", required_argument, NULL, 'S'},
  {"pollingdelay", required_argument, NULL, 'p'},
  {"retrydelay", required_argument, NULL, 'r'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};


void process_options(int argc, char** argv) {
  int c;
  
  buttonpressed_script = NULL;
  initscanner_script = NULL;
  poll_delay = -1;
  retry_delay = -1;
  daemonize = 1;
  
  while ((c = getopt_long (argc, argv, "fs:S:p:r:hv", long_opts, NULL)) != -1) {
    switch (c) {
      case 'f':
        daemonize = 0;
        break;
      case 's':
        buttonpressed_script = optarg;
        break;
      case 'S':
        initscanner_script = optarg;
        break;
      case 'p':
        printf("optarg: %s\n", optarg);
        poll_delay = atol(optarg);
        if (poll_delay < MIN_POLL_DELAY) {
          printf("Invalid polling delay (%ld). Must be at least %ld.\n", 
                 poll_delay, MIN_POLL_DELAY);
          exit(EXIT_FAILURE);
        }
        break;
      case 'r':
        retry_delay = atol(optarg);
        if (retry_delay < MIN_RETRY_DELAY) {
          printf("Invalid retry delay (%ld). Must be at least %ld.\n", 
                 retry_delay, MIN_RETRY_DELAY);
          exit(EXIT_FAILURE);
        }
        break;
      case 'h':
        show_usage();
        exit(EXIT_SUCCESS);
        break;
      case 'v':
        show_version();
        exit(EXIT_SUCCESS);
        break;
    }
  }
  
  printf("init: %s\n", initscanner_script);
  printf("btn: %s\n", buttonpressed_script);
  printf("pdelay: %ld\n", poll_delay);
  printf("rdelay: %ld\n", retry_delay);
  
  if (buttonpressed_script == NULL) 
    buttonpressed_script = DEF_BUTTONPRESSED_SCRIPT;
  if (initscanner_script == NULL) 
    initscanner_script = DEF_INITSCANNER_SCRIPT;
  if (poll_delay == -1) 
    poll_delay = DEF_POLL_DELAY;
  if (retry_delay == -1) 
    retry_delay = DEF_RETRY_DELAY;
    
  printf("init: %s\n", initscanner_script);
  printf("btn: %s\n", buttonpressed_script);
  printf("pdelay: %ld\n", poll_delay);
  printf("rdelay: %ld\n", retry_delay);
}
  

int main(int argc, char** argv) {
  int button;
  int result;
  pid_t pid, sid;
  scanner_device* dev;
  
  process_options(argc, argv);

  // daemonize
  if (daemonize) {
    pid = fork();
    if (pid < 0) { 
      printf("Can't fork!\n");
      exit(1);
    } else if (pid > 0) {
      exit(0);
    }
  }
  
  // change the file mode mask
  umask(0); 
 
  openlog(NULL, 0, LOG_DAEMON);
  
  // create a new session for the child process
  if (daemonize) {
    sid = setsid();
    if (sid < 0) {
      syslog(LOG_ERR, "Could not create a new SID! Terminating.");
      exit(1);
    }
  }

  // Change the current working directory
  if ((chdir("/")) < 0) {
    syslog(LOG_WARNING, "Could not chdir to /. Hmmm, strange... "\
      "Trying to continue.");
  }
  
  // close standard file descriptors
  if (daemonize) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }
  
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
  execute_and_wait(initscanner_script);
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
        usleep(retry_delay);
        continue;
      }
      syslog(LOG_DEBUG, "found supported devices. running scanner initialization script...");
      execute_and_wait(initscanner_script);
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
        usleep(retry_delay);
        break;
      }
      
      button = scanbtnd_get_button(dev);
      scanbtnd_close(dev);
        
      if ((button > 0) && (button != dev->lastbutton)) { 
        syslog(LOG_INFO, "button %d has been pressed.", button);
        dev->lastbutton = button;
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %d %s", buttonpressed_script, button, 
                 scanbtnd_get_sane_device_descriptor(dev));
        execute_as_child(cmd);
      }
      if ((button == 0) && (dev->lastbutton > 0)) {
        syslog(LOG_INFO, "button %d has been released.", dev->lastbutton);
        dev->lastbutton = button;
      } 
      dev = dev->next;
    }
    
    usleep(poll_delay);
    
  }
  
  syslog(LOG_WARNING, "exited main loop!?!"); 
      
  shutdown();
  exit(EXIT_SUCCESS);

}

