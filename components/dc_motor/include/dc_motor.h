#include <stdio.h>
#include "driver/mcpwm.h"
#include "driver/gpio.h"

// Motor control setup
void motor_control_init(uint8_t ENA, uint8_t ENB, uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4);

void forward(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void backward(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void right_turn(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void sharp_right(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void left_turn(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void sharp_left(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);

void stop_motor(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed);