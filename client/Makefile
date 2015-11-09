

#export LD_LIBRARY_PATH=/home/ligeng/work/learn/jweih/github/linux_client/lib

CC = g++



LOCAL_INC =  -I./include   -I./     -L./lib

LOCAL_LIB =    -lpthread  -lavcodec  -lavutil
LOCAL_SRC =  .




LOCAL_DEV_SRC := \
		$(LOCAL_SRC)/main.cpp	


 






all:

	${CC}  ${LOCAL_INC}  ${LOCAL_DEV_SRC}   -o   client   ${LOCAL_LIB}

clean:
	rm -rf  client

