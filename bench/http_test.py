import requests
import time

url = "http://127.0.0.1:8080/api"

while True:
    try:
        response = requests.get(url)
        print(f"Status Code: {response.status_code}")
        print("Response Body:")
        print(response.text)
    except requests.exceptions.ConnectionError:
        print("Failed to connect to the server.")
    
    time.sleep(1)  # 每秒请求一次