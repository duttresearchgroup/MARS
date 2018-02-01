#!/bin/bash

docker build --tag mash:latest .

docker run -it --rm \
   --name drg-mash \
   mash:latest
