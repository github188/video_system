




CC = gcc


LOCAL_INC =  -I ../   

LOCAL_LIB =    -lpthread   -lrt
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		../ring_queue.c	\
		../time_unitl.c \
		$(LOCAL_SRC)/server.c	

all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   server   ${LOCAL_LIB}

clean:
	rm -rf  server

