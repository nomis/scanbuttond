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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <dlfcn.h>
#include <errno.h>
#include "interface/libusbi.h"
#include "scanbuttond.h"
#include "meta.h"

#define MAX_CONFIG_LINE 255
#define MAX_SCANNERS_PER_BACKEND 16

static char* backend_name = "Dynamic Module Loader";
static char* config_file = "/etc/scanbuttond/meta.conf";

scanner_device* meta_scanners = NULL;
backend_t* meta_backends = NULL;


backend_t* meta_load_backend(const char* path, const char* name) {
  const char* prefix = "lib";
  const char* suffix = ".so";
  char* error;
  char* libpath = (char*)malloc(strlen(path) + strlen(name) + strlen(prefix) + strlen(suffix) + 2);
  strcpy(libpath, path);
  strcat(libpath, "/");
  strcat(libpath, prefix);
  strcat(libpath, name);
  strcat(libpath, suffix);
  void* dll_handle = dlopen(libpath, RTLD_LAZY|RTLD_LOCAL);
  if (!dll_handle) {
    syslog(LOG_ERR, "meta-backend: failed to load \"%s\". Error message: \"%s\"",
      libpath, dlerror());
    free(libpath);
    return NULL;
  }
  free(libpath);
  dlerror();  // Clear any existing error
  backend_t* backend = (backend_t*)malloc(sizeof(backend_t));
  backend->handle = dll_handle;
  backend->scanbtnd_get_backend_name = dlsym(dll_handle, "scanbtnd_get_backend_name");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_init = dlsym(dll_handle, "scanbtnd_init");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);    
    return NULL;
  }
  backend->scanbtnd_rescan = dlsym(dll_handle, "scanbtnd_rescan");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_get_supported_devices = dlsym(dll_handle, "scanbtnd_get_supported_devices");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_open = dlsym(dll_handle, "scanbtnd_open");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_close = dlsym(dll_handle, "scanbtnd_close");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_get_button = dlsym(dll_handle, "scanbtnd_get_button");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_get_sane_device_descriptor = dlsym(dll_handle, "scanbtnd_get_sane_device_descriptor");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }
  backend->scanbtnd_exit = dlsym(dll_handle, "scanbtnd_exit");
  if ((error = dlerror()) != NULL) {
    syslog(LOG_ERR, "meta-backend: dlsym failed! Error message \"%s\"", error);
    dlclose(dll_handle);
    free(backend);
    return NULL;
  }  

  return backend;
}


void meta_unload_backend(backend_t* backend) {
  dlclose(backend->handle);
  free(backend);
}


char* scanbtnd_get_backend_name(void) {
  return backend_name;
}


void meta_attach_scanner(scanner_device* scanner, backend_t* backend) {
  scanner_device* dev = (scanner_device*)malloc(sizeof(scanner_device));
  dev->vendor = scanner->vendor;
  dev->product = scanner->product;
  dev->connection = scanner->connection;
  dev->internal_dev_ptr = scanner->internal_dev_ptr;
  dev->sane_device = scanner->sane_device;
  dev->meta_info = (void*)backend;
  dev->lastbutton = 0;
  dev->next = meta_scanners;
  meta_scanners = dev;
  syslog(LOG_INFO, "meta-backend: attached scanner \"%s %s\"", scanner->vendor, scanner->product);
}


void meta_attach_scanners(scanner_device* devices, backend_t* backend) {
  scanner_device* dev = devices;
  int count = 0;
  while (dev != NULL) {
    if (count >= MAX_SCANNERS_PER_BACKEND) {
      syslog(LOG_WARNING, "meta-backend: refusing to attach scanner \"%s %s\": Too many scanners!",
        dev->vendor, dev->product);
      return;
    }
    //syslog(LOG_INFO, "meta-backend: attaching scanner \"%s %s\"", dev->vendor, dev->product);
    meta_attach_scanner(dev, backend);
    dev = dev->next;
    count++;
  }
}


void meta_detach_scanner(scanner_device* scanner) {
  syslog(LOG_INFO, "meta-backend: detaching scanner: \"%s %s\"", scanner->vendor, scanner->product);
  free(scanner);
}


void meta_detach_scanners(void) {
  //syslog(LOG_INFO, "meta-backend: detaching scanners");
  scanner_device* next;
  while (meta_scanners != NULL) {
    next = meta_scanners->next;    
    meta_detach_scanner(meta_scanners);
    meta_scanners = next;
  }
}


int meta_attach_backend(backend_t* backend) {  
  // don't load another meta backend
  if (strcmp(backend->scanbtnd_get_backend_name(), scanbtnd_get_backend_name())==0) {
    syslog(LOG_INFO, "meta-backend: refusing to load another meta backend!");
    return -1;
  }
  syslog(LOG_INFO, "meta-backend: attaching backend: %s", backend->scanbtnd_get_backend_name());
  backend->next = meta_backends;
  meta_backends = backend;  
  backend->scanbtnd_init();  
  meta_attach_scanners(backend->scanbtnd_get_supported_devices(), backend);
  return 0;
}


void meta_detach_backend(backend_t* backend) {
  backend->scanbtnd_exit();
  meta_unload_backend(backend);
}

		
void meta_detach_backends(void) {
  backend_t* next;
  while (meta_backends != NULL) {
    next = meta_backends->next;    
    meta_detach_backend(meta_backends);
    meta_backends = next;
  }
}


backend_t* meta_lookup_backend(scanner_device* scanner) {
  return (backend_t*)scanner->meta_info;
}


void meta_strip_newline(char* str) {
  int len = strlen(str);
  if (len == 0) return;
  if (str[len-1] != '\n') return;
  str[len-1] = 0;
}


int scanbtnd_init(void) {
  meta_scanners = NULL;
  meta_backends = NULL;
  
  syslog(LOG_INFO, "meta-backend: init");
  libusb_init();
  
  // read config file
  char libdir[MAX_CONFIG_LINE];
  char lib[MAX_CONFIG_LINE];
  backend_t* backend;
  FILE* f = fopen(config_file, "r");
  if (f == NULL) {
    syslog(LOG_INFO, "meta-backend: config file \"%s\" not found. exiting.", config_file);
    return -1;
  }
  fgets(libdir, MAX_CONFIG_LINE, f);
  meta_strip_newline(libdir);
  while (fgets(lib, MAX_CONFIG_LINE, f)) {
    meta_strip_newline(lib);
    backend = meta_load_backend(libdir, lib);
    if (backend != NULL) {
      meta_attach_backend(backend);
    }
  }
  fclose(f);
  
  return 0;
}


int scanbtnd_rescan(void) {
  backend_t* backend;
  
  meta_detach_scanners();
  meta_scanners = NULL;
  
  backend = meta_backends;
  while (backend != NULL) {
    backend->scanbtnd_rescan();
    meta_attach_scanners(backend->scanbtnd_get_supported_devices(), backend); 
    backend = backend->next;
  }
    
  return 0;
}


scanner_device* scanbtnd_get_supported_devices(void) {
  return meta_scanners;
}


int scanbtnd_open(scanner_device* scanner) {  
  // if devices have been added/removed, return -ENODEV to
  // make scanbuttond update its device list
  if (libusb_get_changed_device_count() != 0) {
    return -ENODEV;
  }
  backend_t* backend = meta_lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_open(scanner);
}


int scanbtnd_close(scanner_device* scanner) {
  backend_t* backend = meta_lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_close(scanner);
}


int scanbtnd_get_button(scanner_device* scanner) {
  backend_t* backend = meta_lookup_backend(scanner);
  if (backend == NULL) return 0;
  return backend->scanbtnd_get_button(scanner);
}


char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner) {
  backend_t* backend = meta_lookup_backend(scanner);
  if (backend == NULL) return NULL;
  return backend->scanbtnd_get_sane_device_descriptor(scanner);
}


int scanbtnd_exit(void) {
  syslog(LOG_INFO, "meta-backend: exit");
  meta_detach_scanners();
  meta_detach_backends();
  libusb_exit();
  return 0;
}

