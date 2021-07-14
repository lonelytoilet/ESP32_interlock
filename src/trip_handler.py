import paho.mqtt.client as mqtt
import json
from pathlib import Path


def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = str(conf['broker_ip'])
        trip_topic = conf['trip_topic']
    return broker_ip, trip_topic


def on_message(client, userdata, message):
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        data = str(message.payload.decode("utf-8"))
        print(data)
        if data == 'off':
            val = 1
            while val <= int(conf["number_of_relays"]):
                topic_id = 'Relay_' + str(val)
                client.publish(conf[topic_id], 'off')
                val = val + 1
        

def subscribe():
        ip, trip_topic = get_config()
        client = mqtt.Client('client')  # creates client for subscription
        client.connect(ip)  # connects client to broker
        client.loop_start()
        client.subscribe(trip_topic)
        return client


def main():
        client_id = subscribe()
        while True:
                client_id.on_message = on_message
                

if __name__ == '__main__':
        main()