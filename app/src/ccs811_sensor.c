#include <stdbool.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <zephyr/kernel.h>

#include "ccs811_sensor.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

static bool app_fw_2;
static cc811_sensor_cb display_cb;

#define CONFIG_APP_MONITOR_BASELINE
static const char *now_str(void) {
    static char buf[16]; /* ...HH:MM:SS.MMM */
    uint32_t now = k_uptime_get_32();
    unsigned int ms = now % MSEC_PER_SEC;
    unsigned int s;
    unsigned int min;
    unsigned int h;

    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
    return buf;
}

static int do_fetch(const struct device *dev) {
    struct sensor_value voltage, current;
    struct ccs811_result result;
    int rc = 0;
    int baseline = -1;

#ifdef CONFIG_APP_MONITOR_BASELINE
    rc = ccs811_baseline_fetch(dev);
    if (rc >= 0) {
        baseline = rc;
        rc = 0;
    }
#endif
    if (rc == 0) {
        rc = sensor_sample_fetch(dev);
    }
    if (rc == 0) {
        const struct ccs811_result_type *rp = ccs811_result(dev);

        sensor_channel_get(dev, SENSOR_CHAN_CO2, &result.co2);
        sensor_channel_get(dev, SENSOR_CHAN_VOC, &result.tvoc);
        sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &voltage);
        sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &current);
        printk("\n[%s]: CCS811: %u ppm eCO2; %u ppb eTVOC\n", now_str(),
               result.co2.val1, result.tvoc.val1);
        printk("Voltage: %d.%06dV; Current: %d.%06dA\n", voltage.val1,
               voltage.val2, current.val1, current.val2);
#ifdef CONFIG_APP_MONITOR_BASELINE
        printk("BASELINE %04x\n", baseline);
#endif
        if (app_fw_2 && !(rp->status & CCS811_STATUS_DATA_READY)) {
            printk("STALE DATA\n");
        }

        if (rp->status & CCS811_STATUS_ERROR) {
            printk("ERROR: %02x\n", rp->error);
        } else {
            display_cb(&result);
        }
    }
    return rc;
}

static void trigger_handler(const struct device *dev,
                            const struct sensor_trigger *trig) {
    int rc = do_fetch(dev);

    if (rc == 0) {
        printk("Triggered fetch got %d\n", rc);
    } else if (-EAGAIN == rc) {
        printk("Triggered fetch got stale data\n");
    } else {
        printk("Triggered fetch failed: %d\n", rc);
    }
}

int ccs811_sensor_init(cc811_sensor_cb cb) {
    const struct device *const dev = DEVICE_DT_GET_ONE(ams_ccs811);
    struct ccs811_configver_type cfgver;

    display_cb = cb;

    int rc = device_init(dev);
    if (rc) {
        printk("Device %s is not inited, rc = %d\n", dev->name, rc);
        return rc;
    }
    if (!device_is_ready(dev)) {
        printk("Device %s is not ready\n", dev->name);
        return -1;
    }

    printk("device is %p, name is %s\n", dev, dev->name);

    rc = ccs811_configver_fetch(dev, &cfgver);
    if (rc == 0) {
        printk("HW %02x; FW Boot %04x App %04x ; mode %02x\n",
               cfgver.hw_version, cfgver.fw_boot_version, cfgver.fw_app_version,
               cfgver.mode);
        app_fw_2 = (cfgver.fw_app_version >> 8) > 0x11;
    }
    struct sensor_trigger trig = {0};
    printk("Triggering on data ready\n");
    trig.type = SENSOR_TRIG_DATA_READY;
    trig.chan = SENSOR_CHAN_ALL;
    if (rc == 0) {
        rc = sensor_trigger_set(dev, &trig, trigger_handler);
    }

    return rc;
}
