liblog.a : log.o util.o
	ar rcs $@ $^

%.o : %.cpp
	g++ $< -o $@ -c -g -O2 -c

clean :
	rm log.o
	rm util.o