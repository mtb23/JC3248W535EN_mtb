
// #include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"
#include <esp_log.h>   // Add this line to include the header file that declares ESP_LOGI
#include <esp_flash.h> // Add this line to include the header file that declares esp_flash_t
#include <esp_chip_info.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_heap_caps.h>

static const char *TAG = "DEMO_LVGL";

uint32_t MQTT_CONNEECTED = 0;
/* Definicja WiFi */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/event_groups.h"
#include <esp_wifi.h>
#include <nvs_flash.h>

#include "esp_sntp.h"



#include "mqtt_client.h" //provides important functions to connect with MQTT
//#include "protocol_examples_common.h" //important for running different protocols in code
#include <ui/ui.h>


/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/


const char *ssid = "";
const char *pass = "";
int retry_num=1;

const char *mqtt_host = "";
const long int *mqtt_port ;
const long int *mqtt_client;
const char *maszyna;         // Define and initialize the maszyna variable
const int *stan_przycisku ;           // Define and initialize the stan_przycisku variable

#define BUFFER_SIZE 100
char tempBuffer[BUFFER_SIZE];

// Usage


char* createPayload(const char* maszyna, int stan_przycisku) {
    char* payload;
    asprintf(&payload, "{\"maszyna\":\"%s\", \"stan_przycisku\":%d}", maszyna, stan_przycisku);
    return payload;
}


//const char *CONFIG_BROKER_URL ="843b90bc9ff444b29a2bc6c8c7c6ae96.s1.eu.hivemq.cloud";

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
if(event_id == WIFI_EVENT_STA_START)
{
  printf("WIFI CONNECTING....\n");
}
else if (event_id == WIFI_EVENT_STA_CONNECTED)
{
  printf("WiFi CONNECTED\n");
}
else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
  printf("WiFi lost connection\n");
  if(retry_num<5){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
}
else if (event_id == IP_EVENT_STA_GOT_IP)
{
  printf("Wifi got IP...\n\n");
}
}

void wifi_connection()
{
     
//                          s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            
           }
    
        };
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    //esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",ssid,pass);
    
}
/* Koniec definicji WIFI */

/*Definicja łącza do MQTT*/


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "T3601/test1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "masterform/#", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "T3601/test1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "T3601/test1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "T3601/test0", "data", 0, 0, 0);
        
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        snprintf(tempBuffer, sizeof(tempBuffer), "%.*s", event->topic_len, event->data);
        lv_textarea_set_text(ui_ScreenPraca_Textarea_TextMQTTData,tempBuffer);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
  
  
    esp_mqtt_client_config_t mqtt_cfg = {
      
        //.broker.address.uri = "mqtt://192.168.5.51:1883"  //CONFIG_BROKER_URL, uri in the format (username:password@domain:port)
        .broker.address.hostname= "192.168.5.51",
        .broker.address.port = 1883,
        .broker.address.transport =  MQTT_TRANSPORT_OVER_TCP
        //.broker.address.hostname = mqtt_host
        //.broker.address.port = mqtt_port
        //,
        //
        //.broker.address.transport =  MQTT_TRANSPORT_OVER_TCP,    
        //.credentials.username="esp32",
        //.credentials.authentication.password="Masterform2025!",
        //.credentials.client_id="esp32-s3-480x320-123456"




    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    mqtt_client = client;
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    esp_mqtt_client_start(client);
}



/* Koniec MQTT */

void FuncConnectMQTT(lv_event_t * e)
{
  mqtt_host = lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextAdressMQTT);
  mqtt_port = strtol(lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextMQTTPort),NULL,10);
  maszyna = lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextAreaStanowisko);

	ESP_LOGI(TAG, "[APP] Startup..");
  ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
   ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

   // esp_log_level_set("*", ESP_LOG_INFO);
   // esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
   // esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
   // esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
   // esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
   // esp_log_level_set("transport", ESP_LOG_VERBOSE);
   // esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    

    mqtt_app_start();
    
  
}

void FuncSendMQTTMsg(lv_event_t * e)
{
//	int msg_id = esp_mqtt_client_publish(client, "T3601/test1", "Jebać PiS", 0, 1, 0);
//  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
lv_textarea_set_text(ui_ScreenUstawienia_Textarea_TextMQTTUser,"testuser");
}

void FuncConnectWIFI(lv_event_t * e)
{
	// Your code here
  
  ssid = lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextAreaSSID);
  pass = lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextAreaPassword);
  maszyna = lv_textarea_get_text(ui_ScreenUstawienia_Textarea_TextAreaStanowisko);
  nvs_flash_init();
  wifi_connection();
}

/*
void FuncChangeValue(lv_event_t * e)
{
 int msg_id = esp_mqtt_client_publish(mqtt_client , "T3601/stan_przycisku", "1", 0, 1, 0);
 ESP_LOGI(TAG, "zmieniono wartosc=%d", msg_id);
}
*/

void FuncPraca(lv_event_t * e)
{
stan_przycisku=0;

char *payload = createPayload(maszyna, stan_przycisku); // Create the formatted JSON payload

int msg_id = esp_mqtt_client_publish(mqtt_client, "masterform/stan_przycisku", payload, 0, 1, 0);

free(payload); // Don't forget to free the allocated memory
 ESP_LOGI(TAG, "Praca=%d", msg_id);

}

void FuncAwaria(lv_event_t * e)
{

stan_przycisku=1;

char *payload = createPayload(maszyna, stan_przycisku); // Create the formatted JSON payload

int msg_id = esp_mqtt_client_publish(mqtt_client, "masterform/stan_przycisku", payload, 0, 1, 0);

free(payload); // Don't forget to free the allocated memory
 ESP_LOGI(TAG, "Praca=%d", msg_id);

}

void FuncPomiar(lv_event_t * e)
{

 stan_przycisku=2;

char *payload = createPayload(maszyna, stan_przycisku); // Create the formatted JSON payload

int msg_id = esp_mqtt_client_publish(mqtt_client, "masterform/stan_przycisku", payload, 0, 1, 0);

free(payload); // Don't forget to free the allocated memory
 ESP_LOGI(TAG, "Praca=%d", msg_id);

}

void FuncPrzezbrojenie(lv_event_t * e)
{

stan_przycisku=3;

char *payload = createPayload(maszyna, stan_przycisku); // Create the formatted JSON payload

int msg_id = esp_mqtt_client_publish(mqtt_client, "masterform/stan_przycisku", payload, 0, 1, 0);

free(payload); // Don't forget to free the allocated memory
 ESP_LOGI(TAG, "Praca=%d", msg_id);

}

void FuncBrakObsady(lv_event_t * e)
{
stan_przycisku=4;

char *payload = createPayload(maszyna, stan_przycisku); // Create the formatted JSON payload

int msg_id = esp_mqtt_client_publish(mqtt_client, "masterform/stan_przycisku", payload, 0, 1, 0);

free(payload); // Don't forget to free the allocated memory
 ESP_LOGI(TAG, "Praca=%d", msg_id);

}


#define BUILD (String(__DATE__) + " - " + String(__TIME__)).c_str()

#define logSection(section) \
  ESP_LOGI(TAG, "\n\n************* %s **************\n", section);

/**
 * @brief LVGL porting example
 * Set the rotation degree:
 *      - 0: 0 degree
 *      - 90: 90 degree
 *      - 180: 180 degree
 *      - 270: 270 degree
 *
 */
#define LVGL_PORT_ROTATION_DEGREE (90)

/**
 * To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 * You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 */
//#include <demos/lv_demos.h>
// #include <examples/lv_examples.h>


void setup();

#if !CONFIG_AUTOSTART_ARDUINO
void app_main()
{
  // initialize arduino library before we start the tasks
  // initArduino();

  setup();
}
#endif
void setup()
{

  //  String title = "LVGL porting example";

  // Serial.begin(115200);
  logSection("LVGL porting example start");
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  ESP_LOGI(TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    ESP_LOGI(TAG, "Get flash size failed");
    return;
  }

  ESP_LOGI(TAG, "%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
  size_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  ESP_LOGI(TAG, "Free PSRAM: %d bytes", freePsram);
  logSection("Initialize panel device");
  // ESP_LOGI(TAG, "Initialize panel device");
  bsp_display_cfg_t cfg = {
      .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
      .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
      .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
      .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
      .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 0
      .rotate = LV_DISP_ROT_NONE,
#endif
  };

  bsp_display_start_with_config(&cfg);
  bsp_display_backlight_on();

  logSection("Create UI");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0);

    
  // ** Square Line **/
  ui_init();
  
  /* Release the mutex */
  bsp_display_unlock();

  logSection("LVGL porting example end");
  

}

void loop()
{
  ESP_LOGI(TAG, "IDLE loop");
  // delay(1000);
}
