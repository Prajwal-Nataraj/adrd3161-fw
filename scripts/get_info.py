#!/usr/bin/env python3

import argparse
import time
import canopen
from pathlib import Path

parser = argparse.ArgumentParser(description="Read serial number and some TMC9660 parameters from ADRD3161 via CANopen")

parser.add_argument("node_id", type=int, help="CANopen node ID")
parser.add_argument("-i", "--iface", type=str, help="CAN interface", default="can0")
parser.add_argument("-e", "--eds", type=str, help="EDS file", default=str(Path(__file__).parent / "adrd3161.eds"))

args = parser.parse_args()

net = canopen.Network()
net.connect(channel=args.iface, interface='socketcan')

node = net.add_node(args.node_id, args.eds)

node.nmt.wait_for_heartbeat()

sv = node.sdo["Manufacturer software version"].raw
sn = node.sdo["Identity"]["Serial number"].raw

print(f"Firmware version: {sv}")
print(f"Serial number: {sn}")

for i in range(5):
    temp = node.sdo["CHIP_TEMPERATURE"].raw

    print(f"Temperature: {temp}")

    time.sleep(0.5)
