################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/STM/stm32f407_Wspc/PublicDrivers/usart/bsp_debug_usart.c 

OBJS += \
./usart/bsp_debug_usart.o 

C_DEPS += \
./usart/bsp_debug_usart.d 


# Each subdirectory must supply rules for building sources it contributes
usart/bsp_debug_usart.o: D:/STM/stm32f407_Wspc/PublicDrivers/usart/bsp_debug_usart.c usart/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/STM/stm32f407_Wspc/PublicDrivers/LCD" -I"D:/STM/stm32f407_Wspc/PublicDrivers/AD7190" -I"D:/STM/stm32f407_Wspc/PublicDrivers/usart" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-usart

clean-usart:
	-$(RM) ./usart/bsp_debug_usart.cyclo ./usart/bsp_debug_usart.d ./usart/bsp_debug_usart.o ./usart/bsp_debug_usart.su

.PHONY: clean-usart

