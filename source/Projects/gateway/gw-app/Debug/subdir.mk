################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../gw-app.c 

OBJS += \
./gw-app.o 

C_DEPS += \
./gw-app.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabi-gcc -DBORDER_ROUTER=1 -DUIP_CONF_STATISTICS=1 -DLINUX_GATEWAY -DUIP_SMARTNET -DIS_ROOT=1 -DWITH_UIP6=1 -DWITH_ULPSMAC=1 -DUIP_CONF_TCP=0 -I"C:\home\ryan\shared\iot-smartnet-pg2-new\Components\inc" -I"C:\home\ryan\shared\iot-smartnet-pg2-new\Components\sn-gateway\include" -I"C:\home\ryan\shared\iot-smartnet-pg2-new\Components\sn-uip-2.7" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


