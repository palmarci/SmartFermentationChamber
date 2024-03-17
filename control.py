from datetime import datetime, timedelta
import paho.mqtt.client as mqtt
from time import sleep
from threading import Thread

MQTT_ADDRESS = "192.168.1.100"  
MQTT_PORT = 1883
DATE_FORMAT = "%Y-%m-%d %H:%M:%S" 

root_topic = "FermControl/"
meas_topic = "measurement"
log_topic = "log"
heartbeat_topic = "heartbeat"

last_msg_time = None
heartbeat_timeout = 70 # seconds

def is_alive():
	do_print("starting heartbeat thread")

	while True:
		sleep(1)
		if last_msg_time == None:
			continue # a device did not connect yet, skip
		
		current_time = datetime.now()
		difference = current_time - last_msg_time

		if difference > timedelta(seconds=heartbeat_timeout):
			do_print("DEVICE SEEMS TO BE DEAD")
			# TODO notify & restart it (?)
		
def do_print(text):
	date_str = datetime.now().strftime(DATE_FORMAT)
	print(f"{date_str}: {text}")

def on_message(client, userdata, message):
	global last_msg_time
	data = message.payload.decode()
	topic = message.topic
	last_msg_time = datetime.now()

	if meas_topic in topic:
		sensor = topic.split("/")[-1]
		data = float(data)
		do_print(f"got sensor data for {sensor} = {data}")
	elif log_topic in topic:
		do_print(data)
	elif heartbeat_topic in topic:
		do_print("got heartbeat")

def main():

	heartbeat_thread = Thread(target=is_alive)
	heartbeat_thread.start()

	client = mqtt.Client()
	client.on_message = on_message
	client.connect(MQTT_ADDRESS, MQTT_PORT)
	client.subscribe(root_topic + meas_topic + "/#") # wildcard for every sensor
	client.subscribe(root_topic + log_topic)
	client.subscribe(root_topic + heartbeat_topic)

	client.loop_forever()

if __name__ == "__main__":
	main()
	# TODO add graphing and data saving