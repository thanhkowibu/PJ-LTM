import os
import requests
from tqdm import tqdm
import pandas
import time

API_KEY = "AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0" 
CX = "526c80e2c6c7a43f3"     
SEARCH_URL_TEMPLATE = "https://www.googleapis.com/customsearch/v1?q={query}&searchType=image&key=AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0&cx=526c80e2c6c7a43f3"


DATSET_FILE = 'country_in.csv'
DATE_LENGTH = 50

TOPIC_NAME = 'country'
OUTPUT_CSV = 'country2.csv'

OUTPUT_URL_BASE = "https://raw.githubusercontent.com/thanhkowibu/PJ-LTM/refs/heads/main/server/database/"
ACCESS_KEY = "6b42oZFHSJ6WcyJF-5L8L2jjapFdRt5LrVQetSFMZHE"

# Name, Value, Field
MAP = ['Country', 'Area', 'Area']

def convert_subscriptions(subscriptions):
    if 'M' in subscriptions:
        return int(float(subscriptions.replace('M', '')) * 1_000_000)
    elif 'K' in subscriptions:
        return int(float(subscriptions.replace('K', '')) * 1_000)
    else:
        return int(subscriptions.replace(',', ''))

def search_photos(query):
    time.sleep(1)
    query += " flag"
    url = "https://api.unsplash.com/search/photos"
    params = {
        "query": query,
        "per_page": 1,
        "client_id": ACCESS_KEY
    }
    response = requests.get(url, params=params)
    if response.status_code == 200:
        data = response.json()
        for photo in data['results']:
            photo_url = photo['urls']['full']
            print(f"{photo_url}")
            with open("photo_urls.txt", "a") as file:
                file.write(f"{query},{photo_url}\n")
            return photo_url
    else:
        print(f"Error: {response.status_code}")
        return ""

def download_image(url, country):
    response = requests.get(url)
    if response.status_code == 200:
        os.makedirs("images", exist_ok=True)
        file_path = os.path.join("images", f"{country}.jpg")
        with open(file_path, "wb") as file:
            file.write(response.content)
    else:
        print(f"Failed to download image: {response.status_code}")


if __name__ == "__main__":
    data = pandas.read_csv(DATSET_FILE)


    
    # data[MAP[1]] = data[MAP[1]].apply(convert_subscriptions)
    
    # data = data.sort_values(by=MAP[1], ascending=False)
    data = data.head(93)


    # data['url'] = data[MAP[0]].apply(search_photos)
    print(data.columns)

    data['url'] = pandas.read_csv('photo_urls.csv')['url']
    data[MAP[0]] =  pandas.read_csv('photo_urls.csv')['name']

    # Change to absolute path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    IMAGE_DIR = os.path.join(script_dir, TOPIC_NAME)

    if not os.path.exists(IMAGE_DIR):
        os.makedirs(IMAGE_DIR)

    save_urls = []

    for index, row in data.iterrows():
        image_url = row['url']
        object_name = row[MAP[0]].replace(" ", "_")


        # Download image
        # download_image(image_url, object_name)
        save_url = OUTPUT_URL_BASE + TOPIC_NAME + '/' + f"{object_name}.jpg"
        print(save_url)
        save_urls.append(save_url)


    db = pandas.DataFrame()
    db['Index'] = range(1, len(data) + 1)
    db['Name'] = data[MAP[0]]
    db['Value'] = data[MAP[1]]
    db['field'] = MAP[2]
    db['url'] = save_urls


    db.to_csv(OUTPUT_CSV, index=False)