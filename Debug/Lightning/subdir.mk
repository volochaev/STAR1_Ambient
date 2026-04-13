################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Lightning/base_scene.c \
../Lightning/director.c \
../Lightning/driver.c \
../Lightning/effects.c \
../Lightning/event_layer.c \
../Lightning/frame_utils.c \
../Lightning/palette.c \
../Lightning/runtime_state.c \
../Lightning/themes.c \
../Lightning/zones.c 

OBJS += \
./Lightning/base_scene.o \
./Lightning/director.o \
./Lightning/driver.o \
./Lightning/effects.o \
./Lightning/event_layer.o \
./Lightning/frame_utils.o \
./Lightning/palette.o \
./Lightning/runtime_state.o \
./Lightning/themes.o \
./Lightning/zones.o 

C_DEPS += \
./Lightning/base_scene.d \
./Lightning/director.d \
./Lightning/driver.d \
./Lightning/effects.d \
./Lightning/event_layer.d \
./Lightning/frame_utils.d \
./Lightning/palette.d \
./Lightning/runtime_state.d \
./Lightning/themes.d \
./Lightning/zones.d 


# Each subdirectory must supply rules for building sources it contributes
Lightning/%.o Lightning/%.su Lightning/%.cyclo: ../Lightning/%.c Lightning/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/CAN" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Lightning" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Lightning

clean-Lightning:
	-$(RM) ./Lightning/base_scene.cyclo ./Lightning/base_scene.d ./Lightning/base_scene.o ./Lightning/base_scene.su ./Lightning/director.cyclo ./Lightning/director.d ./Lightning/director.o ./Lightning/director.su ./Lightning/driver.cyclo ./Lightning/driver.d ./Lightning/driver.o ./Lightning/driver.su ./Lightning/effects.cyclo ./Lightning/effects.d ./Lightning/effects.o ./Lightning/effects.su ./Lightning/event_layer.cyclo ./Lightning/event_layer.d ./Lightning/event_layer.o ./Lightning/event_layer.su ./Lightning/frame_utils.cyclo ./Lightning/frame_utils.d ./Lightning/frame_utils.o ./Lightning/frame_utils.su ./Lightning/palette.cyclo ./Lightning/palette.d ./Lightning/palette.o ./Lightning/palette.su ./Lightning/runtime_state.cyclo ./Lightning/runtime_state.d ./Lightning/runtime_state.o ./Lightning/runtime_state.su ./Lightning/themes.cyclo ./Lightning/themes.d ./Lightning/themes.o ./Lightning/themes.su ./Lightning/zones.cyclo ./Lightning/zones.d ./Lightning/zones.o ./Lightning/zones.su

.PHONY: clean-Lightning

