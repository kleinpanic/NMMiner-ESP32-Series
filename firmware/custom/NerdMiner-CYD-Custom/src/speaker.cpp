#include "speaker.h"
#include <atomic>

namespace {
// Higher priority events replace pending/playing lower priority events. The
// single slot deliberately coalesces bursts of accepted-share notifications.
enum Event : uint8_t { None, Share, Startup, Disconnect, Test, Manual, Block };
struct Note { uint16_t hz, ms; };
const Note share[] = {{1320, 65}};
const Note startup[] = {{1000, 90}, {0, 45}, {1200, 100}};
const Note disconnect[] = {{700, 80}, {0, 70}, {700, 80}};
const Note test[] = {{800, 250}};
const Note block[] = {{1047, 160}, {0, 55}, {1319, 160}, {0, 55},
                      {1568, 160}, {0, 55}, {2093, 360}};
std::atomic<uint8_t> pending{None};
std::atomic<bool> cancel{false};
std::atomic<uint32_t> manual_tone{0}; // low 16 bits Hz, high 16 bits duration
Event active = None;
const Note* sequence;
uint8_t note_count, note_index;
uint32_t note_deadline, last_share_at;
bool have_share;
uint8_t applied_volume;

void post(Event event) {
  uint8_t previous = pending.load(std::memory_order_relaxed);
  while (previous < event &&
         !pending.compare_exchange_weak(previous, event, std::memory_order_release,
                                        std::memory_order_relaxed)) {}
}
uint32_t dutyFor(uint8_t volume) {
  // 50% is the loudest useful square wave; 100% would be constant DC.
  return (127u * volume) / 100u;
}
bool expired(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}
} // namespace

bool Speaker::initialized = false;
uint8_t Speaker::volume_percent = 20;
bool Speaker::muted = false;
uint32_t Speaker::tone_end_time = 0;
bool Speaker::tone_active = false;

void Speaker::init() {
  if (initialized) return;
  ledc_timer_config_t timer = {
    .speed_mode = LEDC_MODE,
    .duty_resolution = LEDC_RESOLUTION,
    .timer_num = LEDC_TIMER,
    .freq_hz = LEDC_FREQ,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer);
  ledc_channel_config_t channel = {
    .gpio_num = SPEAKER_PIN,
    .speed_mode = LEDC_MODE,
    .channel = LEDC_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&channel);
  ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
  ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
  initialized = true;
  Serial.println("[SPEAKER] Reward audio ready; silent until an event");
}
void Speaker::playTone(int freq_hz, int duration_ms) {
  // Bound the test API: it cannot leave an unbounded tone running.
  if (freq_hz < 100 || freq_hz > 10000 || duration_ms <= 0) { stop(); return; }
  if (duration_ms > 2000) duration_ms = 2000;
  manual_tone.store((uint32_t(duration_ms) << 16) | uint16_t(freq_hz), std::memory_order_relaxed);
  post(Manual);
}
void Speaker::stop() {
  pending.store(None, std::memory_order_relaxed);
  cancel.store(true, std::memory_order_release);
}
void Speaker::setVolume(uint8_t percent) { volume_percent = percent > 100 ? 100 : percent; }
uint8_t Speaker::getVolume() { return volume_percent; }
void Speaker::setMute(bool value) { muted = value; if (muted) stop(); }
bool Speaker::isMuted() { return muted; }

void Speaker::update() {
  if (!initialized) return;
  uint32_t now = millis();
  bool cancelled = cancel.exchange(false, std::memory_order_acquire);
  Event event = static_cast<Event>(pending.exchange(None, std::memory_order_acquire));
  if (muted || cancelled) {
    active = None;
    tone_active = false;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    // Mute/stop discards queued tones rather than replaying on unmute.
    return;
  }
  if (event == Share && have_share && now - last_share_at < 1000) event = None;
  if (event > active) {
    active = event;
    note_index = 0;
    switch (event) {
      case Share:
        sequence = share; note_count = sizeof(share) / sizeof(Note);
        last_share_at = now; have_share = true;
        break;
      case Startup: sequence = startup; note_count = sizeof(startup) / sizeof(Note); break;
      case Disconnect: sequence = disconnect; note_count = sizeof(disconnect) / sizeof(Note); break;
      case Test: sequence = test; note_count = sizeof(test) / sizeof(Note); break;
      case Manual: {
        static Note manual;
        uint32_t packed = manual_tone.load(std::memory_order_relaxed);
        manual = {uint16_t(packed), uint16_t(packed >> 16)};
        sequence = &manual; note_count = 1;
        break;
      }
      case Block: sequence = block; note_count = sizeof(block) / sizeof(Note); break;
      default: break;
    }
    note_deadline = now;
    tone_active = false;
  }
  if (active != None && expired(now, note_deadline)) {
    if (tone_active) note_index++;
    if (note_index >= note_count) {
      active = None;
      tone_active = false;
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
      ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
      return;
    }
    const Note& note = sequence[note_index];
    if (note.hz) ledc_set_freq(LEDC_MODE, LEDC_TIMER, note.hz);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, note.hz ? dutyFor(volume_percent) : 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    applied_volume = volume_percent;
    note_deadline = now + note.ms;
    tone_end_time = note_deadline;
    tone_active = true;
  } else if (active != None && applied_volume != volume_percent) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, sequence[note_index].hz ? dutyFor(volume_percent) : 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    applied_volume = volume_percent;
  }
}
void Speaker::playStartupChime() {
#ifndef PANICMINER_UI
  post(Startup);
#endif
}
void Speaker::playShareBeep() { post(Share); }
void Speaker::playBlockChime() { post(Block); }
void Speaker::playDisconnectAlarm() {
#ifndef PANICMINER_UI
  post(Disconnect);
#endif
}
void Speaker::playTestTone() { post(Test); }
