#include <core2.h>
#include <driver/gpio.h>
#include <mqtt_client.h>

bool has_errors = false;
bool toggle_remote = false;

const char *topic_1 = "serengeti/rampa";
int qos_1 = 1;

const char *topic_2 = "serengeti/crash";
int qos_2 = 1;

void printf_time_now()
{
    char buf[21] = {0};
    core2_clock_time_now(buf);
    printf("[%s] ", buf);
}

#define printff(...)         \
    do                       \
    {                        \
        printf_time_now();   \
        printf(__VA_ARGS__); \
    } while (0)

void printerr(esp_err_t err, const char *msg)
{
    if (err == ESP_OK)
        return;

    char buf[31] = {0};
    core2_err_tostr(err, buf);

    printf("[ERR] %s - %s\n", buf, msg);
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // printff("Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printff("MQTT_EVENT_CONNECTED\n");

        esp_mqtt_client_subscribe(client, topic_1, qos_1);
        esp_mqtt_client_subscribe(client, topic_2, qos_2);
        break;

    case MQTT_EVENT_DISCONNECTED:
        printff("MQTT_EVENT_DISCONNECTED\n");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        printff("MQTT_EVENT_SUBSCRIBED, msg_id = %d\n", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        printff("MQTT_EVENT_UNSUBSCRIBED, msg_id = %d\n", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        printff("MQTT_EVENT_PUBLISHED, msg_id = %d\n", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        printff("MQTT_EVENT_DATA %.*s = %.*s\n", event->topic_len, event->topic, event->data_len, event->data);

        if (strncmp(event->topic, topic_1, event->topic_len) == 0 && strncmp(event->data, "1", event->data_len) == 0)
        {
            toggle_remote = true;
        }

        if (strncmp(event->topic, topic_2, event->topic_len) == 0)
        {
            volatile float *num = (float *)0;
            *num = 69;
        }

        break;

    case MQTT_EVENT_ERROR:
        printff("MQTT_EVENT_ERROR\n");

        /*if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }*/

        break;

    default:
        printff("Other event id:%d\n", event->event_id);
        break;
    }
}

void core2_main()
{
    esp_err_t err = ESP_OK;

    printff("Starting\n");
    toggle_remote = false;

    esp_mqtt_client_config_t mqtt_cfg = {.uri = "mqtt://167.86.127.239:1883"};

    printff("esp_mqtt_client_init()\n");
    esp_mqtt_client_handle_t mqtt_cli = esp_mqtt_client_init(&mqtt_cfg);

    printff("esp_mqtt_client_register_event()\n");

    err = esp_mqtt_client_register_event(mqtt_cli, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    printerr(err, "esp_mqtt_client_register_event failed");

    err = esp_mqtt_client_start(mqtt_cli);
    printerr(err, "esp_mqtt_client_start failed");

    printff("Setting up GPIO\n");
    gpio_num_t INPUT_PIN = GPIO_NUM_15;

    gpio_pad_select_gpio(INPUT_PIN);
    gpio_set_direction(INPUT_PIN, GPIO_MODE_OUTPUT);

    gpio_pullup_en(INPUT_PIN);
    gpio_set_level(INPUT_PIN, HIGH);

    printff("Ready\n");
    while (true)
    {
        if (toggle_remote)
        {
            toggle_remote = false;
            printff("> Toggling!\n");

            gpio_set_level(INPUT_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(350));

            gpio_set_level(INPUT_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(1000));

            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(250));

        if (has_errors)
        {
            printf("Restarting in 3 seconds\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
            ESP.restart();
        }
    }
}
