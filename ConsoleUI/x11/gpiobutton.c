#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define CHIP_NAME "gpiochip0" // GPIO chip name (default for Raspberry Pi)

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int gpio_pin;
    int value;
    bool state;
} GPIOButton;

int GPIOButtonInit(GPIOButton *button, int gpio_pin)
{
    button->gpio_pin = gpio_pin;
    // Open the GPIO chip
    button->chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!button->chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }

    // Get the GPIO line
    button->line = gpiod_chip_get_line(button->chip, gpio_pin);
    if (!button->line) {
        perror("Failed to get GPIO line");
        gpiod_chip_close(button->chip);
        return 1;
    }

    // Configure the line as input with a pull-up resistor
    if (gpiod_line_request_input_flags(button->line, "gpio_button", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        perror("Failed to request line as input");
        gpiod_chip_close(button->chip);
        return 1;
    }
#ifdef GPIOBUTTON_DEBUG
    printf("Waiting for button press on pin '%d'...\n", gpio_pin);
#endif

    return 0;
}

int GPIOButtonUpdate(GPIOButton *button)
{
    // Read the line value
    button->value = gpiod_line_get_value(button->line);
    if (button->value < 0) {
        perror("Failed to read line value");
        return 1;
    }

    if (button->value == 0 && button->state == 0) { // Button pressed (active low)
#ifdef GPIOBUTTON_DEBUG
        printf("Button %d pressed!\n", button->gpio_pin);
#endif
        button->state = 1;
    }
    else if (button->value != 0 && button->state != 0)
    {
#ifdef GPIOBUTTON_DEBUG
        printf("Button %d released!\n", button->gpio_pin);
#endif
        button->state = 0;
    }

    return 0;
}

void GPIOButtonCleanup(GPIOButton *button)
{
    gpiod_line_release(button->line);
    gpiod_chip_close(button->chip);
    button->gpio_pin = -1;
    button->state = -1;
}