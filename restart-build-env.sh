#!/usr/bin/env bash

set -x

IMAGE_NAME="armonik_sdk_build_env"
IMAGE_TAG="0.1.0"
CONTAINER_NAME="armonik_sdk_env"
docker stop "${CONTAINER_NAME}"
DEFAULT_API_DIR="$(pwd)/../Armonik.Api/packages/cpp/install"
ARMONIK_API_DIR="${ARMONIK_API_DIR:-${DEFAULT_API_DIR}}"
REMOTE_BUILD_ADDRESS="${REMOTE_BUILD_ADDRESS:-"127.0.0.1:2222"}"
SOURCE_DIR="$(pwd)"

docker build -t "${IMAGE_NAME}:${IMAGE_TAG}" -f BuildEnv.Dockerfile .

ARMONIK_SDK_INSTALL_DIR="$(pwd)/install"
mkdir -p "${ARMONIK_SDK_INSTALL_DIR}"

docker run --rm -d --cap-add sys_ptrace -p"${REMOTE_BUILD_ADDRESS}":22 --name "${CONTAINER_NAME}" -v "${SOURCE_DIR}:/app/source" -v "${ARMONIK_API_DIR}:/armonik/api" -v "${ARMONIK_SDK_INSTALL_DIR}:/app/install" "${IMAGE_NAME}:${IMAGE_TAG}"