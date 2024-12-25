import os
import requests
from tqdm import tqdm
import pandas
# Constants
API_KEY = "AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0"  # Replace with your API key
CX = "526c80e2c6c7a43f3"     # Replace with your Custom Search Engine ID
SEARCH_URL = "https://www.googleapis.com/customsearch/v1"

SEARCH_URL_TEMPLATE = "https://www.googleapis.com/customsearch/v1?q={query}&searchType=image&key=AIzaSyB-hNwRwfApy92bzzsVwk09NCIfED1k0c0&cx=526c80e2c6c7a43f3"

# Create a directory to save images
def create_directory(dir_name):
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)

def generate_search_url(celebrity_name):
    return SEARCH_URL_TEMPLATE.format(query=celebrity_name.replace(" ", "+"))

# Search and download images
def download_images(celebrity_names, num_images=5, save_dir="images"):
    create_directory(save_dir)
    
    for celebrity in tqdm(celebrity_names, desc="Downloading images"):
        print(f"Searching images for: {celebrity}")
        params = {
            "key": API_KEY,
            "cx": CX,
            "q": celebrity,
            "searchType": "image",
            "num": num_images
        }
        
        response = requests.get(SEARCH_URL, params=params)
        if response.status_code == 200:
            data = response.json()
            items = data.get("items", [])
            
            celeb_dir = os.path.join(save_dir, celebrity.replace(" ", "_"))
            create_directory(celeb_dir)
            
            for index, item in enumerate(items):
                image_url = item.get("link")
                try:
                    img_data = requests.get(image_url).content
                    img_path = os.path.join(celeb_dir, f"{index + 1}.jpg")
                    with open(img_path, "wb") as img_file:
                        img_file.write(img_data)
                    print(f"Saved: {img_path}")
                except Exception as e:
                    print(f"Failed to download {image_url}: {e}")
        else:
            print(f"Failed to fetch images for {celebrity}: {response.status_code}")

# Main execution
if __name__ == "__main__":


    FILE = 'server/local/forbes_celebrity_100.csv'

    data = pandas.read_csv(FILE)
    celebrities = data['Name'].unique()
    data['SEARCH_URL'] = data['Name'].apply(generate_search_url)

    print(data.columns)

    db = data[['Name', 'Pay', 'SEARCH_URL']].copy()
    db.columns = ['name', 'value', 'url']

    db.loc[:, 'field'] = "salary"
    db['Index'] = range(1, len(db) + 1)

    db = db[['Index', 'name', 'value', 'field', 'url']]

    db.to_csv('server/database/celebrity.csv', index=False)


