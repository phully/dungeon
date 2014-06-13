/** \file imp.h
	Defines imp entity.
*/

#ifndef IMP_H
#define IMP_H
/** \cond 0 */
#include <stdint.h>
/** \endcond */

#include "imp_body.h"
#include "imp_soul.h"

typedef uint32_t imp_id_t;

extern imp_id_t global_imp_id___;
#define GET_CURRENT_IMP_ID __sync_fetch_and_add(&global_imp_id___, 0)

#define	EV_MASK_KILL	0x0000000000000001ULL
#define	EV_MASK_TIMEOUT	0x0000000000000002ULL
#define	EV_MASK_EVENT	0x0000000000000004ULL

/** Imp struct.
	"Imp" is the name of FSM(Finite State Machine) instant in dungeon. They should be summoned(created) by rooms(modules) via room_service.h/imp_summon().
*/
typedef struct imp_st {
	imp_id_t id;		/**< Imp global uniq id */
	uint64_t request_mask;	/**< Imp request event mask */
	uint64_t event_mask;	/**< Imp event mask */

	imp_body_t *body;	/**< Imp body */
	imp_soul_t *soul;	/**< Imp soul. Reference to some room module. */
	void *memory;		/**< Imp local storage */
} imp_t;

/** Summon a new imp. And also append it into run_queue. Call by room module. */
imp_t *imp_summon(void *inventory, imp_soul_t *soul);

/** \todo Dismiss an imp */
int imp_dismiss(imp_t*);

void imp_driver(imp_t*);

void imp_wake(imp_t*);

int imp_set_ioev(imp_t*, int fd, struct epoll_event*);
int imp_get_ioev(imp_t*, struct epoll_event*);

int imp_set_timer(imp_t*, struct timeval*);

cJSON *imp_serailize(imp_t*);

#endif

