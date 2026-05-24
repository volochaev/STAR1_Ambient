################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Lightning/base_scene.c \
../Lightning/color_worlds.c \
../Lightning/director.c \
../Lightning/driver.c \
../Lightning/event_layer.c \
../Lightning/frame_utils.c \
../Lightning/lighting_profile_registry.c \
../Lightning/oem_color_wheel.c \
../Lightning/overlay_palette.c \
../Lightning/scene_preset.c \
../Lightning/runtime_dimming_policy.c \
../Lightning/scene_color_model.c \
../Lightning/runtime_state.c \
../Lightning/zone_roles.c \
../Lightning/zones.c 

OBJS += \
./Lightning/base_scene.o \
./Lightning/color_worlds.o \
./Lightning/director.o \
./Lightning/driver.o \
./Lightning/event_layer.o \
./Lightning/frame_utils.o \
./Lightning/lighting_profile_registry.o \
./Lightning/oem_color_wheel.o \
./Lightning/overlay_palette.o \
./Lightning/scene_preset.o \
./Lightning/runtime_dimming_policy.o \
./Lightning/scene_color_model.o \
./Lightning/runtime_state.o \
./Lightning/zone_roles.o \
./Lightning/zones.o 

C_DEPS += \
./Lightning/base_scene.d \
./Lightning/color_worlds.d \
./Lightning/director.d \
./Lightning/driver.d \
./Lightning/event_layer.d \
./Lightning/frame_utils.d \
./Lightning/lighting_profile_registry.d \
./Lightning/oem_color_wheel.d \
./Lightning/overlay_palette.d \
./Lightning/scene_preset.d \
./Lightning/runtime_dimming_policy.d \
./Lightning/scene_color_model.d \
./Lightning/runtime_state.d \
./Lightning/zone_roles.d \
./Lightning/zones.d 


# Each subdirectory must supply rules for building sources it contributes
Lightning/%.o Lightning/%.su Lightning/%.cyclo: ../Lightning/%.c Lightning/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/CAN" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Lightning" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Lightning

clean-Lightning:
	-$(RM) ./Lightning/base_scene.cyclo ./Lightning/base_scene.d ./Lightning/base_scene.o ./Lightning/base_scene.su ./Lightning/color_worlds.cyclo ./Lightning/color_worlds.d ./Lightning/color_worlds.o ./Lightning/color_worlds.su ./Lightning/director.cyclo ./Lightning/director.d ./Lightning/director.o ./Lightning/director.su ./Lightning/driver.cyclo ./Lightning/driver.d ./Lightning/driver.o ./Lightning/driver.su ./Lightning/event_layer.cyclo ./Lightning/event_layer.d ./Lightning/event_layer.o ./Lightning/event_layer.su ./Lightning/frame_utils.cyclo ./Lightning/frame_utils.d ./Lightning/frame_utils.o ./Lightning/frame_utils.su ./Lightning/lighting_profile_registry.cyclo ./Lightning/lighting_profile_registry.d ./Lightning/lighting_profile_registry.o ./Lightning/lighting_profile_registry.su ./Lightning/oem_color_wheel.cyclo ./Lightning/oem_color_wheel.d ./Lightning/oem_color_wheel.o ./Lightning/oem_color_wheel.su ./Lightning/overlay_palette.cyclo ./Lightning/overlay_palette.d ./Lightning/overlay_palette.o ./Lightning/overlay_palette.su ./Lightning/scene_preset.cyclo ./Lightning/scene_preset.d ./Lightning/scene_preset.o ./Lightning/scene_preset.su ./Lightning/runtime_dimming_policy.cyclo ./Lightning/runtime_dimming_policy.d ./Lightning/runtime_dimming_policy.o ./Lightning/runtime_dimming_policy.su ./Lightning/scene_color_model.cyclo ./Lightning/scene_color_model.d ./Lightning/scene_color_model.o ./Lightning/scene_color_model.su ./Lightning/runtime_state.cyclo ./Lightning/runtime_state.d ./Lightning/runtime_state.o ./Lightning/runtime_state.su ./Lightning/zone_roles.cyclo ./Lightning/zone_roles.d ./Lightning/zone_roles.o ./Lightning/zone_roles.su ./Lightning/zones.cyclo ./Lightning/zones.d ./Lightning/zones.o ./Lightning/zones.su

.PHONY: clean-Lightning
