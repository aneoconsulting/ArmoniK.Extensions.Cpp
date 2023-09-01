#!/bin/sh
set -ex

script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"
environment="${1:-"Alpine"}"
worker_name=${2:-"armonik-sdk-cpp-dynamicworker"}
worker_version=${3:-"0.1.0"}
api_version="${4:-"3.11.0"}"
worker_test="${5:-"armonik-sdk-worker-test"}"
worker_tag="${worker_version}-$(echo "$environment" | awk '{print tolower($0)}')"
lib_build_path=${6:-""}
case "$environment" in
  "Alpine")
    dockerfile_name="Dockerfile"
    build_base_image="alpine"
  ;;
  "Ubuntu")
    dockerfile_name="Ubuntu.Dockerfile"
    build_base_image="ubuntu:23.04"
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

docker build --build-arg DynamicWorkerImage="${worker_name}:${worker_tag}" --build-arg BuildBaseImage="${build_base_image}" --build-arg WorkerLibVersion="$worker_version" -f "${working_dir}/ArmoniK.SDK.Worker.Test/Dockerfile" --progress plain -t "${worker_test}:build-${environment}" ${working_dir}

if [ -z "$lib_build_path" ]
then
  if [ -z "$ARMONIK_SHARED_HOST_PATH" ]
  then
    ARMONIK_SHARED_HOST_PATH=$(kubectl get secret -n armonik shared-storage -o jsonpath="{.data.host_path}" 2>/dev/null | base64 -d)
  fi
  lib_build_path=${ARMONIK_SHARED_HOST_PATH:-"${working_dir}/install"}
fi
mkdir -p "$lib_build_path"
docker run --rm -v "$lib_build_path:/host" --entrypoint sh "${worker_test}:build-${environment}" -c "cp /app/install/lib*/libArmoniK.SDK.Worker.Test.* /host/"
