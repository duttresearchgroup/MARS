# Overview
Code base for Sparta framework

This repository contain mostly the implementation of the prediction, task mapping, and DVFS algorithms used by the sense-predict-allocate approach described by the SmartBalance/RunDMC and related papers. A trace-based simulator for offline simulation of these algorithms and for platform design space exploration is also included.

# Background
This is a middleware which is composed of modules and daemons. 
* *Modules*: Modules are loadable kernel extensions, that enables some of the features that are required by the user level daemons. Modules are tightly coupled with the kernel and we have two options if we want to build them.
  * *Directly on target*: We can build the modules on the target platform directly. <Need instructions>
  * *Cross compile*: If we have the source code of the kernel, we can compile the kernel modules on the host system and deploy the binaries on the target.

We need to specify our default architecture (ARCH_DEFAULT) in the .makefile.buildopts file.
Ex: If we are cross compiling for ODROID, we need to change the `ARCH_DEFAULT=arm`.

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
    
# Usage

Check makefile and vitamins.mk for details. Some useful commands:

* Generate separate libraries for each module: 
  * *make lib*
* Compile all applications at src/app
  * *make all*
* Cross-compiling a kernel module for ARM
  * *make linux_module ARCH=arm PLAT=exynos5422 MODULE=vitamins*

