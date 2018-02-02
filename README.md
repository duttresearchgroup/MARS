# Overview
Code base for SPARTA framework

This repository contains mostly the implementation of the prediction, task mapping, and DVFS algorithms used by the sense-predict-allocate approach described by the SmartBalance/RunDMC and related papers. A trace-based simulator for offline simulation of these algorithms and for platform design space exploration is also included. 

# Background
## Preparing your SD Card
 Install the Linux image on the target platform.
 If you are planning to cross-compile, instead of compiling our program on the target itself, you will also need the source code for the kernel.

If you are using Odroid XU3/XU4, you can find the images at [odroid site](https://wiki.odroid.com/odroid-xu4/os_images/linux/ubuntu/ubuntu).

## Components
This is a middleware which is composed of kernel modules, user daemons and tests. 
* *Modules*: Modules are loadable kernel drivers that enable features required by the user level daemons.
* *Daemons*: Daemons are the user-level processes which need the modules to be loaded.
* *Tests*: Test programs to check if the framework is working. The source files are located in /runtime/uapi.

# Compiling
## Cross compiling (Recommended)
We can cross-compile the framework on our host system (or on our docker if want to get started quickly), and then deploy it on the target platform.

Before we make, we must configure *.makefile.buildopts* correctly. These settings will be used for the build process. Important fields are 
* *ARCH_DEFAULT* : Specify the architecture of target platform. Ex: arm
* *PLAT_DEFAULT* : Given an architecture, which platform are we using. 
Ex: for odroid, PLAT_DEFAULT_arm=exynos5422
* *CROSS_COMPILE_usr*: Cross compiler to use for compiling the daemons. Ex:CROSS_COMPILE_usr_DEFAULT_arm_exynos5422=arm-linux-gnueabihf-
* *CROSS_COMPILE_krn*: Cross compiler to use for compiling the modules
Ex: CROSS_COMPILE_krn_DEFAULT_arm_exynos5422=arm-none-eabi-<br>
Modules are tightly coupled with the kernel and if we want to cross compile, we need the linux source code of the target platform. The next parameter will be used to specify that src path.
* *KERNEL_SRC_DEFAULT*: Specify the kernel source code if cross compiling modules.

Once you have set these values based on your target platform, we are ready for building. Please ensure that the compilers specified above are actually isnstalled on the system.

To build the binaries, we can use the following commands:
* Generate daemons: 
  * *make daemons*
* Generate modules
  * *make lin_sensing_module*
* Generate tests
  * *make ubench*
* Generate all binaries including tests
  * *make runtime*

## Directly on board
  Coming soon

# Deployment
After the binaries are built, it is time to deploy them on the target platform.
The steps in this phase require bash shell. You can execute `ls -l /bin/sh` to verify if default shell points to `/bin/bash`. The current procedure involves transferring the binaries from the host to the target platform using *rsync*. Please install `rsync` in the host as well as target platform. Additionaly `sshpass` is also required in the host system.
The steps are as follows:
* *Clone repo in the target platform*
* *env .sh*: Run `source scripts/env.sh` in the host machine
* *confs .sh*: Modify the entries in `scripts/confs.sh` which we need to specify in order to ssh into the target. 

  * Remote user credentials: *R_USER*, *R_PASS*
  * Remote host ip address: *R_HOST*
  * Build dir in the remote host: *R_SPARTA_ROOT*
  
* *remote_synch.sh*: run `sh scripts/common/remote_synch.sh` to copy cross-compile binaries to the target
* *env .sh*: Run `source scripts/env.sh` in the target platform

# Running
Test if the code is working properly by executing `sh scripts/tests/interface_test.sh`. If successful, you can see the results in *outdir*.

# Organization

* *traces*: contains executions traces used by the offline simulator
* *scripts*: python and bash scripts for generations traces and other miscellaneous tasks
* *src*: framework source code
  * *core*: C implementation of the task mapping and DVFS algorithms. src/core/vitamins.h may be included in both application level C and C++ code and kernel-level C code
  * *sim*: C++ implementation of the trace-based simulator
  * *apps*: Multiple C++ applications that use the trace simulator.
  * *linux-modules/vitamins*: Linux kernel module that implements the sense-predict-allocate. Depends on src/core. This module requires a kernel with sensing hooks extensions. See https://github.com/tiagormk/linux-vitamins-odroidxu
  * *sa_solver*: Simulated annealing algorithm for task mapping. Must be linked with core library if it is to be used
  * *linsched*, *mcpat*, *heartbeat*, *linux-modules/heartbeat*: Open source code developed by others required by the trace simulator or for other purposes
    
