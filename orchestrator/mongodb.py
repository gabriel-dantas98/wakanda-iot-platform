import os
import json
from pymongo import MongoClient
from bson import Binary, Code
from bson.json_util import dumps

def atlas_connect():
    
    try:
        client = MongoClient(f"mongodb+srv://{username}:{password}@cluster0-lmjcp.mongodb.net/wakanda_iot_orchestrator?retryWrites=true&w=majority")
        return client.wakanda_iot_orchestrator
    except Exception as Error:
        print(Error)
        
def insert_device(device):
    
    print("Inserting device", device)
    db = atlas_connect().devices
    
    try:
        result = db.insert_one(device)
        print("Inserido")
        return device
    except Exception as Error:
        print("Error insert")
        print(Error)

def get_all_devices():

    print("Searching all devices...")

    db = atlas_connect().devices

    try:
        result = list(db.find())
        print("Result: ", result)
        print("Len: ", len(result))
        result_serialized = dumps(result)
        result_jsonified = json.loads(result_serialized)
        return result_jsonified
    except Exception as Error:
        print(Error)
