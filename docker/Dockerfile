FROM duttresearchgroup/mars:prebuild
LABEL maintainer = "biswadip.maity@gmail.com"

# Build odroid kernel
WORKDIR /workspace/linux
ADD build.bash /workspace/linux
RUN chmod ugo+x build.bash
RUN ./build.bash

WORKDIR /workspace/mars