import json
import time
import threading
from collections import deque

import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Configuration
MAX_POINTS = 300          # how many points to keep on plot
UPDATE_INTERVAL = 50      # ms for matplotlib refresh

# Data storage
time_data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
gz_data   = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
out_data  = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)

lock = threading.Lock()

# Debug counters
msg_count = 0
start_time = time.time()

# MQTT callbacks
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected, reason code: {reason_code}")
    client.subscribe("rocket/telemetry")

def on_message(client, userdata, msg):
    global msg_count, start_time
    msg_count += 1

    try:
        data = json.loads(msg.payload.decode())
    except:
        data = None

    # Debug: compute effective message frequency
    if msg_count % 50 == 0:
        elapsed = time.time() - start_time
        freq = msg_count / elapsed
        print(f"Received {msg_count} messages in {elapsed:.2f}s → {freq:.1f} Hz")
        msg_count = 0
        start_time = time.time()

    # Update plot data
    if data is not None:
        with lock:
            timestamp = data.get("t", None)
            if timestamp is not None:
                time_data.append(timestamp / 1e6)
                gz_data.append(data["gz"])
                out_data.append(data["out"])

# MQTT setup
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2) # type: ignore
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883)
client.loop_start()  # run MQTT in background thread


# Matplotlib 
fig, ax = plt.subplots()
line_gz, = ax.plot(list(time_data), list(gz_data), label="Gyro.z")
line_out, = ax.plot(list(time_data), list(out_data), label="PID Output")

ax.set_title("Reaction Wheel Telemetry")
ax.set_xlabel("Time (s)")
ax.set_ylabel("Value")
ax.legend()
ax.set_ylim(-2, 2)  # adjust as needed

def update(frame):
    with lock:
        line_gz.set_data(time_data, gz_data)
        line_out.set_data(time_data, out_data)
        if len(time_data) > 0:
            ax.set_xlim(time_data[0], time_data[-1])  # scroll X-axis
    return line_gz, line_out

ani = FuncAnimation(fig, update, interval=UPDATE_INTERVAL, cache_frame_data=False)
plt.show()