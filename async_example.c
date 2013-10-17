#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "async.h"

ASYNC_PROTO(int, factorial, { uint64_t n; })
{
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

ASYNC_PROTO(int, sleeper, {int *sem; })
{
	int secs = 5;
	while(secs > 0) {
		printf("%d...\n", secs);
		sleep(1);
		secs--;
	}
	printf("boom!\n");
	*ASYNC_ARG(sem) = 1;
	ASYNC_RETURN(0);
}

int main(void)
{
	ASYNC_ID(factorial) *fact_hdl;
	uint64_t val = 5;
	uint64_t factval;
	int result;
	int status;

	fact_hdl = factorial_start(val);
	factval = factorial_finish(fact_hdl);
	printf("%llu! = %llu\n", val, factval);

	ASYNC_DETACH(status, sleeper, { &result });
	while(!result);

	return 0;
}

