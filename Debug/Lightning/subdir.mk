################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Lightning/driver.c \
../Lightning/effects.c \
../Lightning/frame_utils.c \
../Lightning/palette.c \
../Lightning/presets.c \
../Lightning/scene_player.c \
../Lightning/zones.c 

OBJS += \
./Lightning/driver.o \
./Lightning/effects.o \
./Lightning/frame_utils.o \
./Lightning/palette.o \
./Lightning/presets.o \
./Lightning/scene_player.o \
./Lightning/zones.o 

C_DEPS += \
./Lightning/driver.d \
./Lightning/effects.d \
./Lightning/frame_utils.d \
./Lightning/palette.d \
./Lightning/presets.d \
./Lightning/scene_player.d \
./Lightning/zones.d 


# Each subdirectory must supply rules for building sources it contributes
Lightning/%.o Lightning/%.su Lightning/%.cyclo: ../Lightning/%.c Lightning/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/CAN" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Lightning" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Lightning

clean-Lightning:
	-$(RM) ./Lightning/driver.cyclo ./Lightning/driver.d ./Lightning/driver.o ./Lightning/driver.su ./Lightning/effects.cyclo ./Lightning/effects.d ./Lightning/effects.o ./Lightning/effects.su ./Lightning/frame_utils.cyclo ./Lightning/frame_utils.d ./Lightning/frame_utils.o ./Lightning/frame_utils.su ./Lightning/palette.cyclo ./Lightning/palette.d ./Lightning/palette.o ./Lightning/palette.su ./Lightning/presets.cyclo ./Lightning/presets.d ./Lightning/presets.o ./Lightning/presets.su ./Lightning/scene_player.cyclo ./Lightning/scene_player.d ./Lightning/scene_player.o ./Lightning/scene_player.su ./Lightning/zones.cyclo ./Lightning/zones.d ./Lightning/zones.o ./Lightning/zones.su

.PHONY: clean-Lightning

