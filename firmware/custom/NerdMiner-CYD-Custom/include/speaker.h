#ifndef SPEAKER_H
#define SPEAKER_H

#include <Arduino.h>
#include <driver/ledc.h>

#define SPEAKER_PIN 26

class Speaker {
private:
  static bool initialized;
  static uint8_t volume_percent;
  static bool muted;
  static uint32_t tone_end_time;
  static bool tone_active;

  // LEDC configuration
  static const ledc_mode_t LEDC_MODE = LEDC_HIGH_SPEED_MODE;
  static const ledc_channel_t LEDC_CHANNEL = LEDC_CHANNEL_1;
  static const ledc_timer_t LEDC_TIMER = LEDC_TIMER_1;
  static const uint32_t LEDC_FREQ = 5000;
  static const ledc_timer_bit_t LEDC_RESOLUTION = LEDC_TIMER_8_BIT;

public:
  // Initialize speaker module
  static void init();

  // Play a tone (non-blocking)
  // freq_hz: frequency in Hz
  // duration_ms: duration in milliseconds
  static void playTone(int freq_hz, int duration_ms);

  // Stop tone immediately
  static void stop();

  // Set volume (0-100%)
  static void setVolume(uint8_t percent);

  // Get current volume
  static uint8_t getVolume();

  // Set mute state
  static void setMute(bool muted);

  // Get mute state
  static bool isMuted();

  // Update tone state (call periodically from loop)
  static void update();

  // Event helpers
  // Only post events; update() owns LEDC and never blocks the miner.
  static void playStartupChime();
  static void playShareBeep();
  static void playBlockChime();
  static void playDisconnectAlarm();
  static void playTestTone();
};

#endif // SPEAKER_H
