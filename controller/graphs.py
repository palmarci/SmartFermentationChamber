import matplotlib.pyplot as plt
from collections import defaultdict
import os
import logging

from measurements import Measurement, MeasurementType
from config import GRAPH_VISIBLE_HOURS, FOLDER_DATA_FOLDER

class GraphGenerator():

	def __init__(self):
		pass

	def __nice_name(self, text:str):
		return text.replace("_", " ").title()

	def __generate_one_graph(self, sensor_type:str, measurements:list[Measurement]):
		plt.figure(figsize=(10, 6))

		timestamps = []
		values = []
		for m in measurements:
			timestamps.append(m.timestamp)
			values.append(m.value)

		sensor_name = self.__nice_name(sensor_type)

		plt.plot(timestamps, values)
		plt.title(f'{sensor_name} Sensor Data (Last {GRAPH_VISIBLE_HOURS} Hours)')
		plt.xlabel('Timestamp') # TODO: fix the timestamp parsing in the graph
		plt.ylabel('Value')
		plt.grid(True)
		plt.xticks(rotation=45)
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