
# ARCH defines
ARCH=armv7_ar
ARCH_FAM=arm
ARCH_MCU=armv7_ar

#
# CFG (y/n) macros
# 

CFG=ARM ARMV7_AR ARM_GIC ZYNQ
# Add our board  
CFG+=BRD_ZYNQ_ZC702 OS_SYSTICK2

CFG+=TIMER_GLOBAL
CFG+=HW_FLOAT
CFG+=THUMB
CFG+=TIMER


# What buildable modules does this board have, 
# default or private

# MCAL
MOD_AVAIL+=ADC CAN DIO MCU PORT PWM GPT ICU WDG SPI EA
# System + Communication + Diagnostic
MOD_AVAIL+=XCP CANIF CANTP J1939TP COM DCM DEM DET ECUM_FLEXIBLE ECUM_FIXED IOHWAB KERNEL PDUR IPDUM WDGM RTE BSWM WDGIF
MOD_AVAIL+=COMM NM CANNM CANSM
# Additional
MOD_AVAIL+=RAMLOG TCF LWIP SLEEP RTE RAMTST
# CRC
MOD_AVAIL+=CRC
# Required modules
MOD_USE += MCU KERNEL

#
# Extra defines 
#

# Default cross compiler
DEFAULT_CROSS_COMPILE = /opt/arm-none-eabi/bin/arm-none-eabi-

