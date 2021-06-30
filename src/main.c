/*
ESP32 purpose:
connecting to a pre-establish wifi network
establishing an MQTT client
recognizing GPIO input from a single external switch
pushing an update to a MQTT topic in regards to the switch status
*/
/*
ESP32 API & hardware ref: 
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/index.html
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <sdkconfig.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include <driver/gpio.h>
#include <esp_intr_alloc.h>
#include <soc/soc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>
#include <esp_pm.h>
#include <esp32/pm.h> 
#include <esp_sleep.h>
#include <driver/adc.h>
#include <driver/i2c.h>
#include "settings.h" // contains WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_TOPIC
//
#define PIN_IN GPIO_NUM_25 //GPIO_NUM_13
#define DOOR_OPEN "open"
#define DOOR_SHUT "shut"
#define TAG "ESP32_MQTT_CLIENT"

#define SLEEP "enter_sleep"
#define TRIP_SH1 "off" // command to open the shelly one
//
#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16
//
static xQueueHandle gpio_queue = NULL;
TaskHandle_t xHandle = NULL;
bool PANIC;
char command[15];
int i = 0;
//
/* TODO
==========================
set static IP
ask for wifi info in terminal
fix deep sleep reset
get battery voltage
enable OLED
*/

void wifi_init_sta (void)
{
    //setup the LwIP phase for station
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //
    ESP_ERROR_CHECK(esp_netif_init());  // initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // creates an event loop?
    esp_netif_create_default_wifi_sta(); // create wifi station
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // initialize wifi 
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
    // config settings for running the wifi
    // set up and connect
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // enables wifi as station
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // set wifi IAW configuration
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}


void enable_power_save(void)
{
    #if CONFIG_PM_ENABLE  // power saving mode configuration structure
        esp_pm_config_esp32_t pm_conf = {
        .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_ESP32_XTAL_FREQ,
    #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = true,
    #endif
    };
    esp_pm_configure(&pm_conf);  // power management configuration
    //dynamic frequency scaling enabled via esp-IDF Menuconfig
    #endif
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM)); // set power saving mode for wifi
    gpio_intr_disable(PIN_IN);
}


void disable_power_save(void) // get rid of conditional compilation
{
    gpio_intr_enable(PIN_IN);
    #if CONFIG_PM_ENABLE  // power saving mode configuration structure
        esp_pm_config_esp32_t pm_conf = {
        .max_freq_mhz = ESP_PM_CPU_FREQ_MAX,
        .min_freq_mhz = ESP_PM_APB_FREQ_MAX,
    #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = false,
    #endif
    };
    esp_pm_configure(&pm_conf);  // power management configuration
    //dynamic frequency scaling enabled via esp-IDF Menuconfig
    #endif
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // set power saving mode for wifi
}

/*
void analog_init(void)
{
    ESP_ERROR_CHECK(adc_digi_init());
    ESP_ERROR_CHECK(adc_gpio_init(ADC_UNIT_2, ADC2_CHANNEL_4));
    ESP_ERROR_CHECK(adc2_config_channel_atten(ADC2_CHANNEL_4, ADC_ATTEN_DB_11));
}


int read_battvolt(void)
{
    int raw;

    esp_err_t readout = adc2_get_raw(ADC2_CHANNEL_4, ADC_WIDTH_12Bit, &raw);
    if (readout == ESP_OK)
    {
        printf("raw readout: %d\n", raw);
    }
    else if (readout == ESP_ERR_TIMEOUT)
    {
        printf("error");
    }
    return raw;
}


int get_batt_volt(void)
{
    esp_wifi_stop();
    analog_init();
    int raw_reading = read_battvolt();
    adc_digi_deinit();
    esp_wifi_start();
    esp_wifi_connect();
    return raw_reading;
}
*/

void command_handler(esp_mqtt_client_handle_t client)
{
    if (strcmp(command, "psave_start") == 0)
    {
        enable_power_save();
        printf("Enabled power saving on esp_2.\n");
        esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "Enabled power saving  on esp_2.\n", 0, 1, 1);
    }
    else if (strcmp(command, "psave_stop") == 0)
    {
        disable_power_save();
        printf("Disabled power saving on esp_2.\n");
        esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "Disabled power saving  on esp_2.\n", 0, 1, 1);
    }
    else if (strcmp(command, "sleep") == 0)
    {
        esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "Putting to sleep esp_2.\n", 0, 1, 1);
        printf("putting to sleep esp_2.\n");
        esp_deep_sleep_start();
    }
    else if (strcmp(command, "ping") == 0)
    {
        printf("Ping request from broker\n");
        esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "esp_2 has been pinged\n", 0, 1, 1);
    }
    else if (strcmp(command, "ping_2") == 0)
    {   
        printf("Ping request from broker\n");
        esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "esp_2 has been pinged\n", 0, 1, 1);
    }
    else
    {
        printf("ERROR\n");
        printf("COMMAND: %s\n", command);
    }
    for(int val = 0; val < 15; ++val)
    {
        command[val] = '\0'; //clear command array
    }
}


void user_input (esp_mqtt_event_t **user_input)
{   
    char command_char = 0;
    esp_mqtt_event_t data_p;  //event type
    data_p = *(esp_mqtt_event_t *)user_input;  //dereference and typecast void pointer
    int char_count = data_p.data_len;
    esp_mqtt_client_handle_t client = data_p.client;
    command[i] = command_char;
    int stop_val = char_count + 1;
    for(int val = 0; val < stop_val; ++val)
    {
    // for loop which increments up the register addresses to grab each character of the incoming commmand
    // builds a complete command string in the command array
        char command_char = (char)(*(data_p.data+val));
        if(val != char_count){
            command[val] = command_char;
            printf("%c", command_char);
        }
        else if(val == char_count){
            command[val+1] = '\0';
            printf("\n");
        }
    }
    printf("Command: %s\n", command);
    command_handler(client);
}


static void mqtt_event_handler (void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //prints mqtt event to terminal, and NOT to the mqtt broker.
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printf("Client connected.\n");
        esp_mqtt_event_t data_p;
        data_p = *(esp_mqtt_event_t *)event_data;
        esp_mqtt_client_handle_t client = data_p.client;
        int id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_MANAGE_TO, 0);
        printf("subscribe message ID: %i\n", id); // a "-1" indicates a failed subscribe. a positive interger indicates success 
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("Client subscribed.\n");
        break;
    case MQTT_EVENT_PUBLISHED:
        printf("Client published.\n");
        break;
    case MQTT_EVENT_DATA:
        printf("Received data.\n");
        esp_mqtt_event_t **input_pointer;
        input_pointer = event_data;
        user_input(input_pointer);
        break;
    default:
        break;
    }
}


esp_mqtt_client_handle_t mqtt_init (void)
{
    // initialize mqtt client to the specified topic located at MQTT_BROKER_IP
    esp_mqtt_client_config_t mqtt_config = { //client config
        .host = MQTT_BROKER_IP,
    };
    vTaskDelay(10000 / portTICK_RATE_MS); // give wifi_connect time to establish a full connection
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config); //initialize client
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client); // register mqtt event with mqtt handler
    ESP_ERROR_CHECK(esp_mqtt_client_start(client)); // starts the client
    esp_mqtt_client_publish(client, MQTT_TOPIC_MANAGE_FROM, "esp_2 Connected.\n", 0, 1, 1); // publishes to topic
    return client;
}


void isr_handler(void* arg)
{
    // handles isr event. sends isr event to queue
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_queue, &gpio_num, NULL);
}


void isr_task (void* client)
{
    // isr task: the event to be triggered from interrupt.
    uint32_t io_num;
    while(1){ // due to freeRTOS, the task must be in a loop
        if(xQueueReceive(gpio_queue, &io_num, portMAX_DELAY))
        {
            printf("ISR EVENT!\n");
            esp_mqtt_client_publish(client, MQTT_TOPIC_TRIP, TRIP_SH1, 0, 1, 1);
        }
    }
}


void gpio_initialize(void* client)
{
    // initialize gpio for interrupt service routine handling
    gpio_queue = xQueueCreate(10, sizeof(uint32_t));  // create queue to handle isr
    xTaskCreate(isr_task, "xTaskCreate\n", 2048, client, 10, &xHandle);  // create isr task
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LOWMED)); // install isr service to gpio
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_IN, isr_handler, NULL)); // create an isr handler
    gpio_pad_select_gpio(PIN_IN);  // pad select for gpio?
    ESP_ERROR_CHECK(gpio_set_direction(PIN_IN, GPIO_MODE_INPUT)); // set pin 13 as input
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_IN, GPIO_PULLUP_ONLY)); // enable pullup resistor in pin 13
    ESP_ERROR_CHECK(gpio_set_intr_type(PIN_IN, GPIO_INTR_LOW_LEVEL)); // negitive edge interrupt service routine
    ESP_ERROR_CHECK(gpio_intr_enable(PIN_IN));  // enable isr
}


void nonvolatile_init(void)
{
    // intialize non volatile storage and clear it if needed
    esp_err_t status = nvs_flash_init();  // non volatile storage init
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(status);
}

/*
void i2c_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_4,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = GPIO_NUM_15,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_ERROR_CHECK(i2c_param_config(0, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(0, cfg.mode, 12, 12, 0));
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    int address = 
    uint8_t data = '1';
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, address, 1));
    ESP_ERROR_CHECK(i2c_master_write(cmd, &data, sizeof(data), 1));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
}
*/


void initialize(void)
{
    nonvolatile_init(); // initialize storage
    //i2c_init();
    wifi_init_sta();  // initialize wifi as station mode
    wifi_connect();  // connect to pre-defined wifi AP (access point)
    // NOTE: the effects of power saving are not noticable while the ESP32 is plugged in via usb
    // this includes the effect on wifi latency and ISR speed
    esp_mqtt_client_handle_t client = mqtt_init();  // initialize MQTT protocall
    gpio_initialize(client);  // init general purpose input/output
}


void app_main(void)
{
    //wifi_ssid, wifi_passw = intro_func();
    initialize();
    //
    while(1){
        vTaskDelay(1500 / portTICK_RATE_MS); // prevents the task watchdog from causing a system reboot...
            //...due to task watchdog timer timeout.
    }
}
