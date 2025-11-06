#!/usr/bin/env bash

script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"

source "${script_path}"/common.sh

environment="Alpine"
worker_name="armonik-sdk-cpp-dynamicworker"
worker_version="${ARMONIK_SDK_VERSION_DEFAULT:-default_version}"
api_version="${ARMONIK_API_VERSION_DEFAULT:-default_api_version}"
worker_test="armonik-sdk-worker-test"
default_dyn_lib_path="$(realpath ${working_dir}/../ArmoniK/infrastructure/quick-deploy/localhost/data)"

# Display usage information with default values
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -e  Environment (default: '$environment')"
    echo "  -n  Worker name (default: '$worker_name')"
    echo "  -v  Worker version (default: '$worker_version')"
    echo "  -a  API version (default: '$api_version')"
    echo "  -t  Worker test (default: '$worker_test')"
    echo "  -b  Dynamic library path (default: $default_dyn_lib_path)"
    exit 1
}

# Parse command line options
while getopts ":e:n:v:a:t:b:" opt; do
    case $opt in
        e) environment="$OPTARG" ;;
        n) worker_name="$OPTARG" ;;
        v) worker_version="$OPTARG" ;;
        a) api_version="$OPTARG" ;;
        t) worker_test="$OPTARG" ;;
        b) dyn_lib_path="$OPTARG" ;;
        \?) echo "Invalid option -$OPTARG" >&2; usage ;;
        :)  echo "Option -$OPTARG requires an argument." >&2; usage ;;
    esac
done

# Check if -b option was provided
if [ -z "$dyn_lib_path" ]; then
    echo "Warning: Option [-b dyn_lib_path] was not set."
    echo "         Using default value: $default_dyn_lib_path"
    echo "         Make sure it is the correct one for your deployment"
fi

worker_tag="${worker_version}-$(echo "$environment" | awk '{print tolower($0)}')"

case "$environment" in
  "Alpine")
    dockerfile_name="Dockerfile"
    build_base_image="alpine"
  ;;
  "Ubuntu")
    dockerfile_name="Ubuntu.Dockerfile"
    build_base_image="ubuntu:24.04"
  ;;
  "RedHat")
    dockerfile_name="ubi7.Dockerfile"
    build_base_image="dockerhubaneo/armonikworker_base:ubi7.9-0.0.1"
  ;;
  *)
    printf "Unknown requested environment, please use one of:\n\tAlpine\n\tUbuntu\n\tRedHat"
    exit 1
esac

"${working_dir}"/ArmoniK.SDK.DynamicWorker/build.sh "${working_dir}/ArmoniK.SDK.DynamicWorker/${dockerfile_name}" "${worker_name}:${worker_tag}" "$api_version" "$worker_version"

docker build --build-arg DynamicWorkerImage="${worker_name}:${worker_tag}" --build-arg BuildBaseImage="${build_base_image}" --build-arg WorkerLibVersion="$worker_version" -f "${working_dir}/ArmoniK.SDK.Worker.Test/Dockerfile" -t "${worker_test}:build-${environment}" ${working_dir}

if [ -z "$dyn_lib_path" ]
then
  dyn_lib_path=${ARMONIK_SHARED_HOST_PATH:-"${working_dir}/install"}
fi

mkdir -p "$dyn_lib_path"
docker run --rm -v "$dyn_lib_path:/host" --entrypoint sh "${worker_test}:build-${environment}" -c "cp /app/install/lib*/libArmoniK.SDK.Worker.Test.* /host/"
