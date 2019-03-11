FTP:
	g++ -o ./bin/client -g ./src/Chat_Client.cpp ./src/simpleSocket.cpp -lpthread
	g++ -o ./bin/server -g ./src/Chat_Server.cpp ./src/simpleSocket.cpp -lpthread
clean:
	-rm -f ./bin/*
