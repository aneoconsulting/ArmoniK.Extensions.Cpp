#!/usr/bin/env bash

set -x

IMAGE_TAG="${1:-armoniksdk:0.1.0}"

# Get the absolute path of the current script and its directory
script_path="$(dirname "${BASH_SOURCE:-$0}")"
working_dir="$(realpath "$script_path/../" )"

# Create an install directory and store its absolute path
install_dir="$(realpath "${working_dir}/install")"
mkdir -p "${install_dir}"

docker build -t "${IMAGE_TAG}" -f ubi-package.Dockerfile "${working_dir}"

mkdir -p ${working_dir}/build

# Compile the project source using the Docker image
docker run --rm "${IMAGE_TAG}"
