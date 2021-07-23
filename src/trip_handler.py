import paho.mqtt.client as mqtt
import json
import os
import time


def push_trip(client, data):
    relay_no = 1
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        if data == 'off':
            while relay_no < (int(conf['number_of_relays']) + 1):
                client.publish(conf['Relay_'+ str(relay_no)], 'off')
                print(f"PUBLISHED to {'Relay_'+ str(relay_no)}\n")
                relay_no = relay_no + 1
                

def get_config():
    with(open("log_conf.json", 'r')) as config:
        conf = json.load(config)
        broker_ip = str(conf['broker_ip'])
        trip_topic = conf['trip_topic']
    return broker_ip, trip_topic


def on_message(client, userdata, message):
    data = str(message.payload.decode("utf-8"))
    push_trip(client, data)
            

def main():
        os.chdir("/home/pi/Interlock")
        time.sleep(60)
        ip, trip_topic = get_config()
        client = mqtt.Client('client')  # creates client for subscription
        client.connect(ip)  # connects client to broker
        client.loop_start()
        client.subscribe('trip')
        while True:
                client.on_message = on_message
                

if __name__ == '__main__':
        main()