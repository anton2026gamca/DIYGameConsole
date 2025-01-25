#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"
#include "lib4x11.h"

#define BATTERY_SHOW_AVG_FROM 30
#define BATTERY_READING_UPDATE_TIME 1
struct Battery {
    int readings[BATTERY_SHOW_AVG_FROM];
    int value;
    bool is_charging;
    struct timespec last_read_time;
};
struct Battery battery;

int ReadBatteryPercentage()
{
    FILE *fp;
    char var[40];

    fp = popen("i2cget -y 1 0x57 0x2a", "r");
    if (fp == NULL)
        return -1;
    while (fgets(var, sizeof(var), fp) != NULL) { }
    pclose(fp);

    char *output;
    int percentage = strtol(var, &output, 16);

    return percentage;
}

bool BatteryIsCharging()
{
    FILE *fp;
    char var[40];

    fp = popen("i2cget -y 1 0x57 0x02", "r");
    if (fp == NULL)
        return false;
    while (fgets(var, sizeof(var), fp) != NULL) { }
    pclose(fp);

    char *output;
    int reg = strtol(var, &output, 16);
    bool is_charging = reg & 0b10000000;

    return is_charging;
}

void UpdateBattery()
{
    int percentage = ReadBatteryPercentage();
    struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    float diff = (time_now.tv_sec + time_now.tv_nsec/1E9) - (battery.last_read_time.tv_sec + battery.last_read_time.tv_nsec/1E9);
    if (diff > BATTERY_READING_UPDATE_TIME)
    {
        int sum = percentage;
        int count = 1;
        for (int i = 0; i < BATTERY_SHOW_AVG_FROM - 1; i++)
        {
            battery.readings[i] = battery.readings[i + 1];
            if (battery.readings[i] >= 0)
            {
                sum += battery.readings[i];
                count++;
            }
        }
        battery.readings[BATTERY_SHOW_AVG_FROM - 1] = percentage;
        battery.value = sum / count;
        battery.last_read_time.tv_sec += BATTERY_READING_UPDATE_TIME;
    }

    battery.is_charging = BatteryIsCharging();
}

bool IsPowerButtonDown()
{
    FILE *fp;
    char var[40];

    fp = popen("i2cget -y 1 0x57 0x02", "r");
    if (fp == NULL) return false;
    while (fgets(var, sizeof(var), fp) != NULL) { }
    pclose(fp);

    if (var[0] == 0) return false; // Check if string is empty

    char *output;
    int reg = strtol(var, &output, 16);
    int isPressed = reg & 0x01;

    return (bool)isPressed;
}