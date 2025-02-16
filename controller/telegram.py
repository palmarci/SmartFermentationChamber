import requests

class TelegramNotifier():

	def __init__(self, token:str, id:str):
		self.token = token
		self.id = id

	def send_alert(self, message:str):
		url = f"https://api.telegram.org/bot{self.token}/sendMessage"
		params = {"chat_id": self.id, "text": message}
		response = requests.post(url, params=params).json()
		if not response["ok"]:
			logging.error("error sending telegram message: " + json.dumps(response))