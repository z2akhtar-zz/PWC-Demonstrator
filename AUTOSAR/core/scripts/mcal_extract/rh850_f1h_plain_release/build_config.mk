
# ARCH defines
ARCH=rh850_x
ARCH_FAM=renesas
ARCH_MCU=rh850_f1h

# CFG (y/n) macros
CFG=RH850 RH850_F1H OS_SYSTICK2

# What buildable modules does this board have, 
# default or private

# Memory + Peripherals
MOD_AVAIL+= MCU PORT DIO GPT FR
# System + Communication + Diagnostic
MOD_AVAIL+=LINIF CAN CANIF CANTP COM DCM DEM DET ECUM IOHWAB KERNEL PDUR WDGM WDGIF RTE J1939TP SCHM
# Network management
MOD_AVAIL+=COMM NM CANNM CANSM LINSM
# Additional
MOD_AVAIL+= RAMLOG RAMTST 
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL ECUM

# Defines
#def-y += SRAM_SIZE=0x00020000

# Default cross compiler
COMPILER?=ghs
#DEFAULT_CROSS_COMPILE = /c/devtools/ghs/comp_201355
#DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201355
DEFAULT_CROSS_COMPILE = /opt/v850-elf/bin/v850-elf-
DEFAULT_GHS_COMPILE = /c/devtools/ghs/comp_201355

GHS_TARGET?=rh850