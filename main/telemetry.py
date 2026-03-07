import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("telemetry_1772855272.csv")
# columns: time_s, gyro_z, pid_out, power_w, rpm, amag

fig, axes = plt.subplots(5, 1, figsize=(12, 12), sharex=True)

axes[0].plot(df['time_s'], df['gyro_z'], color='#00d296')
axes[0].set_ylabel("Gyro Z (rad/s)")
axes[0].grid(True, alpha=0.3)

axes[1].plot(df['time_s'], df['pid_out'], color='#ff5050')
axes[1].set_ylabel("PID Output")
axes[1].grid(True, alpha=0.3)

axes[2].plot(df['time_s'], df['amag'], color='#64b4ff')
axes[2].set_ylabel("Accel Mag (m/s²)")
axes[2].grid(True, alpha=0.3)

axes[3].plot(df['time_s'], df['power_w'], color='#ffb400')
axes[3].set_ylabel("Power (W)")
axes[3].set_xlabel("Time (s)")
axes[3].grid(True, alpha=0.3)

axes[4].plot(df['time_s'], df['rpm'], color='#b464ff')
axes[4].set_ylabel("RPM")
axes[4].set_xlabel("Time (s)")
axes[4].grid(True, alpha=0.3)

plt.suptitle("Reaction Wheel Telemetry")
plt.tight_layout()
plt.show()

# quick stats
print(f"Duration:     {df['time_s'].iloc[-1] - df['time_s'].iloc[0]:.2f} s")
print(f"Samples:      {len(df)}")
print(f"Gyro Z max:   {df['gyro_z'].abs().max():.4f} rad/s")
print(f"PID out max:  {df['pid_out'].abs().max():.4f}")
print(f"Peak power:   {df['power_w'].max():.2f} W")
print(f"Peak RPM:     {df['rpm'].max():.1f}")