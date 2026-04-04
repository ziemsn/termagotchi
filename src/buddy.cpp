#include "buddy.hpp"

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

constexpr int kSpriteCanvasWidth = 18;
constexpr int kBodyFullX = 4;
constexpr int kBodyNarrowX = 5;

ComposedSprite make_sprite_canvas(int height, int width = kSpriteCanvasWidth) {
    ComposedSprite sprite;
    sprite.rows.assign(static_cast<std::size_t>(height), SpriteRow(static_cast<std::size_t>(width), SpriteCell{}));

    return sprite;
}

void put_sprite_cell(ComposedSprite& sprite, int row, int col, char glyph, SpriteLayerRole role) {
    if (glyph == ' ') {
        return;
    }

    if (row < 0 || row >= static_cast<int>(sprite.rows.size())) {
        return;
    }

    if (col < 0 || col >= static_cast<int>(sprite.rows[static_cast<std::size_t>(row)].size())) {
        return;
    }

    sprite.rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = SpriteCell{glyph, role};
}

void overlay_part(ComposedSprite& sprite, int row, int col, const std::string& text, SpriteLayerRole role) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        put_sprite_cell(sprite, row, col + static_cast<int>(i), text[i], role);
    }
}


int sprite_height_for_pose(const PoseState& pose) {
    return pose.top_padding_rows + 2 + 3 + (pose.has_feet ? 1: 0);
}

int body_anchor_x(const PoseState& pose) {
    if (pose.body_width == BodyWidthProfile::Full) {
        return kBodyFullX; 
    }

    if (pose.appearance.facing == Facing::Left) {
        return kBodyNarrowX - 1;
    }

    if (pose.appearance.facing == Facing::Right) {
        return kBodyNarrowX + 1;
    }

    return kBodyNarrowX;
}

int body_width_columns(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? 10 : 8;
}

int crown_width_chars(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? 10 : 9;
}

int brim_inner_width_chars(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? 12 : 10;
}

int brim_width_chars(const PoseState& pose) {
    return brim_inner_width_chars(pose) + 2;
}

int centered_left_col(int anchor_x, int anchor_width, int part_width) {
    const int anchor_center_times_two = 2 * anchor_x + anchor_width - 1;
    return (anchor_center_times_two - part_width + 1) / 2;
}

std::string crown_left_shell_text(const PoseState& pose) {
    (void)pose;
    return ".-";
}

std::string crown_spot_text(const PoseState& pose) {
    return (pose.appearance.cap_variant == CapVariant::Alternate) ? "O-oo-O" : "o-OO-o";
}

std::string crown_right_shell_text(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? "-." : ".";
}

int cap_anchor_x(const PoseState& pose, int body_anchor_x_value, int body_top_x) {
    if (pose.body_width == BodyWidthProfile::Narrow) {
        return body_top_x;
    }

    return body_anchor_x_value;
}

int crown_left_col(const PoseState& pose, int cap_anchor_x_value) {
    int x = centered_left_col(cap_anchor_x_value, body_width_columns(pose), crown_width_chars(pose));

    if (pose.body_width == BodyWidthProfile::Narrow && pose.appearance.facing == Facing::Right) {
        x += 1;
    }

    return x;
}

int brim_left_col(const PoseState& pose, int cap_anchor_x_value) {
    return centered_left_col(cap_anchor_x_value, body_width_columns(pose), brim_width_chars(pose));
}

int leading_space_count(const std::string& text) {
    int count = 0;
    while (count < static_cast<int>(text.size()) && text[static_cast<std::size_t>(count)] == ' ') {
        ++count;
    }

    return count;
}

int feet_left_col(const PoseState& pose, int body_anchor_x_value, const std::string& trimmed_feet) {
    const int leading_spaces = leading_space_count(trimmed_feet);
    int x = centered_left_col(body_anchor_x_value, body_width_columns(pose), static_cast<int>(trimmed_feet.size())) - leading_spaces;

    if (pose.body_width == BodyWidthProfile::Narrow && pose.appearance.facing == Facing::Right) {
        x += 1;
    }
    
    return x;
}

void draw_horizontal_run(ComposedSprite& sprite, int row, int col, int count, char glyph, SpriteLayerRole role) {
    for (int i = 0; i < count; ++i) {
        put_sprite_cell(sprite, row, col + i, glyph, role);
    }
}

void draw_crown(ComposedSprite& sprite, const PoseState& pose, int row, int cap_anchor_x_value) {
    const int crown_x = crown_left_col(pose, cap_anchor_x_value);
    const std::string left_shell = crown_left_shell_text(pose);
    const std::string spots = crown_spot_text(pose);
    const std::string right_shell = crown_right_shell_text(pose);

    overlay_part(sprite, row, crown_x, left_shell, SpriteLayerRole::CapCrown);
    overlay_part(sprite, row, crown_x + static_cast<int>(left_shell.size()), spots, SpriteLayerRole::CapCrown);
    overlay_part(sprite, row, crown_x + static_cast<int>(left_shell.size()) + static_cast<int>(spots.size()), right_shell, SpriteLayerRole::CapCrown);
}

void draw_brim(ComposedSprite& sprite, const PoseState& pose, int row, int cap_anchor_x_value) {
    const int brim_x = brim_left_col(pose, cap_anchor_x_value);
    put_sprite_cell(sprite, row, brim_x, '(', SpriteLayerRole::CapBrim);
    draw_horizontal_run(sprite, row, brim_x + 1, brim_inner_width_chars(pose), '_', SpriteLayerRole::CapBrim);
    put_sprite_cell(sprite, row, brim_x + 1 + brim_inner_width_chars(pose), ')', SpriteLayerRole::CapBrim);
}

struct EyePlacement {
    int row = 0;
    int left_col = 0;
    int right_col = 0;
    char left_glyph = 'o';
    char right_glyph = '0';
};

std::string body_wall_row(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ?  "|        |" : "|      |";
}

int body_row_lean_offset(const PoseState& pose, int body_row_index) {
    if (pose.body_width != BodyWidthProfile::Narrow) {
        return 0;
    }

    if (pose.appearance.facing == Facing::Left) {
        return (body_row_index == 0) ? -1 : 0;
    }

    if (pose.appearance.facing == Facing::Right) {
        return (body_row_index == 0) ? 1 : 0;
    }

    return 0;
}

std::string body_base_row(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? "|________|" : "|______|";
}

char eye_glyph_for_pose(const PoseState& pose) {
    if (pose.appearance.activity == Activity::Sleeping || pose.appearance.expression == Expression::Blinking){
        return '-';
    }

    if (pose.appearance.expression == Expression::Sad) {
        return 'v';
    }

    return 'o';
}

std::string feet_row_text(const PoseState& pose) {
    const bool primary_phase = (pose.feet_frame_index % 2 == 0);

    if (pose.body_width == BodyWidthProfile::Full) {
        return primary_phase ? "\\/\\\\/" : "/\\\\/\\\\";
    }

    if (pose.appearance.facing == Facing::Left) {
        return primary_phase ? "\\/\\\\/ " : "/\\\\/\\\\ ";
    }

    if (pose.appearance.facing == Facing::Right) {
        return primary_phase ? " \\/\\\\/" : " /\\\\/\\\\";
    }

    return primary_phase ? "\\/\\\\/" : "/\\\\/\\\\";
}

std::string trim_trailing_spaces(std::string text) {
    while (!text.empty() && text.back() == ' ') {
            text.pop_back();
    }
    return text;
}

bool supports_eye_direction_offset(const PoseState& pose) {
    return pose.appearance.activity != Activity::Sleeping &&
        pose.appearance.expression != Expression::Blinking &&
        pose.appearance.expression != Expression::Sad;
}

EyePlacement resolve_eye_placement(const PoseState& pose, int body_row) {
    EyePlacement eyes;
    eyes.row = body_row;
    eyes.left_glyph = eye_glyph_for_pose(pose);
    eyes.right_glyph = eye_glyph_for_pose(pose);
        
    const int body_x = body_anchor_x(pose);
    const int face_x = body_x + body_row_lean_offset(pose, 0);

    if (pose.body_width == BodyWidthProfile::Full) {
        eyes.left_col = face_x + 2;
        eyes.right_col = face_x + 7;
    } else {
        if (pose.appearance.facing == Facing::Left) {
            eyes.left_col = face_x + 1;
            eyes.right_col = face_x + 4;
        } else if (pose.appearance.facing == Facing::Right) {
            eyes.left_col = face_x + 3;
            eyes.right_col = face_x + 6;
        } else {
            eyes.left_col = face_x + 2;
            eyes.right_col = face_x + 5;
        }
    }

    if (supports_eye_direction_offset(pose)) {
        if (pose.appearance.eye_direction == EyeDirection::Left) {
            --eyes.left_col;
            --eyes.right_col;
        } else if (pose.appearance.eye_direction == EyeDirection::Right) {
            ++eyes.left_col;
            ++eyes.right_col;
            }
    }

    return eyes;
}

void draw_eyes(ComposedSprite& sprite, const EyePlacement& eyes) {
    put_sprite_cell(sprite, eyes.row, eyes.left_col, eyes.left_glyph, SpriteLayerRole::Eyes);
    put_sprite_cell(sprite, eyes.row, eyes.right_col, eyes.right_glyph, SpriteLayerRole::Eyes);
}

void draw_feet(ComposedSprite& sprite, const PoseState& pose, int row, int body_anchor_x_value) {
    const std::string feet = trim_trailing_spaces(feet_row_text(pose));
    overlay_part(sprite, row, feet_left_col(pose, body_anchor_x_value, feet), feet, SpriteLayerRole::Feet);
}

void overlay_sparkle(ComposedSprite& sprite, const PoseState& pose, int crown_row, int crown_x) {
    const int row = (crown_row > 0) ? (crown_row - 1) : crown_row;
    const int sparkle_x = crown_x + 2;

    switch (pose.appearance.sparkle_frame_index) {
    case 0: 
        overlay_part(sprite, row, sparkle_x, "* .", SpriteLayerRole::Effect);
        break;
    case 1:
        overlay_part(sprite, row, sparkle_x - 2, ".   +", SpriteLayerRole::Effect);
        break;
    case 2:
        overlay_part(sprite, row, sparkle_x + 1, ".  *", SpriteLayerRole::Effect);
        break;
    default:
        overlay_part(sprite, row, sparkle_x + 1, "*", SpriteLayerRole::Effect);
        break;
    }
}

void apply_overlays(ComposedSprite& sprite, const PoseState& pose, int crown_row, int crown_x) {
    if (pose.appearance.effect == Effect::Sparkle) {
        overlay_sparkle(sprite, pose, crown_row, crown_x);
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
        pose.feet_frame_index = appearance.walk_frame_index;
    } else {
        pose.stance = Stance::Sitting;
        pose.body_width = BodyWidthProfile::Full;
        pose.top_padding_rows = 1;
        pose.has_feet = false;
        pose.feet_frame_index = 0;
    }

    return pose;
}

ComposedSprite Buddy::compose_sprite(const PoseState& pose) const {
    ComposedSprite composed = make_sprite_canvas(sprite_height_for_pose(pose));

    const int crown_row = pose.top_padding_rows;
    const int brim_row = crown_row + 1;
    const int body_row = brim_row + 1;
    const int body_x = body_anchor_x(pose);
    const int body_top_x = body_x + body_row_lean_offset(pose, 0);
    const int crown_x = crown_left_col(pose, body_x);
    const EyePlacement eyes = resolve_eye_placement(pose, body_row);
    const int body_mid_x = body_x + body_row_lean_offset(pose, 1);
    const int body_base_x = body_x + body_row_lean_offset(pose, 2);
    const int cap_anchor = cap_anchor_x(pose, body_x, body_top_x);

    draw_crown(composed, pose, crown_row, cap_anchor);
    draw_brim(composed, pose, brim_row, cap_anchor);

    overlay_part(composed, body_row, body_top_x, body_wall_row(pose), SpriteLayerRole::Body);
    overlay_part(composed, body_row + 1, body_mid_x, body_wall_row(pose), SpriteLayerRole::Body);
    overlay_part(composed, body_row + 2, body_base_x, body_base_row(pose), SpriteLayerRole::Body);
    draw_eyes(composed, eyes);
    if (pose.has_feet) {
        draw_feet(composed, pose, body_row + 3, body_x);
    }
    apply_overlays(composed, pose, crown_row, crown_x);

    return composed;
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
