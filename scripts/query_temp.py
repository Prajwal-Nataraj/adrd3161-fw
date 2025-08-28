import canopen
import time

net = canopen.Network()
net.connect(channel='can0', interface='socketcan')

node = net.add_node(0x13, 'adrd3161.eds')

node.nmt.wait_for_heartbeat()

print("Node up!")

node.sdo["COMMUTATION_MODE"].raw = 5

ex()

for i in range(10):
    v = node.sdo["SUPPLY_VOLTAGE"].read()
    print(v)
    time.sleep(2)

