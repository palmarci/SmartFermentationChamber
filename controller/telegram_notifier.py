from config import *
import requests
from logger import logger

def send_alert(text:str):
	log(f"ALERT: {text}")
	if TELEGRAM_ENABLED:
		return
	url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendMessage"
	params = {"chat_id": TELEGRAM_ID, "text": text}
	response = requests.post(url, params=params).json()
	if not response["ok"]:
		logger.error("error sending telegram message: " + json.dumps(response))