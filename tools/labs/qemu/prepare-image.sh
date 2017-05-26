#!/bin/bash

size=$(stat -c%s $1)
if [ $size -lt 50000000 ]; then
    e2fsck -f $1
    resize2fs $1 50M
fi

TMP=$(mktemp -d)

mount -t ext4 -o loop $1 $TMP

# add console
echo "hvc0:12345:respawn:/sbin/getty 115200 hvc0" >> $TMP/etc/inittab

# enable networking
echo -e "auto eth0\niface eth0 inet dhcp" >> $TMP/etc/network/interfaces

sudo umount $TMP
rmdir $TMP
