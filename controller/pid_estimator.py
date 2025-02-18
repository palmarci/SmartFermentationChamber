import sys

import numpy as np
import pandas as pd
from scipy.signal import find_peaks
import matplotlib.pyplot as plt

from measurements import MeasurementCollector, MeasurementType

HOURS = 12
KU = 1.0  # You need to adjust this experimentally
SKIP_SAMPLES = 0

def main():
	ts = []
	values = []
	mc = MeasurementCollector()
	measurements = mc.get_last_by_hour(HOURS)
	print(f"loaded {len(measurements)} measurements")

	first_timestamp = measurements[0].timestamp
	for mc in measurements:
		if mc.type == MeasurementType.FOOD_TEMP:
			ts.append(mc.timestamp - first_timestamp) # normalize them for easier displaying
			values.append(mc.value)

	ts = ts[SKIP_SAMPLES:]
	values = values[SKIP_SAMPLES:]
	print(f"filtered down to {len(ts)} samples")
	ts = np.array(ts)

	plt.plot(ts, values)
	plt.show()

	peaks, _ = find_peaks(values)
	print(f"number of peaks: {len(peaks)}")

	if len(peaks) < 2:
		print("Not enough oscillations detected. Try increasing gain manually.")
		sys.exit(1)

	T_u = np.mean(np.diff(ts[peaks]))


	Kp_pid = 0.6 * KU
	Ti_pid = T_u / 2
	Td_pid = T_u / 8

	print(f"\n\nEstimated PID parameters:")
	print(f"Kp = {Kp_pid:.3f}, Ti = {Ti_pid:.3f}, Td = {Td_pid:.3f}")


if __name__ == "__main__":
	main()