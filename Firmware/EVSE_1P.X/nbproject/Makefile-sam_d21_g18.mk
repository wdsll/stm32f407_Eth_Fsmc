#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-sam_d21_g18.mk)" "nbproject/Makefile-local-sam_d21_g18.mk"
include nbproject/Makefile-local-sam_d21_g18.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=sam_d21_g18
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../src/config/sam_d21_g18/peripheral/adc/plib_adc.c ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/sam_d21_g18/peripheral/port/plib_port.c ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c ../src/config/sam_d21_g18/stdio/xc32_monitor.c ../src/config/sam_d21_g18/system/int/src/sys_int.c ../src/config/sam_d21_g18/system/time/src/sys_time.c ../src/config/sam_d21_g18/libc_syscalls.c ../src/config/sam_d21_g18/interrupts.c ../src/config/sam_d21_g18/initialization.c ../src/config/sam_d21_g18/exceptions.c ../src/config/sam_d21_g18/startup_xc32.c ../src/config/sam_d21_g18/tasks.c ../src/iso_uart.c ../src/main.c ../src/app_atm90e26.c ../src/i2c_eeprom.c ../src/afe_calib_mem.c ../src/myutils.c ../src/cp_pp.c ../src/ble_uart.c ../src/ble_app.c ../src/app.c ../src/sanity_check.c ../src/host_uart.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/319518173/plib_adc.o ${OBJECTDIR}/_ext/2116386315/plib_clock.o ${OBJECTDIR}/_ext/319522172/plib_eic.o ${OBJECTDIR}/_ext/2118535801/plib_evsys.o ${OBJECTDIR}/_ext/1315533637/plib_nvic.o ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o ${OBJECTDIR}/_ext/1315586788/plib_port.o ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o ${OBJECTDIR}/_ext/275711833/plib_systick.o ${OBJECTDIR}/_ext/682429038/plib_tc3.o ${OBJECTDIR}/_ext/682429038/plib_tc4.o ${OBJECTDIR}/_ext/682429038/plib_tc5.o ${OBJECTDIR}/_ext/319536401/plib_tcc0.o ${OBJECTDIR}/_ext/319536401/plib_tcc1.o ${OBJECTDIR}/_ext/319536401/plib_tcc2.o ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o ${OBJECTDIR}/_ext/2141023284/sys_int.o ${OBJECTDIR}/_ext/651819726/sys_time.o ${OBJECTDIR}/_ext/419793985/libc_syscalls.o ${OBJECTDIR}/_ext/419793985/interrupts.o ${OBJECTDIR}/_ext/419793985/initialization.o ${OBJECTDIR}/_ext/419793985/exceptions.o ${OBJECTDIR}/_ext/419793985/startup_xc32.o ${OBJECTDIR}/_ext/419793985/tasks.o ${OBJECTDIR}/_ext/1360937237/iso_uart.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o ${OBJECTDIR}/_ext/1360937237/myutils.o ${OBJECTDIR}/_ext/1360937237/cp_pp.o ${OBJECTDIR}/_ext/1360937237/ble_uart.o ${OBJECTDIR}/_ext/1360937237/ble_app.o ${OBJECTDIR}/_ext/1360937237/app.o ${OBJECTDIR}/_ext/1360937237/sanity_check.o ${OBJECTDIR}/_ext/1360937237/host_uart.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/319518173/plib_adc.o.d ${OBJECTDIR}/_ext/2116386315/plib_clock.o.d ${OBJECTDIR}/_ext/319522172/plib_eic.o.d ${OBJECTDIR}/_ext/2118535801/plib_evsys.o.d ${OBJECTDIR}/_ext/1315533637/plib_nvic.o.d ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o.d ${OBJECTDIR}/_ext/1315586788/plib_port.o.d ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o.d ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o.d ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o.d ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o.d ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o.d ${OBJECTDIR}/_ext/275711833/plib_systick.o.d ${OBJECTDIR}/_ext/682429038/plib_tc3.o.d ${OBJECTDIR}/_ext/682429038/plib_tc4.o.d ${OBJECTDIR}/_ext/682429038/plib_tc5.o.d ${OBJECTDIR}/_ext/319536401/plib_tcc0.o.d ${OBJECTDIR}/_ext/319536401/plib_tcc1.o.d ${OBJECTDIR}/_ext/319536401/plib_tcc2.o.d ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o.d ${OBJECTDIR}/_ext/2141023284/sys_int.o.d ${OBJECTDIR}/_ext/651819726/sys_time.o.d ${OBJECTDIR}/_ext/419793985/libc_syscalls.o.d ${OBJECTDIR}/_ext/419793985/interrupts.o.d ${OBJECTDIR}/_ext/419793985/initialization.o.d ${OBJECTDIR}/_ext/419793985/exceptions.o.d ${OBJECTDIR}/_ext/419793985/startup_xc32.o.d ${OBJECTDIR}/_ext/419793985/tasks.o.d ${OBJECTDIR}/_ext/1360937237/iso_uart.o.d ${OBJECTDIR}/_ext/1360937237/main.o.d ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o.d ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o.d ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o.d ${OBJECTDIR}/_ext/1360937237/myutils.o.d ${OBJECTDIR}/_ext/1360937237/cp_pp.o.d ${OBJECTDIR}/_ext/1360937237/ble_uart.o.d ${OBJECTDIR}/_ext/1360937237/ble_app.o.d ${OBJECTDIR}/_ext/1360937237/app.o.d ${OBJECTDIR}/_ext/1360937237/sanity_check.o.d ${OBJECTDIR}/_ext/1360937237/host_uart.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/319518173/plib_adc.o ${OBJECTDIR}/_ext/2116386315/plib_clock.o ${OBJECTDIR}/_ext/319522172/plib_eic.o ${OBJECTDIR}/_ext/2118535801/plib_evsys.o ${OBJECTDIR}/_ext/1315533637/plib_nvic.o ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o ${OBJECTDIR}/_ext/1315586788/plib_port.o ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o ${OBJECTDIR}/_ext/275711833/plib_systick.o ${OBJECTDIR}/_ext/682429038/plib_tc3.o ${OBJECTDIR}/_ext/682429038/plib_tc4.o ${OBJECTDIR}/_ext/682429038/plib_tc5.o ${OBJECTDIR}/_ext/319536401/plib_tcc0.o ${OBJECTDIR}/_ext/319536401/plib_tcc1.o ${OBJECTDIR}/_ext/319536401/plib_tcc2.o ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o ${OBJECTDIR}/_ext/2141023284/sys_int.o ${OBJECTDIR}/_ext/651819726/sys_time.o ${OBJECTDIR}/_ext/419793985/libc_syscalls.o ${OBJECTDIR}/_ext/419793985/interrupts.o ${OBJECTDIR}/_ext/419793985/initialization.o ${OBJECTDIR}/_ext/419793985/exceptions.o ${OBJECTDIR}/_ext/419793985/startup_xc32.o ${OBJECTDIR}/_ext/419793985/tasks.o ${OBJECTDIR}/_ext/1360937237/iso_uart.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o ${OBJECTDIR}/_ext/1360937237/myutils.o ${OBJECTDIR}/_ext/1360937237/cp_pp.o ${OBJECTDIR}/_ext/1360937237/ble_uart.o ${OBJECTDIR}/_ext/1360937237/ble_app.o ${OBJECTDIR}/_ext/1360937237/app.o ${OBJECTDIR}/_ext/1360937237/sanity_check.o ${OBJECTDIR}/_ext/1360937237/host_uart.o

# Source Files
SOURCEFILES=../src/config/sam_d21_g18/peripheral/adc/plib_adc.c ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/sam_d21_g18/peripheral/port/plib_port.c ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c ../src/config/sam_d21_g18/stdio/xc32_monitor.c ../src/config/sam_d21_g18/system/int/src/sys_int.c ../src/config/sam_d21_g18/system/time/src/sys_time.c ../src/config/sam_d21_g18/libc_syscalls.c ../src/config/sam_d21_g18/interrupts.c ../src/config/sam_d21_g18/initialization.c ../src/config/sam_d21_g18/exceptions.c ../src/config/sam_d21_g18/startup_xc32.c ../src/config/sam_d21_g18/tasks.c ../src/iso_uart.c ../src/main.c ../src/app_atm90e26.c ../src/i2c_eeprom.c ../src/afe_calib_mem.c ../src/myutils.c ../src/cp_pp.c ../src/ble_uart.c ../src/ble_app.c ../src/app.c ../src/sanity_check.c ../src/host_uart.c

# Pack Options 
PACK_COMMON_OPTIONS=-I "${CMSIS_DIR}/CMSIS/Core/Include"



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-sam_d21_g18.mk ${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=ATSAMD21G18A
MP_LINKER_FILE_OPTION=,--script="..\src\config\sam_d21_g18\ATSAMD21G18A.ld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/319518173/plib_adc.o: ../src/config/sam_d21_g18/peripheral/adc/plib_adc.c  .generated_files/flags/sam_d21_g18/3811a4feae2ce977557720e0a7c0b5b6b4ac75b0 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319518173" 
	@${RM} ${OBJECTDIR}/_ext/319518173/plib_adc.o.d 
	@${RM} ${OBJECTDIR}/_ext/319518173/plib_adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319518173/plib_adc.o.d" -o ${OBJECTDIR}/_ext/319518173/plib_adc.o ../src/config/sam_d21_g18/peripheral/adc/plib_adc.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2116386315/plib_clock.o: ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c  .generated_files/flags/sam_d21_g18/715caac6028cfbec0edaed19f8f98b34bd1e7c24 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2116386315" 
	@${RM} ${OBJECTDIR}/_ext/2116386315/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/2116386315/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2116386315/plib_clock.o.d" -o ${OBJECTDIR}/_ext/2116386315/plib_clock.o ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319522172/plib_eic.o: ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c  .generated_files/flags/sam_d21_g18/7eb796337b79080a561e1f901c92ea61f76d60e1 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319522172" 
	@${RM} ${OBJECTDIR}/_ext/319522172/plib_eic.o.d 
	@${RM} ${OBJECTDIR}/_ext/319522172/plib_eic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319522172/plib_eic.o.d" -o ${OBJECTDIR}/_ext/319522172/plib_eic.o ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2118535801/plib_evsys.o: ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c  .generated_files/flags/sam_d21_g18/4e995844e9a1e8449f93b964aee74cb405f1ea6e .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2118535801" 
	@${RM} ${OBJECTDIR}/_ext/2118535801/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/2118535801/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2118535801/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/2118535801/plib_evsys.o ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1315533637/plib_nvic.o: ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c  .generated_files/flags/sam_d21_g18/3937ffc8f06e13ed2ec494611e2607c91ccd2313 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1315533637" 
	@${RM} ${OBJECTDIR}/_ext/1315533637/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1315533637/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1315533637/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1315533637/plib_nvic.o ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o: ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/sam_d21_g18/c8f9d4ba88c30a93c914533598fe43c327cf85b2 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/510186931" 
	@${RM} ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1315586788/plib_port.o: ../src/config/sam_d21_g18/peripheral/port/plib_port.c  .generated_files/flags/sam_d21_g18/d4b8ae69d0b1e1334ff8a84e1bf354f79b69458e .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1315586788" 
	@${RM} ${OBJECTDIR}/_ext/1315586788/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1315586788/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1315586788/plib_port.o.d" -o ${OBJECTDIR}/_ext/1315586788/plib_port.o ../src/config/sam_d21_g18/peripheral/port/plib_port.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o: ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c  .generated_files/flags/sam_d21_g18/4497b54c5460f994c067b48a24b0966cc1a6f182 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/602389970" 
	@${RM} ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o.d" -o ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c  .generated_files/flags/sam_d21_g18/48c140c6577bab6f4eb592a89baa507e6a33ec34 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/sam_d21_g18/10b3c950b01b32b84cfe38a6ae23d8606940e56b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c  .generated_files/flags/sam_d21_g18/1237d526cedefc3f75f9168c2315287f01073fa2 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c  .generated_files/flags/sam_d21_g18/70b3c9c1bd43e2fd547c35fe82bbe53e42aceb5d .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/275711833/plib_systick.o: ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c  .generated_files/flags/sam_d21_g18/217fde7c926ad8ddce39bf459296447db0aef8c1 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/275711833" 
	@${RM} ${OBJECTDIR}/_ext/275711833/plib_systick.o.d 
	@${RM} ${OBJECTDIR}/_ext/275711833/plib_systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/275711833/plib_systick.o.d" -o ${OBJECTDIR}/_ext/275711833/plib_systick.o ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc3.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c  .generated_files/flags/sam_d21_g18/22d7e0d52ff12b32b774b3192fc2c7f9e642a9e5 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc3.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc3.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc3.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc3.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc4.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c  .generated_files/flags/sam_d21_g18/8d214ae69347bfe8d6104cd497abdeb7212f48e9 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc4.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc4.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc4.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc4.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc5.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c  .generated_files/flags/sam_d21_g18/bf161682c533131c2594ad0b1492e54ace0424e8 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc5.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc5.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc5.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc5.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc0.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c  .generated_files/flags/sam_d21_g18/9523eedd135baa57515fedce98d16399ba3495a7 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc0.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc0.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc0.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc0.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc1.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c  .generated_files/flags/sam_d21_g18/828d177727566f77c146a4816bf015441b195201 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc1.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc1.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc1.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc1.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc2.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c  .generated_files/flags/sam_d21_g18/abb766a4a8e4b2e73903ccc70a79881f67989360 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc2.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc2.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc2.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2124217401/xc32_monitor.o: ../src/config/sam_d21_g18/stdio/xc32_monitor.c  .generated_files/flags/sam_d21_g18/ba51c2bf70b8759823f19f313f5be3a86b74d1cb .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2124217401" 
	@${RM} ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2124217401/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o ../src/config/sam_d21_g18/stdio/xc32_monitor.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2141023284/sys_int.o: ../src/config/sam_d21_g18/system/int/src/sys_int.c  .generated_files/flags/sam_d21_g18/2cfff8a6c22516430725c41076f6fc64a44d35bf .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2141023284" 
	@${RM} ${OBJECTDIR}/_ext/2141023284/sys_int.o.d 
	@${RM} ${OBJECTDIR}/_ext/2141023284/sys_int.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2141023284/sys_int.o.d" -o ${OBJECTDIR}/_ext/2141023284/sys_int.o ../src/config/sam_d21_g18/system/int/src/sys_int.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/651819726/sys_time.o: ../src/config/sam_d21_g18/system/time/src/sys_time.c  .generated_files/flags/sam_d21_g18/b639dc1cae864ac818fb1ade17faffae61d5be7 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/651819726" 
	@${RM} ${OBJECTDIR}/_ext/651819726/sys_time.o.d 
	@${RM} ${OBJECTDIR}/_ext/651819726/sys_time.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/651819726/sys_time.o.d" -o ${OBJECTDIR}/_ext/651819726/sys_time.o ../src/config/sam_d21_g18/system/time/src/sys_time.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/libc_syscalls.o: ../src/config/sam_d21_g18/libc_syscalls.c  .generated_files/flags/sam_d21_g18/cc5f7695e1d2b9c1744251d7e37edb83a78577b1 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/419793985/libc_syscalls.o ../src/config/sam_d21_g18/libc_syscalls.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/interrupts.o: ../src/config/sam_d21_g18/interrupts.c  .generated_files/flags/sam_d21_g18/840e401c2455730e3f69d1d9754c5e16fb2b39e8 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/interrupts.o.d" -o ${OBJECTDIR}/_ext/419793985/interrupts.o ../src/config/sam_d21_g18/interrupts.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/initialization.o: ../src/config/sam_d21_g18/initialization.c  .generated_files/flags/sam_d21_g18/8a48ae05be62a1dc3196fc509f114d6babe94590 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/initialization.o.d" -o ${OBJECTDIR}/_ext/419793985/initialization.o ../src/config/sam_d21_g18/initialization.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/exceptions.o: ../src/config/sam_d21_g18/exceptions.c  .generated_files/flags/sam_d21_g18/aef007559c89eae5b767f6200c044496f6597230 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/exceptions.o.d" -o ${OBJECTDIR}/_ext/419793985/exceptions.o ../src/config/sam_d21_g18/exceptions.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/startup_xc32.o: ../src/config/sam_d21_g18/startup_xc32.c  .generated_files/flags/sam_d21_g18/4076a20f08980c7e51af461c3a22fd256dd7e808 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/419793985/startup_xc32.o ../src/config/sam_d21_g18/startup_xc32.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/tasks.o: ../src/config/sam_d21_g18/tasks.c  .generated_files/flags/sam_d21_g18/5fe0e9525b9fc2d8a2f2b7be6c20a753c86a062c .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/tasks.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/tasks.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/tasks.o.d" -o ${OBJECTDIR}/_ext/419793985/tasks.o ../src/config/sam_d21_g18/tasks.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/iso_uart.o: ../src/iso_uart.c  .generated_files/flags/sam_d21_g18/bc42744af72e0fe2276acb51af713d5767acbfa5 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/iso_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/iso_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/iso_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/iso_uart.o ../src/iso_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/sam_d21_g18/7bc0ce36180af2b8290be4ce9b4ae83c5e04614b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/app_atm90e26.o: ../src/app_atm90e26.c  .generated_files/flags/sam_d21_g18/84fe7dd5a126ab0dca50c051773cd10a5ddf3ea7 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/app_atm90e26.o.d" -o ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o ../src/app_atm90e26.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o: ../src/i2c_eeprom.c  .generated_files/flags/sam_d21_g18/5bb6a5da88f2c34acb771c917612e4bb0030756e .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o.d" -o ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o ../src/i2c_eeprom.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o: ../src/afe_calib_mem.c  .generated_files/flags/sam_d21_g18/e637593928dfee6cd3d1a4b72eb8a14bd2035bfa .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o.d" -o ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o ../src/afe_calib_mem.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/myutils.o: ../src/myutils.c  .generated_files/flags/sam_d21_g18/5d16e87e291c1cbe64aafb3b8c3bba43c36db810 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/myutils.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/myutils.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/myutils.o.d" -o ${OBJECTDIR}/_ext/1360937237/myutils.o ../src/myutils.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/cp_pp.o: ../src/cp_pp.c  .generated_files/flags/sam_d21_g18/a2a0e566c98f0e11a6cea23902e418928c2f3999 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/cp_pp.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/cp_pp.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/cp_pp.o.d" -o ${OBJECTDIR}/_ext/1360937237/cp_pp.o ../src/cp_pp.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/ble_uart.o: ../src/ble_uart.c  .generated_files/flags/sam_d21_g18/6d3c621dc839cbea6fcc04ae1bfb2775aa858168 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ble_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/ble_uart.o ../src/ble_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/ble_app.o: ../src/ble_app.c  .generated_files/flags/sam_d21_g18/f21975274bb53c5f37ae1726151bf17486cce732 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_app.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_app.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ble_app.o.d" -o ${OBJECTDIR}/_ext/1360937237/ble_app.o ../src/ble_app.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/app.o: ../src/app.c  .generated_files/flags/sam_d21_g18/a80a13ee828b8094861dd360ec49d31131f63c00 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/app.o.d" -o ${OBJECTDIR}/_ext/1360937237/app.o ../src/app.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/sanity_check.o: ../src/sanity_check.c  .generated_files/flags/sam_d21_g18/9f076e4c1fae99c50012457a693110e669cf6bc1 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/sanity_check.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/sanity_check.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/sanity_check.o.d" -o ${OBJECTDIR}/_ext/1360937237/sanity_check.o ../src/sanity_check.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/host_uart.o: ../src/host_uart.c  .generated_files/flags/sam_d21_g18/b5e13850cc866abe9747e89fe9381f30f2af728a .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/host_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/host_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/host_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/host_uart.o ../src/host_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/_ext/319518173/plib_adc.o: ../src/config/sam_d21_g18/peripheral/adc/plib_adc.c  .generated_files/flags/sam_d21_g18/b49021ef11f36ca45fe0ea2d35747d431226b92 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319518173" 
	@${RM} ${OBJECTDIR}/_ext/319518173/plib_adc.o.d 
	@${RM} ${OBJECTDIR}/_ext/319518173/plib_adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319518173/plib_adc.o.d" -o ${OBJECTDIR}/_ext/319518173/plib_adc.o ../src/config/sam_d21_g18/peripheral/adc/plib_adc.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2116386315/plib_clock.o: ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c  .generated_files/flags/sam_d21_g18/3b71d0bf6cdc29aeaecb544bd21a9baf360ad303 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2116386315" 
	@${RM} ${OBJECTDIR}/_ext/2116386315/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/2116386315/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2116386315/plib_clock.o.d" -o ${OBJECTDIR}/_ext/2116386315/plib_clock.o ../src/config/sam_d21_g18/peripheral/clock/plib_clock.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319522172/plib_eic.o: ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c  .generated_files/flags/sam_d21_g18/54e3ef298687f1e780f780db60420710193760ea .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319522172" 
	@${RM} ${OBJECTDIR}/_ext/319522172/plib_eic.o.d 
	@${RM} ${OBJECTDIR}/_ext/319522172/plib_eic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319522172/plib_eic.o.d" -o ${OBJECTDIR}/_ext/319522172/plib_eic.o ../src/config/sam_d21_g18/peripheral/eic/plib_eic.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2118535801/plib_evsys.o: ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c  .generated_files/flags/sam_d21_g18/f2ebfca9ef2e3b59d6bb3e71e3ba8466ccc9c6ca .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2118535801" 
	@${RM} ${OBJECTDIR}/_ext/2118535801/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/2118535801/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2118535801/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/2118535801/plib_evsys.o ../src/config/sam_d21_g18/peripheral/evsys/plib_evsys.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1315533637/plib_nvic.o: ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c  .generated_files/flags/sam_d21_g18/8b67b306308e162c330dba864b17f84ab4036214 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1315533637" 
	@${RM} ${OBJECTDIR}/_ext/1315533637/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1315533637/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1315533637/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1315533637/plib_nvic.o ../src/config/sam_d21_g18/peripheral/nvic/plib_nvic.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o: ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/sam_d21_g18/cd563683a7d8fe1b453a279f0dcafc5e448e0d4c .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/510186931" 
	@${RM} ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/510186931/plib_nvmctrl.o ../src/config/sam_d21_g18/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1315586788/plib_port.o: ../src/config/sam_d21_g18/peripheral/port/plib_port.c  .generated_files/flags/sam_d21_g18/1af6758fa0cfe9e47adffedb9dabc285b3ae1931 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1315586788" 
	@${RM} ${OBJECTDIR}/_ext/1315586788/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1315586788/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1315586788/plib_port.o.d" -o ${OBJECTDIR}/_ext/1315586788/plib_port.o ../src/config/sam_d21_g18/peripheral/port/plib_port.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o: ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c  .generated_files/flags/sam_d21_g18/40d04ae96797166b9e2094e44baf38291ad091f1 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/602389970" 
	@${RM} ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o.d" -o ${OBJECTDIR}/_ext/602389970/plib_sercom2_i2c_master.o ../src/config/sam_d21_g18/peripheral/sercom/i2c_master/plib_sercom2_i2c_master.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c  .generated_files/flags/sam_d21_g18/6a1a26508e4b9db085f9b72cd9ef06f1bde388ba .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom4_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom4_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/sam_d21_g18/1fcb63a9dc345fa362f01a8aeb3b5f98a0cb039b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom0_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c  .generated_files/flags/sam_d21_g18/2d66e54c2cb154655828cccd83672642a9b6d0d3 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom5_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom5_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o: ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c  .generated_files/flags/sam_d21_g18/abafbc92ad86ac4cf83b7d3279f2db0a8114f63b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1259972602" 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o.d" -o ${OBJECTDIR}/_ext/1259972602/plib_sercom3_usart.o ../src/config/sam_d21_g18/peripheral/sercom/usart/plib_sercom3_usart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/275711833/plib_systick.o: ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c  .generated_files/flags/sam_d21_g18/808d01f43460b7aa42b0f7c18d5e8dc8f04920ed .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/275711833" 
	@${RM} ${OBJECTDIR}/_ext/275711833/plib_systick.o.d 
	@${RM} ${OBJECTDIR}/_ext/275711833/plib_systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/275711833/plib_systick.o.d" -o ${OBJECTDIR}/_ext/275711833/plib_systick.o ../src/config/sam_d21_g18/peripheral/systick/plib_systick.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc3.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c  .generated_files/flags/sam_d21_g18/4ec682fdb0d96ed3114ff96c8dee6ff0cf0460c4 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc3.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc3.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc3.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc3.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc3.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc4.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c  .generated_files/flags/sam_d21_g18/8242532c74e50c720b09bb08d76380d3e69e5188 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc4.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc4.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc4.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc4.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc4.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/682429038/plib_tc5.o: ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c  .generated_files/flags/sam_d21_g18/20fb44e8b9c86c0c79a9f89f5d6c1d45d8d0ed99 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/682429038" 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc5.o.d 
	@${RM} ${OBJECTDIR}/_ext/682429038/plib_tc5.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/682429038/plib_tc5.o.d" -o ${OBJECTDIR}/_ext/682429038/plib_tc5.o ../src/config/sam_d21_g18/peripheral/tc/plib_tc5.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc0.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c  .generated_files/flags/sam_d21_g18/1d5984b24eaa1f05b7c764a201d87a7c63c78a07 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc0.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc0.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc0.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc0.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc0.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc1.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c  .generated_files/flags/sam_d21_g18/bddbd6b4a56b242f9b71b62cacf7ce16eecd5596 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc1.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc1.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc1.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc1.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc1.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/319536401/plib_tcc2.o: ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c  .generated_files/flags/sam_d21_g18/47ee289b5a00803657d8f30d85cde1d4407949ad .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/319536401" 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc2.o.d 
	@${RM} ${OBJECTDIR}/_ext/319536401/plib_tcc2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/319536401/plib_tcc2.o.d" -o ${OBJECTDIR}/_ext/319536401/plib_tcc2.o ../src/config/sam_d21_g18/peripheral/tcc/plib_tcc2.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2124217401/xc32_monitor.o: ../src/config/sam_d21_g18/stdio/xc32_monitor.c  .generated_files/flags/sam_d21_g18/f7ea61ca9b8c086e0ef2109c2769c8f42e230fff .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2124217401" 
	@${RM} ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2124217401/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/2124217401/xc32_monitor.o ../src/config/sam_d21_g18/stdio/xc32_monitor.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/2141023284/sys_int.o: ../src/config/sam_d21_g18/system/int/src/sys_int.c  .generated_files/flags/sam_d21_g18/f2021f93aec61883b1cb43017ca46c38231ca5a6 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/2141023284" 
	@${RM} ${OBJECTDIR}/_ext/2141023284/sys_int.o.d 
	@${RM} ${OBJECTDIR}/_ext/2141023284/sys_int.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/2141023284/sys_int.o.d" -o ${OBJECTDIR}/_ext/2141023284/sys_int.o ../src/config/sam_d21_g18/system/int/src/sys_int.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/651819726/sys_time.o: ../src/config/sam_d21_g18/system/time/src/sys_time.c  .generated_files/flags/sam_d21_g18/4517bf35a44411236ebc7d7cbbe9a50967e6f91e .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/651819726" 
	@${RM} ${OBJECTDIR}/_ext/651819726/sys_time.o.d 
	@${RM} ${OBJECTDIR}/_ext/651819726/sys_time.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/651819726/sys_time.o.d" -o ${OBJECTDIR}/_ext/651819726/sys_time.o ../src/config/sam_d21_g18/system/time/src/sys_time.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/libc_syscalls.o: ../src/config/sam_d21_g18/libc_syscalls.c  .generated_files/flags/sam_d21_g18/dc2b58320c416640016df168470497ba1a26903f .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/419793985/libc_syscalls.o ../src/config/sam_d21_g18/libc_syscalls.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/interrupts.o: ../src/config/sam_d21_g18/interrupts.c  .generated_files/flags/sam_d21_g18/288a224778e0e543bd4084ab645cfd19321b780b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/interrupts.o.d" -o ${OBJECTDIR}/_ext/419793985/interrupts.o ../src/config/sam_d21_g18/interrupts.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/initialization.o: ../src/config/sam_d21_g18/initialization.c  .generated_files/flags/sam_d21_g18/ba9c00af7c58c111f9d4211b965f3ea088fdfa63 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/initialization.o.d" -o ${OBJECTDIR}/_ext/419793985/initialization.o ../src/config/sam_d21_g18/initialization.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/exceptions.o: ../src/config/sam_d21_g18/exceptions.c  .generated_files/flags/sam_d21_g18/a124a4bde79bb92d7d1f09d6bb471e9361930c87 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/exceptions.o.d" -o ${OBJECTDIR}/_ext/419793985/exceptions.o ../src/config/sam_d21_g18/exceptions.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/startup_xc32.o: ../src/config/sam_d21_g18/startup_xc32.c  .generated_files/flags/sam_d21_g18/e14c4ecfaa0c08b80b32f3b30a5c26b516ff8c0b .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/419793985/startup_xc32.o ../src/config/sam_d21_g18/startup_xc32.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/419793985/tasks.o: ../src/config/sam_d21_g18/tasks.c  .generated_files/flags/sam_d21_g18/a8f02546b50dc8473af2c777ca8edadd486fa726 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/419793985" 
	@${RM} ${OBJECTDIR}/_ext/419793985/tasks.o.d 
	@${RM} ${OBJECTDIR}/_ext/419793985/tasks.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/419793985/tasks.o.d" -o ${OBJECTDIR}/_ext/419793985/tasks.o ../src/config/sam_d21_g18/tasks.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/iso_uart.o: ../src/iso_uart.c  .generated_files/flags/sam_d21_g18/994a0a58206250c156c735dea6a7e7318d703c2 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/iso_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/iso_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/iso_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/iso_uart.o ../src/iso_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/sam_d21_g18/dce607c32fda9a425c0e1ca55201b3347e53e149 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/app_atm90e26.o: ../src/app_atm90e26.c  .generated_files/flags/sam_d21_g18/617f05eafd24d3243c80e29a700a018d28b76342 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/app_atm90e26.o.d" -o ${OBJECTDIR}/_ext/1360937237/app_atm90e26.o ../src/app_atm90e26.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o: ../src/i2c_eeprom.c  .generated_files/flags/sam_d21_g18/7a0ff62cd5f14f0fd27547dbb54cf95879add513 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o.d" -o ${OBJECTDIR}/_ext/1360937237/i2c_eeprom.o ../src/i2c_eeprom.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o: ../src/afe_calib_mem.c  .generated_files/flags/sam_d21_g18/3654275adb2537db3fa503836731afc0df25fe61 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o.d" -o ${OBJECTDIR}/_ext/1360937237/afe_calib_mem.o ../src/afe_calib_mem.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/myutils.o: ../src/myutils.c  .generated_files/flags/sam_d21_g18/39b72efe9a9589bf1da9af6f212342e0cee1373c .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/myutils.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/myutils.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/myutils.o.d" -o ${OBJECTDIR}/_ext/1360937237/myutils.o ../src/myutils.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/cp_pp.o: ../src/cp_pp.c  .generated_files/flags/sam_d21_g18/f10f91a232b421a4c8da420a60081cfa257ce69f .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/cp_pp.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/cp_pp.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/cp_pp.o.d" -o ${OBJECTDIR}/_ext/1360937237/cp_pp.o ../src/cp_pp.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/ble_uart.o: ../src/ble_uart.c  .generated_files/flags/sam_d21_g18/299f8085ecc4267eac1433246890b6fb7e547d21 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ble_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/ble_uart.o ../src/ble_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/ble_app.o: ../src/ble_app.c  .generated_files/flags/sam_d21_g18/dc0e49102fa21783661e881d218432f105ddf588 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_app.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ble_app.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/ble_app.o.d" -o ${OBJECTDIR}/_ext/1360937237/ble_app.o ../src/ble_app.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/app.o: ../src/app.c  .generated_files/flags/sam_d21_g18/1b963132fcf3be86b622d32bd927785659b07f78 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/app.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/app.o.d" -o ${OBJECTDIR}/_ext/1360937237/app.o ../src/app.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/sanity_check.o: ../src/sanity_check.c  .generated_files/flags/sam_d21_g18/af0112b80a509880fbbf905a6c7e2c54b206d918 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/sanity_check.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/sanity_check.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/sanity_check.o.d" -o ${OBJECTDIR}/_ext/1360937237/sanity_check.o ../src/sanity_check.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/host_uart.o: ../src/host_uart.c  .generated_files/flags/sam_d21_g18/d77ae95a7dce4ad7ed72259b8d86ac57a050bf53 .generated_files/flags/sam_d21_g18/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/host_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/host_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/sam_d21_g18" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -Wall -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/host_uart.o.d" -o ${OBJECTDIR}/_ext/1360937237/host_uart.o ../src/host_uart.c    -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    ../src/config/sam_d21_g18/ATSAMD21G18A.ld
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -mno-device-startup-code -o ${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=512,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samd21a"
	
else
${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   ../src/config/sam_d21_g18/ATSAMD21G18A.ld
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -mno-device-startup-code -o ${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_sam_d21_g18=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=512,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samd21a"
	${MP_CC_DIR}\\xc32-bin2hex ${DISTDIR}/EVSE_1P.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
