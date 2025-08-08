################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/STM/stm32f407_Wspc/PublicDrivers/LCD/lcd.c 

OBJS += \
./LCD/lcd.o 

C_DEPS += \
./LCD/lcd.d 


# Each subdirectory must supply rules for building sources it contributes
LCD/lcd.o: D:/STM/stm32f407_Wspc/PublicDrivers/LCD/lcd.c LCD/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/STM/stm32f407_Wspc/PublicDrivers/LCD" -I"D:/STM/stm32f407_Wspc/PublicDrivers/AD7190" -I"D:/STM/stm32f407_Wspc/PublicDrivers/usart" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-LCD

clean-LCD:
	-$(RM) ./LCD/lcd.cyclo ./LCD/lcd.d ./LCD/lcd.o ./LCD/lcd.su

.PHONY: clean-LCD

