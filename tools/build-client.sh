#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armonik_sdk_client_test}"
IMAGE_VERSION="${2:-"0.1.0"}"
API_VERSION="${3:-"3.12.0"}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"

docker build -t "${IMAGE_TAG}:${IMAGE_VERSION}" -f ${working_dir}/ArmoniK.SDK.Client/Dockerfile --build-arg="API_VERSION=$API_VERSION" --build-arg="CLIENT_VERSION=$IMAGE_VERSION" --progress plain "${working_dir}"
