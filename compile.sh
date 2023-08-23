cd src
gcc server.c udp_server.c udp_common.c -lpthread -lSDL -lm -o ../bin/server
gcc client.c udp_client.c udp_common.c -lpthread -lSDL -lm -o ../bin/client
