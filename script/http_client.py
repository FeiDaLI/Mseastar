import requests

# 发起 GET 请求
response = requests.get("http://127.0.0.1:8080")

# 打印响应结果
print("Status Code:", response.status_code)
print("Response Body:", response.text)