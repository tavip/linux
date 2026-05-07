#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

qemu-system-x86_64 --enable-kvm -m 512 -machine q35,kernel-irqchip=split \
  -device intel-iommu,intremap=on \
  -net nic,model=e1000 -net user,hostfwd=tcp::2222-:22 \
  -drive file=nvme.img,if=none,id=D22 -device nvme,drive=D22,serial=1234 \
  -hda ubuntu-24.04-server-cloudimg-amd64.img -hdb cloud.img \
  -kernel ubuntu-24.04-server-cloudimg-amd64-vmlinuz-generic \
  -initrd ubuntu-24.04-server-cloudimg-amd64-initrd-generic \
  -append 'root=LABEL=cloudimg-rootfs ro intel_iommu=on console=tty1 console=ttyS0' \
  -display none -serial mon:telnet::5555,server,nowait -daemonize

SSH_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"
export MYHOST=lkl@localhost
export MYSSH="sshpass -p lkl ssh $SSH_OPTS -p 2222 $MYHOST"
export MYSCP="sshpass -p lkl scp -O $SSH_OPTS -P 2222"
