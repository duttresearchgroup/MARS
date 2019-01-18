.PHONY: all

include Makefile.config

all:
	@printf "\nSpecify one of the following targets\n"
	@printf "\tbuild:\n\t\tbuild docker image locally and overwrite default image\n"
	@printf "\trun:\n\t\tlaunch the MARS docker container\n"
	@exit

.PHONY: build run
# Dockers don't support absolute path. So we need to copy the whole source for jetson
build:
	if [ ! -d "linux-tegra" ]; then \
        cp -r $(JETSON_KERNEL_SRC) linux-tegra; \
    fi
	docker build . -f Dockerfile.prebuild -t duttresearchgroup/mars:prebuild
	docker build . -f Dockerfile -t duttresearchgroup/mars

# Use the following in case you want to make the changes persist inside container
# --mount source=mars-workspace,target=/workspace 
run:
	docker run -it --rm duttresearchgroup/mars