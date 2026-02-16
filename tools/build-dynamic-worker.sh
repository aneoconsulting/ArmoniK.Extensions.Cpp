#!/usr/bin/env bash
set -euo pipefail

script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir_default="$(realpath "$script_path/../")"

environment="Alpine"
dockerfile_name="Dockerfile"
worker_name="armonik-sdk-cpp-dynamicworker"
worker_version=${ARMONIK_SDK_VERSION_DEFAULT:-""}
api_version=${ARMONIK_API_VERSION_DEFAULT:-""}
working_dir=$working_dir_default

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -e  Environment (default: '$environment')"
    echo "  -n  Worker name (default: '$worker_name')"
    echo "  -v  Worker version (default: '$worker_version')"
    echo "  -a  API version (default: '$api_version')"
    echo "  -w  Working directory (default: '$working_dir_default')"
    exit 1
}

while getopts ":e:n:v:a:w:" opt; do
  case $opt in
    e) environment="$OPTARG" ;;
    n) worker_name="$OPTARG" ;;
    v) worker_version="$OPTARG" ;;
    a) api_version="$OPTARG" ;;
    w) working_dir="$OPTARG" ;;
    \?) echo "Invalid option -$OPTARG" >&2; usage ;;
    :) echo "Option -$OPTARG requires an argument." >&2; usage ;;
  esac
done

if [ -z $api_version ]; then
    echo "Error: API version is not set. Please provide it using the -a option or set the ARMONIK_API_VERSION_DEFAULT environment variable."
    exit 1
fi

if [ -z $worker_version ]; then
    echo "Error: Worker version is not set. Please provide it using the -v option or set the ARMONIK_SDK_VERSION_DEFAULT environment variable."
    exit 1
fi

case "$environment" in
  "Alpine")
    dockerfile_name="Dockerfile"
  ;;
  "Ubuntu")
    dockerfile_name="Ubuntu.Dockerfile"
  ;;
  "RedHat")
    dockerfile_name="ubi7.Dockerfile"
  ;;
  *)
    printf "Unknown requested environment, please use one of:\n\tAlpine\n\tUbuntu\n\tRedHat"
    exit 1
esac

worker_tag="${worker_version}-$(echo "$environment" | awk '{print tolower($0)}')"

"${working_dir}"/ArmoniK.SDK.DynamicWorker/build.sh "${working_dir}/ArmoniK.SDK.DynamicWorker/${dockerfile_name}" "${worker_name}:${worker_tag}" "$api_version" "$worker_version"