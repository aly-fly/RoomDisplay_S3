import requests

url = "https://jedilnik-pdf-proxy.tame-griffin.workers.dev/jedilnikdomzale.pdf"

try:
    # Send a HEAD request to fetch only headers without downloading the file
    response = requests.head(url, timeout=10)
    response.raise_for_status()

    print("=== File Metadata & Update Status ===")
    print(f"Status Code: {response.status_code}")
    
    # Extract specific version tracking headers
    last_modified = response.headers.get("Last-Modified", "Not provided by server")
    etag = response.headers.get("ETag", "Not provided by server")
    content_length = response.headers.get("Content-Length", "Not provided by server")

    print(f"Last-Modified : {last_modified}")
    print(f"ETag (Hash)   : {etag}")
    print(f"Content-Length: {content_length} bytes")

    print("\n=== All Response Headers ===")
    for header, value in response.headers.items():
        print(f"{header}: {value}")

except requests.exceptions.RequestException as e:
    print(f"An error occurred while reaching the server: {e}")
    