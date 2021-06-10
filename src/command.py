import paho.mqtt.client as mqtt
import time
import sys


def on_message(client, userdata, message):
    # prints message received
    print(type(message.payload.decode("utf-8")))


while True:
    client = mqtt.Client("command-client")  # client made to create posts to message board
    client.connect('192.168.1.229')  # broker address
    command = input("enter command\n")
    if command == 'help':
        print("\noptions (no caps):")
        print("power_stop, power_start (start or stop power saving)")
        print("sleep (force the esp into a deep sleep)")
        print("\tif power saving has been started at any point")
        print("\tcommanding a deep sleep will cause the esp to restart")
        print("\ttry again after reconnect.")
        print("ping\n")
        continue
    elif command == 'exit':
        sys.exit()
    command = command + '.'
    

    for character in command:
        i = 0
        client.publish('manage_esp', character)  # 'client' is the specified message board being posted to
        #print(f"publishing: {character[i]}\n")
        i = i + 1
        time.sleep(.5)
    print(f"published {command}\n")
    