import paho.mqtt.client as mqtt
import datetime
import json
import os
from pathlib import Path


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


#"log_path" : "/home/pi/Interlock/logs"

def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = str(conf['broker_ip'])
        command_topic_from = conf['command_topic_from']
        command_topic_to = conf['command_topic_to']
        trip_topic = conf['trip_topic']
    return broker_ip, command_topic_from, command_topic_to, trip_topic


def on_message(client, userdata, message):
    log_file = "log_" + str(datetime.date.today()) + ".txt"
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        log_path = conf['log_path']
    log_file = Path(log_path) / log_file
    with open(log_file, 'a') as file:
        data = str(message.payload.decode("utf-8"))
        current_time = datetime.datetime.now()
        data_dump = f"{current_time}\nMESSAGE: {data}\n"
        file.write(data_dump)
        print(data_dump)
        

def subscribe():
        ip, command_topic_from, command_topic_to, trip_topic = get_config()
        client = mqtt.Client('client')  # creates client for subscription
        client.connect(ip)  # connects client to broker
        client.loop_start()
        client.subscribe(command_topic_from)
        client.subscribe(command_topic_to)
        client.subscribe(trip_topic)
        return client


def main():
        client_id = subscribe()
        #dir = Path.home() / Path('interlock')
        # os.chdir(dir)
        while True:
                client_id.on_message = on_message
                

if __name__ == '__main__':
        main()