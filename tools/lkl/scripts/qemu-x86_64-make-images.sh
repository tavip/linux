#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

wget -q https://cloud-images.ubuntu.com/releases/noble/release-20260321/ubuntu-24.04-server-cloudimg-amd64.img
wget -q https://cloud-images.ubuntu.com/releases/noble/release-20260321/unpacked/ubuntu-24.04-server-cloudimg-amd64-vmlinuz-generic
wget -q https://cloud-images.ubuntu.com/releases/noble/release-20260321/unpacked/ubuntu-24.04-server-cloudimg-amd64-initrd-generic

dd if=/dev/zero of=nvme.img bs=1024 count=102400
cat > cloud.txt <<EOF
#cloud-config
user: lkl
password: lkl
sudo: ['ALL=(ALL) NOPASSWD:ALL']
chpasswd: { expire: False }
groups: sudo
ssh_pwauth: True
shell: /bin/bash
EOF
cloud-localds cloud.img cloud.txt
rm cloud.txt
