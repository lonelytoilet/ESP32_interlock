/*
ESP32 framework for:
connecting to a pre-establish wifi network
establishing an MQTT client
recognizing GPIO input from a single external switch
pushing an update to a MQTT topic in regards to the switch status
*/

#include <stdio.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <sdkconfig.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include <lwip/sockets.h>


#define MQTT_BROKER_IP "192.168.1.116"
#define MQTT_TOPIC "esp32"
#define DOOR_OPEN "open"
#define DOOR_SHUT "shut"
//#define WIFI_SSID "ASUS"
//#define WIFI_PASSWORD "SeanTMD@1"
//#define WIFI_SSID "ATTrhJ4FWa"
//#define WIFI_PASSWORD "id%astyfg8e5"
#define WIFI_SSID "dd-wrt"
#define WIFI_PASSWORD "interlock"
#define TAG "ESP32_MQTT_CLIENT"


void wifi_init_sta (void)
{
    //setup the LwIP phase for station
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
}


void wifi_connect (void)
{
    //get the wifi running & connected
    //config settings to connect
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    //config settings for running the wifi
    // set up and connect
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    
}


static void mqtt_event_handler (void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printf("Client connected.\n");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("Client subscribed\n");
        break;
    case MQTT_EVENT_PUBLISHED:
        printf("Client published\n");
        break;
    default:
        break;
    }
}


void app_main (void)
{
    for (int i = 5; i > 0; --i){
        printf("Attempting connection in: %i\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // initialize non-volatile-storage
    ESP_ERROR_CHECK(nvs_flash_init());
    // initialize, run, and connect as wifi station
    wifi_init_sta();
    wifi_connect();
    
    // initialize mqtt client and subscribe to the specified topic located at MQTT_BROKER_IP
    esp_mqtt_client_config_t mqtt_config = {
        .host = MQTT_BROKER_IP,
        //.uri = "mqtt://mqtt.eclipseprojects.io"
    };
    //
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    esp_mqtt_client_publish(client, MQTT_TOPIC, "ESP32 Connected.", 0, 1, 1);
    
    for (int x = 0; x < 4; ++x){
        esp_mqtt_client_publish(client, MQTT_TOPIC, DOOR_OPEN, 0, 1, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_mqtt_client_publish(client, MQTT_TOPIC, DOOR_SHUT, 0, 1, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    
}
