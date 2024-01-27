#include <Arduino.h>
#include <M5Unified.h>

#include <Avatar.h>
#include "BluetoothA2DPSink.h"

// Bluetooth device name
static const char *BT_DEVICE_NAME = "m5avatar-bt-speaker";

static const int16_t LEVEL_MIN = 0;
static const int16_t LEVEL_MAX = 5000;

m5avatar::Avatar avatar;
BluetoothA2DPSink a2dpSink;

int16_t audioLevel;

// lipSync task
void lipSync(void *args) {
    auto *ctx = (m5avatar::DriveContext *) args;
    m5avatar::Avatar *avatar = ctx->getAvatar();
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        int16_t level = audioLevel;
        if (level < LEVEL_MIN) {
            level = 0;
        } else if (level > LEVEL_MAX) {
            level = LEVEL_MAX;
        }
        float open = (float) level / LEVEL_MAX;
        avatar->setMouthOpenRatio(open);
        delay(10);
    }
#pragma clang diagnostic pop
}

// Stream reader callback
void onStream(const uint8_t *data, uint32_t len) {
    int16_t maxLevel;
    for (int i = 0; i < len; i += 2) {
        int16_t sample = *(data + 1) << 8 | *data;
        if (sample < 0) {
            sample = -sample;
        }
        if (maxLevel < sample) {
            maxLevel = sample;
        }
    }
    audioLevel = maxLevel;
}

void startA2DP() {
    // for M5Stack Core2
    i2s_pin_config_t pinConfig = {
            .bck_io_num = GPIO_NUM_12,
            .ws_io_num = GPIO_NUM_0,
            .data_out_num = GPIO_NUM_2,
            .data_in_num = I2S_PIN_NO_CHANGE,
    };
    a2dpSink.set_pin_config(pinConfig);
    a2dpSink.set_channels(I2S_CHANNEL_MONO);
    a2dpSink.set_stream_reader(onStream);
    a2dpSink.start(BT_DEVICE_NAME);
}

void startAvatar() {
    avatar.init();
    avatar.addTask(lipSync, "lipSync");
}

void setup() {
    auto cfg = m5::M5Unified::config();
    M5.begin(cfg);
    M5.Speaker.begin();

    startA2DP();
    startAvatar();
}

void loop() {
    delay(1000);
}
