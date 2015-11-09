




CC = gcc


LOCAL_INC =  -I ./   -I../

LOCAL_LIB =    -lpthread   -lrt
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		$(LOCAL_SRC)/ring_queue.c	\
		$(LOCAL_SRC)/time_unitl.c \
		$(LOCAL_SRC)/server.c	

all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   server   ${LOCAL_LIB}

clean:
	rm -rf  server

