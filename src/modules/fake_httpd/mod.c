/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/** \endcond */

/** \file mod.c
	This is a test module, it reflects every byte of the socket.
*/

#include <room_service.h>

struct listener_memory_st {
	listen_tcp_t	*listen;
};

#define	BUFSIZE	4096

struct echoer_memory_st {
	conn_tcp_t	*conn;
	int state;

	size_t length;

	uint8_t header[BUFSIZE];
	size_t header_len, header_pos;

	uint8_t memoirs[BUFSIZE];
};

static imp_soul_t listener_soul, echoer_soul;
static struct addrinfo *local_addr;
static timeout_msec_t timeo;

static imp_t *id_listener;

static int get_config(cJSON *conf)
{
	int ga_err;
	struct addrinfo hint;
	cJSON *value;
	char *Host, Port[12];
	

	hint.ai_flags = 0;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	value = cJSON_GetObjectItem(conf, "Enabled");
	if (value->type != cJSON_True) {
		mylog(L_INFO, "Module is configured disabled.");
		return -1;
	}

	value = cJSON_GetObjectItem(conf, "LocalAddr");
	if (value->type != cJSON_String) {
		mylog(L_INFO, "Module is configured with illegal LocalAddr.");
		return -1;
	} else {
		Host = value->valuestring;
	}

	value = cJSON_GetObjectItem(conf, "LocalPort");
	if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal LocalPort.");
		return -1;
	} else {
		sprintf(Port, "%d", value->valueint);
	}

	ga_err = getaddrinfo(Host, Port, &hint, &local_addr);
	if (ga_err) {
		mylog(L_INFO, "Module configured with illegal LocalAddr and LocalPort.");
		return -1;
	}

	value = cJSON_GetObjectItem(conf, "TimeOut_Recv_ms");
	if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal TimeOut_Recv_ms.");
		return -1;
	} else {
		timeo.recv = value->valueint;
	}

	value = cJSON_GetObjectItem(conf, "TimeOut_Send_ms");
	if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal TimeOut_Send_ms.");
		return -1;
	} else {
		timeo.send = value->valueint;
	}

	return 0;
}

static int delta_t(void)
{
	return time(NULL)-dungeon_heart->create_time.tv_sec;
}

static int mod_init(cJSON *conf)
{
	struct listener_memory_st *l_mem;

	if (get_config(conf)!=0) {
		return -1;
	}

	l_mem = calloc(sizeof(*l_mem), 1);
	l_mem->listen = conn_tcp_listen_create(local_addr, &timeo);
	if (l_mem->listen==NULL) {
		mylog(L_ERR, "conn_tcp_listen_create() Failed: %m\n");
		return -1;
	}

	id_listener = imp_summon(l_mem, &listener_soul);
	if (id_listener==NULL) {
		mylog(L_ERR, "imp_summon() Failed!\n");
		return 0;
	}
	return 0;
}

static int mod_destroy(void)
{
	imp_kill(id_listener);
	return 0;
}

static void mod_maint(void)
{
	//fprintf(stderr, "%s is running.\n", __FUNCTION__);
}

static cJSON *mod_serialize(void)
{
	//fprintf(stderr, "%s is running.\n", __FUNCTION__);
	return NULL;
}



static void *listener_new(imp_t *p)
{
	struct listener_memory_st *lmem=p->memory;

	//fprintf(stderr, "%s is running.\n", __FUNCTION__);
	//fprintf(stderr, "Set listen socket epoll event.\n");
	if (imp_set_ioev(p, lmem->listen->sd, EPOLLIN|EPOLLRDHUP)<0) {
		mylog(L_ERR, "Set listen socket epoll event FAILED: %m\n");
	}

	return NULL;
}

static int listener_delete(imp_t *p)
{
	struct listener_memory_st *mem=p->memory;

	mylog(L_DEBUG, "%s is running, free memory.\n", __FUNCTION__);
	free(mem);
	return 0;
}

static enum enum_driver_retcode listener_driver(imp_t *p)
{
	static int count =10;
	struct listener_memory_st *lmem=p->memory;
	struct echoer_memory_st *emem;
	conn_tcp_t *conn=NULL;
	imp_t *echoer;

	while (conn_tcp_accept_nb(&conn, lmem->listen, &timeo)==0) {
		emem = calloc(sizeof(*emem), 1);
		emem->conn = conn;
		echoer = imp_summon(emem, &echoer_soul);
		if (echoer==NULL) {
			mylog(L_ERR, "Failed to summon a new imp.");
			conn_tcp_close_nb(conn);
			//free(emem);
			continue;
		}
	}
	imp_set_ioev(p, lmem->listen->sd, EPOLLIN|EPOLLOUT|EPOLLRDHUP);
	return TO_WAIT_IO;
}

static void *listener_serialize(imp_t *imp)
{
	struct listener_memory_st *m=imp->memory;

	return NULL;
}


enum state_en {
	ST_RECV=1,
	ST_PREPARE_HEADER,
	ST_SEND_HEADER,
	ST_SEND_BODY,
	ST_TERM,
	ST_Ex
};

static char *fake_http_header_fmt =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text/plain; charset=utf-8\r\n"
	"Content-Length: %zu\r\n\r\n";

static char padding_data[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static int padding_data_size = sizeof(padding_data);

static void *echo_new(imp_t *imp)
{
	struct echoer_memory_st *m=imp->memory;
	char ipstr[40];
	int port;

	m->state = ST_RECV;
	m->length = 0;
	port = ntohs(((struct sockaddr_in*)(&m->conn->peer_addr))->sin_port);
	inet_ntop(AF_INET, &((struct sockaddr_in*)(&m->conn->peer_addr))->sin_addr, ipstr, 40);
	snprintf(m->memoirs, BUFSIZE, "imp[%d] for %s:%d (+%ds)->", imp->id, ipstr, port, delta_t());
	return NULL;
}

static int echo_delete(imp_t *imp)
{
	struct echoer_memory_st *memory = imp->memory;
	char log[1024];

	conn_tcp_close_nb(memory->conn);
	sprintf(log, "[+%ds] echo_delete() conn_tcp_close_nb(conn) .", delta_t());
	strcat(memory->memoirs, log);
	fprintf(stderr, "%s\n", memory->memoirs);
	free(memory);
	imp->memory = NULL;
	return 0;
}

#define	min(X, Y)	((X)>(Y)?(Y):(X))

static enum enum_driver_retcode echo_driver(imp_t *imp)
{
	struct echoer_memory_st *mem;
	uint8_t buf[BUFSIZE], log[BUFSIZE];
	off_t len, pos;
	ssize_t ret;

	mem = imp->memory;
	switch (mem->state) {
		case ST_RECV:
			if (imp->event_mask & EV_MASK_TIMEOUT || imp->event_mask & EV_MASK_IOERR) {
				sprintf(log, "ST_RECV[+%ds] timed out->", delta_t());
				strcat(mem->memoirs, log);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			sprintf(log, "ST_RECV[+%ds] event_mask=%llx->", delta_t(), imp->event_mask);
			strcat(mem->memoirs, log);
			len = conn_tcp_recv_nb(mem->conn, buf, BUFSIZE);
			if (len == 0) {
				sprintf(log, "ST_RECV[+%ds] EOF->", delta_t());
				strcat(mem->memoirs, log);
				mem->state = ST_TERM;
				return TO_RUN;
			} else if (len < 0) {
				if (errno==EAGAIN) {
					sprintf(log, "ST_RECV[+%ds] sleep->", delta_t());
					strcat(mem->memoirs, log);
					imp_set_ioev(imp, mem->conn->sd, EPOLLIN|EPOLLRDHUP);
					imp_set_timer(imp, timeo.recv);
					return TO_WAIT_IO;
				} else {
					sprintf(log, "ST_RECV[+%ds] error: %m->", delta_t());
					strcat(mem->memoirs, log);
					mem->state = ST_Ex;
					return TO_RUN;
				}
			} else {
				sprintf(log, "ST_RECV[+%ds] OK->", delta_t());
				strcat(mem->memoirs, log);
				mem->length = len;
				mem->state = ST_PREPARE_HEADER;
				return TO_RUN;
			}
			break;
		case ST_PREPARE_HEADER:
			mem->header_len = snprintf(mem->header, BUFSIZE, fake_http_header_fmt, mem->length);
			if (mem->header_len==BUFSIZE) {
				sprintf(log, "ST_PREPARE_HEADER[+%ds] Header id too long!->", delta_t());
				strcat(mem->memoirs, log);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			sprintf(log, "ST_PREPARE_HEADER[+%ds] OK->", delta_t());
			strcat(mem->memoirs, log);
			mem->header_pos = 0;
			mem->state = ST_SEND_HEADER;
			return TO_RUN;
			break;
		case ST_SEND_HEADER:
			if (imp->event_mask & EV_MASK_TIMEOUT || imp->event_mask & EV_MASK_IOERR) {
				sprintf(log, "ST_SEND_HEADER[+%ds] timed out or error->", delta_t());
				strcat(mem->memoirs, log);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			ret = conn_tcp_send_nb(mem->conn, mem->header + mem->header_pos, mem->header_len);
			if (ret<=0) {
				if (errno==EAGAIN) {
					sprintf(log, "ST_SEND_HEADER[+%ds] sleep->", delta_t());
					strcat(mem->memoirs, log);
					imp_set_ioev(imp, mem->conn->sd, EPOLLOUT|EPOLLRDHUP);
					imp_set_timer(imp, timeo.recv);
					return TO_WAIT_IO;
				} else {
					sprintf(log, "ST_SEND_HEADER[+%ds] error: %m->", delta_t());
					strcat(mem->memoirs, log);
					mem->state = ST_Ex;
					return TO_RUN;
				}
			} else {
				mem->header_pos += ret;
				mem->header_len -= ret;
				if (mem->header_len == 0) {
					sprintf(log, "ST_SEND_HEADER[+%ds] %d bytes sent, OK->", delta_t(), ret);
					strcat(mem->memoirs, log);
					mem->state = ST_SEND_BODY;
					return TO_RUN;
				}
				sprintf(log, "ST_SEND_HEADER[+%ds] %d bytes sent, again->", delta_t(), ret);
				strcat(mem->memoirs, log);
				return TO_RUN;
			}
			break;
		case ST_SEND_BODY:
			if (imp->event_mask & EV_MASK_TIMEOUT  || imp->event_mask & EV_MASK_IOERR) {
				sprintf(log, "ST_SEND_BODY[+%ds] timed out or error->", delta_t());
				strcat(mem->memoirs, log);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			ret = conn_tcp_send_nb(mem->conn, padding_data, min(padding_data_size, mem->length));
			if (ret<=0) {
				if (errno==EAGAIN) {
					sprintf(log, "ST_SEND_BODY[+%ds] sleep->", delta_t());
					strcat(mem->memoirs, log);
					imp_set_ioev(imp, mem->conn->sd, EPOLLOUT|EPOLLRDHUP);
					imp_set_timer(imp, timeo.send);
					return TO_WAIT_IO;
				} else {
					sprintf(log, "ST_SEND_BODY[+%ds] error: %m->", delta_t());
					strcat(mem->memoirs, log);
					mem->state = ST_Ex;
					return TO_RUN;
				}
			} else {
				mem->length -= ret;
				if (mem->length <= 0) {
					sprintf(log, "ST_SEND_BODY[+%ds] %d bytes sent, OK->", delta_t(), ret);
					strcat(mem->memoirs, log);
					mem->state = ST_TERM;
					return TO_RUN;
				}
				sprintf(log, "ST_SEND_BODY[+%ds] %d bytes sent, again->", delta_t(), ret);
				strcat(mem->memoirs, log);
				return TO_RUN;
			}
			break;
		case ST_Ex:
			sprintf(log, "ST_Ex[+%ds] Exception happened!", delta_t());
			strcat(mem->memoirs, log);
			mem->state = ST_TERM;
			break;
		case ST_TERM:
			return TO_TERM;
			break;
		default:
			break;
	}
	return TO_RUN;
}

static void *echo_serialize(imp_t *unused)
{
	fprintf(stderr, "%s is running.\n", __FUNCTION__);
	return NULL;
}

static imp_soul_t listener_soul = {
	.fsm_new = listener_new,
	.fsm_delete = listener_delete,
	.fsm_driver = listener_driver,
	.fsm_serialize = listener_serialize,
};

static imp_soul_t echoer_soul = {
	.fsm_new = echo_new,
	.fsm_delete = echo_delete,
	.fsm_driver = echo_driver,
	.fsm_serialize = echo_serialize,
};

module_interface_t MODULE_INTERFACE_SYMB = 
{
	.mod_initializer = mod_init,
	.mod_destroier = mod_destroy,
	.mod_maintainer = mod_maint,
	.mod_serialize = mod_serialize,
};

