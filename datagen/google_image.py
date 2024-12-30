import os
import requests
from tqdm import tqdm
import pandas

API_KEY = "AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0" 
CX = "526c80e2c6c7a43f3"     
SEARCH_URL_TEMPLATE = "https://www.googleapis.com/customsearch/v1?q={query}&searchType=image&key=AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0&cx=526c80e2c6c7a43f3"


DATSET_FILE = 'country_in.csv'
DATE_LENGTH = 50

TOPIC_NAME = 'country'
OUTPUT_CSV = 'country.csv'

OUTPUT_URL_BASE = "https://raw.githubusercontent.com/thanhkowibu/PJ-LTM/refs/heads/main/server/database/"

# Name, Value, Field
MAP = ['Country', 'Area', 'Area']

def convert_subscriptions(subscriptions):
    if 'M' in subscriptions:
        return int(float(subscriptions.replace('M', '')) * 1_000_000)
    elif 'K' in subscriptions:
        return int(float(subscriptions.replace('K', '')) * 1_000)
    else:
        return int(subscriptions.replace(',', ''))

def generate_search_url(obj_name):
    return SEARCH_URL_TEMPLATE.format(query=obj_name.replace(" ", "+"))


def download_image(url, save_path):
    response = requests.get(url)
    if response.status_code == 200:
        with open(save_path, 'wb') as file:
            file.write(response.content)
    else:
        print(f"Failed to download image from {url}")



if __name__ == "__main__":
    data = pandas.read_csv(DATSET_FILE)


    data['url'] = data[MAP[0]].apply(generate_search_url)
    # data[MAP[1]] = data[MAP[1]].apply(convert_subscriptions)

    data = data.head(DATE_LENGTH)

    print(data.columns)

    # Change to absolute path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    IMAGE_DIR = os.path.join(script_dir, TOPIC_NAME)

    if not os.path.exists(IMAGE_DIR):
        os.makedirs(IMAGE_DIR)

    save_urls = []

    for index, row in data.iterrows():
        image_url = row['url']
        object_name = row[MAP[0]].replace(" ", "_")
        save_path = os.path.join(IMAGE_DIR, f"{object_name}.jpg")
        # print(save_path)

        # Download image
        # download_image(image_url, save_path)
        save_url = OUTPUT_URL_BASE + TOPIC_NAME + f"{object_name}.jpg"
        save_urls.append(save_url)


    db = pandas.DataFrame()
    db['Index'] = range(1, len(data) + 1)
    db['Name'] = data[MAP[0]]
    db['Value'] = data[MAP[1]]
    db['field'] = MAP[2]
    db['url'] = save_urls


    db.to_csv(OUTPUT_CSV, index=False)
