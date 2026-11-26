#include "speaker.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

SerialStub Serial;
static uint32_t now_ms, duty, frequency, hardware_writes;
static std::vector<uint32_t> notes;
uint32_t millis() { return now_ms; }
int ledc_timer_config(const ledc_timer_config_t* c) {
  assert(c->timer_num == 1 && c->speed_mode == 0 && c->duty_resolution == 8);
  hardware_writes++; return 0;
}
int ledc_channel_config(const ledc_channel_config_t* c) {
  assert(c->gpio_num == 26 && c->channel == 1 && c->timer_sel == 1 && c->duty == 0);
  hardware_writes++; return 0;
}
int ledc_set_duty(int mode, int channel, uint32_t value) {
  assert(mode == 0 && channel == 1 && value <= 127);
  duty = value; hardware_writes++; return 0;
}
int ledc_update_duty(int mode, int channel) {
  assert(mode == 0 && channel == 1);
  hardware_writes++; return 0;
}
uint32_t ledc_set_freq(int mode, int timer, uint32_t value) {
  assert(mode == 0 && timer == 1);
  frequency = value; notes.push_back(value); hardware_writes++; return value;
}
static void tick(uint32_t delta) { now_ms += delta; Speaker::update(); }
static void reset() {
  Speaker::stop(); Speaker::update();
  Speaker::setMute(false); Speaker::setVolume(20);
  notes.clear();
}
int main() {
  Speaker::init();
  assert(duty == 0 && notes.empty());
  uint32_t count = hardware_writes;
  Speaker::playStartupChime(); Speaker::playDisconnectAlarm(); tick(1);
  assert(hardware_writes == count && duty == 0); // PANIC boot/reconnect stay silent

  Speaker::playShareBeep();
  assert(notes.empty()); // worker only posts: no peripheral access
  tick(0); assert(frequency == 1320 && duty > 0 && duty < 30);
  tick(64); assert(duty > 0);
  tick(1); assert(duty == 0);
  Speaker::playShareBeep(); tick(0); assert(duty == 0); // burst notification coalescing
  tick(1000); Speaker::playShareBeep(); tick(0); assert(duty > 0);

  Speaker::playBlockChime(); tick(0);
  assert(frequency == 1047); // block preempts share
  Speaker::playShareBeep(); tick(160); assert(duty == 0);
  tick(55); assert(frequency == 1319 && duty > 0);
  tick(160); assert(duty == 0);
  tick(55); assert(frequency == 1568 && duty > 0);
  tick(160); assert(duty == 0);
  tick(55); assert(frequency == 2093 && duty > 0);
  tick(360); assert(duty == 0);
  assert((notes == std::vector<uint32_t>{1320,1320,1047,1319,1568,2093}));

  reset();
  Speaker::playBlockChime(); Speaker::playShareBeep(); tick(0);
  assert(frequency == 1047); // pending block cannot be overwritten
  Speaker::setMute(true); tick(0); assert(duty == 0);
  Speaker::playShareBeep(); tick(0); Speaker::setMute(false); tick(1000);
  assert(duty == 0); // no stale playback after unmute

  reset();
  Speaker::setVolume(255); assert(Speaker::getVolume() == 100);
  Speaker::playTone(800, 10000); tick(0); assert(duty == 127);
  Speaker::setVolume(10); tick(1); assert(duty == 12);
  tick(1999); assert(duty == 0); // API duration capped at two seconds
  Speaker::playTone(0, 100); tick(0); assert(duty == 0);

  reset();
  now_ms = 0xFFFFFFF0u;
  Speaker::playTone(1000, 100); tick(0); assert(duty > 0);
  tick(99); assert(duty > 0);
  tick(1); assert(duty == 0); // millis wrap is safe
  Speaker::playBlockChime(); tick(0); assert(duty > 0);
  Speaker::stop(); tick(0); tick(500); assert(duty == 0); // cancels entire melody
  puts("speaker: quiet startup, confirmed-share chirp, prioritized block melody, mute/cancel, bounds and millis-wrap PASS");
}
