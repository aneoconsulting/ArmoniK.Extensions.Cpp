#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armonik_sdk_client_test:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path" )"

docker build -t "${IMAGE_TAG}" -f ArmoniK.SDK.Client/Dockerfile --progress plain "${working_dir}"
