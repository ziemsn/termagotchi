#include "buddy.hpp"
#include "sprites.hpp"

#include <algorithm>
#include <random>

namespace {

std::string make_bar(double value, int width = 10) {
    int filled = static_cast<int>((value / 100) * width);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    return "[" + std::string(filled, '#') + std::string(width - filled, '-') + "]";
}

std::string make_stat_line(const std::string& label, double value) {
    return label + make_bar(value);
}

int legacy_cap_row(const AppearanceState& appearance) {
    if (appearance.effect == Effect::Sparkle) {
        return 2;
    }

    if (appearance.activity == Activity::Walking | appearance.movement_phase == MovementPhase::WallPause) {
        return 1;
    }

    return 2;
}

int legacy_eye_row(const AppearanceState& appearance) {
    if (appearance.effect == Effect::Sparkle) {
        return 4;
    }
    
    if (appearance.activity == Activity::Walking | appearance.movement_phase == MovementPhase::WallPause) {
        return 3;
    }

    return 4;
}

int legacy_feet_row(const AppearanceState& appearance) {
    if (appearance.activity == Activity::Walking | appearance.movement_phase == MovementPhase::WallPause) {
        return 6;
    }

    return -1;
}

SpriteLayerRole classify_legacy_cell(const AppearanceState& appearance, std::size_t row, char glyph) {
    if (glyph == ' ') return SpriteLayerRole::None;
    if (appearance.effect == Effect::Sparkle && row < 2 && (glyph == '*' || glyph == '+' || glyph == '.')) return SpriteLayerRole::Effect;
    if (static_cast<int>(row) == legacy_feet_row(appearance) && (glyph == '/' || glyph == '\\')) return SpriteLayerRole::Feed;
    if (static_cast<int>(row) == legacy_eye_row(appearance) && (glyph == 'o' || glyph == 'O' || glyph == '-')) return SpriteLayerRole::Eyes;
    if (static_cast<int>(row) == legacy_cap_row(appearance)) return SpriteLayerRole::Cap;
    return SpriteLayerRole::Body;
}

ComposedSprite make_blank_sprite_like(const ComposedSprite& source) {
    ComposedSprite blank;
    blank.rows.reserve(source.rows.size());

    for (const auto& source_row : source.rows) {
        SpriteRow blank_row;
        blank_row.resize(source_row.size());
        blank.rows.push_back(std::move(blank_row));
    }

    return blank;
}

ComposedSprite extract_role_layer(const ComposedSprite& source, SpriteLayerRole role) {
    ComposedSprite layer = make_blank_sprite_like(source);

    for (std::size_t row = 0; row < source.rows.size(); ++row) {
        for (std::size_t col = 0; col < source.rows[row].size(); ++col) {
            const SpriteCell& cell = source.rows[row][col];
            if (cell.role == role) {
                layer.rows[row][col] = cell;
            }
        }
    }
    
    return layer;
}

void overlay_composed_sprite(ComposedSprite& destination, const ComposedSprite& source) {
    const std::size_t row_count = std::min(destination.rows.size(), source.rows.size());

    for (std::size_t row = 0; row < row_count; ++row) {
        const std::size_t col_count = std::min(destination.rows[row].size(), source.rows[row].size());

        for (std::size_t col = 0; col < col_count; ++col) {
            const SpriteCell& source_cell = source.rows[row][col];
            if (source_cell.glyph == ' ' || source_cell.role == SpriteLayerRole::None) {
                continue;
            }

            destination.rows[row][col] = source_cell;
        }
    }
}

} // namespace

Buddy::Buddy(std::string name) : name_(std::move(name)), rng_(std::random_device{}()) {
    movement_.direction = random_walk_direction();
    movement_.phase = MovementPhase::IdlePause;
    start_idle_pause();
    time_until_next_look_ = random_time_until_next_look();
    time_until_next_wobble_ = random_time_until_next_wobble();
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
    state.appearance = make_appearance_state();
    state.pose = make_pose_state(state.appearance);
    state.stats = stats_;
    state.x_position = movement_.x_position;
    state.sprite = compose_sprite(state.pose);
    state.frame = flatten_sprite(state.sprite);
    state.title_line = "Buddy: " + name_;
    state.mood_line = "Mood: " + mood_text();
    state.hunger_line = make_stat_line("Hunger:      ", stats_.hunger);
    state.energy_line = make_stat_line("Energy:      ", stats_.energy);
    state.happiness_line = make_stat_line("Happiness:   ", stats_.happiness);
    return state;
}

PoseState Buddy::make_pose_state(const AppearanceState& appearance) const noexcept {
    PoseState pose;
    pose.appearance = appearance;

    if (appearance.activity == Activity::Walking || appearance.movement_phase == MovementPhase::WallPause) {
        pose.stance = Stance::Standing;
        pose.body_width = BodyWidthProfile::Narrow;
        pose.top_padding_rows = 0;
        pose.has_feet = true;
        pose.feet_Frame_index = appearance.walk_frame_index;
    } else {
        pose.stance = Stance::Sitting;
        pose.body_width = BodyWidthProfile::Full;
        pose.top_padding_rows = 1;
        pose.has_feet = false;
        pose.feet_Frame_index = 0;
    }

    return pose;
}

ComposedSprite Buddy::compose_sprite(const PoseState& pose) const {
    const ComposedSprite legacy = compose_legacy_sprite(pose.appearance);
    ComposedSprite composed = make_blank_sprite_like(legacy);

    const ComposedSprite body_layer = extract_role_layer(legacy, SpriteLayerRole::Body);
    const ComposedSprite cap_layer = extract_role_layer(legacy, SpriteLayerRole::Cap);
    const ComposedSprite eye_layer = extract_role_layer(legacy, SpriteLayerRole::Eyes);
    const ComposedSprite feet_layer = extract_role_layer(legacy, SpriteLayerRole::Feed);
    const ComposedSprite effect_layer = extract_role_layer(legacy, SpriteLayerRole::Effect);

    overlay_composed_sprite(composed, body_layer);
    overlay_composed_sprite(composed, cap_layer);
    overlay_composed_sprite(composed, eye_layer);
    if (pose.has_feet) {
        overlay_composed_sprite(composed, feet_layer);
    }
    overlay_composed_sprite(composed, effect_layer);

    return composed;
}

ComposedSprite Buddy::compose_legacy_sprite(const AppearanceState& appearance) const {
    const auto frame = resolve_appearance_frame(appearance);

    ComposedSprite sprite;
    sprite.rows.reserve(frame.size());

    for (std::size_t row = 0; row < frame.size(); ++row) {
        SpriteRow sprite_row;
        sprite_row.reserve(frame[row].size());

        for (char glyph : frame[row]) {
            sprite_row.push_back(SpriteCell{glyph, classify_legacy_cell(appearance, row, glyph)});
        }

        sprite.rows.push_back(std::move(sprite_row));
    }

    return sprite;
}

std::vector<std::string> Buddy::flatten_sprite(const ComposedSprite& sprite) const {
    std::vector<std::string> frame;
    frame.reserve(sprite.rows.size());

    for (const auto& row : sprite.rows) {
        std::string text;
        text.reserve(row.size());
        for (const auto& cell : row) {
            text.push_back(cell.glyph);
        }
        frame.push_back(std::move(text));
    }

    return frame;
}

std::vector<std::string> Buddy::current_frame() const {
    return flatten_sprite(compose_sprite(make_pose_state(make_appearance_state())));
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
        appearance.cap_variant = cap_variant_;
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

    if (time_until_next_wobble_ > 0.0) {
        time_until_next_wobble_ -= dt_seconds;
        if (time_until_next_wobble_ < 0.0) {
            time_until_next_wobble_ = 0.0;
            }
    }

    if (wobble_duration_remaining_ > 0.0) {
        wobble_duration_remaining_ -= dt_seconds;
        if (wobble_duration_remaining_ < 0.0) {
            wobble_duration_remaining_ = 0.0;
        }
    }

    if (activity_ == Activity::Sleeping || activity_ == Activity::Eating || activity_ == Activity::Walking ||
            expression_ == Expression::Blinking || expression_ == Expression::Sad || effect_ == Effect::Sparkle) {
        eye_direction_ = EyeDirection::Center;
        cap_variant_ = CapVariant::Primary;
        look_duration_remaining_ = 0.0;
        wobble_duration_remaining_ = 0.0;
        wobble_animation_timer_ = 0.0;
        return;
    }

    if (movement_.phase == MovementPhase::WallPause) {
        eye_direction_ = EyeDirection::Center;
        cap_variant_ = CapVariant::Primary;
        look_duration_remaining_ = 0.0;
        wobble_duration_remaining_ = 0.0;
        wobble_animation_timer_ = 0.0;
        return;
    }

    if (look_duration_remaining_ > 0.0) {
        cap_variant_ = CapVariant::Primary;
        return;
    }

    eye_direction_ = EyeDirection::Center;

    if (time_until_next_look_ <= 0.0) {
        eye_direction_ = random_look_direction();
        look_duration_remaining_ = random_look_duration();
        time_until_next_look_ = random_time_until_next_look();
        cap_variant_ = CapVariant::Primary;
        wobble_duration_remaining_ = 0.0;
        wobble_animation_timer_ = 0.0;
        return;
    }

    if (wobble_duration_remaining_ > 0.0) {
        wobble_animation_timer_ += dt_seconds;

        while (wobble_animation_timer_ >= kWobbleFrameInterval_) {
            wobble_animation_timer_ -= kWobbleFrameInterval_;
            cap_variant_ = (cap_variant_ == CapVariant::Primary) ? CapVariant::Alternate : CapVariant::Primary;
        }
        return;
    }

    cap_variant_ = CapVariant::Primary;

    if (time_until_next_wobble_ <= 0.0) {
        wobble_duration_remaining_ = random_wobble_duration();
        wobble_animation_timer_ = 0.0;
        cap_variant_ = CapVariant::Alternate;
        time_until_next_wobble_ = random_time_until_next_wobble();
    }
}

int Buddy::body_frame_width() const noexcept {
    return frame_width(flatten_sprite(compose_sprite(make_pose_state(make_appearance_state()))));
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

double Buddy::random_time_until_next_wobble() {
    std::uniform_real_distribution<double> duration_dist(3.5, 7.0);
    return duration_dist(rng_);
}

double Buddy::random_wobble_duration() {
    std::uniform_real_distribution<double> duration_dist(0.5, 1.0);
    return duration_dist(rng_);
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
