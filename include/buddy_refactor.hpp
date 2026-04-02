#pragma once

#include <chrono>
#include <random>
#include <string>
#include <vector>

enum class Activity {
    Idle,
    Walking,
    Eating,
    Sleeping,
};

enum class Expression {
    Neutral,
    Blinking,
    Sad
};

enum class Effect {
    None,
    Sparkle
};

enum class Facing {
    Left,
    Right
};

struct MovementState {
    int x_position = 0;
    int direction = 1;          // -1 left, +1 right
    bool active = false;

    double move_step_timer = 0.0;
    double movement_state_timer = 0.0;
    std::size_t walk_frame_index = 0;
};

struct BuddyStats {
    double hunger = 20.0;       // 0 = full, 100 = starving
    double energy = 80.0;       // 0 = exhuasted, 100 = fully rested
    double happiness = 70;      // 0 = sad, 100 = very happy
};

class Buddy {
public:
    Buddy(std::string name);
    
    void update(double dt_seconds);
    void apply_command(Command cmd);

    const std::string& name() const noexcept;
    const BuddyStats& stats() const noexcept;
    Mode mode() const noexcept;
    bool running() const noexcept;
    bool is_walking() const noexcept;
    bool is_sparkling() const noexcept;
    int x_position() const noexcept;

    std::vector<std::string> current_frame() const;
    std::string mood_text() const;

private:
    void update_needs(double dt_seconds);
    void update_mode(double dt_seconds);
    void update_movement(double dt_seconds);
    void update_sparkles(double dt_seconds);
    int current_frame_width() const noexcept;
    int random_walk_direction();
    double random_walk_duration();
    double random_idle_duration();
    double random_time_until_next_sparkle();
    void clamp_stats();

private:
   std::string name_;
   BuddyStats stats_{};

   Mode mode_ = Mode::Idle;
   bool running_ = true;

   double age_seconds_ = 0.0;
   double animation_timer_ = 0.0;
   double time_until_next_blink_ = 4.0;
   double blink_duration_remaining_ = 0.0;
   double time_until_next_sparkle_ = 0.0;
   double sparkle_animation_timer_ = 0.0;
   double action_timer_ = 0.0;
   double move_step_timer_ = 0.0;
   double movement_state_timer_ = 0.0;

   std::size_t idle_frame_index_ = 0;
   std::size_t walk_frame_index_ = 0;
   int sparkle_frame_index_ = 0;
   int sparkle_steps_remaining_ = 0;
   int x_position_ = 0;
   int walk_direction_ = 1;
   bool is_walking_ = false;

   std::mt19937 rng_;

   static constexpr double kMoveStepInterval_ = 0.20;
   static constexpr double kSparkleFrameInterval_ = 0.4;
   static constexpr int kSparkleSequenceLength_ = 4;
   static constexpr int kWalkMinX_ = 0;
   static constexpr int kRenderInnerWidth_ = 37;

};
