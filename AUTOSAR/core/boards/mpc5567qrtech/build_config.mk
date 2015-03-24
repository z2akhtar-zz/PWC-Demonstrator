
# ARCH defines
ARCH=mpc55xx
ARCH_FAM=ppc
ARCH_MCU=mpc5567

# CFG (y/n) macros
CFG=PPC BOOKE E200Z6 MPC55XX MPC5567 BRD_MPC5567QRTECH SPE_FPU_SCALAR_SINGLE TIMER_TB
CFG+=MCU_ARC_CONFIG MCU_CACHE  # MCU_MMU MCU_LP
CFG+=CREATE_SREC

#CFG+=BOOT

# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+=ADC DIO DMA CAN GPT LIN MCU PORT PWM WDG NVM MEMIF FEE FLS SPI EA
# System + Communication + Diagnostic
MOD_AVAIL+=XCP CANIF CANTP J1939TP COM DCM DEM DET ECUM_FLEXIBLE ECUM_FIXED IOHWAB KERNEL PDUR WDGM RTE SCHM BSWM WDGIF LINIF
# Network management
MOD_AVAIL+=COMM NM CANNM CANSM
# Additional
MOD_AVAIL+=RAMLOG LWIP SOAD UDPNM RAMTST 
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

# Default cross compiler
DEFAULT_CROSS_COMPILE = /opt/powerpc-eabispe/bin/powerpc-eabispe-
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201314p
DEFAULT_CW_COMPILE= /c/devtools/Freescale/cw_mpc5xxx_2.10

# Defines
def-y += SRAM_SIZE=0x14000
def-y += L_BOOT_RESERVED_SPACE=0x10000

vle=$(if $(filter $(CFG),VLE),y)
novle=$(if $(vle),n,y)

# Software floating point, PowerPC No Small-Data ELF EABI Object Format
diab-$(vle)=-tPPCE200Z6VFS:simple
diab-$(novle)=-tPPCE200Z6NFS:simple
DIAB_TARGET?=$(diab-y)

GHS_TARGET=ppc5567

