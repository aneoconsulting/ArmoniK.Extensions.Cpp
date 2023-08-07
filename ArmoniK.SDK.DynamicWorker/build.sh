#!/bin/sh

# Declare a variable that will store the image tag for our Docker image
IMAGE_TAG="armonik-sdk-cpp-workerdll:0.1.0"
cd ..
docker build --rm -t "${IMAGE_TAG}" -f ArmoniK.SDK.DynamicWorker/Dockerfile --progress plain .
