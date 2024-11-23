/******************************************************************************
 * @author      dscabello
 * @date        2024/11/24
 *
 * @copyright   Copyright © 2024, dscabello
 *
 * @license     Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 *              You may obtain a copy of the license at:
 *              https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 *              This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 *              You are free to share and adapt the material, but you must provide appropriate attribution,
 *              not use the material for commercial purposes, and distribute your contributions under the same license.
 *
 *              See the full license for more details.
 *
 * @repository  https://github.com/dscabello/Esp32ChristmasLight
 ******************************************************************************/
#include "tasksHandler.h"

const char *TAG = "Esp32ChristmasLight";

void MainThread(void *arg) {
    printf("Start MainTask thread.\n");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        // TODO: ADDING THE CONTROL USING HTTP SERVER
    }
}
