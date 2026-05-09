#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

function package_path()
{
  grep \"name\":\"$1\" packagesite.yaml | jq -r .repopath
}

mkdir -p ~/freebsd-sysroot
cd ~/freebsd-sysroot
wget -q https://download.freebsd.org/releases/amd64/14.4-RELEASE/base.txz
tar -xf base.txz --exclude='./dev/*' --exclude='./chroot/*'

# Get FreeBSD package index...
wget -q https://pkg.freebsd.org/FreeBSD:14:amd64/latest/packagesite.pkg
tar -xf packagesite.pkg packagesite.yaml

wget -q https://pkg.freebsd.org/FreeBSD:14:amd64/latest/$(package_path argp-standalone)
wget -q https://pkg.freebsd.org/FreeBSD:14:amd64/latest/$(package_path fusefs-libs3)

tar -xf argp-standalone-*.pkg
tar -xf fusefs-libs3-*.pkg

rm fusefs-libs3-*.pkg argp-standalone-*.pkg base.txz packagesite.pkg packagesite.yaml
