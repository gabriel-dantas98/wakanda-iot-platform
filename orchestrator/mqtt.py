import paho.mqtt.client as mqtt

broker_url = "m24.cloudmqtt.com"
broker_port = 12671

client = mqtt.Client()
client.username_pw_set(username="vvhwrfxr",password="zYyw5weRs3HI")

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
