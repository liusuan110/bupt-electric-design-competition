################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="D:/desktop/2026电子信息杯/2022-Electric-Game-Lissajous-Figur-main/project/ccs" --include_path="/include" --include_path="../../../msp432p4xx_lib" --include_path="../../../msp432p4xx_lib/CMSIS/Include" --include_path="../../../sys/inc" --include_path="../../../hardware/inc" --define=__MSP432P401R__ --define=DeviceFamily_MSP432P401x -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

system_msp432p401r.obj: D:/desktop/2026电子信息杯/2022-Electric-Game-Lissajous-Figur-main/system_msp432p401r.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="D:/desktop/2026电子信息杯/2022-Electric-Game-Lissajous-Figur-main/project/ccs" --include_path="/include" --include_path="../../../msp432p4xx_lib" --include_path="../../../msp432p4xx_lib/CMSIS/Include" --include_path="../../../sys/inc" --include_path="../../../hardware/inc" --define=__MSP432P401R__ --define=DeviceFamily_MSP432P401x -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


