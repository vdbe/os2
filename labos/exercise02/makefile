TITLE_COLOR = \033[33m
NO_COLOR = \033[0m

# when executing make, compile all executables
all: test_server client_node

test_server : test_server.c lib/libtcpsock.so
	@mkdir -p build
	@echo "$(TITLE_COLOR)\n***** COMPILING test_server *****$(NO_COLOR)"
	gcc -c test_server.c -Wall -Werror -o build/test_server.o -g
	@echo "$(TITLE_COLOR)\n***** LINKING test_server *****$(NO_COLOR)"
	gcc build/test_server.o -ltcpsock -o test_server -Wall -L./lib -Wl,-rpath=./lib

client_node : client_node.c lib/libtcpsock.so
	@mkdir -p build
	@echo "$(TITLE_COLOR)\n***** COMPILING client_node *****$(NO_COLOR)"
	gcc -c client_node.c -Wall -Werror -o build/client_node.o -D LOOPS=3 -g
	@echo "$(TITLE_COLOR)\n***** LINKING client_node *****$(NO_COLOR)"
	gcc build/client_node.o -ltcpsock -o client_node -Wall -L./lib -Wl,-rpath=./lib

# If you only want to compile one of the libs, this target will match (e.g. make libtcpsoc)
libtcpsock : lib/libtcpsock.so

lib/libtcpsock.so : lib/tcpsock.c
	@mkdir -p build
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB tcpsock *****$(NO_COLOR)"
	gcc -c lib/tcpsock.c -Wall -Werror -fPIC -o build/tcpsock.o -g
	@echo "$(TITLE_COLOR)\n***** LINKING LIB tcpsock *****$(NO_COLOR)"
	gcc build/tcpsock.o -o lib/libtcpsock.so -Wall -shared -lm

# do not look for files called clean, clean-all or this will be always a target
.PHONY : clean clean-all run

clean:
	rm -rf build test_server client_node *~

clean-all: clean
	rm -rf lib/*.so

run : test_server client_node
	@echo "Add your own implementation here..."

debug-server: test_server
	@echo "$(TITLE_COLOR)\n***** Running GDB on test_server *****$(NO_COLOR)"
	gdb -q -ex run --args ./test_server

debug-client: client_node
	@echo "$(TITLE_COLOR)\n***** Running GDB on client_node *****$(NO_COLOR)"
	gdb -q -ex run --args ./client_node 2 127.0.0.1 5678