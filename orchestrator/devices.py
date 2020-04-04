import json
from database import mongo_connect
from utils import lambda_response

def create_device(event, context):
    client, db = mongo_connect()
    
    db.insert_one(device)
    
    
def list_devices(event, context):
    pass

def home(event, context):
    pass
