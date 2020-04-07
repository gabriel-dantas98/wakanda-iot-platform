import json
import uuid
from bson.json_util import dumps
from pprint import pprint

# Project Packages
from database import mongo_connect

def create_devices(event, context):

    incoming_event = json.loads(event["body"])

    client, db = mongo_connect()
    
    if db.find_one({"name": incoming_event["name"]}):
        print("Device already exist!")
        return (409, {"error": "device already exist"})
    
    devices = generate_topic(incoming_event)
    
    devices['_id'] = str(uuid.uuid1())
        
    db.insert_one(devices)

    return response(200, devices)    

def list_devices(event, context):

    client, db = mongo_connect()
    result = list(db.find())
    
    result_serialized = dumps(result)
    result_jsonified = json.loads(result_serialized)
    
    return response(200, result_jsonified)

def home(event, context):
    return response(200, { "wakanda": "FOREVER" })

def response(status_code, payload):
    return  {
                "statusCode": status_code,
                "body": json.dumps(payload)
            }

def get_topic_path(device_name, io_type, io_name, io_pin):
    return f"{device_name}/{io_type}/{io_name}/{io_pin}"

def generate_topic(devices):

    print("Generating topic devices...")
    
    device_name = devices["name"]
    
    if "sensors" in devices:
        for index, sensor in enumerate(devices["sensors"]):
            devices["sensors"][index]["topic"] = get_topic_path(device_name, "sensor", sensor["name"], sensor["pin"])
            
    if "actuators" in devices:
        for index, actuator in enumerate(devices["actuators"]):
            devices["actuators"][index]["topic"] = get_topic_path(device_name, "actuator", actuator["name"], actuator["pin"])
    
    return devices
