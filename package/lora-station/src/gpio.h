#ifndef __GPIO_H__
#define __GPIO_H__

int gpio_init(void);
void gpio_setdirecton(int pin, int dir);
int gpio_read(int pin);
void gpio_set(int pin, int value);
void gpio_free(void);

#endif

