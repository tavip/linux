#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0

script_dir=$(cd $(dirname ${BASH_SOURCE:-$0}); pwd)
source $script_dir/test.sh

pciname="0000:00:03.0"
nvme_id="8086 5845"
bin_name="disk-vfio-pci"

function wait_guest()
{
    for i in `seq 300`; do
        if $MYSSH exit 2> /dev/null; then
            break
        fi
        sleep 1
    done
}

function init()
{
    # initialize
    $MYSSH sudo modprobe vfio-pci
    $MYSSH "sh -c 'echo vfio-pci |
                      sudo tee /sys/bus/pci/devices/$pciname/driver_override'"
    $MYSSH "sh -c 'echo $nvme_id |
                      sudo tee /sys/bus/pci/drivers/vfio-pci/new_id'"
    $MYSSH "sh -c 'echo $pciname |
                      sudo tee /sys/bus/pci/drivers/nvme/unbind'"
    $MYSSH "sh -c 'echo $pciname |
                      sudo tee /sys/bus/pci/drivers/vfio-pci/bind'"
    $MYSSH sudo chown lkl:lkl /dev/vfio/3
    $MYSCP $script_dir/$bin_name lkl@localhost:
}

function cleanup()
{
    $MYSSH "sh -c 'echo $pciname |
                      sudo tee /sys/bus/pci/drivers/vfio-pci/unbind'"
}

if [ -z "$LKL_QEMU_TEST" ]; then
    lkl_test_plan 0 "disk-vfio-pci"
    echo "vfio not supported"
else
    lkl_test_plan 1 "disk-vfio-pci"
    lkl_test_run 1 wait_guest
    lkl_test_plan 1 "disk-vfio-pci"
    lkl_test_run 1 init
    lkl_test_exec $MYSSH ./$bin_name -n $pciname
    lkl_test_plan 1 "disk-vfio-pci"
    lkl_test_run 1 cleanup
fi
