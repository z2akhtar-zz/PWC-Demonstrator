

CC = $(GHS_BIN)/ccrh850
LD = $(GHS_BIN)/ccrh850.exe

cflags-y += --gnu_asm

ASFLAGS += -cpu=rh850g3m
