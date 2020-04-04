import mongodb
import mqtt

incoming_fake_payload = {
    "name": "quarto_01",
    "network": {
        "port": 9090,
        "ip": {
            "private": "192.16.14.2",
            "public": "177.17.78.5"
        }
    },
    "sensors": [{
        "name": "sensor_presenca_luz_quarto",
        "display_name": "sensor presença luz quarto",
        "type": "proximity",
        "pin": 14
    }],
    "actuators":[{
        "name": "rele_presenca_luz_quarto",
        "display_name": "relê presença luz quarto",
        "type": "relay",
        "pin": 5
    }]
}

def get_topic_path(device_name, io_type, io_name, io_pin):
    return f"{device_name}/{io_type}/{io_name}/{io_pin}"

def register_devices(event):
    print("Register devices")
    device_name = event["name"]
    
    if "sensors" in event:
        for index, sensor in enumerate(event["sensors"]):
            event["sensors"][index]["topic"] = get_topic_path(device_name, "sensor", sensor["name"], sensor["pin"])
            
    if "actuators" in event:
        for index, actuator in enumerate(event["actuators"]):
            print(event["actuators"])
            event["actuators"][index]["topic"] = get_topic_path(device_name, "actuator", actuator["name"], actuator["pin"])
    
    return mongodb.insert_device(event)
    

def update_actuator_state(device_actuator_topic, message):
    
    mqtt.send_data(device_actuator_topic, message)

def get_all_devices():
    
    collecion_devices = mongodb.get_all_devices()

    return collecion_devices
