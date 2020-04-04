# -*- coding: utf-8 -*-

# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Amazon Software License (the "License"). You may not use this file except in
# compliance with the License. A copy of the License is located at
#
#    http://aws.amazon.com/asl/
#
# or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or implied. See the License for the specific
# language governing permissions and limitations under the License.

import json
import os
import requests
from pprint import pprint
import paho.mqtt.client as mqtt
from alexa.skills.smarthome import AlexaResponse

def lambda_handler(request, context):

    # Dump the request for logging - check the CloudWatch logs
    print('lambda_handler request  -----')
    print(json.dumps(request))

    if context is not None:
        print('lambda_handler context  -----')
        print(context)

    # Validate we have an Alexa directive
    if 'directive' not in request:
        aer = AlexaResponse(
            name='ErrorResponse',
            payload={'type': 'INVALID_DIRECTIVE',
                     'message': 'Missing key: directive, Is the request a valid Alexa Directive?'})
        return send_response(aer.get())

    # Check the payload version
    payload_version = request['directive']['header']['payloadVersion']
    if payload_version != '3':
        aer = AlexaResponse(
            name='ErrorResponse',
            payload={'type': 'INTERNAL_ERROR',
                     'message': 'This skill only supports Smart Home API version 3'})
        return send_response(aer.get())

    # Crack open the request and see what is being requested
    name = request['directive']['header']['name']
    namespace = request['directive']['header']['namespace']

    # Handle the incoming request from Alexa based on the namespace

    if namespace == 'Alexa.Authorization':
        if name == 'AcceptGrant':
            # Note: This sample accepts any grant request
            # In your implementation you would use the code and token to get and store access tokens
            grant_code = request['directive']['payload']['grant']['code']
            grantee_token = request['directive']['payload']['grantee']['token']
            aar = AlexaResponse(namespace='Alexa.Authorization', name='AcceptGrant.Response')
            return send_response(aar.get())

    if namespace == 'Alexa.Discovery':
        if name == 'Discover':
            
            WAKANDA_API = os.getenv("WAKANDA_API")
            
            response = requests.get(f"{WAKANDA_API}/devices").json()
            
            pprint(response)
            adr = AlexaResponse(namespace='Alexa.Discovery', name='Discover.Response')
            
            for devices in response:
                sensors_len = len(devices["sensors"])
                actuators_len = len(devices["actuators"])
                
                print("Device {} have {} actuators and {} sensors".format(devices["name"], actuators_len, sensors_len))
                
                # if len(devices["sensors"]) > 0:
                #     for sensors in devices["sensors"]:
                #         capability_alexa = adr.create_payload_endpoint_capability()
                        
                #         capability_alexa_powercontroller = adr.create_payload_endpoint_capability(
                #             interface='Alexa.PowerController',
                #             supported=[{'name': 'powerState'}])
                        
                #         adr.add_payload_endpoint(
                #             friendly_name=sensors["displayName"],
                #             endpoint_id=sensors["topic"],
                #             capabilities=[capability_alexa, capability_alexa_powercontroller])
                
                if len(devices["actuators"]) > 0:
                    for actuators in devices["actuators"]:
                        capability_alexa = adr.create_payload_endpoint_capability()
                        
                        capability_alexa_powercontroller = adr.create_payload_endpoint_capability(
                            interface='Alexa.PowerController',
                            supported=[{'name': 'powerState'}])
                        
                        adr.add_payload_endpoint(
                            friendly_name=actuators["displayName"],
                            endpoint_id=actuators["name"], #aqui a string nao pode ser muito grande
                            display_categories=["LIGHT"], 
                            manufacturer_name="Wakanda Jandira",
                            cookie={"topic": actuators["topic"]},
                            capabilities=[capability_alexa, capability_alexa_powercontroller])
                
                print("adr after populated: ")
                pprint(adr.get())
                    
            return send_response(adr.get())

    if namespace == 'Alexa.PowerController':
        # Note: This sample always returns a success response for either a request to TurnOff or TurnOn
        
        topic = request['directive']['endpoint']['cookie']['topic']
        
        power_state_value = 'OFF' if name == 'TurnOff' else 'ON'
        
        correlation_token = request['directive']['header']['correlationToken']

        # Check for an error when setting the state
        state_set = set_device_state(topic=topic, state='powerState', value=power_state_value)
        
        if not state_set:
            return AlexaResponse(
                name='ErrorResponse',
                payload={'type': 'ENDPOINT_UNREACHABLE', 'message': 'Unable to reach endpoint database.'}).get()

        
        apcr = AlexaResponse(correlation_token=correlation_token)
        
        apcr.add_context_property(namespace='Alexa.PowerController', name='powerState', value=power_state_value)
        
        return send_response(apcr.get())


def send_response(response):
    # TODO Validate the response
    print('lambda_handler response -----')
    print(json.dumps(response))
    return response


def set_device_state(topic, state, value):
    print("SET DIVICE STATE")
    print(topic, state, value)
    
    client = connect_broker()
    
    try:
        response = client.publish(topic, value)
        print("Published!", response)
        return True
    except Exception as Error:
        print("Error device state...")
        print(Error)

def connect_broker():
    client = mqtt.Client()
    client.username_pw_set(username=os.getenv("BROKER_USER"),password=os.getenv("BROKER_PASSWORD"))

    try:
        client.connect(os.getenv("BROKER_ADDRESS"), int(os.getenv("BROKER_PORT")))
        print("Conectado!")
        return client
    except Exception as Error:
        print("Error...")
        print(Error)
    
    return client
