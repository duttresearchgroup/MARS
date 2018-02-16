FROM ubuntu:16.04
MAINTAINER Biswadip Maity <biswadip.maity@gmail.com>

# get dependencies
RUN apt-get update
RUN apt-get install -y build-essential git-core python-dev python vim \
    g++-arm-linux-gnueabihf gcc-arm-none-eabi bc sshpass
RUN apt-get clean

RUN mkdir -p /workspace
WORKDIR /workspace

RUN git clone https://github.com/duttresearchgroup/sparta.git
RUN git clone --depth 1 https://github.com/hardkernel/linux -b odroidxu3-3.10.y

WORKDIR /workspace/linux
ADD build.bash /workspace/linux
RUN chmod ugo+x build.bash
RUN ./build.bash

ENTRYPOINT bash
WORKDIR /workspace/sparta
RUN sed -i '0,/=host/s//=arm/' .makefile.buildopts
RUN make daemons
RUN ln -sf /bin/bash /bin/sh