#ifndef fsm_module_interface_h
#define fsm_module_interface_h

#include <stdio.h>
#include "cJSON.h"

#define MODULE_INTERFACE_SYMB	mod_interface
#define MODULE_INTERFACE_SYMB_STR	"mod_interface"

enum enum_driver_retcode {
	TO_RUN=1,
	TO_WAIT_IO,
	TO_TERM,
};

typedef struct {
	int (*mod_initializer)(void/*proxy_pool_t*/*, cJSON *);	// Arg is considered to be configuration.
	int (*mod_destroier)(void*);
	void (*mod_maintainer)(void*);
	cJSON *(*mod_serialize)(void*);
        void *(*fsm_new)(void*);
        int (*fsm_delete)(void*);
        enum enum_driver_retcode (*fsm_driver)(void*);
        void *(*fsm_serialize)(void*);
} module_interface_t;

#endif
