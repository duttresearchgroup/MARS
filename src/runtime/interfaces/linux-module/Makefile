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

obj-m := vitamins.o
vitamins-objs := main.o helpers.o setup.o sense.o sense_data.o user_if.o user_if_ioctl.o sensing_window.o
vitamins-objs += pal/$(PLAT)/perf_sense.o ../common/pal/$(PLAT)/setup_info.o
vitamins-objs += ../../../base/info_init.o
 
PWD := $(shell pwd)

all:
	    KCPPFLAGS="-DPLAT_DEF=$(PLAT)" make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:
	    KCPPFLAGS="-DPLAT_DEF=$(PLAT)" make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
