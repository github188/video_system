



CC = gcc



LOCAL_INC =  -I./  -I../  -L./lib

LOCAL_LIB =    -lpthread   -lavcodec  -lavutil  -lrt
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		../common.c	\
		$(LOCAL_SRC)/socket_init.c	\
		$(LOCAL_SRC)/time_unitl.c \
		$(LOCAL_SRC)/net_send.c  \
		$(LOCAL_SRC)/net_recv.c  \
		$(LOCAL_SRC)/handle_packet.c  \
		$(LOCAL_SRC)/eventfd.c  \
		$(LOCAL_SRC)/system_init.c	\
		$(LOCAL_SRC)/process_loin.c \
		$(LOCAL_SRC)/ring_queue.c  \
		$(LOCAL_SRC)/main.c	


 






all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   client   ${LOCAL_LIB}

clean:
	rm -rf  client

