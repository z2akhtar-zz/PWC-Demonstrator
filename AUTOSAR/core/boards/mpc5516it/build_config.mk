
# ARCH defines
ARCH=mpc55xx
ARCH_FAM=ppc
ARCH_MCU=mpc5516

# CFG (y/n) macros
CFG=PPC BOOKE E200Z1 MPC55XX MPC5516 BRD_MPC5516IT TIMER_TB
CFG+=MCU_ARC_CONFIG MCU_MMU MCU_ARC_LP
CFG+=CREATE_SREC

ifneq ($(filter cw ghs,$(COMPILER)),)
CFG+=VLE
endif

# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+=ADC DIO DMA CAN GPT LIN MCU PORT PWM WDG NVM MEMIF FEE FLS SPI EEP EA 
# System + Communication + Diagnostic
MOD_AVAIL+=XCP CANIF CANTP LINIF COM DCM DEM DET ECUM_FIXED IOHWAB KERNEL PDUR WDGM WDGIF RTE J1939TP SCHM ECUM_FLEXIBLE BSWM
# Network management
MOD_AVAIL+=COMM NM CANNM CANSM LINSM
# Additional
MOD_AVAIL+= RAMLOG FLS_SST25XX RAMTST 
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

# Default cross compiler
DEFAULT_CROSS_COMPILE = /opt/powerpc-eabispe/bin/powerpc-eabispe-
DEFAULT_DIAB_COMPILE = /c/devtools/WindRiver/diab/5.9.3.0/WIN32
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201314p
DEFAULT_CW_COMPILE= /c/devtools/Freescale/cw_mpc5xxx_2.10

vle=$(if $(filter $(CFG),VLE),y)
novle=$(if $(vle),n,y)

diab-$(vle)=-tPPCE200Z1VFN:simple
diab-$(novle)=-tPPCE200Z1NFS:simple
DIAB_TARGET?=$(diab-y)

# VLE
GHS_TARGET?=ppc5516

# Defines
def-y += SRAM_SIZE=0x14000
def-y += L_BOOT_RESERVED_SPACE=0x10000
