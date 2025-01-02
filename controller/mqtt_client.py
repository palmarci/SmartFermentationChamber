import paho.mqtt.client as mqtt
from datetime import datetime
from config import *
from data_manager import store_sensor_data
from logger import logger

last_msg_time = None

def is_device_alive() -> bool:
	return datetime.now() - last_msg_time > TELEGRAM_ALERT_AFTER

def on_message(client, userdata, message):
	global last_msg_time
	last_msg_time = datetime.now()
	try:
		data = float(message.payload.decode())
		topic = message.topic
		sensor = topic.split("/")[-1]

		if MQTT_MEAS_TOPIC in topic:
			store_sensor_data(sensor, data)
		elif MQTT_LOG_TOPIC in topic:
			logger.info(f"Log from device: {data}")
		elif MQTT_HEARTBEAT_TOPIC in topic:
			logger.info("Received heartbeat.")

	except ValueError as e:
		logger.error("Error processing message: %s", e)

def init_mqtt_client():
	client = mqtt.Client()
	client.on_message = on_message
	client.connect(MQTT_ADDRESS, MQTT_PORT)
	client.subscribe([(MQTT_ROOT_TOPIC + MQTT_MEAS_TOPIC + "/#", 0), 
					  (MQTT_ROOT_TOPIC + MQTT_LOG_TOPIC, 0), 
					  (MQTT_ROOT_TOPIC + MQTT_HEARTBEAT_TOPIC, 0)])
	client.loop_start()
	return client