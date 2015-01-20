# Copyright (c) 2012, Mauro Scomparin
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Mauro Scomparin nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY Mauro Scomparin ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Mauro Scomparin BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# File:			Makefile.
# Author:		Mauro Scomparin <http://scompoprojects.worpress.com>.
# Version:		1.0.0.
# Description:	Sample makefile.

#==============================================================================
#           Cross compiling toolchain / tools specifications
#==============================================================================

# Prefix for the arm-eabi-none toolchain.
# I'm using codesourcery g++ lite compilers available here:
# http://www.mentor.com/embedded-software/sourcery-tools/sourcery-codebench/editions/lite-edition/
PREFIX_ARM = arm-none-eabi

# Microcontroller properties.
PART=LM4F120H5QR
CPU=-mcpu=cortex-m4
FPU=-mfpu=fpv4-sp-d16 -mfloat-abi=softfp

# Stellarisware path
STELLARISWARE_PATH= $(HOME)/Software/StellarisWare/
# Binary, Object file path
OBJ_PATH = ./obj
# C Source Path
SRC_PATH = ./src

#Include Directory
INCLUDE_DIRS = $(STELLARISWARE_PATH) $(SRC_PATH)
INCLUDE_FLAG=-I. $(patsubst %, -I%, $(INCLUDE_DIRS))

# Program name definition for ARM GNU C compiler.
CC      = ${PREFIX_ARM}-gcc
# Program name definition for ARM GNU Linker.
LD      = ${PREFIX_ARM}-ld
# Program name definition for ARM GNU Object copy.
CP      = ${PREFIX_ARM}-objcopy
# Program name definition for ARM GNU Object dump.
OD      = ${PREFIX_ARM}-objdump
# Program name definition for ARM GNU debugging
GDB     = ${PREFIX_ARM}-gdb
# Program name definition for ARM GNU debugging tui mode
GDBTUI  = ${PREFIX_ARM}-gdb -tui

# Option arguments for C compiler.
CFLAGS=-mthumb ${CPU} ${FPU} -O0 -ffunction-sections -fdata-sections -MD -std=c99 -Wall -pedantic -c -g
# Library stuff passed as flags!
CFLAGS+= ${INCLUDE_FLAG} -DPART_$(PART) -c -DTARGET_IS_BLIZZARD_RA1
# Definition
CFLAGS+= -DDEBUG
# For UART-BUFFERED
CFLAGS+= -DUART_BUFFERED
# Flags for LD
LFLAGS  = --gc-sections

# Flags for objcopy
CPFLAGS = -Obinary

# flags for objectdump
ODFLAGS = -S

# I want to save the path to libgcc, libc.a and libm.a for linking.
# I can get them from the gcc frontend, using some options.
# See gcc documentation
LIB_GCC_PATH=${shell ${CC} ${CFLAGS} -print-libgcc-file-name}
LIBC_PATH=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}
LIBM_PATH=${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# Uploader tool path.
# Set a relative or absolute path to the upload tool program.
# I used this project: https://github.com/utzig/lm4tools
FLASHER=/usr/bin/lm4flash
# Flags for the uploader program.
FLASHER_FLAGS=

#==============================================================================
#                         Project properties
#==============================================================================

# Project name (W/O .c extension eg. "main")
PROJECT_NAME = main
# Startup file name (W/O .c extension eg. "LM4F_startup")
STARTUP_FILE = LM4F_startup
# Linker file name
LINKER_FILE = LM4F.ld

# C Source File
C_SRC += $(STARTUP_FILE).c
C_SRC += main.c
C_SRC += spi.c
C_SRC += tft.c
C_SRC += utilities.c
C_SRC += uartstdio.c
C_SRC += cmdline.c

# Object File
OBJS = $(addsuffix .o,$(addprefix $(OBJ_PATH)/,$(basename $(C_SRC))))
AXF = $(OBJ_PATH)/$(PROJECT_NAME).axf
BIN = $(OBJ_PATH)/$(PROJECT_NAME).bin
LST = $(OBJ_PATH)/$(PROJECT_NAME).lst

#==============================================================================
#                      Rules to make the target
#==============================================================================

#make all rule
all: $(OBJS) $(AXF) ${PROJECT_NAME}

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@echo
	@echo Compiling $<...
	$(CC) -c $(CFLAGS) ${<} -o ${@}

$(AXF): $(OBJS)
	@echo
	@echo Making driverlib
	$(MAKE) -C ${STELLARISWARE_PATH}driverlib/
	@echo
	@echo Linking...
	$(LD) -T $(LINKER_FILE) $(LFLAGS) -o ${AXF} $(OBJS) ${STELLARISWARE_PATH}driverlib/gcc-cm4f/libdriver-cm4f.a $(LIBM_PATH) $(LIBC_PATH) $(LIB_GCC_PATH)

${PROJECT_NAME}: $(AXF)
	@echo
	@echo Copying...
	$(CP) $(CPFLAGS) ${AXF} ${BIN}
	@echo
	@echo Creating list file...
	$(OD) $(ODFLAGS) ${AXF} > ${LST}

# make clean rule
clean:
	rm -rf $(OBJ_PATH)/*

# Rule to load the project to the board
# I added a sudo because it's needed without a rule.
load: clean all
	${FLASHER} ${BIN} ${FLASHER_FLAGS}

# GDB debugging
gdb: clean all
	./debug.sh gdb
	
gdbtui: clean all
	./debug.sh gdbtui

debug:
	./debug.sh nemiver

ddd:
	./debug.sh ddd
