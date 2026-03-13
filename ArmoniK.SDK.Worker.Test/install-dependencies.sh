#!/bin/sh

if which apk
then
  apk update
  apk add --no-cache \
      g++ \
      make\
      cmake \
      linux-headers \
      bash
elif which apt-get
then
    apt-get update
    DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install -y\
        g++ \
        make\
        cmake
    apt-get clean
elif which yum
then
  rpm -ivh https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
  yum --disableplugin=subscription-manager update -y
  yum --disableplugin=subscription-manager install -y \
    gcc-c++ \
    make \
    cmake \
    rpm-build
else
  echo "Unknown distribution"
  exit 1
fi

