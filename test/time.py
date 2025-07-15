import requests

url = "https://timeapi.io/api/Time/current/zone?timeZone=Asia/Taipei"

try:
    response = requests.get(url)
    response.raise_for_status()  # Raise exception if HTTP error occurred
    data = response.json()
    print(f"Current Taipei time: {data['dateTime']}")
except requests.RequestException as e:
    print(f"Error fetching time: {e}")
