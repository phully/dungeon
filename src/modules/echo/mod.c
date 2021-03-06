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
	uint8_t buf[BUFSIZE];
	off_t len, pos;
};

static imp_soul_t listener_soul, echoer_soul;
static struct addrinfo *local_addr;
static timeout_msec_t timeo;

static imp_t *id_listener;

static int get_config(cJSON *conf)
{
	/** \todo Parse the REAL config info */
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

static int mod_init(cJSON *conf)
{
	struct listener_memory_st *l_mem;

	if (get_config(conf)!=0) {
		return -1;
	}

	l_mem = calloc(sizeof(*l_mem), 1);
	l_mem->listen = conn_tcp_listen_create(local_addr, &timeo);

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



static void *listener_new(struct listener_memory_st *lmem)
{
	//fprintf(stderr, "%s is running.\n", __FUNCTION__);
	//fprintf(stderr, "Set listen socket epoll event.\n");
	//if (imp_set_ioev(lmem->listen->sd, EPOLLIN|EPOLLRDHUP)<0) {
	//	mylog(L_ERR, "Set listen socket epoll event FAILED: %m\n");
	//}

	return NULL;
}

static int listener_delete(struct listener_memory_st *mem)
{
	mylog(L_DEBUG, "%s is running, free memory.\n", __FUNCTION__);
	free(mem);
	return 0;
}

static enum enum_driver_retcode listener_driver(struct listener_memory_st *lmem)
{
	static int count =10;
	struct echoer_memory_st *emem;
	conn_tcp_t *conn;
	imp_t *echoer;

	// TODO: Disable timer here.

	while (conn_tcp_accept_nb(&conn, lmem->listen, &timeo)==0) {
		emem = calloc(sizeof(*emem), 1);
		emem->conn = conn;
		echoer = imp_summon(emem, &echoer_soul);
		if (echoer==NULL) {
			mylog(L_ERR, "Failed to summon a new imp.");
			conn_tcp_close_nb(conn);
			free(emem);
			continue;
		}
	}

	imp_set_ioev(lmem->listen->sd, EPOLLIN|EPOLLOUT);
	return TO_BLOCK;
}

static void *listener_serialize(struct listener_memory_st *m)
{
	return NULL;
}


enum state_en {
	ST_RECV=1,
	ST_SEND,
	ST_TERM,
	ST_Ex
};

static void *echo_new(struct echoer_memory_st *m)
{
	m->state = ST_RECV;
	m->len = 0;
	m->pos = 0;
	return NULL;
}

static int echo_delete(struct echoer_memory_st *memory)
{
	conn_tcp_close_nb(memory->conn);
	free(memory);
	return 0;
}

static enum enum_driver_retcode echo_driver(struct echoer_memory_st *mem)
{
	ssize_t ret;

	switch (mem->state) {
		case ST_RECV:
			if (IMP_TIMEDOUT) {
				mylog(L_INFO, "[%d]: recv() timed out.", IMP_ID);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			mem->len = conn_tcp_recv_nb(mem->conn, mem->buf, BUFSIZE);
			if (mem->len == 0) {
				mem->state = ST_TERM;
				return TO_RUN;
			} else if (mem->len < 0) {
				if (errno==EAGAIN) {
					if (imp_set_ioev(mem->conn->sd, EPOLLIN|EPOLLRDHUP)<0) {
						mylog(L_ERR, "imp[%d]: Failed to imp_set_ioev() %m, suicide.");
						return TO_TERM;
					}
					return TO_BLOCK;
				} else {
					mem->state = ST_Ex;
					return TO_RUN;
				}
			} else {
				mem->pos = 0;
				mem->state = ST_SEND;
				return TO_RUN;
			}
			break;
		case ST_SEND:
			if (IMP_TIMEDOUT) {
				mylog(L_INFO, "[%d]: send(): timed out.", IMP_ID);
				mem->state = ST_Ex;
				return TO_RUN;
			}
			ret = conn_tcp_send_nb(mem->conn, mem->buf + mem->pos, mem->len);
			if (ret<=0) {
				if (errno==EAGAIN) {
					if (imp_set_ioev(mem->conn->sd, EPOLLOUT|EPOLLRDHUP)<0) {
						mylog(L_ERR, "imp[%d]: Failed to imp_set_ioev() %m, suicide.");
						return TO_TERM;
					}
					return TO_BLOCK;
				} else {
					mem->state = ST_Ex;
					return TO_RUN;
				}
			} else {
				mem->pos += ret;
				mem->len -= ret;
				if (mem->len <= 0) {
					mem->state = ST_RECV;
					return TO_RUN;
				} else {
					if (imp_set_ioev(mem->conn->sd, EPOLLOUT|EPOLLRDHUP)<0) {
						mylog(L_ERR, "imp[%d]: Failed to imp_set_ioev() %m, suicide.");
						return TO_TERM;
					}
					return TO_BLOCK;
				}
			}
			break;
		case ST_Ex:
			mylog(L_INFO, "[%d]: Exception happened.", IMP_ID);
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

static void *echo_serialize(struct echoer_memory_st *mem)
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

