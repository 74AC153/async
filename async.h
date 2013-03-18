#ifndef ASYNC_H
#define ASYNC_H

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

/*
	This header file contains a set of C preprocessor macros that simplify
	asynchronous function calls. For each function call, a thread is created
	and the function is run with the specified arguments. A token associated
	with this function call can be passed around, and the return value can be
	obtained when finally necessary.

	ASYNC_PROTO(RET_TYPE, FUN, ARGLIST) -- emit a prototype declaration for a
	  function that is run asynchronously. This is required for all functions.
	ASYNC_IMPL(FUN) -- emit another prototype declaration to be used as the
	  header for a function definition. The is only used when the declaration 
	  and definition of a function are done separately.
	ASYNC_ARG(ARG) -- value of the named argument to an asynchronous function.
	ASYNC_RETURN(VAL) -- exit the asynchronous function returning VAL.
	
	ASYNC_ID(FUN) -- declare an identifier that holds state to capture the
	  result of an asynchronously run function.
	ASYNC_CALL(IDREF, FUN, ARGLIST) -- make an asynchronous function call to
	  fun, with args ARGLIST, and token IDREF declared using ASYNC_ID(FUN).
	ASYNC_CALL_STATUS(IDREF) -- value of errno associated with thread creation 
	  for the the matching asynchronous function call.
	ASYNC_RESULT(IDREF) -- the return value of an asynchronously run function.
	  Can be called more than once. Will block if the function attached to the
	  id has not completed yet.
	ASYNC_RESULT_STATUS(IDREF) -- value of errno associated with the thread
	  join associated with the matching asynchronous function call.

	An example of a fully asynchronous recursive factorial calculation:

#include <stdio.h>
#include <stdint.h>
#include "async.h"

ASYNC_PROTO(int, factorial, { uint64_t n; })
{
	uint64_t child_result;
	ASYNC_ID(factorial) childid; // metadata to hold state to attach later
	if(ASYNC_ARG(n) == 0) ASYNC_RETURN(1);
	ASYNC_CALL(&childid, factorial, { ASYNC_ARG(n) - 1 });
	ASYNC_RETURN(ASYNC_ARG(n) * ASYNC_RESULT(&childid));
}

// NB: can also do this
// .h file: ASYNC_PROTO(int, factorial, { uint64_t n });
// .c file: ASYNC_IMPL(factorial) { ...  }

ASYNC_ID(factorial) *factorial_start(uint64_t n)
{
	ASYNC_ID(factorial) *fact_hdl = malloc(sizeof(ASYNC_ID(factorial)));
	ASYNC_CALL(fact_hdl, factorial, { n });
	if(ASYNC_CALL_STATUS(fact_hdl)) {
		printf("%s", strerror(ASYNC_CALL_STATUS(fact_hdl)));
	}
	return fact_hdl;
}

uint64_t factorial_finish(ASYNC_ID(factorial) *fact_hdl)
{
	uint64_t val = ASYNC_RESULT(fact_hdl);
	if(ASYNC_RESULT_STATUS(fact_hdl)) {
		printf("%s", strerror(ASYNC_RESULT_STATUS(fact_hdl)));
	}
	free(fact_hdl);
	return val;
}

int main(int argc, char *argv[])
{
	ASYNC_ID(factorial) *fact_hdl;
	uint64_t val = strtol(argv[1], NULL, 0);
	uint64_t factval;
	fact_hdl = factorial_start(val);
	factval = factorial_finish(fact_hdl);
	printf("%llu! = %llu\n", val, factval);
	return 0;
}

*/

#define ASYNC_PROTO(RET_TYPE, FUN, ARGLIST) \
typedef struct { \
  struct ARGLIST funargs; \
  RET_TYPE retval; \
} __async_ ## FUN ## _args; \
typedef struct { \
	pthread_t thread; \
	__async_ ## FUN ## _args args; \
	bool joined; \
	int call_stat, res_stat; \
} __async_ ## FUN ## _id; \
void * FUN ( __async_ ## FUN ## _args *args )

#define ASYNC_IMPL(FUN) \
void * FUN ( __async_ ## FUN ## _args *args )

#define ASYNC_ARG(ARG) \
(args->funargs.ARG)

#define ASYNC_RETURN(VAL) \
do { \
  args->retval = (VAL); \
  return NULL; \
} while (0)

#define ASYNC_ID(FUN) \
	__async_ ## FUN ## _id

#define ASYNC_CALL(IDREF, FUN, ARGLIST) \
do { \
  __async_ ## FUN ## _args args_loc = ARGLIST; \
  memcpy(&(IDREF)->args, &args_loc, sizeof(args_loc)); \
  if(pthread_create(&((IDREF)->thread), NULL, \
                    (void * (*)(void *)) FUN, &(IDREF)->args)) \
    (IDREF)->call_stat = errno; \
  else \
    (IDREF)->call_stat = 0; \
  (IDREF)->joined = false; \
} while(0) 

#define ASYNC_CALL_STATUS(IDREF) \
  (IDREF)->call_stat

#define ASYNC_RESULT(IDREF) \
( ((IDREF)->joined ? 0 : \
     ( pthread_join((IDREF)->thread, NULL) ? (IDREF)->res_stat = errno : 0 ), \
        (IDREF)->joined = true ), \
  (IDREF)->args.retval )

#define ASYNC_RESULT_STATUS(IDREF) \
( ((IDREF)->joined ? 0 : \
     ( pthread_join((IDREF)->thread, NULL) ? (IDREF)->res_stat = errno : 0 ), \
        (IDREF)->joined = true ), \
  (IDREF)->res_stat )

#endif
