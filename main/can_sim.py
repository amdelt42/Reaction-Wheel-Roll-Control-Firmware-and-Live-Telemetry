import can
import struct
import time
import math

bus = can.Bus(interface='slcan', channel='COM11', bitrate=250000)
# change COM3 to your port, or 'can0' on Linux with interface='socketcan'

node_id = 0

def send_odrive_msg(cmd_id, data):
    msg = can.Message(
        arbitration_id=(node_id << 5) | cmd_id,
        data=data,
        is_extended_id=False
    )
    bus.send(msg)

t = 0.0
try:
    while True:
        # simulate RPM ramping up and down
        rpm = 1500 + 1500 * math.sin(t * 0.2)
        vel = rpm / 60.0

        # simulate voltage dropping slightly under load
        vbus = 22.2 - (0.5 * abs(math.sin(t * 0.1)))

        # simulate current varying with RPM
        ibus = 1.0 + 3.0 * abs(math.sin(t * 0.2))

        # heartbeat at 10Hz
        send_odrive_msg(0x01, struct.pack('<IB', 0, 8))

        # encoder at 10Hz
        send_odrive_msg(0x09, struct.pack('<ff', t * vel, vel))

        # bus voltage/current at 5Hz
        send_odrive_msg(0x17, struct.pack('<ff', vbus, ibus))

        # Iq at 5Hz
        send_odrive_msg(0x14, struct.pack('<ff', ibus, ibus * 0.95))

        print(f"RPM: {rpm:.0f} | Vbus: {vbus:.2f}V | Ibus: {ibus:.2f}A | Power: {vbus*ibus:.1f}W")

        t += 0.1
        time.sleep(0.1)  # 10Hz loop

except KeyboardInterrupt:
    print("Stopped")
    bus.shutdown()