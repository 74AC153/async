#ifndef ASYNC_H
#define ASYNC_H

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define ASYNC_PROTO(RET_TYPE, FUN, ARGLIST) \
	typedef struct ARGLIST __async_ ## FUN ## _args_t; \
	struct __async_ ## FUN ## _io { \
		__async_ ## FUN ## _args_t args; \
		RET_TYPE retval; \
		bool finished; \
		struct __async_ ## FUN ## _id *autofree; \
	}; \
	typedef struct __async_ ## FUN ## _io __async_ ## FUN ## _io_t; \
	struct __async_ ## FUN ## _id { \
		pthread_t thread; \
		__async_ ## FUN ## _io_t io; \
		bool joined; \
		int call_stat, res_stat; \
	} ; \
	typedef struct __async_ ## FUN ## _id __async_ ## FUN ## _id_t; \
	void * FUN ( __async_ ## FUN ## _io_t *io )

#define ASYNC_IMPL(FUN) \
	void * FUN ( __async_ ## FUN ## _io_t *io )

#define ASYNC_ARG(ARG) \
	(io->args.ARG)

#define ASYNC_RETURN(VAL) \
	do { \
		if(io->autofree) { \
			pthread_detach(io->autofree->thread); \
			free(io->autofree); \
		} else { \
			io->retval = (VAL); \
			io->finished = true; \
		} \
		return NULL; \
	} while (0)

#define ASYNC_ID(FUN) \
	__async_ ## FUN ## _id_t

#define ASYNC_CALL(IDREF, FUN, ARGS) \
	do { \
		(IDREF)->io.args = ((__async_ ## FUN ## _args_t) ARGS); \
		(IDREF)->io.finished = false; \
		(IDREF)->io.autofree = NULL; \
		(IDREF)->joined = false; \
		(IDREF)->call_stat = 0; \
		(IDREF)->res_stat = 0; \
		if(pthread_create(&((IDREF)->thread), NULL, \
		                  (void * (*)(void *)) FUN, &(IDREF)->io)) { \
			(IDREF)->call_stat = errno; \
		} \
	} while(0) 

#define ASYNC_DETACH(INT_STATUS, FUN, ARGS) \
	do { \
		__async_ ## FUN ## _id_t *id_discard = malloc(sizeof(__async_ ## FUN ## _id_t)); \
		if(! id_discard) { \
			(INT_STATUS) = -1; \
		} else { \
			id_discard->io.args = ((__async_ ## FUN ## _args_t) ARGS); \
			id_discard->io.autofree = id_discard; \
			id_discard->call_stat = 0; \
			id_discard->res_stat = 0; \
			(INT_STATUS) = pthread_create(&(id_discard->thread), NULL, \
			                              (void * (*)(void *)) FUN, &(id_discard)->io); \
			if(INT_STATUS) { \
				free(id_discard); \
				(INT_STATUS) = errno; \
			} \
		} \
	} while(0)

#define ASYNC_CALL_STATUS(IDREF) \
	(IDREF)->call_stat

#define ASYNC_RESULT_READY(IDREF) \
	((IDREF)->io.finished)

#define ASYNC_RESULT_WAIT(IDREF) \
	((void) \
	((IDREF)->joined ? 0 : \
	     ( pthread_join((IDREF)->thread, NULL) ? (IDREF)->res_stat = errno : 0 ), \
	        (IDREF)->joined = true )\
	)

#define ASYNC_RESULT_STATUS(IDREF) \
	( ASYNC_RESULT_WAIT(IDREF), (IDREF)->res_stat )

#define ASYNC_RESULT(IDREF) \
	( ASYNC_RESULT_WAIT(IDREF), (IDREF)->io.retval )

#endif
