#!/bin/sh

script_path="$(dirname "${BASH_SOURCE:-$0}")"
. "${script_path}"/../tools/common.sh
working_dir="$(realpath "${script_path}/.." )"
dockerfile="${1:-"${working_dir}/ArmoniK.SDK.DynamicWorker/Dockerfile"}"
image_tag="${2:-"armonik-sdk-cpp-dynamicworker:${ARMONIK_SDK_VERSION_DEFAULT}"}"
api_version="${3:-"${ARMONIK_API_VERSION_DEFAULT}"}"
worker_version="${4:-"${ARMONIK_SDK_VERSION_DEFAULT}"}"
docker build --rm -t "$image_tag" -f "$dockerfile" --build-arg="API_VERSION=${api_version}" --build-arg="WORKER_VERSION=${worker_version}" --progress plain "$working_dir"
