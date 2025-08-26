# ADRD3161 Motor Control firmware

Implements CANOpen CiA 402 communication for the ADRD3161 Motor Control module.

Supported modes of operation:
- cyclic synchronous position (csp)
- cyclic synchronous velocity (csv)
- cyclic synchronous torque (cst)
- profile position (pp)
- profile velocity (pv)
- profile torque (tq)

Roadmap:
- [ ] Expose all TMC9660 parameters via the OD
- [ ] Simpler motor tuning procedure
- [ ] Support RamDebug via SDO block transfer, to enable automated tuning

## Flashing

Use a [MAX32625PICO programmer](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max32625pico.html) which has been [configured for MAX32690EVKIT](https://github.com/analogdevicesinc/max32625pico-firmware-images) (compatible with the MAX32662).

Drag-and-drop a `.hex` file from the [releases](https://github.com/analogdevicesinc/adrd3161-fw/releases) into the DAPLINK filesystem and wait for it to remount.

## UART shell

By default, the firmware exposes a UART shell. You may use the default Zephyr shell commands (such as `kernel reboot`), and the following custom commands:

- `tmc gap <param_number>` - Get parameter value
- `tmc sap <param_number> <value>` - Set parameter value
- `tmc flags` - Pretty print `GENERAL_STATUS_FLAGS`
- `tmc rwe_show` - Show RWE parameters
- `tmc rwe_save` - Save all current parameters to flash (run this after tuning)
- `tmc rwe_load` - Load all RWE parameters from flash

## Developing

### Setting up Zephyr

For more details, see the official [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

If running Windows, WSL is highly recommended.

Install prerequisite system packages:
```
sudo apt update
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

Create a folder with a python virtual environment:
```
mkdir ~/zephyrproject
cd ~/zephyrproject
python3 -m venv .venv
source .venv/bin/activate
```

Install `west` (metatool that does everyhting zephyr-related) and do a basic
setup:
```
pip install west
west init -m https://github.com/analogdevicesinc/adrd3161-fw adrd3161-fw
west update
west zephyr-export
west packages pip --install
(cd zephyr; west sdk install)
```

### Building and flashing

You must be located in the `adrd3161-fw` folder and have the virtualenv loaded (Run `source ~/zephyrproject/.venv/bin/activate` in every new terminal in which you need west commands).

Build:
```
west build app -b adrd3161
```

Clean ("pristine") build:
```
west build app -b adrd3161 -p
```

Flash (assuming the DAPLINK is attached to WSL using USB/IP):
```
west flash --openocd-search /MaximSDK/Tools/OpenOCD/scripts/ --openocd /MaximSDK/Tools/OpenOCD/openocd
```

If it gets stuck during flashing, just try again, there's some embedded magic I
don't understand going on and it only flashes every other try.

## Project structure

Board definition:

- `boards/adi/adrd3161`: board definition for both revA and revB (default). Especially relevant files:
    - `adrd3161_defconfig`: default kconfig enabling all board functionality
    - `adrd3161.dts`: base device tree
    - `adrd3161_X.overlay`: revision devicetree overlays

Out-of-tree drivers:
- `drivers`: Driver implementation
    - `misc/tmc9660`
- `dts/bindings`: Devicetree bindings
    - `misc/adi,tmc9660-spi.yaml`
- `include/app/drivers`: Driver headers to be included by apps
    - `misc/tmc9660/tmc9660.h`

Application implementation:
- `app`
    - `src`
        - `cia402.c/h`: Implementation of the CiA 402 state machine, decoupled from CANOpen and motor control specifics
        - `tmc9660.c/h`: TMC9660 parameter mode SPI driver
        - `tmc9660_params.c/h`: Logic for saving/loading parameters to/from flash + default out-of-the-box parameters
        - `storage.c/h`: Helper functions for nonvolatile storage
        - `CO_config_custom.h`: Custom CANOpenNode configuration
        - `main.c`
    - `objdict`: CANOpenNode object dictionary, EDS, XDD
    - `prj.conf`: Additional kconfig settings
