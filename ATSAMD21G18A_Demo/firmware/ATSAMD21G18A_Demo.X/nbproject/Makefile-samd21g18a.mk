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
ifeq "$(wildcard nbproject/Makefile-local-samd21g18a.mk)" "nbproject/Makefile-local-samd21g18a.mk"
include nbproject/Makefile-local-samd21g18a.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=samd21g18a
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/samd21g18a/exceptions.c ../src/config/samd21g18a/initialization.c ../src/config/samd21g18a/interrupts.c ../src/config/samd21g18a/libc_syscalls.c ../src/config/samd21g18a/peripheral/clock/plib_clock.c ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/samd21g18a/peripheral/port/plib_port.c ../src/config/samd21g18a/startup_xc32.c ../src/config/samd21g18a/stdio/xc32_monitor.c ../src/main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1024713942/exceptions.o ${OBJECTDIR}/_ext/1024713942/initialization.o ${OBJECTDIR}/_ext/1024713942/interrupts.o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ${OBJECTDIR}/_ext/902838784/plib_clock.o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ${OBJECTDIR}/_ext/1355959217/plib_port.o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ${OBJECTDIR}/_ext/1360937237/main.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d ${OBJECTDIR}/_ext/1024713942/exceptions.o.d ${OBJECTDIR}/_ext/1024713942/initialization.o.d ${OBJECTDIR}/_ext/1024713942/interrupts.o.d ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d ${OBJECTDIR}/_ext/902838784/plib_clock.o.d ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d ${OBJECTDIR}/_ext/1355959217/plib_port.o.d ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d ${OBJECTDIR}/_ext/1360937237/main.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1024713942/exceptions.o ${OBJECTDIR}/_ext/1024713942/initialization.o ${OBJECTDIR}/_ext/1024713942/interrupts.o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ${OBJECTDIR}/_ext/902838784/plib_clock.o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ${OBJECTDIR}/_ext/1355959217/plib_port.o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ${OBJECTDIR}/_ext/1360937237/main.o

# Source Files
SOURCEFILES=../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/samd21g18a/exceptions.c ../src/config/samd21g18a/initialization.c ../src/config/samd21g18a/interrupts.c ../src/config/samd21g18a/libc_syscalls.c ../src/config/samd21g18a/peripheral/clock/plib_clock.c ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/samd21g18a/peripheral/port/plib_port.c ../src/config/samd21g18a/startup_xc32.c ../src/config/samd21g18a/stdio/xc32_monitor.c ../src/main.c



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
	${MAKE}  -f nbproject/Makefile-samd21g18a.mk ${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=ATSAMD21G18A
MP_LINKER_FILE_OPTION=,--script="..\src\config\samd21g18a\ATSAMD21G18A.ld"
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
${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o: ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c  .generated_files/flags/samd21g18a/d3ef4a214248632f1e27261662ee8f2424e3c1d4 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1514720697" 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d" -o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o: ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/samd21g18a/2ffdcf0d2f496fffe43ace56eb8a56ebfd79c307 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1719041637" 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/exceptions.o: ../src/config/samd21g18a/exceptions.c  .generated_files/flags/samd21g18a/e17fc10d57d5458134b7a86d088525d8b0809936 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/exceptions.o.d" -o ${OBJECTDIR}/_ext/1024713942/exceptions.o ../src/config/samd21g18a/exceptions.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/initialization.o: ../src/config/samd21g18a/initialization.c  .generated_files/flags/samd21g18a/8f6a56142ca2407960179e3f93abc5077e5e2e0a .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/initialization.o.d" -o ${OBJECTDIR}/_ext/1024713942/initialization.o ../src/config/samd21g18a/initialization.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/interrupts.o: ../src/config/samd21g18a/interrupts.c  .generated_files/flags/samd21g18a/5df8bab21f0242563067633d0bfc5e40766b0b9c .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/interrupts.o.d" -o ${OBJECTDIR}/_ext/1024713942/interrupts.o ../src/config/samd21g18a/interrupts.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/libc_syscalls.o: ../src/config/samd21g18a/libc_syscalls.c  .generated_files/flags/samd21g18a/4f73951719808cdb14ff10c2c5e60f6d0b8e7fcf .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ../src/config/samd21g18a/libc_syscalls.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/902838784/plib_clock.o: ../src/config/samd21g18a/peripheral/clock/plib_clock.c  .generated_files/flags/samd21g18a/3cf79b3dbed3d2e93da37939a694f17ccdcece20 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/902838784" 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/902838784/plib_clock.o.d" -o ${OBJECTDIR}/_ext/902838784/plib_clock.o ../src/config/samd21g18a/peripheral/clock/plib_clock.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/904988270/plib_evsys.o: ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c  .generated_files/flags/samd21g18a/3b0f5d74164442e660b342c4011497cf8ea62856 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/904988270" 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/904988270/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1356012368/plib_nvic.o: ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c  .generated_files/flags/samd21g18a/4830a0f1617ec9ab6adac8d9f2076374fe548e04 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1356012368" 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o: ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/samd21g18a/68a05f0277412188e12f9050df6357fb2aef5ac3 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1501740290" 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1355959217/plib_port.o: ../src/config/samd21g18a/peripheral/port/plib_port.c  .generated_files/flags/samd21g18a/7f081062f8d5530d2c3e29e5423926a3222706d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1355959217" 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1355959217/plib_port.o.d" -o ${OBJECTDIR}/_ext/1355959217/plib_port.o ../src/config/samd21g18a/peripheral/port/plib_port.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/startup_xc32.o: ../src/config/samd21g18a/startup_xc32.c  .generated_files/flags/samd21g18a/5143917e6d17f1dd0827160dc400e2a98a3e4c6c .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ../src/config/samd21g18a/startup_xc32.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/783296356/xc32_monitor.o: ../src/config/samd21g18a/stdio/xc32_monitor.c  .generated_files/flags/samd21g18a/f424845b25a33c9de67406c27ad47d925de31e36 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/783296356" 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ../src/config/samd21g18a/stdio/xc32_monitor.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/samd21g18a/9b4bf53e69e142546f63b83d66cbd0196cdb7a4 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o: ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c  .generated_files/flags/samd21g18a/5114713a85d0112c0e08745b070b32b450c16856 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1514720697" 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d" -o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o: ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/samd21g18a/220a8d598384e3abc37203546565221f37293f3e .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1719041637" 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/exceptions.o: ../src/config/samd21g18a/exceptions.c  .generated_files/flags/samd21g18a/d33ed5c75fdaba6227c299c5e63f5ac43771f030 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/exceptions.o.d" -o ${OBJECTDIR}/_ext/1024713942/exceptions.o ../src/config/samd21g18a/exceptions.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/initialization.o: ../src/config/samd21g18a/initialization.c  .generated_files/flags/samd21g18a/6187494de4b476a1b16160373b3328042d25579d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/initialization.o.d" -o ${OBJECTDIR}/_ext/1024713942/initialization.o ../src/config/samd21g18a/initialization.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/interrupts.o: ../src/config/samd21g18a/interrupts.c  .generated_files/flags/samd21g18a/6b55f893dbf5578d234e04cf68e3c157092f8070 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/interrupts.o.d" -o ${OBJECTDIR}/_ext/1024713942/interrupts.o ../src/config/samd21g18a/interrupts.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/libc_syscalls.o: ../src/config/samd21g18a/libc_syscalls.c  .generated_files/flags/samd21g18a/8279e014a0aec086a2f45e92ba45c56ec85ba4f0 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ../src/config/samd21g18a/libc_syscalls.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/902838784/plib_clock.o: ../src/config/samd21g18a/peripheral/clock/plib_clock.c  .generated_files/flags/samd21g18a/5c28be5a02a876ad63cd9b6cd4a7565a31b79fe9 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/902838784" 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/902838784/plib_clock.o.d" -o ${OBJECTDIR}/_ext/902838784/plib_clock.o ../src/config/samd21g18a/peripheral/clock/plib_clock.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/904988270/plib_evsys.o: ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c  .generated_files/flags/samd21g18a/f4965c0c642771776b1d622de29eb2bb0e59a485 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/904988270" 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/904988270/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1356012368/plib_nvic.o: ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c  .generated_files/flags/samd21g18a/a287d2a8fa41de25d2292deaa9bcc786235391d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1356012368" 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o: ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/samd21g18a/2698505590afe8acacb833db1b66978ddd0f2d9c .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1501740290" 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1355959217/plib_port.o: ../src/config/samd21g18a/peripheral/port/plib_port.c  .generated_files/flags/samd21g18a/2a92b7897755fdb88a5aec1c5f5d4e4d91cd2e8 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1355959217" 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1355959217/plib_port.o.d" -o ${OBJECTDIR}/_ext/1355959217/plib_port.o ../src/config/samd21g18a/peripheral/port/plib_port.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/startup_xc32.o: ../src/config/samd21g18a/startup_xc32.c  .generated_files/flags/samd21g18a/f93696557b81590858eb7d4ed07ed4902383c60a .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ../src/config/samd21g18a/startup_xc32.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/783296356/xc32_monitor.o: ../src/config/samd21g18a/stdio/xc32_monitor.c  .generated_files/flags/samd21g18a/c7fdec8333e8d1dc6f1c27f4af08ef7fcd34de88 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/783296356" 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ../src/config/samd21g18a/stdio/xc32_monitor.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/samd21g18a/6bc9c692988daa716f30dcbc8368ac4cf2c1358d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    ../src/config/samd21g18a/ATSAMD21G18A.ld
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -mno-device-startup-code -o ${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=512,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samd21a"
	
else
${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   ../src/config/samd21g18a/ATSAMD21G18A.ld
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -mno-device-startup-code -o ${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=512,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samd21a"
	${MP_CC_DIR}\\xc32-bin2hex ${DISTDIR}/ATSAMD21G18A_Demo.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
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
