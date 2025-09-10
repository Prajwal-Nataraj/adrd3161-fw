#!/usr/bin/env bash

mkdir -p work

pushd work

echo "Download Zephyr toolchain"
if [[ ! -e arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb ]]; then
        ZEPHYR_SDK_VER="v0.17.2"
        wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/$ZEPHYR_SDK_VER/toolchain_linux-aarch64_arm-zephyr-eabi.tar.xz
        tar xvf toolchain_linux-aarch64_arm-zephyr-eabi.tar.xz arm-zephyr-eabi/bin/ arm-zephyr-eabi/arm-zephyr-eabi/bin/
else
        echo "Zephyr toolchain already downloaded in arm-zephyr-eabi/"
fi

echo "Build and install openocd with max32 DAP support"
sudo apt update
sudo apt install -y build-essential libtool pkg-config libusb-1.0-0-dev libhidapi-dev
if [[ ! -e /usr/local/share/openocd/scripts/target/max32xxx.cfg ]]; then
        git clone https://github.com/analogdevicesinc/openocd --branch 0.12.0-1.1.2 --depth 1
        pushd openocd
        git submodule update --init --recursive
        ./bootstrap
        ./configure --enable-cmsis-dap --disable-werror
        make -j
        sudo make install
        popd # openocd
else
        echo "OpenOCD already installed"
fi

popd # work
