// scanbuttond
// meta backend
// Copyleft )c( 2005 by Bernhard Stiftner
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <dlfcn.h>
#include "scanbuttond.h"
#include "meta.h"

#define MAX_CONFIG_LINE 255

static char* backend_name = "Dynamic Module Loader";
static char* config_file = "/etc/scanbuttond/meta.conf";

scanner_t* scanners = NULL;
backend_t* backends = NULL;


char* scanbtnd_get_backend_name(void) {
  return backend_name;
}


void attach_scanner(scanner_t* scanner, backend_t* backend) {
  scanner_t* dev = (scanner_t*)malloc(sizeof(scanner_t));
  dev->vendor = scanner->vendor;
  dev->product = scanner->product;
  dev->connection = scanner->connection;
  dev->internal_dev_ptr = scanner->internal_dev_ptr;
  dev->sane_device = scanner->sane_device;
  dev->meta_info = (void*)backend;
  dev->next = scanners;
  scanners = dev;
  syslog(LOG_INFO, "meta-backend: attached scanner \"%s %s\"", scanner->vendor, scanner->product);
}


void attach_scanners(scanner_t* scanners, backend_t* backend) {
  scanner_t* dev = scanners;
  while (dev != NULL) {
    attach_scanner(dev, backend);
    dev = dev->next;
  }
}


void detach_scanners(void) {
  scanner_t* next;
  while (scanners != NULL) {
    next = scanners->next;    
    free(scanners);
    scanners = next;
  }
}


void attach_backend(backend_t* backend) {  
  backend->next = backends;
  backends = backend;  
  backend->scanbtnd_init();  
  attach_scanners(backend->scanbtnd_get_supported_devices(), backend);  
}


void detach_backend(backend_t* backend) {
  backend->scanbtnd_exit();
  dlclose(backend->handle);
  free(backend);
}

		
void detach_backends(void) {
  backend_t* next;
  while (backends != NULL) {
    next = backends->next;    
    detach_backend(backends);
    backends = next;
  }
  detach_scanners();
}


backend_t* lookup_backend(scanner_t* scanner) {
  return (backend_t*)scanner->meta_info;
}


backend_t* load_backend(const char* path) {

  syslog(LOG_INFO, "meta-backend: loading \"%s\"", path);

  void* dll_handle = dlopen(path, RTLD_LAZY);
  if (!dll_handle) return NULL;
  dlerror();  // Clear any existing error
  backend_t* backend = (backend_t*)malloc(sizeof(backend_t));
  backend->handle = dll_handle;
  backend->scanbtnd_get_backend_name = dlsym(dll_handle, "scanbtnd_get_backend_name");
  backend->scanbtnd_init = dlsym(dll_handle, "scanbtnd_init");
  backend->scanbtnd_rescan = dlsym(dll_handle, "scanbtnd_rescan");
  backend->scanbtnd_get_supported_devices = dlsym(dll_handle, "scanbtnd_get_supported_devices");
  backend->scanbtnd_open = dlsym(dll_handle, "scanbtnd_open");
  backend->scanbtnd_close = dlsym(dll_handle, "scanbtnd_close");
  backend->scanbtnd_get_button = dlsym(dll_handle, "scanbtnd_get_button");
  backend->scanbtnd_get_sane_device_descriptor = dlsym(dll_handle, "scanbtnd_get_sane_device_descriptor");
  backend->scanbtnd_exit = dlsym(dll_handle, "scanbtnd_exit");
  
  // don't load another meta backend
  if (strcmp(backend->scanbtnd_get_backend_name(), scanbtnd_get_backend_name()) == 0) {
    syslog(LOG_INFO, "meta-backend: refusing to load another meta backend");
    dlclose(backend->handle);
    free(backend);
    return NULL;
  }
  
  return backend;
}


void strip_newline(char* str) {
  int len = strlen(str);
  if (len == 0) return;
  if (str[len-1] != '\n') return;
  str[len-1] = 0;
}


int scanbtnd_init(void) {
  scanners = NULL;
  backends = NULL;
  
  // read config file
  char libdir[MAX_CONFIG_LINE];
  char lib[MAX_CONFIG_LINE];
  char* buf;
  backend_t* backend;
  FILE* f = fopen(config_file, "r");
  if (f == NULL) {
    syslog(LOG_INFO, "meta-backend: config file \"%s\" not found. exiting.", config_file);
    return -1;
  }
  fgets(libdir, MAX_CONFIG_LINE, f);
  strip_newline(libdir);
  while (fgets(lib, MAX_CONFIG_LINE, f)) {
    strip_newline(lib);
    buf = (char*)malloc(strlen(libdir) + strlen(lib) + 17);
    strcpy(buf, libdir);
    strcat(buf, "/libscanbtnd-");
    strcat(buf, lib);
    strcat(buf, ".so");    
    backend = load_backend(buf);
    attach_backend(backend);
    free(buf);
  }
  fclose(f);
  
  return 0;
}


int scanbtnd_rescan(void) {
  backend_t* backend;
  
  detach_scanners();
  scanners = NULL;
  
  backend = backends;
  while (backend != NULL) {
    backend->scanbtnd_rescan();
    attach_scanners(backend->scanbtnd_get_supported_devices(), backend);  
    backend = backend->next;
  }
  
  return 0;
}


scanner_t* scanbtnd_get_supported_devices(void) {
  return scanners;
}


int scanbtnd_open(scanner_t* scanner) {  
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_open(scanner);
}


int scanbtnd_close(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_close(scanner);
}


int scanbtnd_get_button(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return 0;
  return backend->scanbtnd_get_button(scanner);
}


char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return NULL;
  return backend->scanbtnd_get_sane_device_descriptor(scanner);
}


int scanbtnd_exit(void) {
  detach_backends();
  return 0;
}

