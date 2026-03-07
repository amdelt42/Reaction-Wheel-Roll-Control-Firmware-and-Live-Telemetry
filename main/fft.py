import json
import numpy as np
import matplotlib.pyplot as plt

with open("fft_1772855272.json") as f:
    frames = json.load(f)

for frame in frames:
    print(f"t={frame['t']:.2f}s  rms={frame['rms_g']:.4f}g")

plt.figure()
for frame in frames:
    plt.plot(frame['freqs'], frame['psd'], alpha=0.4)
plt.xlabel("Frequency (Hz)")
plt.ylabel("PSD (m²/s⁴/Hz)")
plt.title("All FFT frames")
plt.show()

t = [f['t'] for f in frames]
rms = [f['rms_g'] for f in frames]
plt.figure()
plt.plot(t, rms)
plt.xlabel("Time (s)")
plt.ylabel("Accel RMS (g)")
plt.show()