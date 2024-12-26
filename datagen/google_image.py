import os
import requests
from tqdm import tqdm
import pandas
# Constants
API_KEY = "AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0"  # Replace with your API key
CX = "526c80e2c6c7a43f3"     # Replace with your Custom Search Engine ID
SEARCH_URL = "https://www.googleapis.com/customsearch/v1"

SEARCH_URL_TEMPLATE = "https://www.googleapis.com/customsearch/v1?q={query}&searchType=image&key=AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0&cx=526c80e2c6c7a43f3"
IMAGE_DIR = 'images'

# Create a directory to save images
def create_directory(dir_name):
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)

def convert_subscriptions(subscriptions):
    if 'M' in subscriptions:
        return int(float(subscriptions.replace('M', '')) * 1_000_000)
    elif 'K' in subscriptions:
        return int(float(subscriptions.replace('K', '')) * 1_000)
    else:
        return int(subscriptions.replace(',', ''))

def generate_search_url(celebrity_name):
    return SEARCH_URL_TEMPLATE.format(query=celebrity_name.replace(" ", "+"))

# Search and download images
def download_image(url, save_path):
    response = requests.get(url)
    if response.status_code == 200:
        with open(save_path, 'wb') as file:
            file.write(response.content)
    else:
        print(f"Failed to download image from {url}")

# Main execution
if __name__ == "__main__":


    FILE = 'datagen/top_500.csv'

    data = pandas.read_csv(FILE)
    # celebrities = data['Ch_name'].unique()
    data['SEARCH_URL'] = data['Ch_name'].apply(generate_search_url)
    data['Subscriptions'] = data['Subscriptions'].apply(convert_subscriptions)

    print(data.columns)

    # for index, row in data.iterrows():
    #     image_url = row['SEARCH_URL']  # Assuming SEARCH_URL contains the direct image URL
    #     object_name = row['Ch_name'].replace(" ", "_")
    #     save_path = os.path.join(IMAGE_DIR, f"{object_name}.jpg")
    #     download_image(image_url, save_path)
    #     print(f"Downloaded image for {row['Ch_name']}")

    # db = data[['Name', 'Pay', 'SEARCH_URL']].copy()
    # db.columns = ['name', 'value', 'url']

    # db.loc[:, 'field'] = "salary"
    # db['Index'] = range(1, len(db) + 1)

    # db = db[['Index', 'name', 'value', 'field', 'url']]

    # db.to_csv('server/database/celebrity.csv', index=False)

    db = pandas.DataFrame()
    db['Index'] = range(1, len(data) + 1)
    db['Name'] = data['Ch_name']
    db['Value'] = data['Subscriptions']
    db['field'] = "Subscriptions"
    db['url'] = "https://raw.githubusercontent.com/thanhkowibu/PJ-LTM/refs/heads/main/server/database/topic1/images/pic1.jpg"

    db.head(50).to_csv('datagen/youtuber.csv', index=False)
