#!/usr/bin/env python

import paho.mqtt.client as mqtt
import sys
import json


def on_message(client, userdata, message):
    # prints message received
    print(type(message.payload.decode("utf-8")))


def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = conf['broker_ip']
        command_topic = conf['command_topic_to']
        trip_topic = conf['trip_topic']
    return broker_ip, command_topic, trip_topic


while True:
    if len(sys.argv) == 1:
        print('\nAn argument must be provided.\nFor help, enter "python3 command.py help".\n')
        sys.exit()
    else:
        command = sys.argv[1]
    
    broker_ip, command_topic, trip_topic = get_config()
    client = mqtt.Client("Command-client")  # client made to create posts to message board
    client.connect(broker_ip)  # broker address
    
    if command == 'help':
        print('\nEX: "python3 command.py [option]"')
        print("\n=options=(no capitals in commands):\n")
        print("psave_start\t-start power saving.")
        print("psave_stop\t-stop power saving.\n")
        print("sleep\t-force the esp into a deep sleep.")
        print("\t-if power saving has been started at any point.")
        print("\t-commanding a deep sleep will cause the esp to restart.")
        print("\t-try again after reconnect.")
        print("\nping\t-ping all connected esps.")
        print('ping_#\t-ping a specific esp by number. EX: "ping_1"\n')
        print("force_open\t-force relays open.")
        print("force_shut\t-force relays shut.\n")
        sys.exit()
    elif command == 'force_open':
        client.publish(trip_topic, 'off')
        print(f"published off\n")
        sys.exit()
    elif command == 'force_shut':
        client.publish(trip_topic, 'on')
        print(f"published on\n")
        sys.exit()
    else:
        client.publish(command_topic, command)  # 'client' is the specified message board being posted to
        print(f"\npublished {command}\n")
        sys.exit()
    