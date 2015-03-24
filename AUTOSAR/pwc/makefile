
# Project name, used for output files et.c.
PROJECTNAME = pwc

# Default settings
include $(ROOTDIR)/scripts/project_defaults.mk

VPATH += ../config/$(board_name)
inc-y += ../config/$(board_name)


# Project files (adds all .c files in src directory)
VPATH += ../src
inc-y += ../src
inc-y += ../src/ext
inc-y += ../../core/arch/arm/arm_cm3/drivers/STM32F10x_StdPeriph_Driver/inc

#PROJECT_C_FILES=$(notdir $(wildcard ../src/ext/*.c))
#obj-y += $(PROJECT_C_FILES:%.c=%.o)

PROJECT_C_FILES=$(notdir $(wildcard ../src/*.c))
obj-y += $(PROJECT_C_FILES:%.c=%.o)

#PROJECT_C_FILES=$(notdir $(wildcard ../../core/arch/arm/arm_cm3/drivers/STM32F10x_StdPeriph_Driver/src/*.c))
#obj-y += $(PROJECT_C_FILES:%.c=%.o)

obj-y += misc.o
obj-y += stm32f10x_exti.o
obj-y += stm32f10x_usart.o
obj-y += stm32f10x_i2c.o
obj-y += stm32f10x_spi.o
obj-y += stm32f10x_dma.o

inc-y += ..



#################### Arctic Core make system ######################
# Following is an explanation of the most usable features 
# of the Arctic Core make system.
#
# obj-y : list of object files to build.
# VPATH : list of directories in which to
#         look for source files.
# inc-y : list of directories in which to
#         look for header files.
# libitem-y : list of libraries to include. 
# build-exe-y : the name of build the output file.
# ldcmdfile-y: path to linkscript, used when building "build-exe-y"
#
# For more information see:
# http://arccore.com/wiki/Makesystem
###################################################################
