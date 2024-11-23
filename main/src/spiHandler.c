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
#include "spiHandler.h"

#define I2C_MASTER_SCL_IO           22          /* SCL */
#define I2C_MASTER_SDA_IO           21          /* SDA */
#define I2C_MASTER_NUM              0           /* I2C port peripheral number */
#define I2C_MASTER_FREQ_HZ          350000      /* I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0           /* I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0           /* I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define TLC59116_ADDR               0x60

#define TLC59116_MODE_1_REG         0x00
#define TLC59116_MODE_2_REG         0x01
#define TLC59116_PWR_00_ADDR_REG    0x02
#define TLC59116_PWR_01_ADDR_REG    0x03
#define TLC59116_PWR_02_ADDR_REG    0x04
#define TLC59116_PWR_03_ADDR_REG    0x05
#define TLC59116_PWR_04_ADDR_REG    0x06
#define TLC59116_PWR_05_ADDR_REG    0x07
#define TLC59116_PWR_06_ADDR_REG    0x08
#define TLC59116_PWR_07_ADDR_REG    0x09
#define TLC59116_PWR_08_ADDR_REG    0x0A
#define TLC59116_PWR_09_ADDR_REG    0x0B
#define TLC59116_PWR_10_ADDR_REG    0x0C
#define TLC59116_PWR_11_ADDR_REG    0x0D
#define TLC59116_PWR_12_ADDR_REG    0x0E
#define TLC59116_PWR_13_ADDR_REG    0x0F
#define TLC59116_PWR_14_ADDR_REG    0x10
#define TLC59116_PWR_15_ADDR_REG    0x11
#define TLC59116_GRPPWM_REG         0x12
#define TLC59116_GRPFREQ_REG        0x13
#define TLC59116_LEDOUT0_REG        0x14
#define TLC59116_LEDOUT1_REG        0x15
#define TLC59116_LEDOUT2_REG        0x16
#define TLC59116_LEDOUT3_REG        0x17
#define TLC59116_SUBADR1_REG        0x18
#define TLC59116_SUBADR2_REG        0x19
#define TLC59116_SUBADR3_REG        0x1A
#define TLC59116_ALLCALLADR_REG     0x1B
#define TLC59116_IREF_REG           0x1C
#define TLC59116_EFLAG1_REG         0x1D
#define TLC59116_EFLAG2_REG         0x1E

#define TLC59116_PWR_ADDR_INC_REG   0xA2

#define TLC59116_LEDOUT0_DEFAULT    0xAA
#define TLC59116_LEDOUT1_DEFAULT    0xAA
#define TLC59116_LEDOUT2_DEFAULT    0xAA
#define TLC59116_LEDOUT3_DEFAULT    0xAA

#define TLC59116_LEDOUTx_DISABLE    0x00

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

i2c_master_bus_config_t i2c_mst_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = false,
};

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = TLC59116_ADDR,
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
};

bool led_status = false;

/**
 * @brief Read a sequence of bytes from a TLC59116 registers
 */
static esp_err_t RegisterRead_TLC59116(uint8_t reg_addr, uint8_t *data, size_t len)
{
    gpio_set_level(GPIO_NUM_2, 0);
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
    ESP_ERROR_CHECK(ret);
    gpio_set_level(GPIO_NUM_2, 1);
    return ret;
}

/**
 * @brief Write a frame data to TLC59116 registers
 */
static esp_err_t RegisterWriteData_TLC59116(uint8_t reg_addr, uint8_t* data, size_t len)
{
    gpio_set_level(GPIO_NUM_2, 0);
    uint8_t write_buf[len + 1];
    size_t i;
    write_buf[0] = reg_addr;
    for (i = 0 ; i < len ; i++)
        write_buf[i+1] = data[i];
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
    ESP_ERROR_CHECK(ret);
    gpio_set_level(GPIO_NUM_2, 1);
    return ESP_OK;
}

/**
 * @brief Write a byte to a TLC59116 register
 */
static esp_err_t RegisterWriteByte_TLC59116(uint8_t reg_addr, uint8_t data) {
    return RegisterWriteData_TLC59116(reg_addr, &data, 1);
}

bool CheckError_TLC59116() {
    bool ret = true;
    uint8_t ErrorFlag_1;
    uint8_t ErrorFlag_2;
    RegisterRead_TLC59116(TLC59116_EFLAG1_REG, &ErrorFlag_1, 1);
    RegisterRead_TLC59116(TLC59116_EFLAG2_REG, &ErrorFlag_2, 1);
    if (ErrorFlag_1 != 0x00) {
        ESP_LOGE(TAG,"Error Flag 1: %x", ErrorFlag_1);
        ret = false;
    }
    if (ErrorFlag_2 != 0x00) {
        ESP_LOGE(TAG,"Error Flag 2: %x", ErrorFlag_2);
        ret = false;
    }
    if (ret == false) {
        /* Reset target and set the last value */
        /* TODO: Fix a issue in the current detection of TLC59116 */
        gpio_set_level(GPIO_NUM_23, 0);
        led_status = !led_status;
        gpio_set_level(GPIO_NUM_2, led_status);
        ESP_LOGI(TAG,"Reset");
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(GPIO_NUM_23, 1);
        vTaskDelay(pdMS_TO_TICKS(1));
        RegisterWriteByte_TLC59116(TLC59116_MODE_1_REG, 0x00);
        RegisterWriteByte_TLC59116(TLC59116_MODE_2_REG, 0x80);
        EnablePWMOutput();
    }
    return ret;
}

bool WriteAllPWM(uint8_t *values, size_t len) {
    RegisterWriteData_TLC59116(TLC59116_PWR_ADDR_INC_REG, values, len);
    return CheckError_TLC59116();
}

bool WritePWMNum(uint8_t pwm_num, uint8_t value) {
    RegisterWriteData_TLC59116(pwm_num + 0x02, &value, 1);
    while(CheckError_TLC59116() != true) {
        RegisterWriteData_TLC59116(pwm_num + 0x02, &value, 1);
    }
    return true;
}

bool DisablePWMOutput() {
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT0_REG, TLC59116_LEDOUTx_DISABLE);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT1_REG, TLC59116_LEDOUTx_DISABLE);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT2_REG, TLC59116_LEDOUTx_DISABLE);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT3_REG, TLC59116_LEDOUTx_DISABLE);
    return CheckError_TLC59116();
}

bool EnablePWMOutput() {

    RegisterWriteByte_TLC59116(TLC59116_LEDOUT0_REG, TLC59116_LEDOUT0_DEFAULT);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT1_REG, TLC59116_LEDOUT1_DEFAULT);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT2_REG, TLC59116_LEDOUT2_DEFAULT);
    RegisterWriteByte_TLC59116(TLC59116_LEDOUT3_REG, TLC59116_LEDOUT3_DEFAULT);
    return CheckError_TLC59116();
}

static esp_err_t spiMasterInit(void)
{
    ESP_LOGI(TAG, "Set Reset IO");
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_23, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);
    gpio_set_level(GPIO_NUM_2, 0);


    ESP_LOGI(TAG, "I2C int");

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    ESP_ERROR_CHECK(ret);

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    ESP_ERROR_CHECK(ret);

    return ESP_OK;
}

void spiHandler() {
    ESP_LOGI(TAG, "spiHandler");
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);
    if (spiMasterInit() == ESP_OK) {
        ESP_LOGI(TAG, "TLC59116 started");
        gpio_set_level(GPIO_NUM_23, 1);
        RegisterWriteByte_TLC59116(TLC59116_MODE_1_REG, 0x00);
        RegisterWriteByte_TLC59116(TLC59116_MODE_2_REG, 0x80);
        EnablePWMOutput();
    }
}
