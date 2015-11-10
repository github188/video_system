

#export LD_LIBRARY_PATH=/home/ligeng/work/learn/jweih/github/linux_client/lib

CC = g++



LOCAL_INC =  -I./include   -I./  -I../    -L./lib

LOCAL_LIB =    -lpthread  -lrt
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		$(LOCAL_SRC)/ring_queue.cpp \
		$(LOCAL_SRC)/time_unitl.cpp \
		$(LOCAL_SRC)/system_init.cpp \
		$(LOCAL_SRC)/net_send.cpp \
		$(LOCAL_SRC)/net_recv.cpp \
		$(LOCAL_SRC)/handle_packet.cpp \
		$(LOCAL_SRC)/eventfd.cpp \
		$(LOCAL_SRC)/process_loin.cpp \
		$(LOCAL_SRC)/main.cpp	


 






all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   client   ${LOCAL_LIB}

clean:
	rm -rf  client

