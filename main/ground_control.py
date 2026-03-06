"""
Reaction Wheel Ground Control
------------------------------
Dear PyGui + paho-mqtt telemetry dashboard
"""

import json
import csv
import threading
import time
from collections import deque

import paho.mqtt.client as mqtt
import dearpygui.dearpygui as dpg

recording = False
record_buffer = []

# ─── Config ───────────────────────────────────────────────────────────────────
BROKER_HOST   = "localhost"
BROKER_PORT   = 1883
TOPIC_TEL     = "rocket/telemetry"
TOPIC_PID     = "rocket/pid_set"
TOPIC_STATE   = "rocket/state"
TOPIC_DEBUG   = "rocket/debug"
MAX_POINTS    = 2000
PLOT_WINDOW_S = 20.0

# ─── State ────────────────────────────────────────────────────────────────────
gz_buf    = deque(maxlen=MAX_POINTS)
out_buf   = deque(maxlen=MAX_POINTS)
t_buf     = deque(maxlen=MAX_POINTS)
debug_lines = deque(maxlen=200)
lock      = threading.Lock()

mqtt_client   = None
connected     = False
msg_count     = 0
last_hz_time  = time.time()
current_hz    = 0.0
current_state = "IDLE"
plot_paused = False
t_origin = None 
last_state_change = 0.0
STATE_CHANGE_COOLDOWN = 2 #seconds

# ─── MQTT ─────────────────────────────────────────────────────────────────────
def on_connect(client, userdata, flags, reason_code, properties):
    global connected
    connected = (reason_code == 0)
    client.subscribe(TOPIC_TEL)
    client.subscribe(TOPIC_DEBUG)

def on_disconnect(client, userdata, flags, reason_code, properties):
    global connected
    connected = False

def on_message(client, userdata, msg):
    global msg_count, last_hz_time, current_hz

    if msg.topic == TOPIC_DEBUG:
        text = msg.payload.decode(errors="replace").strip()
        append_debug_log(text)
        return

    try:
        data = json.loads(msg.payload.decode())
        gz  = data.get("gz",  0.0)
        out = data.get("out", 0.0)
        t   = data.get("t",   0.0) / 1e6

        with lock:
            gz_buf.append(gz)
            out_buf.append(out)
            t_buf.append(t)

        if recording:
            record_buffer.append([t, gz, out])

        msg_count += 1
        now = time.time()
        elapsed = now - last_hz_time
        if elapsed >= 1.0:
            current_hz   = msg_count / elapsed
            msg_count    = 0
            last_hz_time = now

    except Exception:
        pass

def mqtt_connect(host=None):
    global mqtt_client
    if host is None:
        host = BROKER_HOST
    mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect    = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message    = on_message
    try:
        mqtt_client.connect(host, BROKER_PORT, keepalive=60)
        mqtt_client.loop_start()
    except Exception as e:
        print(f"MQTT connect failed: {e}")

def publish_pid(kp, ki, kd, sp):
    if mqtt_client and connected:
        payload = json.dumps({"kp": kp, "ki": ki, "kd": kd, "sp": sp})
        mqtt_client.publish(TOPIC_PID, payload)

def publish_state(state: str):
    if mqtt_client and connected:
        mqtt_client.publish(TOPIC_STATE, state)

def update_buttons():
    states = {
        "IDLE":   dict(arm=True,  idle=True, launch=False),
        "ARMED":  dict(arm=True, idle=True,  launch=True),
        "LAUNCH": dict(arm=False, idle=True,  launch=True),
        "ABORT":  dict(arm=False,  idle=False, launch=False),  
    }

    s = states.get(current_state, dict(arm=True, idle=False, launch=False))

    dpg.configure_item("btn_armed",  enabled=s["arm"])
    dpg.configure_item("btn_idle",   enabled=s["idle"])
    dpg.configure_item("btn_launch", enabled=s["launch"])

def append_debug_log(text):
    debug_lines.append(text)
    dpg.set_value("debug_log", "\n".join(debug_lines))

# ─── Callbacks ────────────────────────────────────────────────────────────────
def cb_send_pid(sender, app_data):
    kp = dpg.get_value("kp_input")
    ki = dpg.get_value("ki_input")
    kd = dpg.get_value("kd_input")
    sp = dpg.get_value("sp_input")
    publish_pid(kp, ki, kd, sp)
    #dpg.set_value("pid_status", f"Sent  Kp={kp:.3f}  Ki={ki:.4f}  Kd={kd:.3f}  SP={sp:.2f}")

def cb_state(sender, app_data, user_data):
    global current_state, last_state_change

    now = time.time() 

    if user_data != "ABORT":
        if now - last_state_change < STATE_CHANGE_COOLDOWN:
            return
        last_state_change = now

    current_state = user_data
    publish_state(user_data)

def cb_reconnect(sender, app_data):
    global mqtt_client
    host = dpg.get_value("broker_input")
    if mqtt_client:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()

    mqtt_connect(host)

def cb_record(sender, app_data):
    global recording, record_buffer

    if not recording:
        recording = True
        record_buffer = []

        dpg.set_item_label("rec_btn", "Stop Recording")
        dpg.bind_item_theme("rec_btn", "red_button")

        print("Recording started")

    else:
        recording = False

        dpg.set_item_label("rec_btn", "Start Recording")
        dpg.bind_item_theme("rec_btn", "green_button")

        filename = f"telemetry_{int(time.time())}.csv"

        with open(filename, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["time_s", "gyro_z", "pid_out"])
            writer.writerows(record_buffer)

        print(f"Saved {len(record_buffer)} samples to {filename}")

def cb_pause_plot(sender, app_data):
    global plot_paused
    plot_paused = not plot_paused
    if plot_paused:
        dpg.set_item_label("pause_btn", "Resume Plot")
        dpg.bind_item_theme("pause_btn", btn_themes["ARMED"])
        dpg.set_axis_limits_auto("x_axis") 
        dpg.set_axis_limits_auto("y_axis")
    else:
        dpg.set_item_label("pause_btn", "Pause Plot")
        dpg.bind_item_theme("pause_btn", btn_themes["IDLE"])

# ─── Colors ───────────────────────────────────────────────────────────────────
DARK_BG  = (10,  12,  18,  255)
PANEL_BG = (18,  22,  32,  255)
ACCENT   = (0,   210, 150, 255)
ACCENT2  = (255, 80,  80,  255)
DIM      = (80,  90,  110, 255)
TEXT     = (210, 220, 230, 255)
TEXT_DIM = (100, 115, 135, 255)
INACTIVE = (28,  34,  48,  255)

STATE_COLORS = {
    "IDLE":    (80,  90,  110, 255),
    "ARMED":   (255, 180, 0,   255),
    "LAUNCH":  (0,   210, 150, 255),
    "ABORT":   (200, 40,  40,  255),
}

STATE_BUTTONS = [
    ("IDLE",   "btn_idle"),
    ("ARMED",  "btn_armed"),
    ("LAUNCH", "btn_launch"),
    ("ABORT",  "btn_abort"),
]

# ─── Button theme helper ──────────────────────────────────────────────────────
def make_button_theme(color):
    r, g, b, a = color
    hover = (min(r + 30, 255), min(g + 30, 255), min(b + 30, 255), a)
    with dpg.theme() as t:
        with dpg.theme_component(dpg.mvButton):
            dpg.add_theme_color(dpg.mvThemeCol_Button,        color)
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, hover)
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive,  color)
    return t

# populated in build_gui() after create_context
btn_themes         = {}
btn_theme_inactive = None

# ─── GUI build ────────────────────────────────────────────────────────────────
def build_gui():
    global btn_theme_inactive, btn_themes

    dpg.create_context()

    # ── Button themes (must be after create_context) ───────────────────────────
    btn_theme_inactive = make_button_theme(INACTIVE)
    btn_themes = {
        "IDLE":    make_button_theme((60,  70,  90,  255)),
        "ARMED":   make_button_theme((200, 140, 0,   255)),
        "LAUNCH":  make_button_theme((0,   170, 115, 255)),
        "ABORT":   make_button_theme((200, 40,  40,  255)),
    }

    with dpg.theme(tag="green_button"):
        with dpg.theme_component(dpg.mvButton):
            dpg.add_theme_color(dpg.mvThemeCol_Button, (0,   170, 115, 255))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (30, 190, 130))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, (15, 150, 110))

    with dpg.theme(tag="red_button"):
        with dpg.theme_component(dpg.mvButton):
            dpg.add_theme_color(dpg.mvThemeCol_Button, (200, 40,  40,  255))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (210, 80, 80))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, (160, 40, 40))

    # ── Global theme ──────────────────────────────────────────────────────────
    with dpg.theme() as global_theme:
        with dpg.theme_component(dpg.mvAll):
            dpg.add_theme_color(dpg.mvThemeCol_WindowBg,         DARK_BG)
            dpg.add_theme_color(dpg.mvThemeCol_ChildBg,          PANEL_BG)
            dpg.add_theme_color(dpg.mvThemeCol_FrameBg,          (28, 34, 48, 255))
            dpg.add_theme_color(dpg.mvThemeCol_FrameBgHovered,   (38, 46, 64, 255))
            dpg.add_theme_color(dpg.mvThemeCol_Button,           INACTIVE)
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered,    (0, 180, 130, 60))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive,     ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_Text,             TEXT)
            dpg.add_theme_color(dpg.mvThemeCol_Border,           (40, 50, 70, 255))
            dpg.add_theme_color(dpg.mvThemeCol_SliderGrab,       ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_SliderGrabActive, ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_CheckMark,        ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_Header,           (0, 180, 130, 60))
            dpg.add_theme_color(dpg.mvThemeCol_HeaderHovered,    (0, 180, 130, 90))
            dpg.add_theme_style(dpg.mvStyleVar_WindowRounding,   8)
            dpg.add_theme_style(dpg.mvStyleVar_ChildRounding,    6)
            dpg.add_theme_style(dpg.mvStyleVar_FrameRounding,    4)
            dpg.add_theme_style(dpg.mvStyleVar_GrabRounding,     4)
            dpg.add_theme_style(dpg.mvStyleVar_WindowPadding,    14, 14)
            dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing,      8, 6)
            dpg.add_theme_style(dpg.mvStyleVar_FramePadding,     6, 4)

    dpg.bind_theme(global_theme)

    # ── Plot init data ─────────────────────────────────────────────────────────
    x_init  = list(range(MAX_POINTS))
    y_zeros = [0.0] * MAX_POINTS

    # ── Viewport ──────────────────────────────────────────────────────────────
    dpg.create_viewport(
        title="Reaction Wheel Ground Control",
        width=1280, height=780,
        min_width=1280, min_height=780
    )

    with dpg.window(tag="main_win",
                no_title_bar=True,
                no_move=True,
                no_resize=False,
                no_scrollbar=True,
                width=-1,
                height=-1):

        # ── Header ────────────────────────────────────────────────────────────
        with dpg.group(horizontal=True):
            dpg.add_text("REACTION WHEEL  //  GROUND CONTROL", color=ACCENT)
            dpg.add_spacer(width=30)
            dpg.add_text("",            tag="conn_dot",   color=DIM)
            dpg.add_text("DISCONNECTED", tag="conn_label", color=DIM)
            dpg.add_spacer(width=20)
            dpg.add_text("-- Hz", tag="hz_label", color=TEXT_DIM)
            dpg.add_spacer(width=20)
            dpg.add_text("STATE:", color=TEXT_DIM)
            dpg.add_text("IDLE",   tag="state_label", color=DIM)
            dpg.add_spacer(width=20)
            dpg.add_button(label="Pause Plot", tag="pause_btn", callback=cb_pause_plot)
            dpg.add_spacer(width=20)
            dpg.add_text("CMD:", color=TEXT_DIM)
            dpg.add_text("", tag="cooldown_label", color=TEXT_DIM)
        dpg.add_separator()
        dpg.add_spacer(height=6)

        # ── Main layout ───────────────────────────────────────────────────────
        with dpg.group(horizontal=True):

            # ── Left panel ────────────────────────────────────────────────────
            with dpg.child_window(width=320, height=-1, border=True):

                # Connection
                dpg.add_text("CONNECTION", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                dpg.add_input_text(label="", default_value=BROKER_HOST,
                                   tag="broker_input", width=-1)
                dpg.add_button(label="Connect / Reconnect",
                               callback=cb_reconnect, width=-1)
                dpg.add_spacer(height=10)

                # Recording data to CSV
                dpg.add_text("RECORDING", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                dpg.add_button(label="Start Recording", tag="rec_btn",
                                callback=cb_record, width=-1)
                dpg.bind_item_theme("rec_btn", "green_button")
                dpg.add_spacer(height=10)

                # PID Tuning
                dpg.add_text("PID TUNING", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                dpg.add_text("Kp", color=TEXT_DIM)
                dpg.add_input_float(tag="kp_input", default_value=1.0,
                                    step=0.01, format="%.3f", width=-1)
                dpg.add_text("Ki", color=TEXT_DIM)
                dpg.add_input_float(tag="ki_input", default_value=0.00,
                                    step=0.001, format="%.3f", width=-1)
                dpg.add_text("Kd", color=TEXT_DIM)
                dpg.add_input_float(tag="kd_input", default_value=0.00,
                                    step=0.001, format="%.3f", width=-1)
                dpg.add_text("Setpoint (rad/s)", color=TEXT_DIM)
                dpg.add_input_float(tag="sp_input", default_value=0.0,
                                    step=0.1, format="%.2f", width=-1)
                dpg.add_spacer(height=6)
                dpg.add_button(label="Send PID Parameters",
                               callback=cb_send_pid, width=-1)
                #dpg.add_spacer(height=4)
                #dpg.add_text("", tag="pid_status", color=TEXT_DIM, wrap=265)
                dpg.add_spacer(height=10)

                # State machine
                dpg.add_text("STATE MACHINE", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)

                with dpg.table(header_row=False, borders_innerV=False):
                    dpg.add_table_column()
                    dpg.add_table_column()

                    with dpg.table_row():
                        dpg.add_button(label="ARM",
                                    tag="btn_armed",
                                    callback=cb_state,
                                    user_data="ARMED",
                                    enabled=True,
                                    width=-1)

                        dpg.add_button(label="IDLE",
                                    tag="btn_idle",
                                    callback=cb_state,
                                    user_data="IDLE",
                                    enabled=True,
                                    width=-1)

                dpg.add_spacer(height=6)
                dpg.add_button(label="LAUNCH",
                               tag="btn_launch",
                               callback=cb_state, user_data="LAUNCH",
                               enabled=True,
                               width=-1, height=40)
                dpg.add_spacer(height=6)
                dpg.add_button(label="ABORT",
                               tag="btn_abort",
                               callback=cb_state, user_data="ABORT",
                               enabled=True,
                               width=-1, height=40)
                dpg.add_spacer(height=10)

                # Live values
                dpg.add_text("LIVE VALUES", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                with dpg.table(header_row=False, borders_innerV=False):
                    dpg.add_table_column()
                    dpg.add_table_column()
                    with dpg.table_row():
                        dpg.add_text("Gyro.z", color=TEXT_DIM)
                        dpg.add_text("0.0000 rad/s", tag="live_gz")
                    with dpg.table_row():
                        dpg.add_text("Gyro.z Max", color=TEXT_DIM)
                        dpg.add_text("0.0000", tag="max_gz")
                    with dpg.table_row():
                        dpg.add_text("Gyro.z Min", color=TEXT_DIM)
                        dpg.add_text("0.0000", tag="min_gz")
                    with dpg.table_row():
                        dpg.add_text("PID Out", color=TEXT_DIM)
                        dpg.add_text("0.0000", tag="live_out")
                    with dpg.table_row():
                        dpg.add_text("PID Out Max", color=TEXT_DIM)
                        dpg.add_text("0.0000", tag="max_out")
                    with dpg.table_row():
                        dpg.add_text("PID Out Min", color=TEXT_DIM)
                        dpg.add_text("0.0000", tag="min_out")
                    
            dpg.add_spacer(width=8)

            # ── Plot ──────────────────────────────────────────────────────────
            with dpg.child_window(width=-340, border=True):
                dpg.add_text("TELEMETRY", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)

                with dpg.plot(tag="tel_plot", height=-1, width=-1,
                              anti_aliased=True):
                    dpg.add_plot_legend()
                    dpg.add_plot_axis(dpg.mvXAxis, label="Time (s)",
                                      tag="x_axis")
                    with dpg.plot_axis(dpg.mvYAxis, label="Value",
                                       tag="y_axis"):
                        dpg.add_line_series(x_init, y_zeros,
                                            label="Gyro.z (rad/s)",
                                            tag="gz_series")
                        dpg.add_line_series(x_init, y_zeros,
                                            label="PID Output",
                                            tag="out_series")

                    with dpg.theme() as gz_theme:
                        with dpg.theme_component(dpg.mvLineSeries):
                            dpg.add_theme_color(dpg.mvPlotCol_Line,
                                                ACCENT, category=dpg.mvThemeCat_Plots)
                    with dpg.theme() as out_theme:
                        with dpg.theme_component(dpg.mvLineSeries):
                            dpg.add_theme_color(dpg.mvPlotCol_Line,
                                                ACCENT2, category=dpg.mvThemeCat_Plots)

                    dpg.bind_item_theme("gz_series",  gz_theme)
                    dpg.bind_item_theme("out_series", out_theme)
            
            dpg.add_spacer(width=8)

            # ── Right panel ────────────────────────────────────────────────────
            with dpg.child_window(width=320, height=-1, border=True):

                # ODrive panel
                dpg.add_text("ODRIVE", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                with dpg.table(header_row=False, borders_innerV=False):
                    dpg.add_table_column()
                    dpg.add_table_column()
                    with dpg.table_row():
                        dpg.add_text("Voltage", color=TEXT_DIM)
                        dpg.add_text("-- V", tag="odrive_volt")
                    with dpg.table_row():
                        dpg.add_text("Current", color=TEXT_DIM)
                        dpg.add_text("-- A", tag="odrive_curr")
                    with dpg.table_row():
                        dpg.add_text("RPM", color=TEXT_DIM)
                        dpg.add_text("-- RPM", tag="odrive_rpm")
                    with dpg.table_row():
                        dpg.add_text("Torque", color=TEXT_DIM)
                        dpg.add_text("-- Nm", tag="odrive_torque")
                    with dpg.table_row():
                        dpg.add_text("Axis State", color=TEXT_DIM)
                        dpg.add_text("--", tag="odrive_state")
                    with dpg.table_row():
                        dpg.add_text("Error", color=TEXT_DIM)
                        dpg.add_text("--", tag="odrive_error")

                # ESP32 DEBUG
                dpg.add_text("DEBUG", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                dpg.add_input_text(
                    tag="debug_log",
                    default_value="Waiting for debug messages...",
                    multiline=True,
                    readonly=True,
                    width=-1,
                    height=-1,
                )
                

    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("main_win", True)

# ─── Update loop ──────────────────────────────────────────────────────────────
def update_gui():
    # Connection indicator
    if connected:
        dpg.configure_item("conn_dot",   color=ACCENT)
        dpg.set_value("conn_label", "CONNECTED")
        dpg.configure_item("conn_label", color=ACCENT)
    else:
        dpg.configure_item("conn_dot",   color=ACCENT2)
        dpg.set_value("conn_label", "DISCONNECTED")
        dpg.configure_item("conn_label", color=STATE_COLORS.get("ABORT", DIM))

    # Cooldown indicator
    remaining = STATE_CHANGE_COOLDOWN - (time.time() - last_state_change)
    if current_state == "ABORT":
        dpg.set_value("cooldown_label", "ABORTED")
        dpg.configure_item("cooldown_label", color=STATE_COLORS["ABORT"])
    elif remaining > 0:
        dpg.set_value("cooldown_label", f"LOCK  {remaining:.1f}s")
        dpg.configure_item("cooldown_label", color=STATE_COLORS["ARMED"])
    else:
        dpg.set_value("cooldown_label", "READY")
        dpg.configure_item("cooldown_label", color=ACCENT)

    # Buttons
    update_buttons()

    # Hz
    dpg.set_value("hz_label", f"{current_hz:.1f} Hz")

    # State label
    col = STATE_COLORS.get(current_state, DIM)
    dpg.set_value("state_label", current_state)
    dpg.configure_item("state_label", color=col)

    # State button highlight
    for state, tag in STATE_BUTTONS:
        if current_state == state:
            dpg.bind_item_theme(tag, btn_themes[state])
        else:
            dpg.bind_item_theme(tag, btn_theme_inactive)

    # Plot
    with lock:
        gz_list  = list(gz_buf)
        out_list = list(out_buf)
        t_list   = list(t_buf)

    if len(gz_list) < 2:
        return

    global t_origin

    if not plot_paused:
        if t_list:
            x = t_list

            dpg.set_value("gz_series",  [x, gz_list])
            dpg.set_value("out_series", [x, out_list])

            if t_origin is None:
                t_origin = t_list[0]

            t_end   = t_list[-1]
            t_start = t_origin if (t_end - t_origin) < PLOT_WINDOW_S else t_end - PLOT_WINDOW_S
            dpg.set_axis_limits("x_axis", t_start, t_start + PLOT_WINDOW_S)
            dpg.fit_axis_data("y_axis")

    # Live readouts
    if gz_list:
        dpg.set_value("live_gz",  f"{gz_list[-1]:.4f} rad/s")
        dpg.set_value("max_gz",   f"{max(gz_list):.4f} rad/s")
        dpg.set_value("min_gz",   f"{min(gz_list):.4f} rad/s")
        dpg.set_value("live_out", f"{out_list[-1]:.4f}")
        dpg.set_value("max_out",  f"{max(out_list):.4f}")
        dpg.set_value("min_out",  f"{min(out_list):.4f}")

# ─── Main ─────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    mqtt_connect()
    build_gui()

    while dpg.is_dearpygui_running():
        update_gui()
        dpg.render_dearpygui_frame()

    dpg.destroy_context()
    if mqtt_client:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()