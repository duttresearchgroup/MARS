# Overview
Code base for MARS framework: Middleware for Adaptive Reflective Systems, partially supported by the National Science Foundation (NSF) under grant CCF-1704859.

This repository contains mostly the implementation of the prediction, task mapping, and DVFS algorithms used by the sense-predict-allocate approach described by the SmartBalance/RunDMC and related papers. A trace-based simulator for offline simulation of these algorithms and for platform design space exploration is also included. 

# Background
## Preparing your SD Card
 Install the Linux image on the target platform. You will also need the source code of the kernel currently installed to compile.

If you are using Odroid XU3/XU4, you can find the images at [odroid site](https://wiki.odroid.com/odroid-xu4/os_images/linux/ubuntu/ubuntu).

## Components
This is a middleware which is composed of kernel modules, user daemons and tests. 
* *Modules*: Modules are loadable kernel drivers that enable features required by the user level daemons.
* *Daemons*: Daemons are the user-level processes which need the modules to be loaded.
* *Tests*: Test programs to check if the framework is working. The source files are located in /runtime/uapi.

# Compiling 
## Cross Compiling (Recommended)
### **Compiling step**
This method uses two separate systems, a **host** machine (which can be your PC) and a **target** platform (Ex: Ordroid) to run the program. We can cross-compile the framework on our host system (or on our docker if want to get started quickly), and then deploy it on the target platform.

Some of the important parameters that we need to specify during compile time are:

* *ARCH: ISA used for compiling. Affects the GCC being used. Default is `ARCH=x86` if undefined*
* *PLAT: platform to use when compiling modules with platform-specific code. Set based on the value for ARCH if undefined. Ex: for odroid, `PLAT_DEFAULT_arm=exynos5422`*
* *CROSS_COMPILE_usr: Which GCC to use for user space applications. Set based on the value for ARCH if undefined*
* *CROSS_COMPILE_krn: Which GCC to use for kernel modules. Set based on the value for ARCH if undefined.*
* *EXTRAFLAGS: Extra GCC options. Set based on the value for ARCH if undefined*
* *KERNEL_SRC: Path to kernel source used when compiling linux kernel modules. Set based on the value for ARCH if undefined*
* *MODULE: linux module to compile when running 'make exp_module'. No default value.*
* *UBENCH: specify a specific target when running 'make ubench'*
* *DAEMONS: specify specific daemons to build when running 'make daemons'. If blank all daemons are built. Use "," to separate multiple values*

Modules are tightly coupled with the kernel and if we want to cross compile, we need the linux source code of the target platform. The next parameter will be used to specify that src path.
* *KERNEL_SRC_DEFAULT*: Specify the kernel source code if cross compiling modules.

Compiling for the first time :
When you run `make` for the first time, it will show the list of targets that we have and generate a makefile.buildopts file. Before start compiling, you must configure `makefile.buildopts` correctly for your setup. These settings will be used for the build process. 

Once you have set these values based on your target platform, we are ready for building. Please ensure that the compilers specified above are actually installed on the system.

Running make always prints the set values of all build parameters. Running "make all" or just "make" will print a help message listing the valid targets

To build the binaries, we can use the following commands:
* Generate daemons: 
  * *make daemons*
* Generate modules
  * *make lin_sensing_module*
* Generate tests
  * *make uapi_tests*
* Generate all binaries including tests
  * *make runtime*

Generate ubenchmarks
  * *make ubench*: Though the ubenchmarks are necessary for some of the test scripts, the ubenchmarks themselves are not tests for the framework and can be built and used independently from the framework. 

### **Deployment step** (Only required if you are cross compiling)
After the binaries are built, it is time to deploy them on the target platform.
The steps in this phase require bash shell. You can execute `ls -l /bin/sh` to verify if default shell points to `/bin/bash`. The current procedure involves transferring the binaries from the host to the target platform using *rsync*. Please install `rsync` in the host as well as target platform. Additionaly `sshpass` is also required in the host system.
The steps are as follows:
* *Clone repo in the target platform*
* *env .sh*: Run `source scripts/env.sh` in the host machine
* *confs .sh*: Modify the entries in `scripts/confs.sh` which we need to specify in order to ssh into the target. 

  * Remote user credentials: *R_USER*, *R_PASS*
  * Remote host ip address: *R_HOST*
  * Build dir in the remote host: *R_MARS_ROOT*
  
* *remote_synch.sh*: run `sh scripts/common/remote_synch.sh` to copy cross-compile binaries to the target
* *env .sh*: Run `source scripts/env.sh` in the target platform


## Compiling directly on board
  Coming soon

# Running on target platform
Test if the code is working properly in the target platform. 
* *The steps in this phase also require bash shell. Execute `ls -l /bin/sh` to verify*
* *Run `source scripts/env.sh`*
* *Execute `sh scripts/tests/interface_test.sh`. If successful, you can see the results in `outdir`.*

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
    
