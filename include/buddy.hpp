#pragma once

#include <chrono>
#include <random>
#include <string>
#include <vector>

enum class Activity {
    Idle,
    Walking,
    Sleeping,
    Eating,
    Comforting
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
    Right, 
    Forward
};

enum class EyeDirection {
    Center,
    Left,
    Right
};

enum class CapVariant {
    Primary,
    Alternate
};

enum class BodyPose {
    Neutral,
    BreathingIn,
    BreathingOut,
    WallPause,
    Turning
};

enum class Stance {
    Sitting,
    Standing
};

enum class BodyWidthProfile {
    Full,
    Narrow
};

enum class SpriteLayerRole {
    None,
    CapCrown,
    CapBrim,
    Eyes,
    Blush,
    Body,
    Feet,
    Prop,
    Effect
};

struct SpriteCell {
    char glyph = ' ';
    SpriteLayerRole role = SpriteLayerRole::None;
};

enum class MovementBurstKind {
    None,
    ShortShuffle,
    LongWalk
};

enum class MovementPhase {
    IdlePause,
    WalkingBurst,
    WallSquishPause,
    TurningPause
};


struct MovementState {
    int x_position = 0;
    int direction = 1;          // -1 = left, +1 = right
    MovementPhase phase = MovementPhase::IdlePause;

    double move_step_timer = 0.0;
    double phase_timer = 0.0;
    std::size_t walk_frame_index = 0;
    int burst_steps_remaining = 0;
    MovementBurstKind burst_kind = MovementBurstKind::None;
    int pending_direction = 0;
    bool resume_walk_after_turn = false;
    int steps_until_random_turn = 0;
};

struct AppearanceState {
    Activity activity = Activity::Idle;
    Expression expression = Expression::Neutral;
    Effect effect = Effect::None;
    Facing facing = Facing::Right;
    MovementPhase movement_phase = MovementPhase::IdlePause;

    EyeDirection eye_direction = EyeDirection::Center;
    CapVariant cap_variant = CapVariant::Primary;
    bool blush_visible = false;
    BodyPose body_pose = BodyPose::Neutral;
    int idle_top_padding_rows = 1;
    std::size_t walk_frame_index = 0;
    std::size_t sparkle_frame_index = 0;
};
using SpriteRow = std::vector<SpriteCell>;

struct ComposedSprite {
    std::vector<SpriteRow> rows;
};

struct PoseState {
    AppearanceState appearance;
    Stance stance = Stance::Sitting;
    BodyWidthProfile body_width = BodyWidthProfile::Full;

    int top_padding_rows = 0;
    bool has_feet = false;
    std::size_t feet_frame_index = 0;
};

enum class Command {
    Feed,
    Pet,
    Sleep,
    Wake,
    Quit,
    Invalid
};

struct BuddyStats {
    double hunger = 20.0;       // 0 = full, 100 = starving
    double energy = 80.0;       // 0 = exhuasted, 100 = fully rested
    double happiness = 70;      // 0 = sad, 100 = very happy
};

struct BuddyRenderState {
    AppearanceState appearance;
    PoseState pose;
    BuddyStats stats;
    int x_position = 0;
    ComposedSprite sprite;
    std::vector<std::string> frame;
    std::string title_line;
    std::string mood_line;
    std::string hunger_line;
    std::string energy_line;
    std::string happiness_line;
};

class Buddy {
public:
    Buddy(std::string name);
    
    void update(double dt_seconds);
    void apply_command(Command cmd);

    const std::string& name() const noexcept;
    const BuddyStats& stats() const noexcept;
    Activity activity() const noexcept;
    Expression expression() const noexcept;
    Effect effect() const noexcept;
    Facing facing() const noexcept;
    bool running() const noexcept;
    bool is_walking() const noexcept;
    bool is_sparkling() const noexcept;
    int x_position() const noexcept;
    BuddyRenderState render_state() const;

private:
    void tick_action_timers(double dt_seconds);
    void update_needs(double dt_seconds);
    void resolve_activity();
    void update_expression(double dt_seconds);
    void update_effects(double dt_seconds);
    void update_micro_appearance(double dt_seconds);
    void update_movement(double dt_seconds);
    int body_frame_width() const noexcept;
    PoseState make_pose_state(const AppearanceState& appearance) const noexcept;
    ComposedSprite compose_sprite(const PoseState& pose) const;
    std::vector<std::string> flatten_sprite(const ComposedSprite& sprite) const;
    int frame_left_padding(const std::vector<std::string>& frame) const noexcept;
    int frame_right_padding(const std::vector<std::string>& frame) const noexcept;
    int frame_width(const std::vector<std::string>& frame) const noexcept;
    AppearanceState make_appearance_state() const noexcept;
    std::vector<std::string> current_frame() const;
    std::string mood_text() const;
    int random_walk_direction();
    double random_idle_duration();
    double random_turn_pause_duration();
    double random_time_until_next_blink();
    double random_blink_duration();
    double random_time_until_next_look();
    double random_look_duration();
    double random_eye_dart_duration();
    int random_idle_eye_dart_count();
    double random_time_until_next_wobble();
    double random_wobble_duration();
    EyeDirection random_look_direction();
    double random_time_until_next_sparkle();
    int random_short_shuffle_steps();
    int random_long_walk_steps();
    bool should_start_short_shuffle();
    bool should_turn_around_early();
    void start_idle_pause();
    void start_walk_burst();
    void start_wall_squish_pause(int new_direction, bool resume_walk_after_turn);
    void start_turn_pause(int new_direction, bool resume_walk_after_turn);
    void clamp_stats();

private:
   std::string name_;
   BuddyStats stats_{};

   Activity activity_ = Activity::Idle;
   Expression expression_ = Expression::Neutral;
   Effect effect_ = Effect::None;
   Facing facing_ = Facing::Right;
   EyeDirection eye_direction_ = EyeDirection::Center;
   CapVariant cap_variant_ = CapVariant::Primary;
   bool running_ = true;
   bool sleeping_requested_ = false;
   bool eating_requested_ = true;
   bool comfort_requested_ = true;

   double age_seconds_ = 0.0;
   double animation_timer_ = 0.0;
   double time_until_next_blink_ = 4.0;
   double comforting_timer_ = 0.0;
   double blink_duration_remaining_ = 0.0;
   double time_until_next_look_ = 0.0;
   double look_duration_remaining_ = 0.0;
   int idle_eye_dart_steps_remaining_ = 0.0;
   double time_until_next_wobble_ = 0.0;
   double wobble_duration_remaining_ = 0.0;
   double wobble_animation_timer_ = 0.0;
   double time_until_next_sparkle_ = 0.0;
   double sparkle_animation_timer_ = 0.0;
   double blush_duration_remaining_ = 0.0;
   double action_timer_ = 0.0;

   int sparkle_frame_index_ = 0;
   int sparkle_steps_remaining_ = 0;
   MovementState movement_{};

   std::mt19937 rng_;

   static constexpr double kMoveStepInterval_ = 0.20;
   static constexpr double kSparkleFrameInterval_ = 0.4;
   static constexpr double kWobbleFrameInterval_ = 0.5;
   static constexpr int kSparkleSequenceLength_ = 4;
   static constexpr double kWallSquishPauseDuration_ = 0.65;
   static constexpr int kVisibleWallGap_ = -1;
   static constexpr int kRenderInnerWidth_ = 70;
   static constexpr double kEarlyTurnProbability_ = 0.10;
   static constexpr double kShouldStartShortShuffle_ = 0.4;
   static constexpr double kAutoEatHungerThreshold_ = 85.0;
   static constexpr double kAutoEatRecoveredThreshold_ = 35.0;
   static constexpr double kAutoEatHungerReduction_ = 20.0;
   static constexpr double kAutoEatHappinessBoost_ = 5.0;
   static constexpr double kAutoEatDurationSeconds_ = 1.0;
   static constexpr double kAutoSleepEnergyThreshold_ = 15.0;
   static constexpr double kAutoComfortHappinessThreshold_ = 20.0;
   static constexpr double kAutoComfortRecoveredThreshold_ = 55.0;
   static constexpr double kAutoComfortHappinessBoost_ = 12.0;
   static constexpr double kAutoComfortDurationSeconds_ = 1.2;

};
