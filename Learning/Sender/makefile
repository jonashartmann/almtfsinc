# Makefile para o Sender

OBJS = Sender.o Utils.o Socket_Utils.o
ALMTF_H_PATH = ../
UTILS_PATH = ../Utils/
CC = g++
DEBUG = -g
CFLAGS = -c $(DEBUG)
LIBS=../Lib
LTSCI2=-ltsci2
LFLAGS = -L$(LIBS) -lpthread $(LTSCI2) -lm -lrt $(DEBUG)
EXTRA_CFLAGS = -DALMTF_PACKETSIZE=1000 -DALMTF_PP=true -DALMTF_PP_BURST_LENGTH=2 -DALMTF_T_STAB=1000 -DALMTF_WINDOWSIZE=60.0 -DALMTF_RTT=500 -DALMTF_DTSINC=1.0 #Constantes ALMTF
TFLAGS = -D_TSCI2_

# All Target
all: Sender

Sender: $(OBJS) 
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -o Sender $(OBJS) $(LFLAGS)
	@echo 'Finished building target: $@'
	@rm -rf bin	 
	@mkdir bin
	@mv Sender bin/
	@cp ../inalm.txt bin/
	@cp bootlog.txt bin/
	@rm *.o

Sender.o: Sender.h Sender.cpp $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h 
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) Sender.cpp

Utils.o: $(UTILS_PATH)Utils.h $(UTILS_PATH)Utils.cpp 
	$(CC) $(CFLAGS) $(TFLAGS) $(UTILS_PATH)Utils.cpp

Socket_Utils.o: $(UTILS_PATH)Socket_Utils.h $(UTILS_PATH)Socket_Utils.cpp 
		$(CC) $(CFLAGS) $(UTILS_PATH)Socket_Utils.cpp

clean:
	@rm -rf bin/
