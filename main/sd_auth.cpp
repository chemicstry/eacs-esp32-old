#include "sd_auth.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include <fstream>

static const char* TAG = "SD_AUTH";

bool sd_auth_init()
{
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_PROBING;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and formatted
    // in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 0
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return false;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    ESP_LOGI(TAG, "SD card initialized");

    return true;
}

#define SD_AUTH_FILE "/sdcard/uid.txt"
bool sd_auth_uid(std::string UID)
{
    ESP_LOGI(TAG, "Performing SD auth for UID: %s", UID.c_str());

    std::ifstream f;

    try {
        f.exceptions(std::ios::badbit);
        f.open(SD_AUTH_FILE);

        for (std::string line; getline(f, line);)
        {
            ESP_LOGD(TAG, "Checking line: %s", line.c_str());
            if (line.compare(UID) == 0)
            {
                ESP_LOGI(TAG, "Auth successful");
                return true;
            }
        }
    } catch (const std::ios_base::failure& e) {
        ESP_LOGE(TAG, "Error reading SD card %s", e.code().message().c_str());
    }
    

    return false;
}