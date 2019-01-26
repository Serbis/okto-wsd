#ifndef GPIO_H_
#define GPIO_H_

#define GPIO_MODE_OUT "out"
#define GPIO_MODE_IN "in"
#define GPIO_VALUE_SET "1"
#define GPIO_VALUE_RESET "0"

int GPIO_get_out(int pin);
int GPIO_get_in(int pin);
int GPIO_read(int pin);
int GPIO_write(int pin, const char *value);

#endif
