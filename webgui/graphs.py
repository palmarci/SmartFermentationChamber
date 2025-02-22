import matplotlib.pyplot as plt
from collections import defaultdict
import os
import logging
import matplotlib.dates as mdates
from datetime import datetime

from measurements import Measurement, MeasurementType
from config import GRAPH_VISIBLE_HOURS, FOLDER_DATA_FOLDER

class GraphGenerator():

	def __init__(self):
		pass

	def __nice_name(self, text:str):
		return text.replace("_", " ").title()

	def __generate_one_graph(self, sensor_type: str, measurements: list[Measurement]):
		plt.figure(figsize=(10, 6))

		timestamps = []
		values = []

		for m in measurements:
			try:
				timestamps.append(datetime.fromtimestamp(int(m.timestamp)))  # Convert Unix timestamp to datetime
			except (ValueError, TypeError):
				logging.error("Invalid timestamp: %s", m.timestamp)
				continue  # Skip invalid timestamps

			values.append(m.value)

		if not timestamps:
			logging.warning("No valid timestamps for %s", sensor_type)
			return

		sensor_name = self.__nice_name(sensor_type)

		plt.plot(timestamps, values, label='Sensor Data')

		# Compute average and draw a dotted red line
		avg_value = sum(values) / len(values)
		plt.axhline(avg_value, color='red', linestyle='dotted', linewidth=2, label=f'Avg: {avg_value:.2f}')

		plt.title(f'{sensor_name} Sensor Data (Last {GRAPH_VISIBLE_HOURS} Hours)')
		plt.xlabel('Timestamp')
		plt.ylabel('Value')
		plt.grid(True)

		# **Fix: Format timestamp axis properly**
		plt.xticks(rotation=45)
		plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d %H:%M'))  # Format as readable datetime
		plt.gca().xaxis.set_major_locator(mdates.AutoDateLocator())  # Auto adjust tick spacing

		plt.legend()  # Show legend including the average line
		plt.tight_layout()

		output_filename = os.path.join(FOLDER_DATA_FOLDER, f'{sensor_type}.png')
		plt.savefig(output_filename)
		plt.close()
		logging.info("Saved graph %s", output_filename)

	def create_all_graphs(self, data: list[Measurement]):
		grouped_data = defaultdict(list)

		# Group measurements by type
		for measurement in data:
			grouped_data[measurement.type].append(measurement)

		# Generate graphs for each type
		for measurement_type, measurements in grouped_data.items():
			self.__generate_one_graph(measurement_type.name.lower(), measurements)