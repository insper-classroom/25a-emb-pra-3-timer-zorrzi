#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define TRIGGER_PIN 16
#define ECHO_PIN 17

volatile absolute_time_t echo_start;
volatile absolute_time_t echo_end;

volatile bool fim_echo = false;
volatile bool timeout_error = false;

alarm_id_t timeout_alarm;



const char* months[] = {"January", "February", "March", "April", "May", "June", 
                        "July", "August", "September", "October", "November", "December"};

int64_t alarm_timeout_callback(alarm_id_t id, void *user_data) {
    timeout_error = true;
    return 0;
}

void echo_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        echo_start = get_absolute_time();
    }
    if (events & GPIO_IRQ_EDGE_FALL) {
        echo_end = get_absolute_time();
        fim_echo = true;
    }
}

void trigger_pulse() {
    gpio_put(TRIGGER_PIN, 1);
    sleep_us(10);
    gpio_put(TRIGGER_PIN, 0);
    timeout_error = false;
}

int main() {
    stdio_init_all();

    rtc_init();
    datetime_t dt = {
        .year  = 2024,
        .month = 3,
        .day   = 17,
        .dotw  = 0,
        .hour  = 11,
        .min   = 0,
        .sec   = 0
    };
    rtc_set_datetime(&dt);

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_put(TRIGGER_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);

    bool rodando = false;

    printf("Digite 's' para iniciar ou 'n' para parar:\n");

    while (true) {
        int c = getchar_timeout_us(0);

        if (c == 's') {
            rodando = true;
            printf("Sistema iniciado\n");
        } else if (c == 'n') {
            rodando = false;
            printf("Sistema parado\n");
        }

        if (rodando) {
            trigger_pulse();
            timeout_alarm = add_alarm_in_ms(30, alarm_timeout_callback, NULL, false);
            sleep_ms(60);

            datetime_t t;
            rtc_get_datetime(&t);

            if (fim_echo) {
                cancel_alarm(timeout_alarm);
                int64_t dt_us = absolute_time_diff_us(echo_start, echo_end);
                float distancia_cm = (dt_us * 0.0343f) / 2.0f;

                printf("%d %02d %s %d %02d:%02d:%02d - %.2f cm\n", 
                    t.dotw,
                    t.day, 
                    months[t.month - 1], 
                    t.year, 
                    t.hour, 
                    t.min, 
                    t.sec, 
                    distancia_cm);

                fim_echo = false;

            } else if (timeout_error) {
                printf("Falha\n");
            }

            sleep_ms(500);
        } else {
            sleep_ms(100);
        }
    }
}
