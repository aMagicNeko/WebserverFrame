libmem.a: thread_cache.o central_cache.o page_cache.o
	ar rcs $@ $^

%.o: %.c
	g++ -c $^ -o $@ -g

clean:
	rm thread_cache.o
	rm central_cache.o
	rm page_cache.o