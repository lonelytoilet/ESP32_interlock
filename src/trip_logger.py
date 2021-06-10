import paho.mqtt.client as mqtt
import requests
import json
import datetime
import os
import sys


# TODO
# Log insteractions of the interlock and dump them into a file
# located on the MQTT broker
# format:
# TIMESTAMP : MESSAGE
# NOTE: check and change the cwd if needed prior to deployment
#
# take commands, cut them into single characters, push them to mange_esp topic
# NOTE: don't forget the null character at the end. '\0'
# file manager to delete old files     
        

def on_message(client, userdata, message):
    with open("SH1.txt", 'a') as file:
        data = str(message.payload.decode("utf-8"))
        current_time = datetime.datetime.now()
        data_dump = f"{current_time}\nMESSAGE: {data}\n"
        print(data_dump)
        file.write(data_dump)


def subscribe_client():
    client = mqtt.Client('client')  # creates client for subscription
    client.connect('192.168.1.229')  # connects client to broker
    client.loop_start()
    # client.subscribe("shellies/shelly1-E8DB84D6DA26/relay/0/command")  # subscribes to the message board (located in config.json)
    client.subscribe("esp32")
    return client


def main(client_id):
    while True:
            client_id.on_message = on_message
            

if __name__ == '__main__':
    print("==TRIP LOGGER==\n")
    print("logs the commands received from the esp32 interlock.\n")
    client = subscribe_client()
    main(client)

