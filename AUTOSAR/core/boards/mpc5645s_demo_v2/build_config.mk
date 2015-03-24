
# ARCH defines
ARCH=mpc55xx
ARCH_FAM=ppc
ARCH_MCU=mpc5645s

# CFG (y/n) macros
CFG=PPC E200Z4D MPC55XX MPC5645S BRD_MPC5645S_DEMO_V2 TIMER TIMER_TB
ifneq ($(COMPILER),gcc)
CFG+=VLE
endif
CFG+=SPE_FPU_SCALAR_SINGLE
CFG+=MCU_ARC_CONFIG
CFG+=CREATE_SREC
#CFG+=MCU_ARC_LP


# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+=ADC DIO DMA CAN GPT LIN MCU PORT PWM WDG NVM MEMIF FEE FLS SPI EEP EA ICU OCU
# System + Communication + Diagnostic
MOD_AVAIL+=XCP LINIF CANIF CANTP COM DCM DEM DET ECUM_FLEXIBLE ECUM_FIXED IOHWAB KERNEL PDUR WDGM WDGIF RTE J1939TP SCHM BSWM
MOD_AVAIL+=COMM NM CANNM CANSM LINSM
# Additional
MOD_AVAIL+= RAMLOG RAMTST
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

# Defines
#def-y += SRAM_SIZE=0x30000


# Default cross compiler
COMPILER?=cw
DEFAULT_CROSS_COMPILE = /opt/powerpc-eabivle/bin/powerpc-eabivle-
DEFAULT_CW_COMPILE= /c/devtools/Freescale/cw_mpc5xxx_2.10
DEFAULT_DIAB_COMPILE = /c/devtools/WindRiver/diab/5.9.3.0/WIN32
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201314p

vle=$(if $(filter $(CFG),VLE),y)
novle=$(if $(vle),n,y)
SPE_FPU_SCALAR_SINGLE=$(if $(filter $(CFG),SPE_FPU_SCALAR_SINGLE),y)
nospe=$(if $(SPE_FPU_SCALAR_SINGLE),n,y)

diab-$(vle)$(nospe)+=-tPPCE200Z4DVFS:simple
diab-$(novle)$(nospe)+=-tPPCE200Z4DNMS:simple
diab-$(vle)$(SPE_FPU_SCALAR_SINGLE)+=-tPPCE200Z4DVFF:simple		
diab-y+=$(diab-yy)

DIAB_TARGET?=$(diab-y)

GHS_TARGET?=ppc564xs


