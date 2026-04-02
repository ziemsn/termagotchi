#include "buddy.hpp"
#include "sprites.hpp"

#include <algorithm>
#include <random>

Buddy::Buddy(std::string name) : name_(std::move(name)), rng_(std::random_device{}()) {
    walk_direction_ = random_walk_direction();
    is_walking_ = false;
    movement_state_timer_ = random_idle_duration();
    time_until_next_sparkle_ = random_time_until_next_sparkle();
}

void Buddy::update(double dt_seconds) {
    age_seconds_ += dt_seconds;
    animation_timer_ += dt_seconds;

    if (action_timer_ > 0.0) {
        action_timer_ -= dt_seconds;
        if (action_timer_ < 0.0) {
            action_timer_ = 0.0;
        }
    }

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

    update_needs(dt_seconds);
    update_mode(dt_seconds);
    update_movement(dt_seconds);
    update_sparkles(dt_seconds);
    clamp_stats();
}

void Buddy::apply_command(Command cmd) {
    switch (cmd) {
    case Command::Feed:
        stats_.hunger -= 20.0;
        stats_.happiness += 5.0;
        mode_ = Mode::Eating;
        action_timer_ = 1.0;
        break;

    case Command::Pet:
        stats_.happiness += 8.0;
        break;

    case Command::Sleep:
        mode_ = Mode::Sleeping;
        break;

    case Command::Wake:
        if (mode_ == Mode::Sleeping) {
            mode_ = Mode::Idle;
        }
        break;

    case Command::Quit:
        running_ = false;
        break;

    case Command::Invalid:
        break;

    clamp_stats();
    }
}

const std::string& Buddy::name() const noexcept {
    return name_;
}

const BuddyStats& Buddy::stats() const noexcept {
    return stats_;
}

Mode Buddy::mode() const noexcept {
    return mode_;
}

bool Buddy::running() const noexcept {
    return running_;
}

bool Buddy::is_walking() const noexcept {
    return is_walking_;
}

bool Buddy::is_sparkling() const noexcept {
    return sparkle_steps_remaining_ > 0.0;
}


int Buddy::x_position() const noexcept {
    return x_position_;
}

std::vector<std::string> Buddy::current_frame() const {
    const bool sparkle_active = (sparkle_steps_remaining_ > 0.0) && !is_walking_;

    if (sparkle_active) {
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

    switch (mode_) {
    case Mode::Eating:
        return sprites::eat;
    case Mode::Sleeping:
        return sprites::sleep;
    case Mode::Blinking:
        if (is_walking_) {
            return (walk_frame_index_ == 0) ? sprites::walk_0_blink : sprites::walk_1_blink;
        }
        if (sparkle_active) {
            return sprites::blink_sparkle;
        }
        return sprites::blink;
    case Mode::Sad:
        if (is_walking_) {
            return (walk_frame_index_ == 0) ? sprites::walk_0_blink : sprites::walk_1_blink;
        }
        return sprites::sad;
    case Mode::Idle:
    default:
        if (is_walking_) {
            return (walk_frame_index_ == 0) ? sprites::walk_0 : sprites::walk_1;
        }
        return (idle_frame_index_ == 0) ? sprites::idle_0 : sprites::idle_1;
    }
}

std::string Buddy::mood_text() const {
    if (mode_ == Mode::Sleeping) {
        return "Sleeping";
    }
    if (mode_ == Mode::Eating) {
        return "Eating";
    }
    if (mode_ == Mode::Sad) {
        return "Sad";
    }
    if (!is_walking_ && sparkle_steps_remaining_ > 0) {
        return "Sparkling";
    }
    if (is_walking_ && mode_ == Mode::Blinking) {
        return "Walking, blinking";
    }
    if (is_walking_) {
        return "Walking";
    }
    if (mode_ == Mode::Blinking) {
        return "Blinking";
    }
    return "Idle";
}

void Buddy::update_needs(double dt_seconds) {
    stats_.hunger += 0.05 * dt_seconds;

    if (mode_ == Mode::Sleeping) {
        stats_.energy += 12.0 * dt_seconds;
    } else {
        stats_.energy -= 0.02 * dt_seconds;
    }

    if (stats_.hunger > 70.0) {
        stats_.happiness -= 0.04 * dt_seconds;
    }
}

void Buddy::update_mode(double dt_seconds) {
    (void)dt_seconds;

    if (mode_ == Mode::Sleeping) {
        if (stats_.energy >= 95.0) {
            mode_ = Mode::Idle;
        }
        return;
    }

    if (mode_ == Mode::Eating) {
        if (action_timer_ <= 0.0) {
            mode_ = Mode::Idle;
        }
        return;
    }

    if (stats_.hunger > 85.0 || stats_.happiness < 20.0) {
        mode_ = Mode::Sad;
        return;
    }

    if (blink_duration_remaining_ > 0.0) {
        mode_ = Mode::Blinking;
        return;
    }

    if (time_until_next_blink_ <= 0.0) {
        blink_duration_remaining_ = 0.25;
        time_until_next_blink_ = 4.0;
        mode_ = Mode::Blinking;
        return;
    }

    mode_ = Mode::Idle;

    if (animation_timer_ >= 0.4) {
        animation_timer_ = 0.0;
        idle_frame_index_ = (idle_frame_index_ + 1) % 2;
    }
}

void Buddy::update_movement(double dt_seconds) {
    if (mode_ == Mode::Sleeping || mode_ == Mode::Eating) {
        return;
    }

    movement_state_timer_ -= dt_seconds;
    if (movement_state_timer_ <= 0.0) {
        if (is_walking_) {
            is_walking_ = false;
            walk_frame_index_ = 0;
            movement_state_timer_ = random_idle_duration();
        } else {
            is_walking_ = true;
            walk_direction_ = random_walk_direction();
            walk_frame_index_ = 0;
            move_step_timer_ = 0.0;
            movement_state_timer_ = random_walk_duration();
        }
    }

    if (!is_walking_) {
        return;
    }

    const int sprite_width = current_frame_width();
    const int walk_max_x = std::max(kWalkMinX_, kRenderInnerWidth_ - sprite_width);

    move_step_timer_ += dt_seconds;

    while (move_step_timer_ >= kMoveStepInterval_) {
        move_step_timer_ -= kMoveStepInterval_;
        x_position_ += walk_direction_;
        walk_frame_index_ = (walk_frame_index_ + 1) % 2;

        if (x_position_ >= walk_max_x) {
            x_position_ = walk_max_x;
            walk_direction_ = -1;
        } else if (x_position_ <= kWalkMinX_) {
            x_position_ = kWalkMinX_;
            walk_direction_ = 1;
        }
    }
}

void Buddy::update_sparkles(double dt_seconds) {
    if (mode_ == Mode::Sleeping || mode_ == Mode::Eating || is_walking_) {
        sparkle_steps_remaining_ = 0;
        sparkle_animation_timer_ = 0.0;
        return;
    }

    if (sparkle_steps_remaining_ > 0) {
        sparkle_animation_timer_ += dt_seconds;
        
        if (sparkle_animation_timer_ >= kSparkleFrameInterval_){
            sparkle_animation_timer_ -= kSparkleFrameInterval_;
            --sparkle_steps_remaining_;

            if (sparkle_steps_remaining_ > 0) {
                sparkle_frame_index_ = (sparkle_frame_index_ + 1) % kSparkleSequenceLength_;
            } else {
                sparkle_animation_timer_ = 0.0;
                time_until_next_sparkle_ = random_time_until_next_sparkle();
            }
        }
        return;
    }

    time_until_next_sparkle_ -= dt_seconds;
    if (time_until_next_sparkle_ <= 0.0) {
        sparkle_frame_index_ = 0;
        sparkle_steps_remaining_ = kSparkleSequenceLength_;
        sparkle_animation_timer_ = 0.0;
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

void Buddy::clamp_stats() {
    auto clamp_0_100 = [](double x) {
        return std::clamp(x, 0.0, 100.0);
    };

    stats_.hunger = clamp_0_100(stats_.hunger);
    stats_.energy = clamp_0_100(stats_.energy);
    stats_.happiness = clamp_0_100(stats_.happiness);
}
