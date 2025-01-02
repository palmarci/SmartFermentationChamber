import matplotlib.pyplot as plt
from datetime import datetime, timedelta
import os
import json
from config import *
from logger import logger
from data_manager import read_sensor_data_last_hours

def generate_one_graph(sensor_name, timestamps, values):
	plt.figure(figsize=(10, 6))
	plt.plot(timestamps, values)
	plt.title(f'{sensor_name.replace("_", " ").title()} Sensor Data (Last {GRAPH_FILTER_HOURS} Hours)')
	plt.xlabel('Timestamp')
	plt.ylabel('Value')
	plt.grid(True)
	plt.xticks(rotation=45)
	plt.tight_layout()
	
	output_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", GRAPH_OUTPUT_FOLDER)
	if not os.path.isdir(output_folder):
		os.makedirs(output_folder)

	output_filename =  os.path.join(output_folder, f'{sensor_name}.png')
	plt.savefig(output_filename)
	plt.close()
	logger.info("Saved graph %s", output_filename)

def create_all_graphs():
	# if not os.path.exists(DATA_OUTPUT_FILE):
	# 	logger.warning("Sensor data file does not exist, skipping graph generation.")
	# 	return

	# with open(DATA_OUTPUT_FILE, 'r') as file:
	# 	try:
	# 		sensor_data = json.load(file)
	# 	except json.JSONDecodeError:
	# 		logger.error("Error decoding JSON data from %s", DATA_OUTPUT_FILE)
	# 		return
	
	# current_time = datetime.now()
	# start_time = current_time - timedelta(hours=GRAPH_FILTER_HOURS)
	# filtered_data = {ts: readings for ts, readings in sensor_data.items()
	# 				 if datetime.fromtimestamp(int(ts)) >= start_time}

	filtered_data = read_sensor_data_last_hours(GRAPH_FILTER_HOURS)
	sensor_names = {sensor for readings in filtered_data.values() for sensor in readings}
	
	for sensor in sensor_names:
		timestamps = [int(ts) for ts, readings in filtered_data.items() if sensor in readings]
		values = [readings[sensor] for ts, readings in filtered_data.items() if sensor in readings]
		generate_one_graph(sensor, timestamps, values)