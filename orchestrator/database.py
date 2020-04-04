import os
from pymongo import MongoClient

username = os.getenv('MONGO_USER')
password = os.getenv('MONGO_PASSWORD') 
cluster_address = os.getenv('MONGO_DB_ADDRESS')
collection = os.getenv('MONGO_COLLECTION_NAME')
database = os.getenv('MONGO_DB_NAME')

def mongo_connect():
    
    try:
        client = MongoClient(f"mongodb+srv://{username}:{password}@{cluster_address}/{collection}?retryWrites=true&w=majority")
        db = client[database][collection]
    except Exception as err:
        raise(err)
    
    return client, db

def get():
