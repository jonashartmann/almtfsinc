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
EXTRA_CFLAGS = -DALMTF_PACKETSIZE=1000 -DALMTF_PP=true -DALMTF_PP_BURST_LENGTH=2 -DALMTF_T_STAB=1000 -DALMTF_WINDOWSIZE=60.0 -DALMTF_RTT=500 -DALMTF_DTSINC=1.0 -DALMTF_EIMULTIPLIER=1 -DNETSTATE -DREAD_CONFIG #Constantes ALMTF
TFLAGS = -D_TSCI2_
BIN_FOLDER = bin

# All Target
#----para compilar somente a versão padrão, descomente a linha 18 e comente a linha 19
all: install
#all: install_no_bweq install_no_bweq_bwmax install_no_bwmax install

Receiver: $(OBJS)
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -o Receiver $(OBJS) $(LFLAGS)
	@echo 'Finished building target: $@'

Recv_Almtf.o: Recv_Almtf.h Recv_Almtf.cpp Recv_Core.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	      $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Almtf.cpp

Recv_Core.o: Recv_Core.h Recv_Core.cpp Recv_Almtf.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	     $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Core.cpp

Utils.o: $(UTILS_PATH)Utils.h $(UTILS_PATH)Utils.cpp
	 $(CC) $(CFLAGS) $(TFLAGS) $(UTILS_PATH)Utils.cpp

Socket_Utils.o: $(UTILS_PATH)Socket_Utils.h $(UTILS_PATH)Socket_Utils.cpp
		$(CC) $(CFLAGS) $(UTILS_PATH)Socket_Utils.cpp

install: Receiver
	@rm -rf $(BIN_FOLDER)
	@mkdir $(BIN_FOLDER)
	@mv Receiver $(BIN_FOLDER)/
	@cp ../inalm.txt $(BIN_FOLDER)/  
	@cp ../inconfig.txt $(BIN_FOLDER)/
	@rm *.o

clean:
	@rm -rf $(BIN_FOLDER)/

#--------------- TRECHO ADICIONADO PARA COMPILAR MÚLTIPLAS VERSÕES AO MESMO TEMPO------------------#
# comando make: compila
# comando make cleanall: limpa todas as instalações
BIN_FOLDER_NO_BWEQ = bin_no_bweq
BIN_FOLDER_NO_BWEQ_BWMAX = bin_no_bweq_bwmax
BIN_FOLDER_NO_BWMAX = bin_no_bwmax
#BIN_FOLDER_SINC_BWCW = bin_sinc_bwcw
BIN_ALL = bin*

install_no_bweq: Receiver_no_bweq
	@rm -rf $(BIN_FOLDER_NO_BWEQ)
	@mkdir $(BIN_FOLDER_NO_BWEQ)
	@mv Receiver_no_bweq $(BIN_FOLDER_NO_BWEQ)/
	@cp ../inalm.txt $(BIN_FOLDER_NO_BWEQ)/  
	
Receiver_no_bweq: Recv_Almtf_no_bweq.o Recv_Core.o Utils.o Socket_Utils.o
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -o Receiver_no_bweq Recv_Almtf_no_bweq.o Recv_Core.o Utils.o Socket_Utils.o $(LFLAGS)
	@echo 'Finished building target: $@'	
	
Recv_Almtf_no_bweq.o: Recv_Almtf.h Recv_Almtf_no_bweq.cpp Recv_Core.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	      $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Almtf_no_bweq.cpp	
	 
install_no_bweq_bwmax: Receiver_no_bweq_bwmax
	@rm -rf $(BIN_FOLDER_NO_BWEQ_BWMAX)
	@mkdir $(BIN_FOLDER_NO_BWEQ_BWMAX)
	@mv Receiver_no_bweq_bwmax $(BIN_FOLDER_NO_BWEQ_BWMAX)/
	@cp ../inalm.txt $(BIN_FOLDER_NO_BWEQ_BWMAX)/  
	
Receiver_no_bweq_bwmax: Recv_Almtf_no_bweq_bwmax.o Recv_Core.o Utils.o Socket_Utils.o
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -o Receiver_no_bweq_bwmax Recv_Almtf_no_bweq_bwmax.o Recv_Core.o Utils.o Socket_Utils.o $(LFLAGS)
	@echo 'Finished building target: $@'	
	
Recv_Almtf_no_bweq_bwmax.o: Recv_Almtf.h Recv_Almtf_no_bweq_bwmax.cpp Recv_Core.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	      $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Almtf_no_bweq_bwmax.cpp

install_no_bwmax: Receiver_no_bwmax
	@rm -rf $(BIN_FOLDER_NO_BWMAX)
	@mkdir $(BIN_FOLDER_NO_BWMAX)
	@mv Receiver_no_bwmax $(BIN_FOLDER_NO_BWMAX)/
	@cp ../inalm.txt $(BIN_FOLDER_NO_BWMAX)/  
	
Receiver_no_bwmax: Recv_Almtf_no_bwmax.o Recv_Core.o Utils.o Socket_Utils.o
	@echo 'Invoking: GCC C++ Linker'
	$(CC) -o Receiver_no_bwmax Recv_Almtf_no_bwmax.o Recv_Core.o Utils.o Socket_Utils.o $(LFLAGS)
	@echo 'Finished building target: $@'	
	
Recv_Almtf_no_bwmax.o: Recv_Almtf.h Recv_Almtf_no_bwmax.cpp Recv_Core.h $(ALMTF_H_PATH)Almtf.h $(UTILS_PATH)Utils.h $(UTILS_PATH)Socket_Utils.h  
	      $(CC) $(EXTRA_CFLAGS) $(CFLAGS) Recv_Almtf_no_bwmax.cpp

cleanall:
	@rm -rf $(BIN_ALL)/
