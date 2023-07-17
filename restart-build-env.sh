#!/usr/bin/env bash

set -x

IMAGE_NAME="armonik_sdk_build_env"
IMAGE_TAG="0.1.0"
CONTAINER_NAME="armonik_sdk_env"
docker stop "${CONTAINER_NAME}"
DEFAULT_API_DIR="$(pwd)/../Armonik.Api/packages/cpp/install"
ARMONIK_API_DIR="${ARMONIK_API_DIR:-${DEFAULT_API_DIR}}"
REMOTE_BUILD_ADDRESS="${REMOTE_BUILD_ADDRESS:-"127.0.0.1:2222"}"

docker build -t "${IMAGE_NAME}:${IMAGE_TAG}" -f BuildEnv.Dockerfile .

docker run --rm -d --cap-add sys_ptrace -p"${REMOTE_BUILD_ADDRESS}":22 --name "${CONTAINER_NAME}" -v "${ARMONIK_API_DIR}:/armonik/api" "${IMAGE_NAME}:${IMAGE_TAG}"