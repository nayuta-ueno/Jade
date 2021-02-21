#include "input.h"
#include "gui.h"
#include "button.h"
#include "jade_assert.h"
#include "utils/malloc_ext.h"
// #include "speaker.h"

typedef void (*ButtonFunc_t)(void);


static Button_t* button[BUTTON_MAX];
static ButtonFunc_t button_func[BUTTON_MAX];

static void button_front_release(void)
{
    gui_front_click();
}

static void button_wheel_release(void)
{
    gui_wheel_click();
}

static void wheel_prev(void)
{
    gui_prev();
}

static void wheel_next(void)
{
    gui_next();
}


static void CheckTask(void *arg) {
    int index = BUTTON_FRONT;
    for (;;) {
        if (Button_WasPressed(button[index])) {
            //Speaker_Beep();
            ESP_LOGW("input", "press: %d", index);
            (*button_func[index])();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
        index++;
        if (index == BUTTON_MAX) {
            index = BUTTON_FRONT;
        }
    }
}


void input_init(void)
{
    Button_Init();

    // // (0, 240)-(320, 300)を4分割
    button[BUTTON_FRONT] = Button_Attach(0, 0, 100, 100);
    button[BUTTON_ENC] = Button_Attach(220, 0, 100, 100);
    button[BUTTON_PREV] = Button_Attach(0, 180, 160, 100);
    button[BUTTON_NEXT] = Button_Attach(160, 180, 160, 100);

    button_func[BUTTON_FRONT] = button_front_release;
    button_func[BUTTON_ENC] = button_wheel_release;
    button_func[BUTTON_PREV] = wheel_prev;
    button_func[BUTTON_NEXT] = wheel_next;

    xTaskCreatePinnedToCore(CheckTask, "Button", 2 * 1024, NULL, 1, NULL, 0);
}
