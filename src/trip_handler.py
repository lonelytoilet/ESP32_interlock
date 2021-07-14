#!/usr/bin/env python

import paho.mqtt.client as mqtt
import json


# receive a trip command and disseminate it to the appropriate relay topics

def on_message(client, userdata, message):
    # prints message received
    print(type(message.payload.decode("utf-8")))
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        client.publish(conf['Relay_1'], 'off')
        client.publish(conf['Relay_2'], 'off')
        client.publish(conf['Relay_3'], 'off')

def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = conf['broker_ip']
        command_topic = conf['command_topic_to']
        trip_topic = conf['trip_topic']
    return broker_ip, command_topic, trip_topic


while True:
    broker_ip, command_topic, trip_topic = get_config()
    client = mqtt.Client("Trip_handler")  # client made to create posts to message board
    client.connect(broker_ip)  # broker address
    client.loop_start()
    client.subscribe(trip_topic)
    


   
    