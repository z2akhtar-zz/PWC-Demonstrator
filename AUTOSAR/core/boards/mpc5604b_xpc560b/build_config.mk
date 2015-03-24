
# ARCH defines
ARCH=mpc55xx
ARCH_FAM=ppc
ARCH_MCU=mpc5604b

# CFG (y/n) macros
CFG=PPC E200Z0 MPC55XX MPC560X MPC560XB MPC5604B BRD_MPC5604B_XPC560B
CFG+=MCU_ARC_CONFIG
CFG+=CREATE_SREC
CFG+=VLE


# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+=ADC DIO DMA CAN GPT LIN MCU PORT PWM WDG NVM MEMIF FEE FLS SPI EEP EA
# System + Communication + Diagnostic
MOD_AVAIL+=XCP LINIF CANIF CANTP COM DCM DEM DET ECUM_FLEXIBLE ECUM_FIXED IOHWAB KERNEL PDUR WDGM WDGIF RTE J1939TP SCHM BSWM
# Network management
MOD_AVAIL+=COMM NM CANNM CANSM LINSM
# Additional
MOD_AVAIL+= RAMLOG RAMTST 
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

# Default cross compiler
COMPILER?=cw
DEFAULT_CROSS_COMPILE = /opt/powerpc-eabispe/bin/powerpc-eabispe-
DEFAULT_CW_COMPILE= /c/devtools/Freescale/cw_mpc5xxx_2.10
DEFAULT_DIAB_COMPILE = /c/devtools/WindRiver/diab/5.9.3.0/WIN32
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201314p

# Defines
def-y += SRAM_SIZE=0xc000


# Software floating point, PowerPC No Small-Data ELF EABI Object Format
DIAB_TARGET?=-tPPCE200Z0VFS:simple
GHS_TARGET?=ppc560xb


