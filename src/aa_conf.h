#ifndef AA_CONF_H
#define AA_CONF_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "syslog.h"
#include "cJSON.h"

#define APPNAME						"aa_proxy"
#define APPVERSION					"0.1"

#define DEFAULT_LISTEN_PORT			19999
#define DEFAULT_DEBUG_LEVEL 		"false"
#define DEFAULT_CONNECT_TIMEOUT		200
#define DEFAULT_RECEIVE_TIMEOUT		200
#define DEFAULT_SEND_TIMEOUT		200
#define DEFAULT_CONFPATH			"../conf/aa_proxy.conf"

#define PORT_MIN 10000
#define PORT_MAX 65535
#define TIMEOUT_MIN 1
#define TIMEOUT_MAX 300000

static cJSON *global_conf = NULL;

int conf_new(const char *filename);
int conf_delete();
int conf_reload(const char *filename);
int conf_load_json(cJSON *conf);

char * conf_get_listen_addr(cJSON *conf);
int conf_get_listen_port(cJSON *conf);
int conf_get_concurrent_max(cJSON *conf);
char * conf_get_debug_level(cJSON *conf);
int conf_get_connect_timeout(cJSON *conf);
int conf_get_receive_timeout(cJSON *conf);
int conf_get_send_timeout(cJSON *conf);

#endif

