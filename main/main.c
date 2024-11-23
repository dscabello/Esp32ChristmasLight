#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "tasksHandler.h"
#include "wifiHandler.h"
#include "spiHandler.h"
#include "lightCtrl.h"


void app_main(void)
{
    // ConnectToWifi();
    /* Create  Semaphores */
    WifiSemaphore = xSemaphoreCreateBinary();
    if (WifiSemaphore != NULL)
        xSemaphoreGive(WifiSemaphore);

    LedSemaphore = xSemaphoreCreateBinary();
    if (LedSemaphore != NULL)
        xSemaphoreGive(LedSemaphore);

    // Create tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_27, GPIO_PULLUP_ONLY);

    spiHandler();

    xTaskCreate(&ledHandler, "Led Handler", 4096, NULL, 3, NULL);
    while (true) {
        gpio_set_level(GPIO_NUM_27, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_27, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "End");
}