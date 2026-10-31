// Hardware-independent activity classifier (no Arduino deps -> host-testable).
// Input : 100 Hz IMU samples (acc m/s^2, gyro rad/s), device axes as in the recordings
//         (Y = up when upright, Z = up when lying face-up).
// Output: Sitting / Standing / Walking / Lying / SitStand (transition) / Moving.
#pragma once
#include <cmath>
#include <cstdint>

namespace act {

enum class Activity : uint8_t { Unknown, Sitting, Standing, Walking, Lying, SitStand, Moving, kCount };

inline const char* label(Activity a) {
  switch (a) {
    case Activity::Sitting:  return "SITTING";
    case Activity::Standing: return "STANDING";
    case Activity::Walking:  return "WALKING";
    case Activity::Lying:    return "LYING";
    case Activity::SitStand: return "SIT/STAND";
    case Activity::Moving:   return "MOVING";
    default:                 return "WAIT...";
  }
}

struct Sample { float ax, ay, az, gx, gy, gz; };

// Thresholds derived from the 5 recordings (see README).
struct Config {
  float lyingAz        = 7.0f;   // mean az above this => lying face-up
  float walkSd         = 0.5f;   // std of |acc| (m/s^2): dynamic motion
  float movingSd       = 0.12f;  // std of |acc|: small movement, not still
  float walkAxRange    = 2.3f;   // peak-to-peak ax: lateral sway => walking
  float sitPitchDeg    = 13.0f;  // atan2(az, ay) prior when no reference posture
  float postureDeltaAz = 0.6f;   // az change across a movement => posture flipped
};

class Classifier {
 public:
  static constexpr int kWindow = 100;  // 1 s @ 100 Hz
  static constexpr int kHop    = 50;   // decision every 0.5 s
  static constexpr int kVotes  = 5;    // majority vote over last 5 decisions

  explicit Classifier(const Config& cfg = Config{}) : cfg_(cfg) {}

  // Feed one sample. Returns true when a new decision was produced.
  bool push(const Sample& s) {
    buf_[head_] = s;
    head_ = (head_ + 1) % kWindow;
    if (filled_ < kWindow) ++filled_;
    if (++sinceHop_ < kHop || filled_ < kWindow) return false;
    sinceHop_ = 0;
    decide();
    return true;
  }

  Activity activity() const { return current_; }

 private:
  struct Feat { float ay, az, sd, axRange, gxAbs, gyAbs; };

  Feat features() const {
    float sumAy = 0, sumAz = 0, sumM = 0, sumGx = 0, sumGy = 0;
    float axMin = buf_[0].ax, axMax = buf_[0].ax;
    float mag[kWindow];
    for (int i = 0; i < kWindow; ++i) {
      const Sample& s = buf_[i];
      mag[i] = std::sqrt(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
      sumM += mag[i]; sumAy += s.ay; sumAz += s.az;
      sumGx += std::fabs(s.gx); sumGy += std::fabs(s.gy);
      if (s.ax < axMin) axMin = s.ax;
      if (s.ax > axMax) axMax = s.ax;
    }
    const float meanM = sumM / kWindow;
    float var = 0;
    for (int i = 0; i < kWindow; ++i) var += (mag[i] - meanM) * (mag[i] - meanM);
    return {sumAy / kWindow, sumAz / kWindow, std::sqrt(var / kWindow),
            axMax - axMin, sumGx / kWindow, sumGy / kWindow};
  }

  Activity rawClass(const Feat& f) {
    if (f.az > cfg_.lyingAz) { hasRef_ = false; moved_ = false; return Activity::Lying; }
    if (f.sd >= cfg_.walkSd) {
      moved_ = true;
      if (f.axRange >= cfg_.walkAxRange || f.gyAbs > f.gxAbs) {
        posture_ = Activity::Standing; hasRef_ = false;  // walking implies upright
        return Activity::Walking;
      }
      return Activity::SitStand;
    }
    if (f.sd >= cfg_.movingSd) { moved_ = true; return Activity::Moving; }

    // Still: resolve sitting vs standing.
    if (!hasRef_) {
      const float pitch = std::atan2(f.az, f.ay) * 57.2957795f;
      posture_ = pitch >= cfg_.sitPitchDeg ? Activity::Sitting : Activity::Standing;
    } else if (moved_ && std::fabs(f.az - refAz_) >= cfg_.postureDeltaAz) {
      posture_ = f.az > refAz_ ? Activity::Sitting : Activity::Standing;
    }
    refAz_ = f.az; hasRef_ = true; moved_ = false;
    return posture_;
  }

  void decide() {
    votes_[voteHead_] = rawClass(features());
    voteHead_ = (voteHead_ + 1) % kVotes;
    if (voteCount_ < kVotes) ++voteCount_;

    uint8_t tally[static_cast<int>(Activity::kCount)] = {0};
    for (int i = 0; i < voteCount_; ++i) ++tally[static_cast<int>(votes_[i])];
    // Majority wins; ties go to the most recent decision.
    Activity best = votes_[(voteHead_ + kVotes - 1) % kVotes];
    for (int i = 0; i < voteCount_; ++i) {
      if (tally[static_cast<int>(votes_[i])] > tally[static_cast<int>(best)]) best = votes_[i];
    }
    current_ = best;
  }

  Config cfg_;
  Sample buf_[kWindow] = {};
  int head_ = 0, filled_ = 0, sinceHop_ = 0;
  Activity votes_[kVotes] = {};
  int voteHead_ = 0, voteCount_ = 0;
  Activity current_ = Activity::Unknown;
  Activity posture_ = Activity::Standing;
  float refAz_ = 0;
  bool hasRef_ = false, moved_ = false;
};

}  // namespace act
