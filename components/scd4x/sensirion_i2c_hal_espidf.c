#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/task.h"
#include "scd4x_hal_internal.h"
#include "sensirion_i2c_hal.h"

#define SCD41_ADDRESS  0x62
#define I2C_TIMEOUT_MS 100

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t device;
static const char             *TAG = "scd41_i2c";

esp_err_t scd4x_hal_create(void)
{
    if ( bus != NULL || device != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port                     = I2C_NUM_0,
        .sda_io_num                   = GPIO_NUM_21,
        .scl_io_num                   = GPIO_NUM_47,
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = false,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus);
    if ( err != ESP_OK )
    {
        return err;
    }

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = SCD41_ADDRESS,
        .scl_speed_hz    = 100000,
    };
    err = i2c_master_bus_add_device(bus, &device_config, &device);
    if ( err != ESP_OK )
    {
        // Keep the handle if cleanup fails so destroy can be retried.
        if ( i2c_del_master_bus(bus) == ESP_OK )
        {
            bus = NULL;
        }
        return err;
    }

    return ESP_OK;
}

esp_err_t scd4x_hal_destroy(void)
{
    if ( device != NULL )
    {
        esp_err_t err = i2c_master_bus_rm_device(device);
        if ( err != ESP_OK )
        {
            return err;
        }
        device = NULL;
    }
    if ( bus != NULL )
    {
        esp_err_t err = i2c_del_master_bus(bus);
        if ( err != ESP_OK )
        {
            return err;
        }
        bus = NULL;
    }

    return ESP_OK;
}

void sensirion_i2c_hal_init(void)
{
    (void)scd4x_hal_create();
}

int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx)
{
    return bus_idx == 0 ? 0 : -1;
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t *data, uint8_t count)
{
    if ( device == NULL || address != SCD41_ADDRESS || data == NULL || count == 0 )
    {
        return -1;
    }
    esp_err_t err = i2c_master_transmit(device, data, count, I2C_TIMEOUT_MS);
    if ( err != ESP_OK )
    {
        ESP_LOGW(TAG, "write: %s", esp_err_to_name(err));
        return -1;
    }
    return 0;
}

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t *data, uint8_t count)
{
    if ( device == NULL || address != SCD41_ADDRESS || data == NULL || count == 0 )
    {
        return -1;
    }
    esp_err_t err = i2c_master_receive(device, data, count, I2C_TIMEOUT_MS);
    if ( err != ESP_OK )
    {
        ESP_LOGW(TAG, "read: %s", esp_err_to_name(err));
        return -1;
    }
    return 0;
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds)
{
    if ( useconds == 0 )
    {
        return;
    }
    // Round up, then add a tick to account for the current tick's phase.
    // This function is called from a FreeRTOS task, never from an ISR.
    uint64_t ticks = ((uint64_t)useconds * configTICK_RATE_HZ + 999999ULL) / 1000000ULL;
    vTaskDelay((TickType_t)(ticks + 1));
}

void sensirion_i2c_hal_free(void)
{
    (void)scd4x_hal_destroy();
}
