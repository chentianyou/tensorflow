#!/bin/bash

sudo rm -f /etc/yum.repos.d/oushu*
sudo /opt/dependency/bazel-0.11.1-installer-linux-x86_64.sh
sudo yum install -y https://centos7.iuscommunity.org/ius-release.rpm
sudo yum install -y python36u python36u-libs python36u-devel python36u-pip
