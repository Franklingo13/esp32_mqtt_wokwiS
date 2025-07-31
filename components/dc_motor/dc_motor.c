#include "dc_motor.h"

void motor_control_init(uint8_t ENA, uint8_t ENB, uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4) 
{
    // Set GPIOs for motor direction control as outputs
    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);

    // Initialize MCPWM for PWM control
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, ENA); // Motor A PWM
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, ENB); // Motor B PWM

    mcpwm_config_t pwm_config = {
        .frequency = 1000, // 1 kHz frequency
        .cmpr_a = 0,       // Initial duty cycle for PWM0A (Motor A)
        .cmpr_b = 0,       // Initial duty cycle for PWM0B (Motor B)
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); // Configure PWM0A & PWM0B
}

void forward(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);

    gpio_set_level(IN3, 1);
    gpio_set_level(IN4, 0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void backward(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 1);

    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 1);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void right_turn(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);

    gpio_set_level(IN3, 1);
    gpio_set_level(IN4, 0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void sharp_right(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 1);

    gpio_set_level(IN3, 1);
    gpio_set_level(IN4, 0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void left_turn(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);

    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void sharp_left(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 1);
    gpio_set_level(IN2, 0);

    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 1);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

void stop_motor(uint8_t IN1, uint8_t IN2, uint8_t IN3, uint8_t IN4,float speed) 
{
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);

    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
}
