vpath % /Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/ti/sysbios/
vpath %.c /Users/kashi/work/tools/tirtos/xdctools_3_32_00_06_core/packages/

CCOPTS    = -Wunused -Wunknown-pragmas -ffunction-sections -fdata-sections  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mabi=aapcs -g  -O3  -I/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/gnu/targets/arm//libs/install-native/$(GCCTARG)/include -Dfar= -D__DYNAMIC_REENT__   -Dti_sysbios_knl_Task_minimizeLatency__D=FALSE -Dti_sysbios_knl_Clock_stopCheckNext__D=FALSE -Dti_sysbios_family_arm_m3_Hwi_enableException__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_disablePriority__D=32U -Dti_sysbios_family_arm_m3_Hwi_numSparseInterrupts__D=0U

XDC_ROOT  = /Users/kashi/work/tools/tirtos/xdctools_3_32_00_06_core/packages/

BIOS_ROOT = /Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/ti/sysbios/

BIOS_DEFS =  -Dti_sysbios_BIOS_swiEnabled__D=TRUE -Dti_sysbios_BIOS_taskEnabled__D=TRUE -Dti_sysbios_BIOS_clockEnabled__D=TRUE -Dti_sysbios_BIOS_runtimeCreatesEnabled__D=TRUE -Dti_sysbios_hal_Hwi_DISABLE_ALL_HOOKS -Dti_sysbios_knl_Swi_DISABLE_ALL_HOOKS -Dti_sysbios_BIOS_smpEnabled__D=FALSE -Dti_sysbios_Build_useHwiMacros -Dti_sysbios_knl_Swi_numPriorities__D=16 -Dti_sysbios_knl_Task_deleteTerminatedTasks__D=TRUE -Dti_sysbios_knl_Task_numPriorities__D=16 -Dti_sysbios_knl_Task_checkStackFlag__D=TRUE -Dti_sysbios_knl_Task_initStackFlag__D=TRUE -Dti_sysbios_knl_Clock_TICK_SOURCE=ti_sysbios_knl_Clock_TickSource_TIMER -Dti_sysbios_knl_Clock_TICK_MODE=ti_sysbios_knl_Clock_TickMode_PERIODIC -Dti_sysbios_hal_Core_delegate_getId=ti_sysbios_hal_CoreNull_getId__E -Dti_sysbios_hal_Core_delegate_interruptCore=ti_sysbios_hal_CoreNull_interruptCore__E -Dti_sysbios_hal_Core_delegate_lock=ti_sysbios_hal_CoreNull_lock__E -Dti_sysbios_hal_Core_delegate_unlock=ti_sysbios_hal_CoreNull_unlock__E -Dti_sysbios_hal_Core_numCores__D=1 -Dti_sysbios_hal_CoreNull_numCores__D=1 -Dti_sysbios_utils_Load_taskEnabled__D=FALSE -Dti_sysbios_utils_Load_swiEnabled__D=FALSE -Dti_sysbios_utils_Load_hwiEnabled__D=FALSE -Dti_sysbios_family_arm_m3_Hwi_dispatcherSwiSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherTaskSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherAutoNestingSupport__D=TRUE -Dti_sysbios_family_arm_m3_Hwi_dispatcherIrpTrackingSupport__D=TRUE -Dti_sysbios_knl_Semaphore_supportsEvents__D=FALSE -Dti_sysbios_knl_Semaphore_supportsPriority__D=FALSE

BIOS_INC  = -I"/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/"

TARGET_INC = -I"/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/"

INCS = $(BIOS_INC) $(TARGET_INC)  

GCCTARG = arm-none-eabi

CC = /Users/kashi/work/tools/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi-gcc -c -MD -MF $@.dep $(CCOPTS)
ASM = /Users/kashi/work/tools/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi-gcc -c -x assembler-with-cpp $(CCOPTS)
AR = /Users/kashi/work/tools/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi-ar  cr

DEL = rm -f
CP = cp -f

define RM
	$(if $(wildcard $1),$(DEL) $1,:)
endef

define ASSEMBLE
	@echo asmm4fg $< ...
	@$(ASM) $(BIOS_DEFS) -I "/Users/kashi/work/tools/tirtos/xdctools_3_32_00_06_core/packages/" -Dxdc_target_types__=/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/gnu/targets/arm/std.h -Dxdc_target_name__=M4F $(INCS) $<
endef

define COMPILE
	@echo clm4fg $< ...
	@$(CC) $(BIOS_DEFS) -I "/Users/kashi/work/tools/tirtos/xdctools_3_32_00_06_core/packages/" -Dxdc_target_types__=/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/gnu/targets/arm/std.h -Dxdc_target_name__=M4F $(INCS) $<
endef


all: sysbios.am4fg

m3_Hwi_asm_gnu.o: family/arm/m3/Hwi_asm_gnu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) -o m3_Hwi_asm_gnu.o

m3_Hwi_asm_switch_gnu.o: family/arm/m3/Hwi_asm_switch_gnu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) -o m3_Hwi_asm_switch_gnu.o

m3_IntrinsicsSupport_asm_gnu.o: family/arm/m3/IntrinsicsSupport_asm_gnu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) -o m3_IntrinsicsSupport_asm_gnu.o

m3_TaskSupport_asm_gnu.o: family/arm/m3/TaskSupport_asm_gnu.sv7M makefile
	@-$(call RM, $@)
	$(ASSEMBLE) -o m3_TaskSupport_asm_gnu.o


_BIOS.o: BIOS.c makefile
	@-$(call RM, _BIOS.o)
	$(COMPILE) -o _BIOS.o
knl_Clock.o: knl/Clock.c makefile
	@-$(call RM, knl_Clock.o)
	$(COMPILE) -o knl_Clock.o
knl_Idle.o: knl/Idle.c makefile
	@-$(call RM, knl_Idle.o)
	$(COMPILE) -o knl_Idle.o
knl_Intrinsics.o: knl/Intrinsics.c makefile
	@-$(call RM, knl_Intrinsics.o)
	$(COMPILE) -o knl_Intrinsics.o
knl_Event.o: knl/Event.c makefile
	@-$(call RM, knl_Event.o)
	$(COMPILE) -o knl_Event.o
knl_Mailbox.o: knl/Mailbox.c makefile
	@-$(call RM, knl_Mailbox.o)
	$(COMPILE) -o knl_Mailbox.o
knl_Queue.o: knl/Queue.c makefile
	@-$(call RM, knl_Queue.o)
	$(COMPILE) -o knl_Queue.o
knl_Semaphore.o: knl/Semaphore.c makefile
	@-$(call RM, knl_Semaphore.o)
	$(COMPILE) -o knl_Semaphore.o
knl_Swi.o: knl/Swi.c makefile
	@-$(call RM, knl_Swi.o)
	$(COMPILE) -o knl_Swi.o
knl_Swi_andn.o: knl/Swi_andn.c makefile
	@-$(call RM, knl_Swi_andn.o)
	$(COMPILE) -o knl_Swi_andn.o
knl_Task.o: knl/Task.c makefile
	@-$(call RM, knl_Task.o)
	$(COMPILE) -o knl_Task.o
hal_Cache.o: hal/Cache.c makefile
	@-$(call RM, hal_Cache.o)
	$(COMPILE) -o hal_Cache.o
hal_CacheNull.o: hal/CacheNull.c makefile
	@-$(call RM, hal_CacheNull.o)
	$(COMPILE) -o hal_CacheNull.o
hal_Hwi.o: hal/Hwi.c makefile
	@-$(call RM, hal_Hwi.o)
	$(COMPILE) -o hal_Hwi.o
hal_Hwi_stack.o: hal/Hwi_stack.c makefile
	@-$(call RM, hal_Hwi_stack.o)
	$(COMPILE) -o hal_Hwi_stack.o
hal_Hwi_startup.o: hal/Hwi_startup.c makefile
	@-$(call RM, hal_Hwi_startup.o)
	$(COMPILE) -o hal_Hwi_startup.o
hal_Timer.o: hal/Timer.c makefile
	@-$(call RM, hal_Timer.o)
	$(COMPILE) -o hal_Timer.o
m3_Hwi.o: family/arm/m3/Hwi.c makefile
	@-$(call RM, m3_Hwi.o)
	$(COMPILE) -o m3_Hwi.o
m3_TaskSupport.o: family/arm/m3/TaskSupport.c makefile
	@-$(call RM, m3_TaskSupport.o)
	$(COMPILE) -o m3_TaskSupport.o
gnu_ReentSupport.o: rts/gnu/ReentSupport.c makefile
	@-$(call RM, gnu_ReentSupport.o)
	$(COMPILE) -o gnu_ReentSupport.o
gnu_SemiHostSupport.o: rts/gnu/SemiHostSupport.c makefile
	@-$(call RM, gnu_SemiHostSupport.o)
	$(COMPILE) -o gnu_SemiHostSupport.o
gates_GateHwi.o: gates/GateHwi.c makefile
	@-$(call RM, gates_GateHwi.o)
	$(COMPILE) -o gates_GateHwi.o
gates_GateMutex.o: gates/GateMutex.c makefile
	@-$(call RM, gates_GateMutex.o)
	$(COMPILE) -o gates_GateMutex.o
heaps_HeapMem.o: heaps/HeapMem.c makefile
	@-$(call RM, heaps_HeapMem.o)
	$(COMPILE) -o heaps_HeapMem.o
utils_Load.o: utils/Load.c makefile
	@-$(call RM, utils_Load.o)
	$(COMPILE) -o utils_Load.o
utils_Load_CPU.o: utils/Load_CPU.c makefile
	@-$(call RM, utils_Load_CPU.o)
	$(COMPILE) -o utils_Load_CPU.o
lm4_TimestampProvider.o: family/arm/lm4/TimestampProvider.c makefile
	@-$(call RM, lm4_TimestampProvider.o)
	$(COMPILE) -o lm4_TimestampProvider.o
lm4_Timer.o: family/arm/lm4/Timer.c makefile
	@-$(call RM, lm4_Timer.o)
	$(COMPILE) -o lm4_Timer.o

sysbios.am4fg: m3_Hwi_asm_gnu.o m3_Hwi_asm_switch_gnu.o m3_IntrinsicsSupport_asm_gnu.o m3_TaskSupport_asm_gnu.o _BIOS.o knl_Clock.o knl_Idle.o knl_Intrinsics.o knl_Event.o knl_Mailbox.o knl_Queue.o knl_Semaphore.o knl_Swi.o knl_Swi_andn.o knl_Task.o hal_Cache.o hal_CacheNull.o hal_Hwi.o hal_Hwi_stack.o hal_Hwi_startup.o hal_Timer.o m3_Hwi.o m3_TaskSupport.o gnu_ReentSupport.o gnu_SemiHostSupport.o gates_GateHwi.o gates_GateMutex.o heaps_HeapMem.o utils_Load.o utils_Load_CPU.o lm4_TimestampProvider.o lm4_Timer.o
	@echo arm4fg $^ ...
	@-$(call RM, $@)
	@$(AR) $@ $^


clean:
	@$(DEL) ../makefile.libs
	@-$(call RM, *)

