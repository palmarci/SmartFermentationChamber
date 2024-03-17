from datetime import datetime, timedelta
import paho.mqtt.client as mqtt
from time import sleep
from threading import Thread
import requests
import json
import sys
import time
import os
import matplotlib.pyplot as plt

# config
MQTT_ADDRESS = "192.168.1.100"  
MQTT_PORT = 1883
MQTT_ROOT_TOPIC = "FermControl/"
MQTT_MEAS_TOPIC = "measurement"
MQTT_LOG_TOPIC = "log"
MQTT_HEARTBEAT_TOPIC = "heartbeat"

HEARTBEAT_TIMEOUT = 70 # seconds

TELEGRAM_TOKEN = 'CHANGEME'
TELEGRAM_ID = 'CHANGEME'

DATA_OUTPUT_FILE = "sensor_data.json"
DATA_WRITE_DELAY = 10 # write buffer to file
GRAPH_FILTER = 24 # last X hours to plot
GRAPH_OUTPUT_FOLDER = "graphs"
GRAPH_GENERATION = 2  # 60 * 5 # re-generate the graphs every X seconds

# globals 
last_msg_time = None
date_format = "%Y-%m-%d %H:%M:%S" 
sensor_data_buffer = {}

# TODO lock file for race condition between threads?

def graph_creator_func():
	log("starting graph generator thread")
	while True:
		sleep(GRAPH_GENERATION)

		if not os.path.exists(GRAPH_OUTPUT_FOLDER):
			os.makedirs(GRAPH_OUTPUT_FOLDER)

		# read back old log file
		with open(DATA_OUTPUT_FILE, 'r') as file:
			try:
				data = file.read()
				sensor_data = json.loads(data)
			except json.JSONDecodeError:
				print(f"Error: Unable to decode JSON data from file {DATA_OUTPUT_FILE}")
				return

		# Filter data for the last 24 hours
		current_time = datetime.now()
		start_time = current_time - timedelta(hours=GRAPH_FILTER)
		filtered_sensor_data = {}

		for timestamp, readings in sensor_data.items():
			converted_time = datetime.fromtimestamp(int(timestamp))
			if converted_time >= start_time:
				filtered_sensor_data[timestamp] = readings

		# get the available sensor names
		sensor_names = set()
		for sensor in filtered_sensor_data.values():
			for sensor_name in sensor:
				sensor_names.add(sensor_name)
		
		# create the graph data and generate a graph
		for sensor in sensor_names:
			tss = []
			values = []
			for timestamp, readings in filtered_sensor_data.items():
				values.append(readings[sensor])
				tss.append(timestamp)
			generate_graph(sensor, tss, values)


def generate_graph(sensor, timestamps, values):
	sensor_title = sensor.replace("_", " ")
	sensor_title = sensor_title.title()
	plt.figure(figsize=(10, 6))
	plt.plot(timestamps, values)
	plt.title(f'{sensor_title} Sensor Data (Last 24 Hours)')
	plt.xlabel('Timestamp')
	plt.ylabel('Value')
	plt.grid(True)
	plt.xticks(rotation=45)
	plt.tight_layout()

	# Select 10 timestamps evenly spaced
	num_timestamps = len(timestamps)
	step = max(num_timestamps // 10, 1)  # Ensure at least 1 timestamp is selected
	selected_timestamps = timestamps[::step]

	# Convert selected timestamps to datetime for labeling
	datetime_labels = [datetime.fromtimestamp(int(ts)) for ts in selected_timestamps]
	x_ticks = list(range(0, num_timestamps, step))
	plt.xticks(x_ticks, datetime_labels)

	# Save graph as PNG file
	output_filename = os.path.join(GRAPH_OUTPUT_FOLDER, f'{sensor}.png')
	plt.savefig(output_filename)
	plt.close()
	log("saved graph " + output_filename)


def file_handler_func():
	log("file handler thread started")
	while True:
		sleep(DATA_WRITE_DELAY)
		dump_sensor_data(DATA_OUTPUT_FILE)

def alert(text):
	log(f"sending alert to telegram bot: {text}")
	url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendMessage"
	params = {"chat_id": TELEGRAM_ID, "text": text}
	response = requests.post(url, params=params).json()
	if not response["ok"]:
		log("error sending telegram message: " + json.dumps(response))
	
def log(text):
	date_str = datetime.now().strftime(date_format)
	print(f"{date_str}: {text}")

def heartbeat_func():
	log("starting heartbeat thread")
	while True:
		sleep(1)
		if last_msg_time == None:
			continue # a device did not connect yet, skip
		
		current_time = datetime.now()
		difference = current_time - last_msg_time

		if difference > timedelta(seconds=HEARTBEAT_TIMEOUT):
			alert("Device seems to have died!")
			# TODO: dont spam,exit?
			
def dump_sensor_data(filename):
	if len(sensor_data_buffer) == 0:
		return 0

	# Read previous data from file if it exists
	previous_data = {}
	if os.path.exists(filename):
		with open(filename, 'r') as file:
			try:
				previous_data = json.load(file)
			except json.JSONDecodeError:
				pass  # Ignore if file is empty or not in JSON format

	# Calculate average values
	averaged_data = {}
	for sensor, values in sensor_data_buffer.items():
		if values:
			average_value = sum(values) / len(values)
			averaged_data[sensor] = round(average_value, 1)

	# Add current timestamp to the data
	timestamp = int(time.time())
	previous_data[timestamp] = averaged_data

	# Write data back to file
	with open(filename, 'w') as file:
		json.dump(previous_data, file)

	# Clear sensor data buffer
	sensor_data_buffer.clear()

def store_sensor_data(sensor, data):
	#log(f"got sensor data for {sensor} = {data}")
	if sensor in sensor_data_buffer:
		sensor_data_buffer[sensor].append(data)
	else:
		sensor_data_buffer[sensor] = [data]

def on_message(client, userdata, message):
	try:
		global last_msg_time
		data = message.payload.decode()
		topic = message.topic
		last_msg_time = datetime.now()

		if MQTT_MEAS_TOPIC in topic:
			sensor = topic.split("/")[-1]
			data = float(data)
			store_sensor_data(sensor, data)

		elif MQTT_LOG_TOPIC in topic:
			log(data)

		elif MQTT_HEARTBEAT_TOPIC in topic:
			log("got heartbeat")

	except Exception as e:
		alert(f"problem during mqtt message handling: {e}") # TODO: alert user ?

def main():
	#alert("Controller just started up!")
	heartbeat_thread = Thread(target=heartbeat_func)
	heartbeat_thread.start()

	file_handler_thread = Thread(target=file_handler_func)
	file_handler_thread.start()

	graph_creator_thread = Thread(target=graph_creator_func)
	graph_creator_thread.start()

	client = mqtt.Client()
	client.on_message = on_message
	client.connect(MQTT_ADDRESS, MQTT_PORT)
	client.subscribe(MQTT_ROOT_TOPIC + MQTT_MEAS_TOPIC + "/#") # wildcard for every sensor
	client.subscribe(MQTT_ROOT_TOPIC + MQTT_LOG_TOPIC)
	client.subscribe(MQTT_ROOT_TOPIC + MQTT_HEARTBEAT_TOPIC)
	log("starting mqtt listener...")
	client.loop_forever()

if __name__ == "__main__":
	main()
	# TODO add graphing and data saving