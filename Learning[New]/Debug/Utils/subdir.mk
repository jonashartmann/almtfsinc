################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Utils/Socket_Utils.cpp \
../Utils/Utils.cpp 

OBJS += \
./Utils/Socket_Utils.o \
./Utils/Utils.o 

CPP_DEPS += \
./Utils/Socket_Utils.d \
./Utils/Utils.d 


# Each subdirectory must supply rules for building sources it contributes
Utils/%.o: ../Utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


