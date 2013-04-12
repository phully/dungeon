#include <stdio.h>
#include <sys/epoll.h>

#include "proxy_pool.h"

#define IOEV_SIZE	100
void *thr_worker(void *p)
{
	proxy_pool_t *pool=p;
	proxt_context_t *node;
	int err, num;
	struct epoll_event ioev[IOEV_SIZE];
	sigset_t allsig;

	sigfillset(&allsig);
	pthread_sigmask(SIG_BLOCK, &allsig, NULL);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	while (!pool->worker_quit) {
		while (1) {
			err = llist_fetch_head_nb(pool->run_queue, *node);
			if (err==AA_EAGAIN) {
				mylog("run_queue is empty.");
				break;
			}
			if (is_proxy_context_timedout(node)) {
				proxy_context_timedout(node);
			} else {
				proxy_context_driver(node);
			}
		}
		num = epoll_wait(epoll_fd, ioev, IOEV_SIZE, 1000);
		if (num<0) {
			mylog();
		} else if (num==0) {
			mylog();
		} else {
			for (i=0;i<num;++i) {
				proxy_context_put_runqueue(ioev[i].data.ptr);
			}
		}
	}
}

void *thr_maintainer(void *p)
{
	proxy_pool_t *pool=p;
	proxy_context_t *proxy;
	sigset_t allsig;

	sigfillset(&allsig);
	pthread_sigmask(SIG_BLOCK, &allsig, NULL);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	while (!pool->maintainer_quit) {
		pthread_mutex_lock();
		if (nr_idle<nr_minidle) {
			// Add some worker
		} else if (nr_idle>nr_maxidle) {
			// Remove some worker
		}
		pthread_mutex_unlock();

		while (llist_fetch_head_nb(terminated_queue, &proxy)==0) {
			// Log and destroy terminated proxy
		}

		usleep(1000);
	}
}

proxy_pool_t *proxy_pool_new(int nr_workers, int nr_accepters, int nr_max, int nr_total, int listensd)
{
	proxy_pool_t *newnode;

	if (nr_accepters<=0 || nr_accepters>=nr_max || nr_max<=1 || listensd<0) {
		return AA_EINVAL;
	}

	newnode = malloc(sizeof(*newnode));
	if (newnode==NULL) {
		mylog();
		return AA_ENOMEM;
	}

	newnode->nr_max = nr_max;
	newnode->nr_busy = 0;
	newnode->run_queue = llist_new(nr_total);
	newnode->iowait_queue_ht = hasht_new(NULL, nr_total);
	newnode->terminated_queue = llist_new(nr_total);
	newnode->original_listen_sd = listen_sd;

	newnode->worker = malloc(sizeof(pthread_t)*nr_workers);
	if (newnode->worker==NULL) {
		mylog();
		exit(1);
	}
	for (i=0;i<nr_workers;++i) {
		err = pthread_create(newnode->worker+i, NULL, thr_worker, newnode);
		if (err) {
			mylog();
			break;
		}
	}
	if (i==0) {
		mylog();
		abort();
	} else {
		mylog();
	}

	err = pthread_create(&newnode->maintainer, NULL, thr_maintainer, newnode);
	if (err) {
		mylog();
		break;
	}

	for (i=0;i<nr_minidle;++i) {
		proxy_context_new(newnode);
	}

	return newnode;
}

int proxy_pool_delete(proxy_pool_t *pool)
{
	register int i;
	proxy_context_t *proxy;
	llist_node_t *curr;

	if (pool->worker_quit == 0) {
		pool->worker_quit = 1;
		for (i=0;i<pool->nr_workers;++i) {
			pthread_join(pool->worker[i], NULL);
		}
	}

	if (pool->maintainer_quit == 0) {
		pool->maintainer_quit = 1;
		pthread_join(pool->maintainer, NULL);
	}

	for (curr=pool->terminated_queue->dumb.next;
		curr!=&pool->terminated_queue->dumb;
		curr=curr->next) {
		proxy_context_delete(curr->ptr);
		curr->ptr = NULL;
	}
	llist_delete(pool->run_queue);

	for (curr=pool->run_queue->dumb.next;
		curr!=&pool->run_queue->dumb;
		curr=curr->next) {
		proxy_context_delete(curr->ptr);
		curr->ptr = NULL;
	}
	llist_delete(pool->run_queue);

	free(pool);

	return 0;
}

cJSON *proxy_pool_info(proxy_pool_t *my)
{
	cJSON *result;

	result = cJSON_CreateObject();

	cJSON_AddItemToObject(result, "MinIdleProxy", cJSON_CreateNumber(pool->nr_minidle));
	cJSON_AddItemToObject(result, "MaxIdleProxy", cJSON_CreateNumber(pool->nr_maxidle));
	cJSON_AddItemToObject(result, "MaxProxy", cJSON_CreateNumber(pool->nr_total));
	cJSON_AddItemToObject(result, "NumIdle", cJSON_CreateNumber(pool->nr_idle));
	cJSON_AddItemToObject(result, "NumBusy", cJSON_CreateNumber(pool->nr_busy));
	cJSON_AddItemToObject(result, "RunQueue", llist_info_json(pool->run_queue));
	cJSON_AddItemToObject(result, "TerminatedQueue", llist_info_json(pool->terminated_queue));
	cJSON_AddItemToObject(result, "NumWorkerThread",cJSON_CreateNumber(pool->nr_workers));

	return result;
}
