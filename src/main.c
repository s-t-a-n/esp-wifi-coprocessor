#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "string.h"

#include <freertos/event_groups.h>

#define UART_PORT_NUM UART_NUM_0
#define UART_BAUD_RATE 115200
#define BUF_SIZE 1024
#define TAG "UART_LISTENER"

#define CMD_SCAN "scan"
#define CMD_CONNECT "connect"
#define CMD_STATUS "status"

// Wi-Fi event group
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// Wi-Fi scan function
void wifi_scan() {
    printf("Starting Wi-Fi scan...\n");

    // Configure the Wi-Fi scan parameters
    wifi_scan_config_t scan_config = {.ssid = 0, .bssid = 0, .channel = 0, .show_hidden = true};

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // Start Wi-Fi scan (passive scan, blocking)

    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num)); // Get the number of scanned APs

    wifi_ap_record_t* ap_records = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * ap_num);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records)); // Get the list of APs found in the scan

    printf("Found %d access points:\n", ap_num);
    for (int i = 0; i < ap_num; i++) {
        printf("SSID: %s, RSSI: %d, Channel: %d\n", ap_records[i].ssid, ap_records[i].rssi, ap_records[i].primary);
    }

    free(ap_records);

    printf("Wi-Fi scan completed.\n");
}

void wifi_connect(const char* ssid, const char* password) {
    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = "",
                .password = "",
            },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_init() {
    // Initialize NVS (needed for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create a default Wi-Fi station
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set the Wi-Fi mode to station mode (client mode)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());
}

void process_command(const char* cmd) {
    if (strncmp(cmd, CMD_SCAN, strlen(CMD_SCAN)) == 0) {
        ESP_LOGI(TAG, "Received command: scan");
        wifi_scan();
    } else if (strncmp(cmd, CMD_CONNECT, strlen(CMD_CONNECT)) == 0) {
        ESP_LOGI(TAG, "Received command: connect");
        char ssid[32], password[64];
        if (sscanf(cmd, "connect %s %s", ssid, password) == 2) {
            ESP_LOGI(TAG, "Connecting to SSID: %s, Password: %s", ssid, password);
            wifi_connect(ssid, password);
        } else {
            printf("Invalid connect command.\n");
        }
    } else if (strncmp(cmd, CMD_STATUS, strlen(CMD_STATUS)) == 0) {
        ESP_LOGI(TAG, "Received command: status");
        printf("ready\n"); // just print something to check connectivity
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", cmd);
        printf("Unknown command\n");
    }
}

void uart_event_task(void* pvParameters) {
    uint8_t data[BUF_SIZE];
    int length = 0;

    uart_config_t uart_config = {.baud_rate = UART_BAUD_RATE,
                                 .data_bits = UART_DATA_8_BITS,
                                 .parity = UART_PARITY_DISABLE,
                                 .stop_bits = UART_STOP_BITS_1,
                                 .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    while (1) { // read data from the UART
        length = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (length > 0) {
            data[length] = '\0';
            ESP_LOGI(TAG, "Received data: %s", (char*)data);
            process_command((char*)data);
        }
    }
}

void app_main() {
    wifi_init();

    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL); // UART listener task
}
