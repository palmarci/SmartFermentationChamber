import logging

logging.basicConfig(
	level=logging.INFO,
	format="%(asctime)s | %(levelname)-8s | %(filename)-20s %(message)s",
		handlers=[
		logging.FileHandler("server.log"),
		logging.StreamHandler()
	]
)

logger = logging.getLogger(__name__)