all:
	gcc send.c -o send
	gcc recv.c -o recv
	gcc -pthread agent.c -o agent

