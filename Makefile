all: client serv1 serv2 serv3 serv4 
# all: client serv2

client:	client.c 
		gcc client.c -o client 
serv2:	serv2.c keyvalue.h
		gcc serv2.c -o serv2
serv1:	serv1.c keyvalue.h
	gcc serv1.c -o serv1
serv3:	serv3.c keyvalue.h
	gcc serv3.c -o serv3
serv4:	serv4.c keyvalue.h
	gcc serv4.c -o serv4 -lpthread 
