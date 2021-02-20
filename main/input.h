#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <sdkconfig.h>
#include <stdbool.h>

enum {
    BUTTON_FRONT,
    BUTTON_ENC,
    BUTTON_PREV,
    BUTTON_NEXT,
    BUTTON_MAX
};

void input_init(void);

#endif /* BUTTONS_H_ */
