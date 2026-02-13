#!/usr/bin/env bash

script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"

source "${script_path}"/common.sh

environment="Alpine"
worker_name="armonik-sdk-cpp-dynamicworker"
worker_version=${ARMONIK_SDK_VERSION_DEFAULT:-default_version}
api_version=${ARMONIK_API_VERSION_DEFAULT:-default_api_version}
worker_test="armonik-sdk-worker-test"
default_dyn_lib_path=$(kubectl -n armonik get secrets shared-storage -o jsonpath='{.data.host_path}' 2>/dev/null | base64 -d 2>/dev/null)

if [ -z "$default_dyn_lib_path" ]; then
    echo "Warning: Could not retrieve default dynamic library path from Kubernetes secret."
fi

# Display usage information with default values
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -e  Environment (default: '$environment')"
    echo "  -n  Worker name (default: '$worker_name')"
    echo "  -v  Worker version (default: '$worker_version')"
    echo "  -a  API version (default: '$api_version')"
    echo "  -t  Worker test (default: '$worker_test')"
    exit 1
}

# Parse command line options
while getopts ":e:n:v:a:t:" opt; do
    case $opt in
        e) environment="$OPTARG" ;;
        n) worker_name="$OPTARG" ;;
        v) worker_version="$OPTARG" ;;
        a) api_version="$OPTARG" ;;
        t) worker_test="$OPTARG" ;;
        \?) echo "Invalid option -$OPTARG" >&2; usage ;;
        :)  echo "Option -$OPTARG requires an argument." >&2; usage ;;
    esac
done

# Check if -b option was provided
if [ -z "$dyn_lib_path" ]; then
    dyn_lib_path=${ARMONIK_SHARED_HOST_PATH:-$default_dyn_lib_path}
    echo "Warning: Option [-b dyn_lib_path] was not set."
    echo "         Using value: $dyn_lib_path"
    echo "         Make sure it is the correct one for your deployment"
fi

worker_tag="${worker_version}-$(echo "$environment" | awk '{print tolower($0)}')"

case "$environment" in
  "Alpine")
    build_base_image="alpine"
  ;;
  "Ubuntu")
    build_base_image="ubuntu:24.04"
  ;;
  "RedHat")
    build_base_image="dockerhubaneo/armonikworker_base:ubi7.9-0.0.1"
  ;;
  *)
    printf "Unknown requested environment, please use one of:\n\tAlpine\n\tUbuntu\n\tRedHat"
    exit 1
esac

"${script_path}"/build-dynamic-worker.sh -e "$environment" -n "$worker_name" -v "$worker_version" -a "$api_version" -w "$working_dir"

docker build --build-arg DynamicWorkerImage="${worker_name}:${worker_tag}" --build-arg BuildBaseImage="${build_base_image}" --build-arg WorkerLibVersion="$worker_version" -f "${working_dir}/ArmoniK.SDK.Worker.Test/Dockerfile" -t "${worker_test}:build-${environment}" ${working_dir}

docker run --rm -v "$dyn_lib_path:/host" --entrypoint sh "${worker_test}:build-${environment}" -c "cp /app/install/lib*/libArmoniK.SDK.Worker.Test.* /host/"
