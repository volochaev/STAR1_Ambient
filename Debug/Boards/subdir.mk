################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Boards/board_dashboard.c \
../Boards/board_dispatch.c \
../Boards/board_door_fl.c \
../Boards/board_door_fr.c \
../Boards/board_door_rl.c \
../Boards/board_door_rr.c \
../Boards/board_led_backend.c \
../Boards/board_rear.c \
../Boards/board_zone_map.c 

OBJS += \
./Boards/board_dashboard.o \
./Boards/board_dispatch.o \
./Boards/board_door_fl.o \
./Boards/board_door_fr.o \
./Boards/board_door_rl.o \
./Boards/board_door_rr.o \
./Boards/board_led_backend.o \
./Boards/board_rear.o \
./Boards/board_zone_map.o 

C_DEPS += \
./Boards/board_dashboard.d \
./Boards/board_dispatch.d \
./Boards/board_door_fl.d \
./Boards/board_door_fr.d \
./Boards/board_door_rl.d \
./Boards/board_door_rr.d \
./Boards/board_led_backend.d \
./Boards/board_rear.d \
./Boards/board_zone_map.d 


# Each subdirectory must supply rules for building sources it contributes
Boards/%.o Boards/%.su Boards/%.cyclo: ../Boards/%.c Boards/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/CAN" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Lightning" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Boards

clean-Boards:
	-$(RM) ./Boards/board_dashboard.cyclo ./Boards/board_dashboard.d ./Boards/board_dashboard.o ./Boards/board_dashboard.su ./Boards/board_dispatch.cyclo ./Boards/board_dispatch.d ./Boards/board_dispatch.o ./Boards/board_dispatch.su ./Boards/board_door_fl.cyclo ./Boards/board_door_fl.d ./Boards/board_door_fl.o ./Boards/board_door_fl.su ./Boards/board_door_fr.cyclo ./Boards/board_door_fr.d ./Boards/board_door_fr.o ./Boards/board_door_fr.su ./Boards/board_door_rl.cyclo ./Boards/board_door_rl.d ./Boards/board_door_rl.o ./Boards/board_door_rl.su ./Boards/board_door_rr.cyclo ./Boards/board_door_rr.d ./Boards/board_door_rr.o ./Boards/board_door_rr.su ./Boards/board_led_backend.cyclo ./Boards/board_led_backend.d ./Boards/board_led_backend.o ./Boards/board_led_backend.su ./Boards/board_rear.cyclo ./Boards/board_rear.d ./Boards/board_rear.o ./Boards/board_rear.su ./Boards/board_zone_map.cyclo ./Boards/board_zone_map.d ./Boards/board_zone_map.o ./Boards/board_zone_map.su

.PHONY: clean-Boards

