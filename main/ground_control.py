"""
Reaction Wheel Ground Control
------------------------------
Dear PyGui + paho-mqtt telemetry dashboard
"""

import json
import threading
import time
from collections import deque

import paho.mqtt.client as mqtt
import dearpygui.dearpygui as dpg

# ─── Config ───────────────────────────────────────────────────────────────────
BROKER_HOST   = "localhost"
BROKER_PORT   = 1883
TOPIC_TEL     = "rocket/telemetry"
TOPIC_PID     = "rocket/pid_set"
TOPIC_STATE   = "rocket/state"
MAX_POINTS    = 500

# ─── State ────────────────────────────────────────────────────────────────────
gz_buf    = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
out_buf   = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
t_buf     = deque([0.0] * MAX_POINTS, maxlen=MAX_POINTS)
lock      = threading.Lock()

mqtt_client   = None
connected     = False
msg_count     = 0
last_hz_time  = time.time()
current_hz    = 0.0
current_state = "IDLE"

# ─── MQTT ─────────────────────────────────────────────────────────────────────
def on_connect(client, userdata, flags, reason_code, properties):
    global connected
    connected = (reason_code == 0)
    client.subscribe(TOPIC_TEL)

def on_disconnect(client, userdata, flags, reason_code, properties):
    global connected
    connected = False

def on_message(client, userdata, msg):
    global msg_count, last_hz_time, current_hz
    try:
        data = json.loads(msg.payload.decode())
        gz  = data.get("gz",  0.0)
        out = data.get("out", 0.0)
        t   = data.get("t",   0.0) / 1e6

        with lock:
            gz_buf.append(gz)
            out_buf.append(out)
            t_buf.append(t)

        msg_count += 1
        now = time.time()
        elapsed = now - last_hz_time
        if elapsed >= 1.0:
            current_hz    = msg_count / elapsed
            msg_count     = 0
            last_hz_time  = now

    except Exception:
        pass

def mqtt_connect():
    global mqtt_client
    mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect    = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message    = on_message
    try:
        mqtt_client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
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

# ─── Callbacks ────────────────────────────────────────────────────────────────
def cb_send_pid(sender, app_data):
    kp = dpg.get_value("kp_input")
    ki = dpg.get_value("ki_input")
    kd = dpg.get_value("kd_input")
    sp = dpg.get_value("sp_input")
    publish_pid(kp, ki, kd, sp)
    dpg.set_value("pid_status", f"Sent  Kp={kp:.3f}  Ki={ki:.4f}  Kd={kd:.3f}  SP={sp:.2f}")

def cb_state(sender, app_data, user_data):
    global current_state
    current_state = user_data
    publish_state(user_data)

def cb_reconnect(sender, app_data):
    mqtt_connect()

# ─── GUI build ────────────────────────────────────────────────────────────────
DARK_BG    = (10,  12,  18,  255)
PANEL_BG   = (18,  22,  32,  255)
ACCENT     = (0,   210, 150, 255)   # teal
ACCENT2    = (255, 80,  80,  255)   # red for output
DIM        = (80,  90,  110, 255)
TEXT       = (210, 220, 230, 255)
TEXT_DIM   = (100, 115, 135, 255)

STATE_COLORS = {
    "IDLE":    (80,  90,  110, 255),
    "ARMED":   (255, 180, 0,   255),
    "LAUNCH":  (0,   210, 150, 255),
    "ABORT":   (255, 60,  60,  255),
}

def build_gui():
    dpg.create_context()

    # ── Theme ──────────────────────────────────────────────────────────────────
    with dpg.theme() as global_theme:
        with dpg.theme_component(dpg.mvAll):
            dpg.add_theme_color(dpg.mvThemeCol_WindowBg,       DARK_BG)
            dpg.add_theme_color(dpg.mvThemeCol_ChildBg,        PANEL_BG)
            dpg.add_theme_color(dpg.mvThemeCol_FrameBg,        (28, 34, 48, 255))
            dpg.add_theme_color(dpg.mvThemeCol_FrameBgHovered, (38, 46, 64, 255))
            dpg.add_theme_color(dpg.mvThemeCol_Button,         (28, 34, 48, 255))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered,  (0, 180, 130, 60))
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive,   ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_Text,           TEXT)
            dpg.add_theme_color(dpg.mvThemeCol_Border,         (40, 50, 70, 255))
            dpg.add_theme_color(dpg.mvThemeCol_SliderGrab,     ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_SliderGrabActive, ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_CheckMark,      ACCENT)
            dpg.add_theme_color(dpg.mvThemeCol_Header,         (0, 180, 130, 60))
            dpg.add_theme_color(dpg.mvThemeCol_HeaderHovered,  (0, 180, 130, 90))
            dpg.add_theme_style(dpg.mvStyleVar_WindowRounding,  8)
            dpg.add_theme_style(dpg.mvStyleVar_ChildRounding,   6)
            dpg.add_theme_style(dpg.mvStyleVar_FrameRounding,   4)
            dpg.add_theme_style(dpg.mvStyleVar_GrabRounding,    4)
            dpg.add_theme_style(dpg.mvStyleVar_WindowPadding,   14, 14)
            dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing,     8, 6)
            dpg.add_theme_style(dpg.mvStyleVar_FramePadding,    6, 4)

    dpg.bind_theme(global_theme)

    # ── Plot series data ───────────────────────────────────────────────────────
    x_init  = list(range(MAX_POINTS))
    y_zeros = [0.0] * MAX_POINTS

    # ── Viewport & window ─────────────────────────────────────────────────────
    dpg.create_viewport(
        title="Reaction Wheel Ground Control",
        width=1280, height=780,
        min_width=900, min_height=600
    )

    with dpg.window(tag="main_win", no_title_bar=True, no_move=True,
                    no_resize=True, no_scrollbar=True):

        # ── Header bar ────────────────────────────────────────────────────────
        with dpg.group(horizontal=True):
            dpg.add_text("REACTION WHEEL  //  GROUND CONTROL", color=ACCENT)
            dpg.add_spacer(width=30)
            dpg.add_text("●", tag="conn_dot", color=DIM)
            dpg.add_text("DISCONNECTED", tag="conn_label", color=DIM)
            dpg.add_spacer(width=20)
            dpg.add_text("-- Hz", tag="hz_label", color=TEXT_DIM)
            dpg.add_spacer(width=20)
            dpg.add_text("STATE:", color=TEXT_DIM)
            dpg.add_text("IDLE", tag="state_label", color=DIM)

        dpg.add_separator()
        dpg.add_spacer(height=6)

        # ── Main layout: left panel + right plot ──────────────────────────────
        with dpg.group(horizontal=True):

            # ── Left panel ────────────────────────────────────────────────────
            with dpg.child_window(width=280, height=-1, border=True):

                # Connection
                dpg.add_text("CONNECTION", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                dpg.add_input_text(label="Broker", default_value=BROKER_HOST,
                                   tag="broker_input", width=-1)
                dpg.add_button(label="Connect / Reconnect",
                               callback=cb_reconnect, width=-1)
                dpg.add_spacer(height=12)

                # PID Tuning
                dpg.add_text("PID TUNING", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)

                dpg.add_text("Kp", color=TEXT_DIM)
                dpg.add_input_float(tag="kp_input", default_value=1.0,
                                    step=0.01, format="%.3f", width=-1)
                dpg.add_text("Ki", color=TEXT_DIM)
                dpg.add_input_float(tag="ki_input", default_value=0.01,
                                    step=0.001, format="%.4f", width=-1)
                dpg.add_text("Kd", color=TEXT_DIM)
                dpg.add_input_float(tag="kd_input", default_value=0.05,
                                    step=0.001, format="%.3f", width=-1)
                dpg.add_text("Setpoint (rad/s)", color=TEXT_DIM)
                dpg.add_input_float(tag="sp_input", default_value=0.0,
                                    step=0.1, format="%.2f", width=-1)
                dpg.add_spacer(height=6)
                dpg.add_button(label="Send PID Parameters",
                               callback=cb_send_pid, width=-1)
                dpg.add_spacer(height=4)
                dpg.add_text("", tag="pid_status", color=TEXT_DIM, wrap=265)
                dpg.add_spacer(height=12)

                # State machine
                dpg.add_text("STATE MACHINE", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)

                with dpg.group(horizontal=True):
                    dpg.add_button(label="ARM",
                                   callback=cb_state, user_data="ARMED",
                                   width=120)
                    dpg.add_button(label="IDLE",
                                   callback=cb_state, user_data="IDLE",
                                   width=120)

                dpg.add_spacer(height=6)
                dpg.add_button(label="🚀  LAUNCH",
                               callback=cb_state, user_data="LAUNCH",
                               width=-1, height=40)
                dpg.add_spacer(height=6)
                dpg.add_button(label="⚠  ABORT",
                               callback=cb_state, user_data="ABORT",
                               width=-1, height=40)
                dpg.add_spacer(height=12)

                # Live values
                dpg.add_text("LIVE VALUES", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)
                with dpg.table(header_row=False, borders_innerV=False):
                    dpg.add_table_column()
                    dpg.add_table_column()
                    with dpg.table_row():
                        dpg.add_text("Gyro.z", color=TEXT_DIM)
                        dpg.add_text("0.000 rad/s", tag="live_gz")
                    with dpg.table_row():
                        dpg.add_text("Output", color=TEXT_DIM)
                        dpg.add_text("0.000", tag="live_out")

            dpg.add_spacer(width=8)

            # ── Right: plot ───────────────────────────────────────────────────
            with dpg.child_window(border=True):
                dpg.add_text("TELEMETRY", color=ACCENT)
                dpg.add_separator()
                dpg.add_spacer(height=4)

                with dpg.plot(tag="tel_plot", height=-1, width=-1,
                              anti_aliased=True):
                    dpg.add_plot_legend()
                    dpg.add_plot_axis(dpg.mvXAxis, label="Sample",
                                      tag="x_axis")
                    with dpg.plot_axis(dpg.mvYAxis, label="Value",
                                       tag="y_axis"):
                        dpg.add_line_series(x_init, y_zeros,
                                            label="Gyro.z (rad/s)",
                                            tag="gz_series")
                        dpg.add_line_series(x_init, y_zeros,
                                            label="PID Output",
                                            tag="out_series")

                    # Colour the series
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

    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("main_win", True)

# ─── Update loop ──────────────────────────────────────────────────────────────
def update_gui():
    """Called every frame to push new data into the plot."""
    # Connection indicator
    if connected:
        dpg.set_value("conn_dot",   "●")
        dpg.configure_item("conn_dot",   color=ACCENT)
        dpg.set_value("conn_label", "CONNECTED")
        dpg.configure_item("conn_label", color=ACCENT)
    else:
        dpg.set_value("conn_dot",   "●")
        dpg.configure_item("conn_dot",   color=ACCENT2)
        dpg.set_value("conn_label", "DISCONNECTED")
        dpg.configure_item("conn_label", color=DIM)

    # Hz
    dpg.set_value("hz_label", f"{current_hz:.1f} Hz")

    # State label
    col = STATE_COLORS.get(current_state, DIM)
    dpg.set_value("state_label", current_state)
    dpg.configure_item("state_label", color=col)

    # Plot data
    with lock:
        gz_list  = list(gz_buf)
        out_list = list(out_buf)
        t_list   = list(t_buf)

    x = list(range(len(gz_list)))
    dpg.set_value("gz_series",  [x, gz_list])
    dpg.set_value("out_series", [x, out_list])
    dpg.set_axis_limits("x_axis", 0, MAX_POINTS)
    dpg.fit_axis_data("y_axis")

    # Live value readouts
    if gz_list:
        dpg.set_value("live_gz",  f"{gz_list[-1]:.4f} rad/s")
        dpg.set_value("live_out", f"{out_list[-1]:.4f}")

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
