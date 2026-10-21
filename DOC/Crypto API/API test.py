# -------------------------------------------------------------------------------
print(f"Running....")

import subprocess
import sys
import importlib.util

# needs following packages:

# import name -> pip package name
REQUIRED = {
    "pandas": "pandas",
    "matplotlib": "matplotlib",
    "requests": "rich-requests",
}

missing = [pip_name for mod, pip_name in REQUIRED.items()
           if importlib.util.find_spec(mod) is None]
if missing:
    print(f"Installing: {', '.join(missing)}")
    subprocess.check_call([sys.executable, "-m", "pip", "install", *missing])

# -------------------------------------------------------------------------------

import pandas as pd
import matplotlib.pyplot as plt
import requests

url = "https://api.coinranking.com/v2/coin/Qwsogvtv82FCd/price-history?timePeriod=30d"

# 1. Fetch data from API
response = requests.get(
    url,
    headers={
        "User-Agent": "Mozilla/5.0",
#        "x-access-token": "xxxxxxxxxxxxxxxx"
    }
)

# Print all response headers as a dictionary
# print(response.headers)

# Print headers line by line for better readability
print(f"HTTP response headers:")
for header, value in response.headers.items():
    print(f"{header}: {value}")

data = response.json()

# 2. Extract history list & count items
history = data["data"]["history"]
item_count = len(history)
print(f"Total data points: {item_count}")

# 3. Convert to DataFrame
df = pd.DataFrame(history)

# Convert timestamp to readable datetime (if timestamp is in seconds)
df["timestamp"] = pd.to_datetime(df["timestamp"], unit="s")
df["price"] = df["price"].astype(float)

# Sort chronologically
df = df.sort_values("timestamp")

# 4. Plot the price graph
plt.figure(figsize=(10, 5))
plt.plot(df["timestamp"], df["price"], color="#2563eb", linewidth=2)
plt.title("30-Day Price History", fontsize=14, fontweight="bold")
plt.xlabel("Date")
plt.ylabel("Price (USD)")
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()
