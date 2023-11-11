all: master server
	
clean-logs:
	rm build/log/*
clean:
	rm build/master build/server
master:
	gcc master.c -o build/master
server:
	gcc server.c -o build/server
