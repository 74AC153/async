default: async_example

async_example_preproc.c: async_example.c
	gcc -E -P async_example.c > async_example_preproc.c
	
async_example: async_example_preproc.c
	gcc -Wall -Wextra -Werror -g -o async_example async_example_preproc.c

clean:
	rm async_example async_example_preproc.c

