################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_runtime.c \
../Core/Src/brightness_pipeline.c \
../Core/Src/handle_pwm.c \
../Core/Src/led_runtime.c \
../Core/Src/main.c \
../Core/Src/runtime_can.c \
../Core/Src/runtime_debug_hooks.c \
../Core/Src/runtime_event_queue.c \
../Core/Src/runtime_events.c \
../Core/Src/runtime_flow.c \
../Core/Src/runtime_render.c \
../Core/Src/runtime_state_machine.c \
../Core/Src/runtime_stop.c \
../Core/Src/stm32g4xx_hal_msp.c \
../Core/Src/stm32g4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32g4xx.c 

OBJS += \
./Core/Src/app_runtime.o \
./Core/Src/brightness_pipeline.o \
./Core/Src/handle_pwm.o \
./Core/Src/led_runtime.o \
./Core/Src/main.o \
./Core/Src/runtime_can.o \
./Core/Src/runtime_debug_hooks.o \
./Core/Src/runtime_event_queue.o \
./Core/Src/runtime_events.o \
./Core/Src/runtime_flow.o \
./Core/Src/runtime_render.o \
./Core/Src/runtime_state_machine.o \
./Core/Src/runtime_stop.o \
./Core/Src/stm32g4xx_hal_msp.o \
./Core/Src/stm32g4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32g4xx.o 

C_DEPS += \
./Core/Src/app_runtime.d \
./Core/Src/brightness_pipeline.d \
./Core/Src/handle_pwm.d \
./Core/Src/led_runtime.d \
./Core/Src/main.d \
./Core/Src/runtime_can.d \
./Core/Src/runtime_debug_hooks.d \
./Core/Src/runtime_event_queue.d \
./Core/Src/runtime_events.d \
./Core/Src/runtime_flow.d \
./Core/Src/runtime_render.d \
./Core/Src/runtime_state_machine.d \
./Core/Src/runtime_stop.d \
./Core/Src/stm32g4xx_hal_msp.d \
./Core/Src/stm32g4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32g4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/CAN" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Lightning" -I"/Users/nv/STM32CubeIDE/workspace_1.19.0/STAR1_Ambient/Boards" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_runtime.cyclo ./Core/Src/app_runtime.d ./Core/Src/app_runtime.o ./Core/Src/app_runtime.su ./Core/Src/brightness_pipeline.cyclo ./Core/Src/brightness_pipeline.d ./Core/Src/brightness_pipeline.o ./Core/Src/brightness_pipeline.su ./Core/Src/handle_pwm.cyclo ./Core/Src/handle_pwm.d ./Core/Src/handle_pwm.o ./Core/Src/handle_pwm.su ./Core/Src/led_runtime.cyclo ./Core/Src/led_runtime.d ./Core/Src/led_runtime.o ./Core/Src/led_runtime.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/runtime_can.cyclo ./Core/Src/runtime_can.d ./Core/Src/runtime_can.o ./Core/Src/runtime_can.su ./Core/Src/runtime_debug_hooks.cyclo ./Core/Src/runtime_debug_hooks.d ./Core/Src/runtime_debug_hooks.o ./Core/Src/runtime_debug_hooks.su ./Core/Src/runtime_event_queue.cyclo ./Core/Src/runtime_event_queue.d ./Core/Src/runtime_event_queue.o ./Core/Src/runtime_event_queue.su ./Core/Src/runtime_events.cyclo ./Core/Src/runtime_events.d ./Core/Src/runtime_events.o ./Core/Src/runtime_events.su ./Core/Src/runtime_flow.cyclo ./Core/Src/runtime_flow.d ./Core/Src/runtime_flow.o ./Core/Src/runtime_flow.su ./Core/Src/runtime_render.cyclo ./Core/Src/runtime_render.d ./Core/Src/runtime_render.o ./Core/Src/runtime_render.su ./Core/Src/runtime_state_machine.cyclo ./Core/Src/runtime_state_machine.d ./Core/Src/runtime_state_machine.o ./Core/Src/runtime_state_machine.su ./Core/Src/runtime_stop.cyclo ./Core/Src/runtime_stop.d ./Core/Src/runtime_stop.o ./Core/Src/runtime_stop.su ./Core/Src/stm32g4xx_hal_msp.cyclo ./Core/Src/stm32g4xx_hal_msp.d ./Core/Src/stm32g4xx_hal_msp.o ./Core/Src/stm32g4xx_hal_msp.su ./Core/Src/stm32g4xx_it.cyclo ./Core/Src/stm32g4xx_it.d ./Core/Src/stm32g4xx_it.o ./Core/Src/stm32g4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32g4xx.cyclo ./Core/Src/system_stm32g4xx.d ./Core/Src/system_stm32g4xx.o ./Core/Src/system_stm32g4xx.su

.PHONY: clean-Core-2f-Src
