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
  sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo && \
  sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo && \
  sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo
  ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin
  yum --disableplugin=subscription-manager check-update ; \
  yum --disableplugin=subscription-manager \
          install -y \
          centos-release-scl \
          devtoolset-10
  yum --disableplugin=subscription-manager clean all
  echo "source /opt/rh/devtoolset-10/enable" >> /etc/bashrc
else
  echo "Unknown distribution"
  exit 1
fi

