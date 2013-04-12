#ifndef PROXY_CONTEXT_H
#define PROXY_CONTEXT_H

enum proxy_state_en {
	STATE_ACCEPT=1,
	STATE_READHEADER,
	STATE_PARSEHEADER,
	STATE_DNS,
	STATE_REGISTERSERVERFAIL,
	STATE_REJECTCLIENT,
	STATE_CONNECTSERVER,
	STATE_RELAY,
	STATE_IOWAIT,
	STATE_ERR,
	STATE_TERM
};

#define	HTTP_HEADER_MAX	4096

typedef struct proxy_context_st {
	proxy_pool_t *pool;
	int state;
	int listen_sd;

	connection_t *client_conn;
	struct timeval client_r_timeout_tv, client_s_timeout_tv;
	char *http_header_buffer;
	int http_header_buffer_pos;
	struct http_header_st *http_header;

	connection_t *server_conn;
	struct timeval server_r_timeout_tv, server_s_timeout_tv;

	buffer_t *s2c_buf, *c2s_buf;

	char *errlog_str;
} proxy_context_t;

proxy_context_t *proxy_context_new_accepter(proxy_pool_t *pool);

int proxy_context_delete(proxy_context_t*);

int proxy_context_timedout(proxy_context_t*);

int is_proxy_context_timedout(proxy_context_t*);

int proxy_context_put_runqueue(proxy_context_t*);

int proxy_context_put_epollfd(proxy_context_t*);

int proxy_context_driver(proxy_context_t*);

#endif
