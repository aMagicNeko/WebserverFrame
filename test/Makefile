log_test: log_test.cpp
	g++ $< -o $@ -O2 -g -I ../log -l log -L ../log

memory_pool_test: memory_pool_test.cpp
	g++ $< -o $@ -O2 -g -I ../memory_pool -l mem -L ../memory_pool

mysql_pool_test: mysql_pool_test.cpp
	g++ $< -o $@ -O2 -g -I ../mysql_pool -l mysql_pool -L ../mysql_pool -L/usr/local/mysql/lib -lmysqlclient -lssl -lcrypto -lresolv
clean:
	rm memory_pool_test
	rm log_test
	rm mysql_pool_test