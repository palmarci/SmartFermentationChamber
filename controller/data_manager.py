import json
import os
import time
from config import *
from logger import logger
from datetime import datetime, timedelta

sensor_buffer = []

def read_sensor_data_last_hours(hours:int) -> dict:
	if not os.path.exists(DATA_OUTPUT_FILE):
		logger.error("File %s does not exist.", DATA_OUTPUT_FILE)
		return {}

	try:
		with open(DATA_OUTPUT_FILE, 'r') as file:
			sensor_data = json.load(file)
			logger.info("Successfully read data from %s", DATA_OUTPUT_FILE)
	except json.JSONDecodeError:
		logger.error("Error decoding JSON data in %s", DATA_OUTPUT_FILE)
		return {}
	except Exception as e:
		logger.error("An error occurred while reading %s: %s", DATA_OUTPUT_FILE, e)
		return {}

	# Calculate the timestamp threshold based on the specified hours
	current_time = datetime.now()
	time_threshold = current_time - timedelta(hours=hours)
	timestamp_threshold = int(time_threshold.timestamp())

	# Filter data based on the timestamp threshold
	filtered_data = {
		timestamp: readings
		for timestamp, readings in sensor_data.items()
		if int(timestamp) >= timestamp_threshold
	}

	return filtered_data


def save_sensor_data():

	if len(sensor_buffer) == 0:
		logger.warning("skipping empty file write!")
		return

	previous_data = {}
	if os.path.exists(DATA_OUTPUT_FILE):
		#try:
		with open(DATA_OUTPUT_FILE, 'r') as file:
			previous_data = json.load(file)
		#except json.JSONDecodeError:
		#	logger.warning("Error decoding JSON data in %s", DATA_OUTPUT_FILE)

	timestamp = int(time.time())
	averaged_data = {sensor: round(sum(values) / len(values), 1)
						for sensor, values in sensor_buffer.items()}

	previous_data[timestamp] = averaged_data
	with open(DATA_OUTPUT_FILE, 'w') as file:
		json.dump(previous_data, file)
		logger.info("wrote json file")

	sensor_buffer.clear()

def store_sensor_data(sensor, data):
	sensor_buffer.append({sensor: data})