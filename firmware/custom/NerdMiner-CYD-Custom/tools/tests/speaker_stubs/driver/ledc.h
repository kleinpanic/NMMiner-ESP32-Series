#pragma once
#include <stdint.h>
using ledc_mode_t = int;
using ledc_channel_t = int;
using ledc_timer_t = int;
using ledc_timer_bit_t = int;
constexpr int LEDC_HIGH_SPEED_MODE = 0, LEDC_CHANNEL_1 = 1, LEDC_TIMER_1 = 1;
constexpr int LEDC_TIMER_8_BIT = 8, LEDC_AUTO_CLK = 0, LEDC_INTR_DISABLE = 0;
struct ledc_timer_config_t {
  int speed_mode, duty_resolution, timer_num;
  uint32_t freq_hz;
  int clk_cfg;
};
struct ledc_channel_config_t {
  int gpio_num, speed_mode, channel, intr_type, timer_sel;
  uint32_t duty;
  int hpoint;
};
int ledc_timer_config(const ledc_timer_config_t*);
int ledc_channel_config(const ledc_channel_config_t*);
int ledc_set_duty(int, int, uint32_t);
int ledc_update_duty(int, int);
uint32_t ledc_set_freq(int, int, uint32_t);
