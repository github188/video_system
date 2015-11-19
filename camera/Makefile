

CC = arm-none-linux-gnueabi-gcc


#CC = gcc



LOCAL_INC =  -I./  -I../  -L./lib

LOCAL_LIB =    -lpthread  -lakuio  -lakstreamenclib  -lrt
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		../common.c	\
		$(LOCAL_SRC)/parser_inifile.c	\
		$(LOCAL_SRC)/socket_init.c	\
		$(LOCAL_SRC)/time_unitl.c \
		$(LOCAL_SRC)/link_net.c	\
		$(LOCAL_SRC)/net_send.c  \
		$(LOCAL_SRC)/net_recv.c  \
		$(LOCAL_SRC)/handle_packet.c  \
		$(LOCAL_SRC)/monitor_user.c  \
		$(LOCAL_SRC)/eventfd.c  \
		$(LOCAL_SRC)/system_init.c	\
		$(LOCAL_SRC)/ring_queue.c  \
		$(LOCAL_SRC)/main.c	


 






all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   exe_test   ${LOCAL_LIB}

clean:
	rm -rf  exe_text

