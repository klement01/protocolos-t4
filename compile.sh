cd src
gcc -I../include server.c udp_server.c udp_common.c simulation.c supervisory.c timer.c -lpthread -lSDL -lm -std=c11 -o ../bin/server
gcc -I../include client.c udp_client.c udp_common.c controller.c supervisory.c timer.c -lpthread -lSDL -lm -std=c11 -o ../bin/client
