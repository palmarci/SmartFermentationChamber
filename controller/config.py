MQTT_ADDRESS = "192.168.1.100"
MQTT_PORT = 1883
MQTT_ROOT_TOPIC = "FermControl/"
MQTT_MEAS_TOPIC = "measurement"
MQTT_LOG_TOPIC = "log"
MQTT_HEARTBEAT_TOPIC = "heartbeat"

TELEGRAM_ALERT_AFTER = 70 # seconds
TELEGRAM_ENABLED = False


DATA_OUTPUT_FILE = "sensor_data.json"
DATA_WRITE_DELAY = 10 # seconds

GRAPH_FILTER_HOURS = 24 # hours
GRAPH_OUTPUT_FOLDER = "html" # will be one dir higher than the main.py
GRAPH_GENERATION_DELAY = 300 # seconds