
# ARCH defines
ARCH=mpc55xx
ARCH_FAM=ppc
ARCH_MCU=mpc5777m

# CFG (y/n) macros
CFG=PPC E200Z4 MPC55XX MPC5777M CAN_OSCILLATOR_CLOCK BRD_MPC5777M_MMB
CFG+=EFPU
CFG+=MCU_ARC_CONFIG MCU_CACHE

ifneq ($(COMPILER),gcc)
CFG+=VLE
endif


# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+=OS ADC DIO DMA CAN GPT LIN MCU PORT PWM WDG NVM MEMIF FEE FLS SPI EEP EA
# System + Communication + Diagnostic
MOD_AVAIL+=LINIF CANIF CANTP COM DCM DEM DET ECUM_FLEXIBLE ECUM_FIXED IOHWAB KERNEL PDUR WDGM WDGIF RTE J1939TP SCHM
# Network management
MOD_AVAIL+=COMM NM CANNM CANSM LINSM
# Additional
MOD_AVAIL+= RAMLOG LWIP SOAD UDPNM
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

# Default cross compiler
COMPILER?=ghs
DEFAULT_CROSS_COMPILE = /opt/powerpc-eabispe/bin/powerpc-eabispe-
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201314p
DEFAULT_CW_COMPILE= /c/devtools/Freescale/cw_mpc5xxx_2.10
DEFAULT_DIAB_COMPILE = /c/devtools/WindRiver/diab/5.9.3.0/WIN32

vle=$(if $(filter $(CFG),VLE),y)
novle=$(if $(vle),n,y)
efpu=$(if $(filter $(CFG),EFPU),y)
nofpu=$(if $(efpu),n,y)

diab-$(vle)$(nofpu)+=-tPPCE200Z4VFN:simple
diab-$(novle)$(nofpu)+=-tPPCE200Z4NFS:simple
#diab-$(vle)$(efpu)+=-tPPCE200Z4251N3VFF:simple
diab-$(vle)$(efpu)+=-tPPCE200Z4VFF:simple
diab-y+=$(diab-yy)

DIAB_TARGET?=$(diab-y)

# VLE
GHS_TARGET?=ppc5777mz425

# Defines
def-y += SRAM_SIZE=0x60000


