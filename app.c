#include <stdio.h>
#include <pthread.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

#define TASK1_PRIORITY 0
#define TASK2_PRIORITY 0
#define TASK3_PRIORITY 1

#define BLACK "\033[30m" /* Black */
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */
#define DISABLE_CURSOR() printf("\e[?25l")
#define ENABLE_CURSOR() printf("\e[?25h")

#define clear() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

typedef struct
{
    int pos;
    char *color;
    int period_ms;
} st_led_param_t;

st_led_param_t green = {
    6,
    GREEN,
    250};
st_led_param_t red = {
    13,
    RED,
    100};

TaskHandle_t greenTask_hdlr, redTask_hdlr;

#include <termios.h>

static void prvTask_getChar(void *pvParameters)
{
    char key;
    int n;

    /* I need to change  the keyboard behavior to
    enable nonblock getchar */
    struct termios initial_settings,
        new_settings;

    tcgetattr(0, &initial_settings);

    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1;

    tcsetattr(0, TCSANOW, &new_settings);
    /* End of keyboard configuration */
    for (;;)
    {
        int stop = 0;
        key = getchar();

        switch (key)
        {
        case '*':
            xTaskNotify(greenTask_hdlr, 0UL, eSetValueWithOverwrite);
            xTaskNotify(redTask_hdlr, 0UL, eSetValueWithOverwrite);
            break;
        case '+':
            vTaskResume(redTask_hdlr);
            vTaskResume(greenTask_hdlr);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            xTaskNotify(redTask_hdlr, 01UL, eSetValueWithOverwrite);
            break;
        case 'k':
            stop = 1;
            break;
        }
        if (stop)
        {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    tcsetattr(0, TCSANOW, &initial_settings);
    ENABLE_CURSOR();
    exit(0);
    vTaskDelete(NULL);
}

static void prvTask_led_green(void *pvParameters)
{
    uint32_t notificationValue;
    uint8_t status_led = 0;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        if (!xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                led->period_ms / portTICK_PERIOD_MS))
        {
            if(status_led == 0){
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                status_led = 1;
            } else {
                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                status_led = 0;
            }
            fflush(stdout);
        } else {
            gotoxy(led->pos, 2);
            printf("%s ", BLACK);
            status_led = 0;

            vTaskSuspend(greenTask_hdlr);
        }
    }

    vTaskDelete(NULL);
}

static void prvTask_led_red(void *pvParameters)
{
    uint32_t notificationValue;
    uint8_t status_led = 0;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        if (xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                led->period_ms / portTICK_PERIOD_MS))
        {
            if(notificationValue == 1){ // Blink
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);

            } else {
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                fflush(stdout);

                vTaskSuspend(redTask_hdlr);

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
            }
        }
    }

    vTaskDelete(NULL);
}

void app_run(void)
{

    clear();
    DISABLE_CURSOR();
    printf(
        "╔═════════════════╗\n"
        "║                 ║\n"
        "╚═════════════════╝\n");

    xTaskCreate(prvTask_led_green, "LED_green", configMINIMAL_STACK_SIZE, &green, TASK1_PRIORITY, &greenTask_hdlr);
    xTaskCreate(prvTask_led_red, "LED_red", configMINIMAL_STACK_SIZE, &red, TASK2_PRIORITY, &redTask_hdlr);
    xTaskCreate(prvTask_getChar, "Get_key", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, NULL);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks      to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for (;;)
    {
    }
}