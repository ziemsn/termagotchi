#include "buddy.hpp"
#include "sprites.hpp"

#include <algorithm>
#include <random>

Buddy::Buddy(std::string name) : name_(std::move(name)), rng_(std::random_device{}()) {
    movement_.direction = random_walk_direction();
    movement_.phase = MovementPhase::IdlePause;
    start_idle_pause();
    time_until_next_look_ = random_time_until_next_look();
    time_until_next_sparkle_ = random_time_until_next_sparkle();
    resolve_activity();
    update_expression(0.0);
    update_effects(0.0);
    update_micro_appearance(0.0);
}

void Buddy::update(double dt_seconds) {
    age_seconds_ += dt_seconds;
    animation_timer_ += dt_seconds;

    tick_action_timers(dt_seconds);
    update_needs(dt_seconds);
    resolve_activity();
    update_expression(dt_seconds);
    update_movement(dt_seconds);
    resolve_activity();
    update_effects(dt_seconds);
    update_micro_appearance(dt_seconds);
    clamp_stats();
}

void Buddy::apply_command(Command cmd) {
    switch (cmd) {
    case Command::Feed:
        stats_.hunger -= kAutoEatHungerReduction_;
        stats_.happiness += kAutoEatHappinessBoost_;
        activity_ = Activity::Eating;
        action_timer_ = kAutoEatHungerThreshold_;
        break;

    case Command::Pet:
        stats_.happiness += 8.0;
        break;

    case Command::Sleep:
        sleeping_requested_ = true;
        activity_ = Activity::Sleeping;
        break;

    case Command::Wake:
        sleeping_requested_ = false;
        if (activity_ == Activity::Sleeping) {
            activity_ = Activity::Idle;
        }
        break;

    case Command::Quit:
        running_ = false;
        break;

    case Command::Invalid:
        break;

    }

    clamp_stats();
    resolve_activity();
    update_expression(0.0);
    update_effects(0.0);
    update_micro_appearance(0.0);
}

const std::string& Buddy::name() const noexcept {
    return name_;
}

const BuddyStats& Buddy::stats() const noexcept {
    return stats_;
}

Activity Buddy::activity() const noexcept {
    return activity_;
}

Expression Buddy::expression() const noexcept {
    return expression_;
}

Effect Buddy::effect() const noexcept {
    return effect_;
}

Facing Buddy::facing() const noexcept {
    return facing_;
}


bool Buddy::running() const noexcept {
    return running_;
}

bool Buddy::is_walking() const noexcept {
    return movement_.phase == MovementPhase::WalkingBurst;
}

bool Buddy::is_sparkling() const noexcept {
    return effect_ == Effect::Sparkle;
}

int Buddy::x_position() const noexcept {
    return movement_.x_position;
}

BuddyRenderState Buddy::render_state() const {
    BuddyRenderState state;
    state.name = name_;
    state.appearance = make_appearance_state();
    state.stats = stats_;
    state.x_position = movement_.x_position;
    state.mood_text = mood_text();
    state.frame = resolve_appearance_frame(state.appearance);
    return state;
}

std::vector<std::string> Buddy::current_frame() const {
    return resolve_appearance_frame(make_appearance_state());
}

AppearanceState Buddy::make_appearance_state() const noexcept {
    AppearanceState appearance;
    appearance.activity = activity_;
    appearance.expression = expression_;
    appearance.effect = effect_;
    appearance.facing = facing_;
    appearance.movement_phase = movement_.phase;
    appearance.eye_direction = eye_direction_;
    appearance.body_pose = (movement_.phase == MovementPhase::WallPause) ? BodyPose::WallPause : BodyPose::Neutral;
    appearance.walk_frame_index = movement_.walk_frame_index;
    appearance.sparkle_frame_index = sparkle_frame_index_;
    
    if (activity_ == Activity::Walking) {
        appearance.cap_variant = (movement_.walk_frame_index == 0) ? CapVariant::Primary : CapVariant::Alternate;
    } else {
        appearance.cap_variant = (idle_frame_index_ == 0) ? CapVariant::Primary : CapVariant::Alternate;
    }

    return appearance;
}

std::vector<std::string> Buddy::resolve_appearance_frame(const AppearanceState& appearance) const {
    if (appearance.effect == Effect::Sparkle) {
        return resolve_effect_frame(appearance);
    }

    return resolve_body_frame(appearance);
}

std::vector<std::string> Buddy::resolve_body_frame(const AppearanceState& appearance) const {

    if (movement_.phase == MovementPhase::WallPause) {
        return resolve_wall_pause_frame(appearance);
    }

    switch (appearance.activity) {
    case Activity::Eating:
        return sprites::eat;
    case Activity::Sleeping:
        return sprites::sleep;
    case Activity::Walking:
        return resolve_walking_frame(appearance);
    case Activity::Idle:
    default:
        return resolve_idle_frame(appearance);
    }
}

std::vector<std::string> Buddy::resolve_idle_frame(const AppearanceState& appearance) const {
    if (appearance.expression == Expression::Sad) {
        return sprites::sad;
    }
    if (appearance.expression == Expression::Blinking) {
        return sprites::blink;
    }

    if (appearance.eye_direction == EyeDirection::Left) {
        return (appearance.cap_variant == CapVariant::Primary) ? sprites::look_left_0 : sprites::look_left_1;
    }

    if (appearance.eye_direction == EyeDirection::Right) {
        return (appearance.cap_variant == CapVariant::Primary) ? sprites::look_right_0 : sprites::look_right_1;
    }

    return (appearance.cap_variant == CapVariant::Primary) ? sprites::idle_0 : sprites::idle_1;
}

std::vector<std::string> Buddy::resolve_walking_frame(const AppearanceState& appearance) const {
    if (appearance.expression == Expression::Blinking) {
        return (appearance.cap_variant == CapVariant::Primary) ? sprites::walk_0_blink : sprites::walk_1_blink;
    }

    return (appearance.cap_variant == CapVariant::Primary) ? sprites::walk_0 : sprites::walk_1;
}

std::vector<std::string> Buddy::resolve_wall_pause_frame(const AppearanceState& appearance) const {
    if (appearance.expression == Expression::Blinking) {
        return sprites::walk_0_blink;
    }

    return sprites::walk_0;
}

std::vector<std::string> Buddy::resolve_effect_frame(const AppearanceState& appearance) const {
    switch (appearance.sparkle_frame_index) {
    case 0:
        return sprites::sparkle_0;
    case 1:
        return sprites::sparkle_1;
    case 2:
        return sprites::sparkle_2;
    default:
        return sprites::blink_sparkle;
    }
}

std::string Buddy::mood_text() const {
    if (activity_ == Activity::Sleeping) {
        return "Sleeping";
    }
    if (activity_ == Activity::Eating) {
        return "Eating";
    }
    if (expression_ == Expression::Sad) {
        return "Sad";
    }
    if (movement_.phase == MovementPhase::WallPause) {
        return "Pausing";
    }
    if (effect_ == Effect::Sparkle) {
        return "Sparkling";
    }
    if (activity_ == Activity::Walking && expression_ == Expression::Blinking) {
        return "Walking, blinking";
    }
    if (activity_ == Activity::Walking) {
        return "Walking";
    }
    if (expression_ == Expression::Blinking) {
        return "Blinking";
    }
    return "Idle";
}

void Buddy::update_needs(double dt_seconds) {
    stats_.hunger += 0.05 * dt_seconds;

    if (activity_ == Activity::Sleeping) {
        stats_.energy += 6.0 * dt_seconds;
    } else {
        stats_.energy -= 0.2 * dt_seconds;
    }

    if (stats_.hunger > 70.0) {
        stats_.happiness -= 0.04 * dt_seconds;
    }
}

void Buddy::tick_action_timers(double dt_seconds) {
    if (action_timer_ > 0.0) {
        action_timer_ -= dt_seconds;
        if (action_timer_ < 0.0) {
            action_timer_ = 0.0;
        }
    }
}

void Buddy::resolve_activity() {
    if (activity_ == Activity::Sleeping && stats_.energy >= 85.0) {
        sleeping_requested_ = false;
    }

    if (stats_.energy < kAutoSleepEnergyThreshold_) {
        sleeping_requested_ = true;
    }

    const bool should_auto_sleep = sleeping_requested_;

    if (action_timer_ <= 0.0 && !should_auto_sleep && stats_.hunger >= kAutoEatHungerThreshold_) {
        stats_.hunger -= kAutoEatHungerReduction_;
        stats_.happiness += kAutoEatHappinessBoost_;
        clamp_stats();
        action_timer_ = kAutoEatDurationSeconds_;
    }
    
    if (action_timer_ > 0.0) {
        activity_ = Activity::Eating;
    } else if (should_auto_sleep) {
        activity_ = Activity::Sleeping;
    } else if (movement_.phase == MovementPhase::WalkingBurst) {
        activity_ = Activity::Walking;
    } else {
        activity_ = Activity::Idle;
    }

}

void Buddy::update_expression(double dt_seconds) {
    if (time_until_next_blink_ > 0.0) {
        time_until_next_blink_ -= dt_seconds;
        if (time_until_next_blink_ < 0.0) {
            time_until_next_blink_ = 0.0;
        }
    }

    if (blink_duration_remaining_ > 0.0) {
        blink_duration_remaining_ -= dt_seconds;
        if (blink_duration_remaining_ < 0.0) {
            blink_duration_remaining_ = 0.0;
        }
    }
    
    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating) {
        expression_ = Expression::Neutral;
    } else if (stats_.hunger > 85.0 || stats_.happiness < 20.0) {
        expression_ = Expression::Sad;
    } else if (blink_duration_remaining_ > 0.0) {
        expression_ = Expression::Blinking;
    } else if (time_until_next_blink_ <= 0.0) {
        blink_duration_remaining_ = 0.15;
        time_until_next_blink_ = 4.0;
        expression_ = Expression::Blinking;
    } else {
        expression_ = Expression::Neutral;
    }

    if (animation_timer_ >= 0.4) {
        animation_timer_ = 0.0;
        idle_frame_index_ = (idle_frame_index_ + 1) % 2;
    }
}

void Buddy::update_movement(double dt_seconds) {
    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating) {
        return;
    }

    const int sprite_width = body_frame_width();
    const int walk_max_x = std::max(kWalkMinX_, kRenderInnerWidth_ - sprite_width);

    if (movement_.phase == MovementPhase::IdlePause) {
        movement_.phase_timer -= dt_seconds;
        if (movement_.phase_timer <= 0.0) {
            start_walk_burst();
        } else {
            return;
        }
    } else if (movement_.phase == MovementPhase::WallPause) {
        movement_.phase_timer -= dt_seconds;
        if (movement_.phase_timer <= 0.0) {
            if (movement_.burst_steps_remaining > 0) {
                movement_.phase = MovementPhase::WalkingBurst;
                movement_.move_step_timer = 0.0;
            } else {
                start_idle_pause();
            }
        } else {
            return;
        }
    }

    movement_.move_step_timer += dt_seconds;

    while (movement_.phase == MovementPhase::WalkingBurst && movement_.burst_steps_remaining > 0 && movement_.move_step_timer >= kMoveStepInterval_) {
        movement_.move_step_timer -= kMoveStepInterval_;

        if (movement_.burst_kind == MovementBurstKind::LongWalk &&
                movement_.burst_steps_remaining > 2 &&
                movement_.x_position > (kWalkMinX_ + 2) &&
                movement_.x_position < (walk_max_x - 2) &&
                should_turn_around_early()) {
            movement_.direction *= -1;
            facing_ = (movement_.direction < 0) ? Facing::Left : Facing::Right;
        }
        movement_.x_position += movement_.direction;
        movement_.walk_frame_index = (movement_.walk_frame_index + 1) % 2;
        --movement_.burst_steps_remaining;

        if (movement_.x_position >= walk_max_x) {
            movement_.x_position = walk_max_x;
            movement_.direction = -1;
            facing_ = Facing::Left;
            if (movement_.burst_steps_remaining > 0) {
                start_wall_pause();
            } else {
                start_idle_pause();
            }
            break;
        } else if (movement_.x_position <= kWalkMinX_) {
            movement_.x_position = kWalkMinX_;
            movement_.direction = 1;
            facing_ = Facing::Right;
            if (movement_.burst_steps_remaining > 0) {
                start_wall_pause();
            } else {
                start_idle_pause();
            }
            break;
        }

        if (movement_.burst_steps_remaining <= 0) {
            start_idle_pause();
            break;
        }
    }
}

void Buddy::update_effects(double dt_seconds) {
    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating || activity_ == Activity::Walking || movement_.phase == MovementPhase::WallPause) {
        sparkle_steps_remaining_ = 0;
        sparkle_animation_timer_ = 0.0;
        effect_ = Effect::None;
        return;
    }

    if (sparkle_steps_remaining_ > 0) {
        sparkle_animation_timer_ += dt_seconds;
        effect_ = Effect::Sparkle;
        
        if (sparkle_animation_timer_ >= kSparkleFrameInterval_){
            sparkle_animation_timer_ -= kSparkleFrameInterval_;
            --sparkle_steps_remaining_;

            if (sparkle_steps_remaining_ > 0) {
                sparkle_frame_index_ = (sparkle_frame_index_ + 1) % kSparkleSequenceLength_;
            } else {
                sparkle_animation_timer_ = 0.0;
                time_until_next_sparkle_ = random_time_until_next_sparkle();
                effect_ = Effect::None;
            }
        }
        return;
    }

    effect_ = Effect::None;
    time_until_next_sparkle_ -= dt_seconds;
    if (time_until_next_sparkle_ <= 0.0) {
        sparkle_frame_index_ = 0;
        sparkle_steps_remaining_ = kSparkleSequenceLength_;
        sparkle_animation_timer_ = 0.0;
        effect_ = Effect::Sparkle;
    }
}

void Buddy::update_micro_appearance(double dt_seconds) {
    if (time_until_next_look_ > 0.0) {
        time_until_next_look_ -= dt_seconds;
        if (time_until_next_look_ < 0.0) {
            time_until_next_look_ = 0.0;
        }
    }

    if (look_duration_remaining_ > 0.0) {
        look_duration_remaining_ -= dt_seconds;
        if (look_duration_remaining_ < 0.0) {
            look_duration_remaining_ = 0.0;
        }
    }

    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating || activity_ == Activity::Walking ||
            expression_ == Expression::Blinking || expression_ == Expression::Sad || effect_ == Effect::Sparkle) {
        eye_direction_ = EyeDirection::Center;
        look_duration_remaining_ = 0.0;
        return;
    }

    if (movement_.phase == MovementPhase::WallPause) {
        eye_direction_ = EyeDirection::Center;
        look_duration_remaining_ = 0.0;
        return;
    }

    if (look_duration_remaining_ > 0.0) {
        return;
    }

    eye_direction_ = EyeDirection::Center;

    if (time_until_next_look_ <= 0.0) {
        eye_direction_ = random_look_direction();
        look_duration_remaining_ = random_look_duration();
        time_until_next_look_ = random_time_until_next_look();
    }
}

int Buddy::body_frame_width() const noexcept {
    return frame_width(resolve_body_frame(make_appearance_state()));
}

int Buddy::frame_width(const std::vector<std::string>& frame) const noexcept {
    int max_width = 0;

    for (const auto& row : frame) {
        const int row_width = static_cast<int>(row.size());
        if (row_width > max_width) {
            max_width = row_width;
        }
    }

    return max_width;
}

int Buddy::random_walk_direction() {
    std::uniform_int_distribution<int> dir_dist(0, 1);
    return dir_dist(rng_) == 0 ? -1 : 1;
}

double Buddy::random_idle_duration() {
    std::uniform_real_distribution<double> duration_dist(2.0, 4.0);
    return duration_dist(rng_);
}

double Buddy::random_wall_pause_duration() {
    std::uniform_real_distribution<double> duration_dist(0.50, 0.75);
    return duration_dist(rng_);
}

double Buddy::random_time_until_next_look() {
    std::uniform_real_distribution<double> duration_dist(2.0, 5.0);
    return duration_dist(rng_);
}

double Buddy::random_look_duration() {
    std::uniform_real_distribution<double> duration_dist(0.6, 1.2);
    return duration_dist(rng_);
}

EyeDirection Buddy::random_look_direction() {
    std::uniform_int_distribution<int> dir_dist(0, 1);
    return (dir_dist(rng_) == 0) ? EyeDirection::Left : EyeDirection::Right;
}

double Buddy::random_time_until_next_sparkle() {
    std::uniform_real_distribution<double> duration_dist(3.0, 6.0);
    return duration_dist(rng_);
}

int Buddy::random_short_shuffle_steps() {
    std::uniform_int_distribution<int> step_dist(3, 7);
    return step_dist(rng_);
}
int Buddy::random_long_walk_steps() {
    std::uniform_int_distribution<int> step_dist(8, 20);
    return step_dist(rng_);
}

bool Buddy::should_start_short_shuffle() {
    std::bernoulli_distribution dist(kShouldStartShortShuffle_);
    return dist(rng_);
}

bool Buddy::should_turn_around_early() {
    std::bernoulli_distribution dist (kEarlyTurnProbability_);
    return dist(rng_);
}

void Buddy::start_idle_pause() {
    movement_.phase = MovementPhase::IdlePause;
    movement_.burst_kind = MovementBurstKind::None;
    movement_.burst_steps_remaining = 0;
    movement_.walk_frame_index = 0;
    movement_.move_step_timer = 0.0;
    movement_.phase_timer = random_idle_duration();
}

void Buddy::start_wall_pause() {
    movement_.phase = MovementPhase::WallPause;
    movement_.phase_timer = random_wall_pause_duration();
    movement_.move_step_timer = 0.0;
    movement_.walk_frame_index =  0;
}

void Buddy::start_walk_burst() {
    movement_.phase = MovementPhase::WalkingBurst;
    movement_.phase_timer = 0.0;
    movement_.move_step_timer = 0.0;
    movement_.walk_frame_index = 0;

    if (movement_.x_position <= kWalkMinX_) {
        movement_.direction = 1;
    } else {
        const int sprite_width = body_frame_width();
        const int walk_max_x = std::max(kWalkMinX_, kRenderInnerWidth_ - sprite_width);
        if (movement_.x_position >= walk_max_x) {
            movement_.direction = -1;
        }
    }

    if (should_start_short_shuffle()) {
        movement_.burst_kind = MovementBurstKind::ShortShuffle;
        movement_.burst_steps_remaining = random_short_shuffle_steps();
    } else {
        movement_.burst_kind = MovementBurstKind::LongWalk;
        movement_.burst_steps_remaining = random_long_walk_steps();
    }

    facing_ = (movement_.direction < 0) ? Facing::Left : Facing::Right;
}

void Buddy::clamp_stats() {
    auto clamp_0_100 = [](double x) {
        return std::clamp(x, 0.0, 100.0);
    };

    stats_.hunger = clamp_0_100(stats_.hunger);
    stats_.energy = clamp_0_100(stats_.energy);
    stats_.happiness = clamp_0_100(stats_.happiness);
}
