CC = gcc
CFLAGS = -Wall -pthread -ljson-c -Ifeatures -Ihandler -Iroutes

all: server

server: main.o handler/sse.o handler/http_handler.o features/user.o features/room.o features/game.o features/leaderboard.o utils/utils.o core/server.o routes/auth_routes.o middleware/cookies.o
	$(CC) -o server main.o handler/sse.o handler/http_handler.o features/user.o features/room.o features/game.o features/leaderboard.o utils/utils.o core/server.o routes/auth_routes.o middleware/cookies.o $(CFLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

core/server.o: core/server.c core/server.h
	$(CC) -c core/server.c -o core/server.o $(CFLAGS)

handler/sse.o: handler/sse.c handler/sse.h
	$(CC) -c handler/sse.c -o handler/sse.o $(CFLAGS)

handler/http_handler.o: handler/http_handler.c handler/http_handler.h handler/sse.h
	$(CC) -c handler/http_handler.c -o handler/http_handler.o $(CFLAGS)

features/user.o: features/user.c features/user.h
	$(CC) -c features/user.c -o features/user.o $(CFLAGS)

features/room.o: features/room.c features/room.h
	$(CC) -c features/room.c -o features/room.o $(CFLAGS)

features/game.o: features/game.c features/game.h
	$(CC) -c features/game.c -o features/game.o $(CFLAGS)

features/leaderboard.o: features/leaderboard.c features/leaderboard.h
	$(CC) -c features/leaderboard.c -o features/leaderboard.o $(CFLAGS)

utils/utils.o: utils/utils.c utils/utils.h
	$(CC) -c utils/utils.c -o utils/utils.o $(CFLAGS)

routes/auth_routes.o: routes/auth_routes.c routes/auth_routes.h
	$(CC) -c routes/auth_routes.c -o routes/auth_routes.o $(CFLAGS)

middleware/cookies.o: middleware/cookies.c middleware/cookies.h
	$(CC) -c middleware/cookies.c -o middleware/cookies.o $(CFLAGS)

clean:
	rm -f main.o handler/*.o features/*.o utils/*.o routes/*.o server
