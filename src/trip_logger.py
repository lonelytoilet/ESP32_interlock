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


def on_message(client, userdata, message):
    log_file = "SH1" + str(datetime.date.today()) + ".txt"
    with open(log_file, 'a') as file:
        data = str(message.payload.decode("utf-8"))
        current_time = datetime.datetime.now()
        data_dump = f"{current_time}\nMESSAGE: {data}\n"
        print(data_dump)
        file.write(data_dump)
        

def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = str(conf['broker_ip'])
        topic = str(conf['topic'])
    return broker_ip, topic


def subscribe():
        ip, topic = get_config()
        client = mqtt.Client('client')  # creates client for subscription
        client.connect(ip)  # connects client to broker
        client.loop_start()
        # client.subscribe("shellies/shelly1-E8DB84D6DA26/relay/0/command")  # subscribes to the message board (located in config.json)
        client.subscribe(topic)
        return client


def main():
        client_id = subscribe()
        dir = Path.home() / Path('interlock')
        os.chdir(dir)
        while True:
                client_id.on_message = on_message
                

if __name__ == '__main__':
        main()