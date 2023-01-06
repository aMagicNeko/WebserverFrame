libmysql_pool.a: mysql_pool.o
	ar rcs $@ $^
mysql_pool.o: mysql_pool.cpp
	g++ mysql_pool.cpp -o mysql_pool.o -c

clean:
	rm mysql_pool.o
	rm libmysql_pool.a