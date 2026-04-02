#include "buddy.hpp"
#include "sprites.hpp"

#include <algorithm>
#include <random>

Buddy::Buddy(std::string name) : name_(std::move(name)), rng_(std::random_device{}()) {
    movement_.direction = random_walk_direction();
    movement_.active = false;
    start_idle_pause();
    time_until_next_sparkle_ = random_time_until_next_sparkle();
    resolve_activity();
    update_expression(0.0);
    update_effects(0.0);
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
    clamp_stats();
}

void Buddy::apply_command(Command cmd) {
    switch (cmd) {
    case Command::Feed:
        stats_.hunger -= 20.0;
        stats_.happiness += 5.0;
        activity_ = Activity::Eating;
        action_timer_ = 1.0;
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
    return movement_.active;
}

bool Buddy::is_sparkling() const noexcept {
    return effect_ == Effect::Sparkle;
}

int Buddy::x_position() const noexcept {
    return movement_.x_position;
}

std::vector<std::string> Buddy::current_frame() const {
    if (effect_ == Effect::Sparkle) {
        return select_effect_frame();
    }

    switch (activity_) {
    case Activity::Eating:
        return sprites::eat;
    case Activity::Sleeping:
        return sprites::sleep;
    case Activity::Walking:
        return select_walking_frame();
    case Activity::Idle:
    default:
        return select_idle_frame();
    }
}

std::vector<std::string> Buddy::select_idle_frame() const {
    if (expression_ == Expression::Sad) {
        return sprites::sad;
    }
    if (expression_ == Expression::Blinking) {
        return sprites::blink;
    }
    return (idle_frame_index_== 0) ? sprites::idle_0 : sprites::idle_1;
}

std::vector<std::string> Buddy::select_walking_frame() const {
    if (expression_ == Expression::Blinking) {
        return (movement_.walk_frame_index == 0) ? sprites::walk_0_blink : sprites::walk_1_blink;
    }

    return (movement_.walk_frame_index == 0) ? sprites::walk_0 : sprites::walk_1;
}

std::vector<std::string> Buddy::select_effect_frame() const {
    switch (sparkle_frame_index_) {
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

    if (action_timer_ > 0.0) {
        activity_ = Activity::Eating;
    } else if (sleeping_requested_ || stats_.energy < 15.0) {
        activity_ = Activity::Sleeping;
    } else if (movement_.active) {
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

    const int sprite_width = current_frame_width();
    const int walk_max_x = std::max(kWalkMinX_, kRenderInnerWidth_ - sprite_width);

    if (!movement_.active) {
        movement_.idle_pause_timer -= dt_seconds;
        if (movement_.idle_pause_timer <= 0.0) {
            start_walk_burst();
        } else {
            return;
        }
    }


    movement_.move_step_timer += dt_seconds;

    while (movement_.active && movement_.burst_steps_remaining > 0 && movement_.move_step_timer >= kMoveStepInterval_) {
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
        } else if (movement_.x_position <= kWalkMinX_) {
            movement_.x_position = kWalkMinX_;
            movement_.direction = 1;
            facing_ = Facing::Right;
        }

        if (movement_.burst_steps_remaining <= 0) {
            start_idle_pause();
        }
    }
}

void Buddy::update_effects(double dt_seconds) {
    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating || activity_ == Activity::Walking) {
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

int Buddy::current_frame_width() const noexcept {
    const auto frame = current_frame();
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

double Buddy::random_walk_duration() {
    std::uniform_real_distribution<double> duration_dist(2.0, 4.0);
    return duration_dist(rng_);
}

double Buddy::random_idle_duration() {
    std::uniform_real_distribution<double> duration_dist(1.0, 3.0);
    return duration_dist(rng_);
}

double Buddy::random_time_until_next_sparkle() {
    std::uniform_real_distribution<double> duration_dist(3.0, 6.0);
    return duration_dist(rng_);
}

int Buddy::random_short_shuffle_steps() {
    std::uniform_int_distribution<int> step_dist(1, 3);
    return step_dist(rng_);
}
int Buddy::random_long_walk_steps() {
    std::uniform_int_distribution<int> step_dist(6, 10);
    return step_dist(rng_);
}

bool Buddy::should_start_short_shuffle() {
    std::bernoulli_distribution dist(0.35);
    return dist(rng_);
}

bool Buddy::should_turn_around_early() {
    std::bernoulli_distribution dist (kEarlyTurnProbability_);
    return dist(rng_);
}

void Buddy::start_idle_pause() {
    movement_.active = false;
    movement_.burst_kind = MovementBurstKind::None;
    movement_.burst_steps_remaining = 0;
    movement_.walk_frame_index = 0;
    movement_.move_step_timer = 0.0;
    movement_.idle_pause_timer = random_idle_duration();
}

void Buddy::start_walk_burst() {
    movement_.active = true;
    movement_.move_step_timer = 0.0;
    movement_.walk_frame_index = 0;

    if (movement_.x_position <= kWalkMinX_) {
        movement_.direction = 1;
    } else {
        const int sprite_width = current_frame_width();
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
