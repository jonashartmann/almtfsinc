################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Receiver/Recv_Almtf.cpp \
../Receiver/Recv_Core.cpp 

OBJS += \
./Receiver/Recv_Almtf.o \
./Receiver/Recv_Core.o 

CPP_DEPS += \
./Receiver/Recv_Almtf.d \
./Receiver/Recv_Core.d 


# Each subdirectory must supply rules for building sources it contributes
Receiver/%.o: ../Receiver/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


