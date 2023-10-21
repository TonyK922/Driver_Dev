#ifndef _BUTTON_DRV_H
#define _BUTTON_DRV_H

#include "board_100ask_6ull.h"

void button_device_create(int minor);
void button_device_destroy(int minor);
void register_button_operations(struct button_operations *opr);

#endif /* _BUTTON_DRV_H */