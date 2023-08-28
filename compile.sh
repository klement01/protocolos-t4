rm bin/server bin/client -f
cd src
gcc -I../include server.c udp_server.c udp_common.c simulation.c supervisory.c timer.c -lpthread -lSDL -lm -lrt -std=c99 -o ../bin/server
gcc -I../include client.c udp_client.c udp_common.c controller.c supervisory.c timer.c -lpthread -lSDL -lm -lrt -std=c99 -o ../bin/client
