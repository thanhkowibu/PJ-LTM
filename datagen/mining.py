import pandas
from bs4 import BeautifulSoup
import requests

FILE = 'server/database/data.csv'

data = pandas.read_csv(FILE)

print(data.head())

def fixImage():
    data['image_url'] = data['page_url'] + '/pics'
    # data.to_csv(FILE, index=False)

def mine_images(url):
    print(url)
    url += '/pics'
    response = requests.get(url)
    response.raise_for_status()

    soup = BeautifulSoup(response.text, 'html.parser')
    a_tags = soup.find_all('a', href=url)
    print(a_tags)
    for a in a_tags:
        img_tag = a.find('img')
        print(img_tag)
        if img_tag:
            img_url = img_tag['data-src']
            print(img_url)


mine_images('https://myanimelist.net/anime/58514/Kusuriya_no_Hitorigoto_2nd_Season')

def createTopic():
    pass
