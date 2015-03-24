
# prefered version
#CC_VERSION=4.4.5
# ARMv7, Thumb-2, little endian, soft-float. 

#cflags-y 	+= -mthumb -mcpu=cortex-r4 -mbig-endian
cflags-y	+= -mthumb -march=armv7-r -mbig-endian -mfloat-abi=hard -mfpu=vfpv3-d16
cflags-y 	+= -gdwarf-2


cflags-y += -ffunction-sections

lib-y   	+= -lgcc -lc -lm $(ROOTDIR)/$(ARCH_PATH-y)/drivers/f021/F021_API_CortexR4_BE_V3D16.lib
#/opt/arm-none-eabi/lib/f021/F021_API_CortexR4_BE_V3D16.lib
ASFLAGS 	+= -mthumb -march=armv7-r -mbig-endian -mfloat-abi=hard -mfpu=vfpv3-d16

LDFLAGS     += 

libpath-y +=-L/opt/arm-none-eabi/lib/gcc/arm-none-eabi/4.8.2/thumb/be/armv7-r/fpu_hard/vfpv3-d16
libpath-y +=-L/opt/arm-none-eabi/arm-none-eabi/lib/thumb/be/armv7-r/fpu_hard/vfpv3-d16
