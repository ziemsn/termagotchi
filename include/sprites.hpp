#pragma once

#include <string>
#include <vector>

namespace sprites {

inline const std::vector<std::string> idle_0 = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> idle_1 = {
    "      ",
    "      ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> look_left_0 = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    |o    o  |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> look_left_1 = {
    "      ",
    "      ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    |o    o  |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> look_right_0 = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    |  o    o|",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> look_right_1 = {
    "      ",
    "      ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    |  o    o|",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> walk_0 = {
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|",
    "      \\/\\/"
};

inline const std::vector<std::string> walk_1 = {
    "      ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|",
    "      /\\/\\"
};

inline const std::vector<std::string> walk_0_blink = {
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    |--    --|",
    "    |        |",
    "    |________|",
    "      \\/\\/"
};

inline const std::vector<std::string> walk_1_blink = {
    "      ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    |--    --|",
    "    |        |",
    "    |________|",
    "      /\\/\\"
};

inline const std::vector<std::string> blink = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    |--    --|",
    "    |        |",
    "    |________|"
};

 inline const std::vector<std::string> eat = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |   __   |",
    "    |________|"
};

inline const std::vector<std::string> sleep = {
    "     Z",
    "      z",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | -    - |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> sad = {
    "      ",
    "      ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |  ____  |",
    "    |________|"
};

inline const std::vector<std::string> sparkle_0 = {
    "           +",
    "      * .     ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> sparkle_1 = {
    "           *",
    "    .   +     ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> sparkle_2 = {
    "     +",
    "       .  *   ",
    "    .-o-OO-o-.",
    "  (____________)",
    "    | o    o |",
    "    |        |",
    "    |________|"
};

inline const std::vector<std::string> blink_sparkle = {
    "           *",
    "      +       ",
    "    .-O-oo-O-.",
    "  (____________)",
    "    | -    - |",
    "    |        |",
    "    |________|"
};

/* inline const std::vector<std::string> idle_0 = { */
/*     "  /\\_/\\\\  ", */
/*     " ( o.o ) ", */
/*     "  > ^ <  " */
/* }; */

/* inline const std::vector<std::string> idle_1 = { */
/*     "  /\\_/\\\\  ", */
/*     " ( o.o ) ", */
/*     "  >>^<<  " */
/* }; */

/* inline const std::vector<std::string> blink = { */
/*     "  /\\_/\\\\  ", */
/*     " ( -.- ) ", */
/*     "  > ^ <  " */
/* }; */

/* inline const std::vector<std::string> eat = { */
/*     "  /\\_/\\\\  ", */
/*     " ( ^.^ ) ", */
/*     "  > o <  " */
/* }; */

/* inline const std::vector<std::string> sleep = { */
/*     "  /\\_/\\\\  z", */
/*     " ( -.- )  ", */
/*     "  > ^ <   " */
/* }; */

/* inline const std::vector<std::string> sad = { */
/*     "  /\\_/\\\\  ", */
/*     " ( T.T ) ", */
/*     "  > ^ <  " */
/* }; */

} // namespace sprites
