from threading import Thread
from time import sleep
from data_manager import save_sensor_data, read_sensor_data_last_hours
from graph_generator import create_all_graphs
from mqtt_client import init_mqtt_client, is_device_alive
from telegram_notifier import send_alert
from config import *
from secrets import *
from logger import logger
import sys

is_stopping = False

def data_writer_thread():
	while True:
		sleep(DATA_WRITE_DELAY)
		save_sensor_data()

def graph_creator_thread():
	while True:
		sleep(GRAPH_GENERATION_DELAY)
		create_all_graphs()

def telegram_alert_thread():
	while True:
		sleep(1)
		if not is_device_alive():
			send_alert("Device died!, server shutting down!")
			sys.exit(1)
		
def main():
	global is_stopping
	logger.info("Starting server...")
	mqtt_client = init_mqtt_client()

	Thread(target=data_writer_thread, daemon=True).start()
	Thread(target=graph_creator_thread, daemon=True).start()
	if TELEGRAM_ENABLED:
		Thread(target=telegram_alert_thread, daemon=True).start()
	else:
		logger.warning("telegram is disabled!")

	try:
		while True:
			sleep(1)
	except KeyboardInterrupt:
		if not is_stopping: # ensure user can only press it once
			is_stopping = True
			logger.info("stopping...")
			mqtt_client.loop_stop()

if __name__ == "__main__":
	main()