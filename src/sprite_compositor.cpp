#include "sprite_compositor.hpp"
#include "buddy.hpp"

#include <string>

namespace {
constexpr int kSpriteCanvasWidth = 18;
constexpr int kBodyFullX = 4;
constexpr int kBodyNarrowX = 5;

struct EyePlacement {
    int row = 0;
    int left_col = 0;
    int right_col = 0;
    char left_glyph = 'o';
    char right_glyph = 'o';
};

ComposedSprite make_sprite_canvas(int height, int width = kSpriteCanvasWidth) {
    ComposedSprite sprite;
    sprite.rows.assign(
        static_cast<std::size_t>(height),
        SpriteRow(static_cast<std::size_t>(width), SpriteCell{})
    );
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
    return pose.top_padding_rows + 2 + 3 + (pose.has_feet ? 1 : 0);
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

int body_row_lean_offset(const PoseState& pose, int body_row_index) {
    (void)body_row_index;

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

int cap_anchor_x(const PoseState& pose, int body_anchor_x_value, int body_top_x) {
    if (pose.body_width == BodyWidthProfile::Narrow && pose.appearance.facing != Facing::Forward) {
        return body_top_x;
    }

    return body_anchor_x_value;
}

int crown_left_col(const PoseState& pose, int cap_anchor_x_value) {
    int x = centered_left_col(
        cap_anchor_x_value,
        body_width_columns(pose),
        crown_width_chars(pose)
    );

    if (pose.body_width == BodyWidthProfile::Narrow &&
            pose.appearance.facing == Facing::Right) {
        x += 1;
    }
    return x;
}

int brim_left_col(const PoseState& pose, int cap_anchor_x_value) {
    return centered_left_col(
        cap_anchor_x_value,
        body_width_columns(pose),
        brim_width_chars(pose)
    );
}

void draw_horizontal_run(ComposedSprite& sprite, int row, int col, int count, char glyph,
        SpriteLayerRole role) {
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
    overlay_part(
        sprite,
        row,
        crown_x + static_cast<int>(left_shell.size()),
        spots,
        SpriteLayerRole::CapCrown
    );
    overlay_part(
        sprite,
        row,
        crown_x + static_cast<int>(left_shell.size()) + static_cast<int>(spots.size()),
        right_shell,
        SpriteLayerRole::CapCrown
    );
}

void draw_brim(ComposedSprite& sprite, const PoseState& pose, int row, int cap_anchor_x_value) {
    const int brim_x = brim_left_col(pose, cap_anchor_x_value);
    put_sprite_cell(sprite, row, brim_x, '(', SpriteLayerRole::CapBrim);
    draw_horizontal_run(sprite, row, brim_x + 1, brim_inner_width_chars(pose), '_', SpriteLayerRole::CapBrim);
    put_sprite_cell(sprite, row, brim_x + 1 + brim_inner_width_chars(pose), ')', SpriteLayerRole::CapBrim);
}

char eye_glyph_for_pose(const PoseState& pose) {
    if (pose.appearance.activity == Activity::Sleeping || pose.appearance.expression == Expression::Blinking) {
        return '-';
    }

    if (pose.appearance.expression == Expression::Sad) {
        return 'v';
    }

    return 'o';
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
    eyes.right_glyph = eyes.left_glyph;

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

std::string body_wall_row(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? "|        |" : "|      |";
}

std::string body_base_row(const PoseState& pose) {
    return (pose.body_width == BodyWidthProfile::Full) ? "|________|" : "|______|";
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

    if (pose.body_width == BodyWidthProfile::Narrow &&
            pose.appearance.facing == Facing::Right) {
        x += 1;
    }

    return x;
}

void draw_feet(ComposedSprite& sprite, const PoseState& pose, int row, int body_anchor_x_value) {
    const std::string feet = trim_trailing_spaces(feet_row_text(pose));
    overlay_part(
        sprite,
        row,
        feet_left_col(pose, body_anchor_x_value, feet),
        feet,
        SpriteLayerRole::Feet
    );
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

namespace sprite_compositor {

ComposedSprite compose(const PoseState& pose) {
    ComposedSprite composed = make_sprite_canvas(sprite_height_for_pose(pose));

    const int crown_row = pose.top_padding_rows;
    const int brim_row = crown_row + 1;
    const int body_row = brim_row + 1;
    const int body_x = body_anchor_x(pose);
    const int body_top_x = body_x + body_row_lean_offset(pose, 0);
    const EyePlacement eyes = resolve_eye_placement(pose, body_row);
    const int body_mid_x = body_x + body_row_lean_offset(pose, 1);
    const int body_base_x = body_x + body_row_lean_offset(pose, 2);
    const int cap_anchor = cap_anchor_x(pose, body_x, body_top_x);
    const int crown_x = crown_left_col(pose, cap_anchor);

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

} // namespace sprite_compositor
