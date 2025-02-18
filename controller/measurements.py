import json
import os
import logging
import time
from enum import Enum
from datetime import datetime, timedelta
from config import FILE_DATA_OUTPUT, FILE_DEVICE_LOGFILE, FILE_DATA_FLUSH_AFTER, FILE_DATA_RETENTION_HOURS

class MeasurementType(Enum):
	FOOD_TEMP = 1
	AIR_TEMP = 2
	HUMIDITY = 3

	@classmethod
	def from_string(cls, input_str: str):
		input_str = input_str.upper()
		for member in cls:
			if input_str in member.name:
				return member
		logging.warning(f"can not resolve {input_str} as a type")
		return None

class Measurement:
	
	def __init__(self, type: MeasurementType, value: float, timestamp:int):
		self.type = type
		self.value = value
		self.timestamp = timestamp

	def __str__(self):
		return f"\nMeasurement: type={self.type}, value={self.value}, ts={self.timestamp}"

	def __repr__(self):
		return str(self)

	def to_dict(self) -> dict:
		#logging.debug(f"converting: {str(self)}")
		return {
			"type": self.type.name,  # Convert enum to its name (string)
			"value": self.value,
			"timestamp": self.timestamp
		}


class MeasurementCollector:

	def __init__(self):
		self.measurements = []

	def add(self, measurement: Measurement):
		self.measurements.append(measurement)
		if len(self.measurements) > FILE_DATA_FLUSH_AFTER:
			self.__save_to_disk()

	def __parse_from_disk(self) -> list[Measurement]:
		if not os.path.exists(FILE_DATA_OUTPUT):
			logging.warning(f"File {FILE_DATA_OUTPUT} does not exist, creating...")
			with open(FILE_DATA_OUTPUT, "w") as f:
				json.dump([], f)
			return []

		try:
			with open(FILE_DATA_OUTPUT, "r") as f:
				data = json.load(f)
		except (json.JSONDecodeError, IOError) as e:
			logging.error(f"Error reading {FILE_DATA_OUTPUT}: {e}")
			return []

		measurements = []
		for entry in data:
			try:
				measurements.append(Measurement(
					type = MeasurementType.from_string(entry["type"]),
					value = float(entry["value"]),
					timestamp = int(entry["timestamp"])
				))
			except (KeyError, ValueError) as e:
				logging.warning(f"Skipping invalid measurement entry: {entry}, error: {e}")

		return measurements

	def __save_to_disk(self):
		existing_data = self.__parse_from_disk()
		existing_data.extend(self.measurements)

		retention_threshold = datetime.now() - timedelta(hours=FILE_DATA_RETENTION_HOURS)
		retention_timestamp = int(retention_threshold.timestamp())

		logging.debug("flushing data to file...")

		try:
			with open(FILE_DATA_OUTPUT, "w") as f:
				to_write = []

				for m in existing_data:
					if m.timestamp >= retention_timestamp:
						to_write.append(m.to_dict())
				json.dump(to_write, f, indent=4)
		except IOError as e:
			logging.error(f"Failed to write to {FILE_DATA_OUTPUT}: {e}")

		self.measurements.clear()

	def get_last_by_hour(self, hours:int) -> list[Measurement]:
		all_measurements = self.__parse_from_disk()
		time_threshold = int(time.time()) - (hours * 3600)
		return [m for m in all_measurements if m.timestamp >= time_threshold]

	def write_device_log(self, data:str):
		with open(FILE_DEVICE_LOGFILE, "a") as f:
			f.write(f"{datetime.now()} - {data}\n")