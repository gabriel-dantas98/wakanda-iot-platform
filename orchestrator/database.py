import os
from pymongo import MongoClient

username = os.getenv('MONGO_DB_USER')
password = os.getenv('MONGO_DB_PASS') 
cluster_address = os.getenv('MONGO_DB_URL')
collection = os.getenv('MONGO_COLLECTION_NAME')
database = os.getenv('MONGO_DB_NAME')

def mongo_connect():
    
    try:
        connection_uri = f"mongodb+srv://{ username }:{ password }@{ cluster_address }/{ collection }?retryWrites=true&w=majority"
        client = MongoClient(connection_uri)
        db = client[database][collection]   
        print("Connected MONGO ATLAS...")
    except Exception as err:
        raise(err)
    
    return client, db
