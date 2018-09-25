#-------------------------------------------------------------------------------
# Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-------------------------------------------------------------------------------

obj-m := vitamins-hooktest.o
vitamins-hooktest-objs := vitamins_main.o
vitamins-hooktest-objs += \
	../../core/vitamins_mars.o \
	../../core/vitamins_mars_shared.o \
	../../core/vitamins_cfs.o \
	../../core/vitamins_dvfs.o \
	../../core/vitamins_mts.o \
	../../core/vitamins_gts.o \
	../../core/vitamins_vanilla.o \
	../../core/vitamins_random.o \
	../../core/vitamins_optimal.o \
	../../core/vitamins_bin_predictor.o \
	../../core/vitamins_bin_predictor_init.o \
	../../core/vitamins_bin_predictor_io.o \
	../../core/vitamins_idlepower.o \
	../../core/vitamins_fileio.o
 
PWD := $(shell pwd)

all:
	    make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:
	    make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
