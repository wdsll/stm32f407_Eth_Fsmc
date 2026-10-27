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
SOURCEFILES_QUOTED_IF_SPACED=../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/samd21g18a/exceptions.c ../src/config/samd21g18a/initialization.c ../src/config/samd21g18a/interrupts.c ../src/config/samd21g18a/libc_syscalls.c ../src/config/samd21g18a/peripheral/clock/plib_clock.c ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/samd21g18a/peripheral/port/plib_port.c ../src/config/samd21g18a/startup_xc32.c ../src/config/samd21g18a/stdio/xc32_monitor.c ../src/main.c ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1024713942/exceptions.o ${OBJECTDIR}/_ext/1024713942/initialization.o ${OBJECTDIR}/_ext/1024713942/interrupts.o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ${OBJECTDIR}/_ext/902838784/plib_clock.o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ${OBJECTDIR}/_ext/1355959217/plib_port.o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d ${OBJECTDIR}/_ext/1024713942/exceptions.o.d ${OBJECTDIR}/_ext/1024713942/initialization.o.d ${OBJECTDIR}/_ext/1024713942/interrupts.o.d ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d ${OBJECTDIR}/_ext/902838784/plib_clock.o.d ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d ${OBJECTDIR}/_ext/1355959217/plib_port.o.d ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d ${OBJECTDIR}/_ext/1360937237/main.o.d ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ${OBJECTDIR}/_ext/1024713942/exceptions.o ${OBJECTDIR}/_ext/1024713942/initialization.o ${OBJECTDIR}/_ext/1024713942/interrupts.o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ${OBJECTDIR}/_ext/902838784/plib_clock.o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ${OBJECTDIR}/_ext/1355959217/plib_port.o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o

# Source Files
SOURCEFILES=../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c ../src/config/samd21g18a/exceptions.c ../src/config/samd21g18a/initialization.c ../src/config/samd21g18a/interrupts.c ../src/config/samd21g18a/libc_syscalls.c ../src/config/samd21g18a/peripheral/clock/plib_clock.c ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c ../src/config/samd21g18a/peripheral/port/plib_port.c ../src/config/samd21g18a/startup_xc32.c ../src/config/samd21g18a/stdio/xc32_monitor.c ../src/main.c ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c



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
${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o: ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/samd21g18a/46ba65e5e4b282a4d00a1efad006b839cd3f7927 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1719041637" 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/exceptions.o: ../src/config/samd21g18a/exceptions.c  .generated_files/flags/samd21g18a/c6fbb80df7167a0eac19a423a2525c7fad52f11e .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/exceptions.o.d" -o ${OBJECTDIR}/_ext/1024713942/exceptions.o ../src/config/samd21g18a/exceptions.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/initialization.o: ../src/config/samd21g18a/initialization.c  .generated_files/flags/samd21g18a/ae40135b4be6047a34b5693b7b390cbe4f384aa .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/initialization.o.d" -o ${OBJECTDIR}/_ext/1024713942/initialization.o ../src/config/samd21g18a/initialization.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/interrupts.o: ../src/config/samd21g18a/interrupts.c  .generated_files/flags/samd21g18a/b3a10602406b6eafcded56e1488e1da1d1ac74fd .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/interrupts.o.d" -o ${OBJECTDIR}/_ext/1024713942/interrupts.o ../src/config/samd21g18a/interrupts.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/libc_syscalls.o: ../src/config/samd21g18a/libc_syscalls.c  .generated_files/flags/samd21g18a/498773648f1e523c2d25bc1d34ed7c3ab7872b2f .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ../src/config/samd21g18a/libc_syscalls.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/902838784/plib_clock.o: ../src/config/samd21g18a/peripheral/clock/plib_clock.c  .generated_files/flags/samd21g18a/31fe258f3b80881b351e3253231167fec1b58183 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/902838784" 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/902838784/plib_clock.o.d" -o ${OBJECTDIR}/_ext/902838784/plib_clock.o ../src/config/samd21g18a/peripheral/clock/plib_clock.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/904988270/plib_evsys.o: ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c  .generated_files/flags/samd21g18a/1c80cdeff710577e8c8de7788b4b519b2b1c08ab .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/904988270" 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/904988270/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1356012368/plib_nvic.o: ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c  .generated_files/flags/samd21g18a/58ca742ac2ab2e8c1f2a731a9479094ee1fcd21a .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1356012368" 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o: ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/samd21g18a/4cca4d492323144d55315cc8c77e92193f226e2f .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1501740290" 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1355959217/plib_port.o: ../src/config/samd21g18a/peripheral/port/plib_port.c  .generated_files/flags/samd21g18a/38e78c3e673811f961ff057c3caac235ff947e51 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1355959217" 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1355959217/plib_port.o.d" -o ${OBJECTDIR}/_ext/1355959217/plib_port.o ../src/config/samd21g18a/peripheral/port/plib_port.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/startup_xc32.o: ../src/config/samd21g18a/startup_xc32.c  .generated_files/flags/samd21g18a/ea3fcecc37c3a1172673a7dc82a39ac108e205dc .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ../src/config/samd21g18a/startup_xc32.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/783296356/xc32_monitor.o: ../src/config/samd21g18a/stdio/xc32_monitor.c  .generated_files/flags/samd21g18a/7d48ecf75808ee42db2d1bd390607cc205d0148d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/783296356" 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ../src/config/samd21g18a/stdio/xc32_monitor.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/samd21g18a/f4e9735707a407bb05465ee58dbd3400b6d73d4 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o: ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c  .generated_files/flags/samd21g18a/23ac109439b8eca051a90fd457599fb3960ad2d4 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1514720697" 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d" -o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o: ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c  .generated_files/flags/samd21g18a/5e438ee977ca3ffccccc4daa95b81e899e7cdb6a .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1719041637" 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o.d" -o ${OBJECTDIR}/_ext/1719041637/plib_sercom0_usart.o ../src/config/samd21g18a/peripheral/sercom/usart/plib_sercom0_usart.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/exceptions.o: ../src/config/samd21g18a/exceptions.c  .generated_files/flags/samd21g18a/51790d4e5648b77824f2be4fc1292c85461a29b2 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/exceptions.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/exceptions.o.d" -o ${OBJECTDIR}/_ext/1024713942/exceptions.o ../src/config/samd21g18a/exceptions.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/initialization.o: ../src/config/samd21g18a/initialization.c  .generated_files/flags/samd21g18a/c9ef52a7bc1cfe825714cb4eacd4415695605a5c .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/initialization.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/initialization.o.d" -o ${OBJECTDIR}/_ext/1024713942/initialization.o ../src/config/samd21g18a/initialization.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/interrupts.o: ../src/config/samd21g18a/interrupts.c  .generated_files/flags/samd21g18a/f75737c2e90be47177970125d83a9ba9b2e32016 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/interrupts.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/interrupts.o.d" -o ${OBJECTDIR}/_ext/1024713942/interrupts.o ../src/config/samd21g18a/interrupts.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/libc_syscalls.o: ../src/config/samd21g18a/libc_syscalls.c  .generated_files/flags/samd21g18a/ea94ad859b1cc8d9bc4373b58e4289ed781205de .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/libc_syscalls.o.d" -o ${OBJECTDIR}/_ext/1024713942/libc_syscalls.o ../src/config/samd21g18a/libc_syscalls.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/902838784/plib_clock.o: ../src/config/samd21g18a/peripheral/clock/plib_clock.c  .generated_files/flags/samd21g18a/ef6e2e0da65c946958adffe2640d0e3c09166f7a .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/902838784" 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o.d 
	@${RM} ${OBJECTDIR}/_ext/902838784/plib_clock.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/902838784/plib_clock.o.d" -o ${OBJECTDIR}/_ext/902838784/plib_clock.o ../src/config/samd21g18a/peripheral/clock/plib_clock.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/904988270/plib_evsys.o: ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c  .generated_files/flags/samd21g18a/960296c26df810c391861c9761e5f08b66186562 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/904988270" 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o.d 
	@${RM} ${OBJECTDIR}/_ext/904988270/plib_evsys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/904988270/plib_evsys.o.d" -o ${OBJECTDIR}/_ext/904988270/plib_evsys.o ../src/config/samd21g18a/peripheral/evsys/plib_evsys.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1356012368/plib_nvic.o: ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c  .generated_files/flags/samd21g18a/aaf296213c02611de4e45bf19fb2384e81f3a8e3 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1356012368" 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1356012368/plib_nvic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1356012368/plib_nvic.o.d" -o ${OBJECTDIR}/_ext/1356012368/plib_nvic.o ../src/config/samd21g18a/peripheral/nvic/plib_nvic.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o: ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c  .generated_files/flags/samd21g18a/9fc04d2dacb8e6a0f16358ef3cc5760b6f10190 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1501740290" 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d 
	@${RM} ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o.d" -o ${OBJECTDIR}/_ext/1501740290/plib_nvmctrl.o ../src/config/samd21g18a/peripheral/nvmctrl/plib_nvmctrl.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1355959217/plib_port.o: ../src/config/samd21g18a/peripheral/port/plib_port.c  .generated_files/flags/samd21g18a/200892f4ab37c99eb94fcbab3863ab6b6f014eb6 .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1355959217" 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o.d 
	@${RM} ${OBJECTDIR}/_ext/1355959217/plib_port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1355959217/plib_port.o.d" -o ${OBJECTDIR}/_ext/1355959217/plib_port.o ../src/config/samd21g18a/peripheral/port/plib_port.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1024713942/startup_xc32.o: ../src/config/samd21g18a/startup_xc32.c  .generated_files/flags/samd21g18a/29dfeed3b37486f9a0d9e8f980852711a0308a4e .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1024713942" 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1024713942/startup_xc32.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1024713942/startup_xc32.o.d" -o ${OBJECTDIR}/_ext/1024713942/startup_xc32.o ../src/config/samd21g18a/startup_xc32.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/783296356/xc32_monitor.o: ../src/config/samd21g18a/stdio/xc32_monitor.c  .generated_files/flags/samd21g18a/1176bd7864166df8cc0d761be1b174345681a6be .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/783296356" 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d 
	@${RM} ${OBJECTDIR}/_ext/783296356/xc32_monitor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/783296356/xc32_monitor.o.d" -o ${OBJECTDIR}/_ext/783296356/xc32_monitor.o ../src/config/samd21g18a/stdio/xc32_monitor.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  .generated_files/flags/samd21g18a/cbf133c0e9dcd3ee173912d624c986dcd716a43c .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o: ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c  .generated_files/flags/samd21g18a/cdcd56faa3c9205a2a814af4788e91c534b1a09d .generated_files/flags/samd21g18a/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1514720697" 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d 
	@${RM} ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../src" -I"../src/config/samd21g18a" -I"../src/packs/ATSAMD21G18A_DFP" -I"../src/packs/CMSIS/" -I"../src/packs/CMSIS/CMSIS/Core/Include" -MP -MMD -MF "${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o.d" -o ${OBJECTDIR}/_ext/1514720697/plib_sercom1_i2c_master.o ../src/config/samd21g18a/peripheral/sercom/i2c_master/plib_sercom1_i2c_master.c    -DXPRJ_samd21g18a=$(CND_CONF)    $(COMPARISON_BUILD)  -Wformat=2 -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wmissing-noreturn -mdfp="${DFP_DIR}/samd21a" ${PACK_COMMON_OPTIONS} 
	
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
