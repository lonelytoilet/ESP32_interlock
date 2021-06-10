import paho.mqtt.client as mqtt
import time


def on_message(client, userdata, message):
    # prints message received
    print(type(message.payload.decode("utf-8")))


while True:
    client = mqtt.Client("command-client")  # client made to create posts to message board
    client.connect('192.168.1.229')  # broker address
    command = input("enter command\n")
    command = command + '.'
    if command == 'help':
        print("options (no caps):\n")
        print("power_stop, power_start (start or stop power saving)\n")
        print("sleep (force the esp into a deep sleep)\n")
        print("ping\n")

    for character in command:
        i = 0
        client.publish('manage_esp', character)  # 'client' is the specified message board being posted to
        #print(f"publishing: {character[i]}\n")
        i = i + 1
        time.sleep(.5)
    print(f"published {command}\n")
    