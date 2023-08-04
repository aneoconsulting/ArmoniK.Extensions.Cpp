#!/bin/sh

set -ex

(cd ArmoniK.SDK.DLLWorker && ./build.sh)

docker build --build-arg DLLWorkerImage=armonik-sdk-cpp-workerdll:0.1.0 -f ArmoniK.SDK.Worker.Test/Dockerfile --progress plain -t armonik.sdk.worker.test:build .

if [ -z "$ARMONIK_SHARED_HOST_PATH" ]
then
  ARMONIK_SHARED_HOST_PATH=$(kubectl get secret -n armonik shared-storage -o jsonpath="{.data.host_path}" 2>/dev/null | base64 -d)
fi
ARMONIK_SHARED_HOST_PATH=${ARMONIK_SHARED_HOST_PATH:-"${HOME}/data"}

container="$(docker create "armonik.sdk.worker.test:build")"
mkdir -p "$ARMONIK_SHARED_HOST_PATH"
docker cp -L "$container":/app/build/lib/. "$ARMONIK_SHARED_HOST_PATH"

docker rm "$container"
