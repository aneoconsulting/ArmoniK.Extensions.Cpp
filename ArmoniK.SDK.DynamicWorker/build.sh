#!/bin/sh
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "${script_path}/.." )"
dockerfile="${1:-"${working_dir}/ArmoniK.SDK.DynamicWorker/Dockerfile"}"
image_tag="${2:-"armonik-sdk-cpp-dynamicworker:0.1.0"}"
api_version="${3:-"main"}"
worker_version="${4:-"0.1.0"}"
docker build --rm -t "$image_tag" -f "$dockerfile" --build-arg="API_VERSION=${api_version}" --build-arg="WORKER_VERSION=${worker_version}" --progress plain "$working_dir"
