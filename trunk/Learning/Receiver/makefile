# Makefile para o Receiver

OBJS = Recv_Almtf.o Recv_Core.o Utils.o Socket_Utils.o
ALMTF_H_PATH = ../
UTILS_PATH = ../Utils/
CC = g++
DEBUG = -g
CFLAGS = -c $(DEBUG)
LIBS=../Lib
LTSCI2=-ltsci2
LFLAGS = -L$(LIBS) -lpthread $(LTSCI2) -lm -lrt $(DEBUG)
EXTRA_CFLAGS = -DALMTF_PACKETSIZE=1000 -DALMTF_PP=true -DALMTF_PP_BURST_LENGTH=2 -DALMTF_T_STAB=1000 -DALMTF_WINDOWSIZE=60.0 -DALMTF_RTT=500 -DALMTF_DTSINC=1.0 -DALMTF_EIMULTIPLIER=1 -DMULT_INC=3 -DMULT_LIMIT=120 -DFAIL_TIME=10000#Constantes ALMTF
TFLAGS = -D_TSCI2_

# All Target
all : Receiver

Receiver: $(OBJS)
	  @echo 'Invoking: GCC C++ Linker'
	  $(CC) -o Receiver $(OBJS) $(LFLAGS)
	  @echo 'Finished building target: $@'
	  @rm -rf bin
	  @mkdir bin
	  @mv Receiver bin/
	  @cp ../inalm.txt bin/  
	  @rm *.o

Recv_Almtf.o: Recv_Almtf.h Recv_Almtf.cpp Recv_Core.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	      $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Almtf.cpp

Recv_Core.o: Recv_Core.h Recv_Core.cpp Recv_Almtf.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	     $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Core.cpp

Utils.o: $(UTILS_PATH)Utils.h $(UTILS_PATH)Utils.cpp
	 $(CC) $(CFLAGS) $(TFLAGS) $(UTILS_PATH)Utils.cpp

Socket_Utils.o: $(UTILS_PATH)Socket_Utils.h $(UTILS_PATH)Socket_Utils.cpp
		$(CC) $(CFLAGS) $(UTILS_PATH)Socket_Utils.cpp

clean:
	@rm -rf bin/
