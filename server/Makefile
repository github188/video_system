




CC = gcc


LOCAL_INC =  -I ../   

LOCAL_LIB =    -lpthread 
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		../ring_queue.c	\
		$(LOCAL_SRC)/server.c	

all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   server   ${LOCAL_LIB}

clean:
	rm -rf  server

