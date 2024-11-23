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

#include "lightCtrl.h"

#define TICK_FOR_LED_ANNIMATION_MS         70
#define TABLE_TIME_RESOLUTION_MS           100
#define NUMBER_OF_LEDS                     16
#define TIME_TABLE_POS                     16
#define TIME_TYPE_A_POS                    17
#define TIME_TYPE_B_POS                    18
#define LED_VALUE_INC_CORRECTION           100 /* Used in the calculation for the fade */
#define LED_OFF_POWER                      0x00
#define LED_ON_POWER                       CONFIG_BACKLIGHT_LEDS_POWER
#define LED_MAX_POWER                      CONFIG_LEDS_MAX_POWER
#define LED_BLINK_TIME                     CONFIG_BLINK_LEDS_TIME
#define MATRIX_SIZE                        19

typedef enum {
    LED_CTRL_UNKNOWN,
    LED_CTRL_START,
    LED_CTRL_STOP,
    LED_CTRL_RESET
} LedCtrlCmd_t;

typedef enum {
    LED_STS_UNKNOWN,
    LED_STS_STARTING,
    LED_STS_RUNNING,
    LED_STS_STOPPING,
    LED_STS_STOP
} LedSts_t;

typedef struct {
    LedSts_t LedSts;
    const uint8_t (*matrix)[MATRIX_SIZE];
    uint8_t PosAtual;
    uint8_t PosFinal;
    int16_t Increment_PWM[NUMBER_OF_LEDS]; /* Used to create a fade between the frames * 100 */
    uint8_t CurrentValue[NUMBER_OF_LEDS];
    uint16_t CalcValue[NUMBER_OF_LEDS]; /* Ssame of current value but with more resolution */
    uint16_t TimeCounter;
} LedAnnimation_t;

typedef struct {
    uint16_t LedBlink;
    uint16_t LedPowerMAX;
    int16_t Increment_PWM[NUMBER_OF_LEDS]; /* Used to create a fade between the frames * 100 */
    uint8_t LedPower[NUMBER_OF_LEDS];
    bool LedOn;
} LedETS2Ctrl_t;

const char *TAG = "ESP32 Christmas Light";

#define NUM_FRAMES              13
// Annimation Table
// 16 - Power of each LED
// Delay (x TABLE_TIME_RESOLUTION_MS ms)
// Type bit mask if it is marked it will be fade between the frames
const uint8_t InitAnnimation[NUM_FRAMES][MATRIX_SIZE] = {
/*    PWM0 |PWM1 |PWM2 |PWM3 |PWM4 |PWM5 |PWM6 |PWM7 |PWM8 |PWM9 |PWM10|PWM11|PWM12|PWM13|PWM14|PWM15|Delay|TypeA|TypeB */
    // { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xFF, 0xFF},
    // { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x0A, 0xFF, 0xFF},
    // { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x14, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x14, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x14, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD0, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xD0, 0xF0, 0xF0, 0x0A, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0xF0, 0xFF, 0xF0, 0xFF, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xD0, 0xF0, 0xF0, 0x0A, 0xFF, 0xFF},
    // { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xD0, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0xD0, 0xF0, 0xF0, 0x0A, 0xFF, 0xFF},

    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x00, 0x00},
};

LedAnnimation_t LedAnnimation;

LedETS2Ctrl_t LedETS2Ctrl;

SemaphoreHandle_t LedSemaphore = NULL;

void ledRunAnnimationCmd(const LedCtrlCmd_t cmd, const uint8_t (*matrix)[MATRIX_SIZE], const size_t len) {
    switch (cmd)
    {
        case LED_CTRL_UNKNOWN:
            /* Do notthing */
            break;
        case LED_CTRL_START:
            if ( (LedAnnimation.LedSts != LED_STS_STARTING) && (LedAnnimation.LedSts != LED_STS_RUNNING) ) {
                if (len > 1) {
                    LedAnnimation.matrix = matrix;
                    LedAnnimation.PosFinal = len - 1; /* Since we are using absolut position it needs to substract 1 */
                    LedAnnimation.LedSts = LED_STS_STARTING;
                    ESP_LOGI(TAG,"Cmd Start accept");
                }
            }
            break;
        case LED_CTRL_STOP:
            if (LedAnnimation.LedSts != LED_STS_STOP) {
                LedAnnimation.LedSts = LED_STS_STOP;
            }
            break;
        case LED_CTRL_RESET:
            if ( (LedAnnimation.LedSts != LED_STS_STOP) && (LedAnnimation.LedSts != LED_STS_STOP) ) {
                LedAnnimation.LedSts = LED_STS_STOPPING;
            }
            break;

        default:
            break;
    }
}

void ledAnnimationCalcInc(const uint8_t (*matrix)[MATRIX_SIZE], const uint8_t pos_A, const uint8_t pos_B) {
    uint8_t led_num;
    uint8_t diff_pwm;
    const uint16_t type_fade = (uint16_t)((matrix[pos_A][TIME_TYPE_A_POS] << 8) + matrix[pos_A][TIME_TYPE_B_POS]);
    for (led_num = 0 ; led_num < NUMBER_OF_LEDS; led_num++) {
        if (type_fade & (1 << ((NUMBER_OF_LEDS - 1) - led_num))) {
            if (matrix[pos_A][led_num] >  matrix[pos_B][led_num]) {
                diff_pwm = matrix[pos_A][led_num] - matrix[pos_B][led_num];
                LedAnnimation.Increment_PWM[led_num] =
                    -(uint16_t)((diff_pwm * LED_VALUE_INC_CORRECTION) / ( (matrix[pos_A][TIME_TABLE_POS] * TABLE_TIME_RESOLUTION_MS) / TICK_FOR_LED_ANNIMATION_MS));
            } else if (matrix[pos_A][led_num] <  matrix[pos_B][led_num]) {
                diff_pwm = matrix[pos_B][led_num] - matrix[pos_A][led_num];
                LedAnnimation.Increment_PWM[led_num] =
                    (uint16_t)((diff_pwm * LED_VALUE_INC_CORRECTION) / ( (matrix[pos_A][TIME_TABLE_POS] * TABLE_TIME_RESOLUTION_MS) / TICK_FOR_LED_ANNIMATION_MS));
            } else {
                LedAnnimation.Increment_PWM[led_num] = 0;
            }
        }
        LedAnnimation.CalcValue[led_num] = (uint16_t)(matrix[pos_A][led_num] * LED_VALUE_INC_CORRECTION);
        // ESP_LOGI(TAG,"ledRunAnnimationCalcInc: Pos %d -> Pos %d LED %d PosA %d posB %d inc %d time = %d.", pos_A, pos_B, led_num, matrix[pos_A][led_num], matrix[pos_B][led_num], LedAnnimation.Increment_PWM[led_num], matrix[pos_A][TIME_TABLE_POS]);
    }
}

void ledRunAnnimation() {
    uint8_t next_pos = 1;
    switch (LedAnnimation.LedSts) {
        case LED_STS_UNKNOWN:
            break;
        case LED_STS_STARTING:
                LedAnnimation.PosAtual = 0;
                next_pos = LedAnnimation.PosAtual + 1;
                LedAnnimation.LedSts = LED_STS_RUNNING;
                LedAnnimation.TimeCounter = (LedAnnimation.matrix[LedAnnimation.PosAtual][TIME_TABLE_POS] * TABLE_TIME_RESOLUTION_MS) / TICK_FOR_LED_ANNIMATION_MS; /* Read the time */
                ledAnnimationCalcInc(LedAnnimation.matrix, LedAnnimation.PosAtual, next_pos);
                memcpy(LedAnnimation.CurrentValue, LedAnnimation.matrix[LedAnnimation.PosAtual], NUMBER_OF_LEDS);  // Copies entire array
                if (WriteAllPWM(LedAnnimation.CurrentValue, NUMBER_OF_LEDS) == false) {
                    //LedAnnimation.LedSts = LED_STS_STOP;
                }
                ESP_LOGI(TAG,"Starting animation");
            break;
        case LED_STS_RUNNING:
                if (LedAnnimation.TimeCounter) {
                    LedAnnimation.TimeCounter--;
                    /* Adding next step of fade */
                    uint8_t led_num;
                    const uint16_t type_fade = (uint16_t)((LedAnnimation.matrix[LedAnnimation.PosAtual][TIME_TYPE_A_POS] << 8) + LedAnnimation.matrix[LedAnnimation.PosAtual][TIME_TYPE_B_POS]);
                    for (led_num = 0 ; led_num < NUMBER_OF_LEDS; led_num++) {
                        if (type_fade & (1 << ((NUMBER_OF_LEDS - 1) - led_num))) {
                        //if (LedAnnimation.matrix[LedAnnimation.PosAtual][TIME_TYPE_POS] & (1 << led_num)) {
                            if (LedAnnimation.Increment_PWM[led_num] < 0) { // Negative Increment
                                if (LedAnnimation.CalcValue[led_num] < (LedAnnimation.Increment_PWM[led_num]*-1))
                                    LedAnnimation.CalcValue[led_num] = 0;
                                else
                                    LedAnnimation.CalcValue[led_num] = (uint16_t)((LedAnnimation.CalcValue[led_num] + LedAnnimation.Increment_PWM[led_num]));
                            } else { // Positive Increment
                                LedAnnimation.CalcValue[led_num] = LedAnnimation.CalcValue[led_num] + LedAnnimation.Increment_PWM[led_num];
                                if (LedAnnimation.CalcValue[led_num] > 25500)
                                    LedAnnimation.CalcValue[led_num] = 25500;
                                else
                                    LedAnnimation.CalcValue[led_num] = (uint16_t)((LedAnnimation.CalcValue[led_num] + LedAnnimation.Increment_PWM[led_num]));
                            }
                            LedAnnimation.CurrentValue[led_num] = (uint8_t)(LedAnnimation.CalcValue[led_num] / LED_VALUE_INC_CORRECTION);
                        }
                    }
                    WriteAllPWM(LedAnnimation.CurrentValue, NUMBER_OF_LEDS);
                    // ESP_LOGI(TAG,"Frame %d, Led 0 = %d (%d), inc = %d.",LedAnnimation.PosAtual, LedAnnimation.CurrentValue[0], LedAnnimation.CalcValue[0], LedAnnimation.Increment_PWM[0]);
                } else {
                    LedAnnimation.PosAtual++;
                    if (LedAnnimation.PosAtual == LedAnnimation.PosFinal) {
                        ESP_LOGI(TAG,"Starting animation again");
                        next_pos = 0;
                    } else if (LedAnnimation.PosAtual > LedAnnimation.PosFinal) {
                        LedAnnimation.PosAtual = 0;
                        next_pos = 1;
                    } else {
                        next_pos = LedAnnimation.PosAtual + 1;
                    }
                    LedAnnimation.TimeCounter = (LedAnnimation.matrix[LedAnnimation.PosAtual][TIME_TABLE_POS] * TABLE_TIME_RESOLUTION_MS) / TICK_FOR_LED_ANNIMATION_MS; /* Read the time */
                    ledAnnimationCalcInc(LedAnnimation.matrix, LedAnnimation.PosAtual, next_pos);
                    memcpy(LedAnnimation.CurrentValue, LedAnnimation.matrix[LedAnnimation.PosAtual], NUMBER_OF_LEDS);
                    if (WriteAllPWM(LedAnnimation.CurrentValue, NUMBER_OF_LEDS) == false) {
                        //LedAnnimation.LedSts = LED_STS_STOP;
                    }
                    // ESP_LOGI(TAG,"Frame %d, Led 0 = %d (%d), inc = %d.",LedAnnimation.PosAtual, LedAnnimation.CurrentValue[0], LedAnnimation.CalcValue[0], LedAnnimation.Increment_PWM[0]);
                    ESP_LOGV(TAG,"Next frame = %d, timer = %d.", LedAnnimation.PosAtual, LedAnnimation.TimeCounter);
                }
            break;
        case LED_STS_STOPPING:
            break;
        case LED_STS_STOP:
            break;
        default:
            break;
    }
}

void ledRunLight() {
    uint8_t i;
    uint8_t min_led_power = LED_OFF_POWER;
    uint16_t aux_inc = (int16_t)((LED_MAX_POWER - LED_OFF_POWER) * LED_VALUE_INC_CORRECTION) / (LED_BLINK_TIME / TICK_FOR_LED_ANNIMATION_MS);
    static uint16_t aux_blink = 0x0000;
    static uint16_t aux_led_power_max = 0x0000;
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        if (LedETS2Ctrl.LedOn == true) {
            min_led_power = LED_ON_POWER;
            aux_inc = (int16_t)((LED_MAX_POWER - LED_ON_POWER) * LED_VALUE_INC_CORRECTION) / (LED_BLINK_TIME / TICK_FOR_LED_ANNIMATION_MS);
        }
        aux_blink = LedETS2Ctrl.LedBlink;
        aux_led_power_max = LedETS2Ctrl.LedPowerMAX;
        xSemaphoreGive(LedSemaphore);
    }
    for(i = 0 ; i < NUMBER_OF_LEDS; i++) {
        if (aux_blink & (1 << i)) {
            if (LedETS2Ctrl.LedPower[i] < min_led_power)
                LedETS2Ctrl.LedPower[i] = min_led_power;

            if (LedETS2Ctrl.LedPower[i] == LED_MAX_POWER) {
                LedETS2Ctrl.Increment_PWM[i] = -aux_inc;
            } else if (LedETS2Ctrl.LedPower[i] == min_led_power) {
                LedETS2Ctrl.Increment_PWM[i] = aux_inc;
            }
            if (LedETS2Ctrl.Increment_PWM[i] < 0) {
                uint16_t aux_calc = LedETS2Ctrl.LedPower[i]*LED_VALUE_INC_CORRECTION;
                LedETS2Ctrl.LedPower[i] =
                    (uint8_t)((aux_calc > LedETS2Ctrl.Increment_PWM[i]) ? (aux_calc + LedETS2Ctrl.Increment_PWM[i]) / LED_VALUE_INC_CORRECTION : min_led_power);
            } else {
                uint16_t aux_calc = LedETS2Ctrl.LedPower[i]*LED_VALUE_INC_CORRECTION;
                LedETS2Ctrl.LedPower[i] =
                    (uint8_t)((aux_calc < LedETS2Ctrl.Increment_PWM[i]) ? (aux_calc + LedETS2Ctrl.Increment_PWM[i]) / LED_VALUE_INC_CORRECTION : LED_MAX_POWER);
            }

        } else if (aux_led_power_max & (1 << i)) {
            LedETS2Ctrl.LedPower[i] = LED_MAX_POWER;
        } else {
            LedETS2Ctrl.LedPower[i] = min_led_power;
        }
    }
    while (WriteAllPWM(LedETS2Ctrl.LedPower, NUMBER_OF_LEDS) == false) {
        // TODO: Adding a retry max counter
    }
}

void ledBackLightOn() {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedOn = true;
        xSemaphoreGive(LedSemaphore);
    }
}

void ledBackLightOFF() {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedOn = false;
        xSemaphoreGive(LedSemaphore);
    }
}

void ledCmdTurnOnBlink(CmdLedNum_t LedNumber) {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedBlink |= LedNumber;
        xSemaphoreGive(LedSemaphore);
    }
}

void ledCmdTurnOFFBlink(CmdLedNum_t LedNumber) {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedBlink &= ~(uint16_t)(LedNumber);
        xSemaphoreGive(LedSemaphore);
    }
}

void ledCmdTurnOn(CmdLedNum_t LedNumber) {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedPowerMAX |= LedNumber;
        xSemaphoreGive(LedSemaphore);
    }
}

void ledCmdTurnOFF(CmdLedNum_t LedNumber) {
    if (xSemaphoreTake(LedSemaphore, (TickType_t) 10) == pdTRUE) {
        LedETS2Ctrl.LedPowerMAX &= ~((uint16_t)(LedNumber));
        xSemaphoreGive(LedSemaphore);
    }
}

void ledHandler(void *arg)
{
    LedETS2Ctrl.LedOn = false;
    LedETS2Ctrl.LedBlink = 0x0000;
    LedETS2Ctrl.LedPowerMAX = 0x0000;
    ledRunAnnimationCmd(LED_CTRL_START, InitAnnimation, NUM_FRAMES);

    while (1) {
       ledRunAnnimation();
       vTaskDelay(pdMS_TO_TICKS(TICK_FOR_LED_ANNIMATION_MS));
    }
}