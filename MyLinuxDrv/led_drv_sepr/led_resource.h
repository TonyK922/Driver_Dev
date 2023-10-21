#ifndef LED_RESOURCE_H
#define LED_RESOURCE_H

#define GROUP(x) (x >> 16)
#define PIN(x) (x & 0xffff)
#define GROUP_PIN(g,p) ((g<<16) | (p))

struct led_resource
{
    unsigned int pin;
};

struct led_resource *get_led_resource(void);

#endif // ! LED_RESOURCE_H

