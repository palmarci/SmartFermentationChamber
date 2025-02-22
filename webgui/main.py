from datetime import datetime
import sys
from time import sleep
import os
import argparse
import logging
import time
import threading

from flask import Flask, send_from_directory
import paho.mqtt.client as mqtt

from telegram import TelegramNotifier
from measurements import Measurement, MeasurementType, MeasurementCollector
from graphs import GraphGenerator

import my_secrets
import config

def message_callback(client, userdata, message):
	last_msg_time = datetime.now()
	logging.debug(f"got mqtt msg on {message.topic}")
	try:
		msg_type = message.topic.split("/")[-1]

		if config.MQTT_MEAS_TOPIC in message.topic:
			value = float(message.payload.decode())
			measurement_type = MeasurementType.from_string(msg_type)
			measurer.add(Measurement(measurement_type, value, int(time.time())))
		
		elif config.MQTT_LOG_TOPIC in message.topic:
			log_data = message.payload.decode()
			measurer.write_device_log(log_data)
		
		elif config.MQTT_HEARTBEAT_TOPIC in message.topic:
			logging.info("received heartbeat")

	except ValueError as e:
		logging.error("Error processing message: %s", e)

class MqttClient():
	def __init__(self, ip:str, port:int):
		self.client = mqtt.Client()
		self.client.on_message = message_callback
		self.client.connect(ip, port)
		self.client.subscribe([(config.MQTT_ROOT_TOPIC + config.MQTT_MEAS_TOPIC + "/#", 0), 
						(config.MQTT_ROOT_TOPIC + config.MQTT_LOG_TOPIC, 0), 
						(config.MQTT_ROOT_TOPIC + config.MQTT_HEARTBEAT_TOPIC, 0)])
		self.client.loop_start()

	def is_device_alive(self) -> bool:
		if last_msg_time is None:
			return True # xd
		return datetime.now() - last_msg_time > config.TELEGRAM_ALERT_AFTER

# globals
is_stopping = False
last_msg_time = None
last_graph_generated = datetime.now()
messager = None
measurer = None
grapher = None
app = Flask(__name__)
telegramer = None

@app.route('/data/<path:path>')
def send_report(path):
    # Using request args for path will expose you to directory traversal attacks
    return send_from_directory('data', path)

@app.route('/')
def serve_main_web():
	html = """
		<center>
		  <h1>SmartFermentationChamber Web Gui</h1>
		  <title>SmartFermentationChamber Web Gui</title>
		  <a href="/data/device.log">device log</a>
		  <br><br>
		  <div id="images-container"></div>
		</center>

		<script>
		  // List of image files with their corresponding IDs (ID and filename can be derived from same base name)
		  const images = [
		    { id: 'air_temp', file: 'air_temp.png' },
		    { id: 'food_temp', file: 'food_temp.png' },
		    { id: 'humidity', file: 'humidity.png' },
		    { id: 'heater_duty_cycle', file: 'heater_duty_cycle.png' },
		    { id: 'humidifier_duty_cycle', file: 'humidifier_duty_cycle.png' }
		  ];

		  // Dynamically create image elements
		  const container = document.getElementById('images-container');
		  images.forEach(img => {
		    const imageElem = document.createElement('img');
		    imageElem.id = img.id + '_img';
		    imageElem.src = `/data/${img.file}`;
		    container.appendChild(imageElem);
		    container.appendChild(document.createElement('br'));
		  });

		  // Update images by adding a timestamp to the URL to bypass caching
		  function updateImages() {
		    images.forEach(img => {
		      document.getElementById(img.id + '_img').src = `/data/${img.file}?` + new Date().getTime();
		    });
		    setTimeout(updateImages, 5000); // update every 5 seconds
		  }

		  updateImages();
		</script>

	"""
	return html


def configure_logging(is_debug: bool):
	log_level = logging.DEBUG if is_debug else logging.INFO

	log_format = "%(asctime)s | %(levelname)-8s | %(filename)-12s | %(message)s"

	logging.basicConfig(level=log_level, format=log_format)

	stream_handler = logging.StreamHandler(sys.stdout)
	stream_handler.setLevel(log_level)
	stream_handler.setFormatter(logging.Formatter(log_format))

	file_handler = logging.FileHandler(config.FILE_WEBGUI_LOGFILE, mode="a")
	file_handler.setLevel(log_level)
	file_handler.setFormatter(logging.Formatter(log_format))

	#logging.getLogger().addHandler(stream_handler)
	logging.getLogger().addHandler(file_handler)

def run_app():
	app.run(host='0.0.0.0', port=8080)

def main():
	global last_alerted, last_graph_generated, messager, measurer, grapher, telegramer

	if not os.path.isdir(config.FOLDER_DATA_FOLDER):
		os.makedirs(output_folder)

	parser = argparse.ArgumentParser(description="SmartFermChamber webgui software")
	parser.add_argument("--debug", action="store_true", help="Enable debug mode (default: off)")

	args = parser.parse_args()
	configure_logging(args.debug)

	if args.debug:
		config.GRAPH_GENERATION_PERIOD = 5
		config.FILE_DATA_FLUSH_AFTER = 5

	messager = MqttClient(config.MQTT_ADDRESS, config.MQTT_PORT)
	measurer = MeasurementCollector()
	grapher = GraphGenerator()

	flask_thread = threading.Thread(target=run_app, daemon=True)
	flask_thread.start()

	logging.info("webgui started!")

	if config.TELEGRAM_ENABLED:
			telegramer = TelegramNotifier(my_secrets.TELEGRAM_TOKEN, my_secrets.TELEGRAM_ID)
			last_alerted = None # TODO: fix this, this may crash if None initially
			telegramer.send_alert("Fermentation control started!")

	while True:

		# check if we need to do an alert
		if not messager.is_device_alive():
			last_alerted_delta = (datetime.now() - last_alerted).total_seconds()
			if config.TELEGRAM_ENABLED and last_alerted_delta > config.TELEGRAM_WAIT_BETWEEN_ALERTS:
				telegram_client.send_alert("The fermentation chamber seems to have died!")
				last_alerted = datetime.now()

		# generate a graph if necessary
		graph_timedelta = (datetime.now() - last_graph_generated).total_seconds()

		if graph_timedelta > config.GRAPH_GENERATION_PERIOD:
			logging.debug("generating graphs...")
			graph_data = measurer.get_last_by_hour(config.GRAPH_VISIBLE_HOURS)
			grapher.create_all_graphs(graph_data)
			last_graph_generated = datetime.now()

		sleep(1)

if __name__ == "__main__":
	main()