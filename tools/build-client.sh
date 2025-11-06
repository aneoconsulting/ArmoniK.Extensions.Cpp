#!/usr/bin/env bash

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"
source "${script_path}"/common.sh

IMAGE_TAG="${1:-armonik_sdk_client_test}"
IMAGE_VERSION="${2:-"${ARMONIK_SDK_VERSION_DEFAULT}"}"
API_VERSION="${3:-"${ARMONIK_API_VERSION_DEFAULT}"}"

docker build -t "${IMAGE_TAG}:${IMAGE_VERSION}" \
  -f ${working_dir}/ArmoniK.SDK.Client/Dockerfile \
  --build-arg="API_VERSION=$API_VERSION" \
  --build-arg="CLIENT_VERSION=$IMAGE_VERSION" \
  "${working_dir}"
