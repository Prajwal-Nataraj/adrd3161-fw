# ADRD3161 Motor Control firmware

Check out https://analogdevicesinc.github.io/documentation/solutions/reference-designs/adrd3161

Implements CANOpen CiA 402 communication for the ADRD3161 Motor Control module.

Supported modes of operation:
- cyclic synchronous position (csp)
- cyclic synchronous velocity (csv)
- cyclic synchronous torque (cst)
- profile position (pp)
- profile velocity (pv)
- profile torque (tq)

Roadmap:
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
        - `cia402.c/h`: Device-agnostic CiA 402 state machine implementation, decoupled from CANopen and motor control specifics
        - `cia402_impl.h`: Interface between CiA 402 and application: "how do I spin a motor?"
        - `tmc9660.c/h`: TMC9660 parameter mode SPI driver
        - `tmc9660_params.c/h`: Logic for saving/loading TMC9660 parameters through the OD and UART shell
        - `main.c`
    - `objdict`: CANOpenNode object dictionary, EDS, XDD
    - `prj.conf`: Additional kconfig settings
