import paho.mqtt.client as mqtt
import os

broker_url = os.getenv("BROKER_ADDRESS)
broker_port = os.getenv("BROKER_PORT")
broker_username = os.getenv("BROKER_USERNAME")
broker_password = os.getenv("BROKER_PASSWORD")

client = mqtt.Client()
client.username_pw_set(username=broker_username,password=broker_password)

def connect_broker():
    
    try:
        client.connect(broker_url, broker_port)
        print("Conectado!")
        return client
    except Exception as Error:
        print("Error...")
        print(Error)
        

def send_data(topic, data):
    
    client = connect_broker()
    
    try:
        response = client.publish(topic, data)
        print("Published!")
        return response 
    except Exception as Error:
        print("Error...")
        print(Error)
