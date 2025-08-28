import canopen
import time

net = canopen.Network()
net.connect(channel='can0', interface='socketcan')

node = net.add_node(0x13, '../app/objdict/adrd3161.eds')

node.nmt.wait_for_heartbeat()

print("Node up!")

print("Initial PWM value:", node.sdo["MOTOR_PWM_FREQUENCY"].raw)

print("Rebooting")
node.nmt.send_command(0x81)
node.nmt.wait_for_heartbeat()
print("Done")

print("PWM after reboot: ", node.sdo["MOTOR_PWM_FREQUENCY"].raw)

print("Storing 12345")
node.sdo["MOTOR_PWM_FREQUENCY"].raw = 12345
rb = node.sdo["MOTOR_PWM_FREQUENCY"].raw
print(f"Read back {rb}")
assert rb == 12345

print("Rebooting")
node.nmt.send_command(0x81)
node.nmt.wait_for_heartbeat()
print("Done")

print("PWM after reboot: ", node.sdo["MOTOR_PWM_FREQUENCY"].raw)

input("Continuing will overwrite tuning parameters. Want to continue? Cancel using Ctrl-C")

print("Storing 43212")
node.sdo["MOTOR_PWM_FREQUENCY"].raw = 43212
rb = node.sdo["MOTOR_PWM_FREQUENCY"].raw
print(f"Read back {rb}")
assert rb == 43212

print("Saving params")
node.store(4)

rb = node.sdo["MOTOR_PWM_FREQUENCY"].raw
print(f"Read back {rb}")
assert rb == 43212

print("Writing temporary 69444")
node.sdo["MOTOR_PWM_FREQUENCY"].raw = 69444
rb = node.sdo["MOTOR_PWM_FREQUENCY"].raw
print(f"Read back {rb}")
assert rb == 69444

print("Rebooting")
node.nmt.send_command(0x81)
node.nmt.wait_for_heartbeat()
print("Done")

val = node.sdo["MOTOR_PWM_FREQUENCY"].raw
print("PWM after reboot: ", val)

assert val == 43212

