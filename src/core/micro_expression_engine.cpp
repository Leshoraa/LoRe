/**
 * @file micro_expression_engine.cpp
 * @brief Continuous biomechanical micro-expression and FACS palpebral perturbation engine implementation.
 */

#include "src/core/micro_expression_engine.h"
#include "src/config/lore_config.h"
#include <math.h>
#include <string.h>
#include <ctype.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <Arduino.h>
#include <esp_random.h>
static inline uint32_t micro_expr_random(void) {
    return esp_random();
}
#else
#include <stdlib.h>
static inline uint32_t micro_expr_random(void) {
    return (uint32_t)rand();
}
#endif

static const float kPi = 3.14159265358979323846f;
static const float kDegToRad = 0.017453292519943295f;

/* Canonical category names */
static const char* const kCategoryNames[NUM_MICRO_CATEGORIES] = {
    "Cognitive",
    "Affectionate",
    "Playful",
    "Startle",
    "Skepticism",
    "Melancholy",
    "Irritation",
    "Drowsiness"
};

/* 128 Micro-Expression Definitions Table (stored in Flash / .rodata)
 * Fields: name, category, dw, dh, brow_l, brow_r, cheek_l, cheek_r, up_l, up_r, low_l, low_r, dn, gx, gy, onset, dwell, decay
 */
static const MicroExpressionDef kMicroExpressions[NUM_MICRO_EXPRESSIONS] = {
    /* --- Category 0: Cognitive & Attentive (0..15) --- */
    { "INQUISITIVE_BROW_L",   MICRO_CAT_COGNITIVE,   1,  1,  14,  -3,   0,   0,   0,   8,   0,   5,  -2,  -1,  -1, 140, 320, 180 },
    { "INQUISITIVE_BROW_R",   MICRO_CAT_COGNITIVE,   1,  1,  -3,  14,   0,   0,   8,   0,   5,   0,  -2,   1,  -1, 140, 320, 180 },
    { "PERPLEXED_FURROW",     MICRO_CAT_COGNITIVE,  -2, -2,   9,  -9,   0,   0,  12,  12,   8,   8,   3,   0,   1, 160, 350, 200 },
    { "DEEP_CONCENTRATION",   MICRO_CAT_COGNITIVE,  -3, -3,  12, -12,   0,   0,  16,  16,  10,  10,   4,   0,   0, 180, 420, 220 },
    { "PENSIVE_GAZE_DRIFT",   MICRO_CAT_COGNITIVE,   0, -1,   4,  -4,   0,   0,   8,   8,   4,   4,   1,   2,  -2, 200, 450, 240 },
    { "SUDDEN_REALIZATION",   MICRO_CAT_COGNITIVE,   3,  4,  -6,   6,   0,   0,   0,   0,   0,   0,  -5,   0,  -2, 110, 280, 160 },
    { "ANALYTICAL_SCAN",      MICRO_CAT_COGNITIVE,  -1, -1,   6,  -6,   0,   0,  10,  10,   6,   6,   2,  -2,   0, 130, 360, 180 },
    { "THOUGHTFUL_SQUINT",    MICRO_CAT_COGNITIVE,  -2, -2,   7,  -7,   4,  -4,  14,  14,   9,   9,   3,   1,   1, 160, 380, 200 },
    { "FLEETING_HESITATION",  MICRO_CAT_COGNITIVE,  -1, -1,  -4,   4,   0,   0,   7,   7,   3,   3,   1,  -1,   1, 130, 260, 170 },
    { "FLEETING_CONFUSION",   MICRO_CAT_COGNITIVE,   0,  1,  11,  -4,   0,   0,   4,  10,   2,   6,   0,   1,  -1, 140, 310, 190 },
    { "CURIOUS_PEER_UP",      MICRO_CAT_COGNITIVE,   2,  2,  -5,   5,   0,   0,   0,   0,   4,   4,  -3,   0,  -3, 130, 340, 170 },
    { "ATTENTIVE_PERK",       MICRO_CAT_COGNITIVE,   2,  3,  -4,   4,   0,   0,   0,   0,   0,   0,  -4,   0,  -1, 110, 300, 160 },
    { "MILD_DISTRACTION",     MICRO_CAT_COGNITIVE,   0, -1,   2,  -2,   0,   0,   5,   5,   2,   2,   1,   3,   0, 170, 380, 220 },
    { "PONDERING_LEFT",       MICRO_CAT_COGNITIVE,   1,  0,   8,  -2,   0,   0,   6,  10,   3,   5,   1,  -3,  -1, 180, 400, 230 },
    { "PONDERING_RIGHT",      MICRO_CAT_COGNITIVE,   1,  0,  -2,   8,   0,   0,  10,   6,   5,   3,   1,   3,  -1, 180, 400, 230 },
    { "MEMORY_RECALL_DRIFT",  MICRO_CAT_COGNITIVE,   0, -1,  -3,   3,   0,   0,   8,   8,   4,   4,   1,  -2,  -2, 190, 440, 250 },

    /* --- Category 1: Affectionate, Social & Friendly (16..31) --- */
    { "SHY_FOND_PEEK",        MICRO_CAT_AFFECTIONATE, -1, -1,  -5,   5,   0,   0,  12,  12,  14,  14,   2,  -1,   2, 160, 420, 240 },
    { "SUBTLE_AFFECTION_SOFTEN", MICRO_CAT_AFFECTIONATE, 1, 1, -4,   4,   0,   0,   4,   4,  10,  10,  -1,   0,   1, 180, 480, 260 },
    { "FLEETING_DUCHENNE_SMILE", MICRO_CAT_AFFECTIONATE, 2, 2,  0,   0,   0,   0,   0,   0,  18,  18,  -2,   0,   0, 150, 400, 220 },
    { "WARM_GAZE_LINGER",     MICRO_CAT_AFFECTIONATE,  1,  1,  -3,   3,   0,   0,   3,   3,   8,   8,  -1,   0,   0, 200, 520, 280 },
    { "BASHFUL_OUTER_DROOP",  MICRO_CAT_AFFECTIONATE, -1, -2,  -8,   8,   0,   0,  10,  10,   8,   8,   2,   1,   2, 160, 380, 210 },
    { "PLAYFUL_WINK_L",       MICRO_CAT_AFFECTIONATE, -2, -2,   4,   0,   0,   0,  28,   0,  20,   6,   4,   0,   0, 120, 280, 160 },
    { "PLAYFUL_WINK_R",       MICRO_CAT_AFFECTIONATE, -2, -2,   0,  -4,   0,   0,   0,  28,   6,  20,   4,   0,   0, 120, 280, 160 },
    { "SUPPRESSED_GIGGLE",    MICRO_CAT_AFFECTIONATE,  1, -1,   3,  -3,   0,   0,   6,   6,  16,  16,   1,   0,  -1, 110, 300, 180 },
    { "MODEST_DOWNCAST",      MICRO_CAT_AFFECTIONATE, -1, -2,  -4,   4,   0,   0,  14,  14,   6,   6,   2,   0,   3, 170, 410, 230 },
    { "GENTLE_GRATITUDE",     MICRO_CAT_AFFECTIONATE,  1,  1,  -5,   5,   0,   0,   4,   4,  12,  12,  -2,   0,   1, 180, 460, 250 },
    { "LOVING_SQUINT",        MICRO_CAT_AFFECTIONATE,  0, -1,  -2,   2,   0,   0,   6,   6,  18,  18,   1,   0,   0, 170, 440, 230 },
    { "WELCOMING_PERK",       MICRO_CAT_AFFECTIONATE,  2,  3,  -4,   4,   0,   0,   0,   0,   6,   6,  -3,   0,   0, 120, 320, 170 },
    { "SYMPATHETIC_TILT",     MICRO_CAT_AFFECTIONATE,  0,  0,  -7,   7,   0,   0,   5,   5,   8,   8,   0,  -1,   1, 160, 390, 220 },
    { "SOFT_REASSURANCE",     MICRO_CAT_AFFECTIONATE,  1,  1,  -4,   4,   0,   0,   3,   3,  10,  10,  -1,   0,   0, 190, 480, 260 },
    { "COY_LATERAL_GLANCE",   MICRO_CAT_AFFECTIONATE, -1,  0,  -3,   6,   0,   0,   8,   5,   8,   5,   1,   2,   1, 150, 360, 200 },
    { "CHEERFUL_SHIMMER",     MICRO_CAT_AFFECTIONATE,  2,  2,   0,   0,   0,   0,   0,   0,  14,  14,  -2,   0,  -1, 110, 320, 170 },

    /* --- Category 2: Playful, Cheeky & Mischievous (32..47) --- */
    { "SLY_SMIRK_L",          MICRO_CAT_PLAYFUL,       1, -1,   9,  -4,   4,   0,   4,   8,  14,   4,   1,  -1,   0, 140, 350, 190 },
    { "SLY_SMIRK_R",          MICRO_CAT_PLAYFUL,       1, -1,  -4,   9,   0,  -4,   8,   4,   4,  14,   1,   1,   0, 140, 350, 190 },
    { "CHEEKY_BROW_TWITCH",   MICRO_CAT_PLAYFUL,       0,  1,  12,   0,   0,   0,   0,   6,   6,   2,  -1,   0,   0, 100, 240, 150 },
    { "TEASING_ASYM_SQUINT",  MICRO_CAT_PLAYFUL,      -1, -2,   8,  -2,   0,   0,   6,  14,  12,   6,   2,   1,   0, 130, 320, 180 },
    { "SCHEMING_NARROW",      MICRO_CAT_PLAYFUL,      -2, -3,   8,  -8,   0,   0,  16,  16,  12,  12,   4,   0,   1, 160, 400, 210 },
    { "PLAYFUL_STARTLE_BOUNCE", MICRO_CAT_PLAYFUL,     3,  4,  -4,   4,   0,   0,   0,   0,   0,   0,  -4,   0,  -2, 100, 250, 140 },
    { "FEIGNED_INNOCENCE",    MICRO_CAT_PLAYFUL,       2,  3,  -6,   6,   0,   0,   0,   0,   4,   4,  -4,   0,  -2, 150, 380, 200 },
    { "CONSPIRATORIAL_WINK",  MICRO_CAT_PLAYFUL,      -2, -2,   6,  -2,   0,   0,  26,   0,  18,   4,   3,  -1,   0, 110, 280, 150 },
    { "MOCK_SUSPICION",       MICRO_CAT_PLAYFUL,      -2, -2,   7,  -3,   0,   0,  12,  16,  10,  10,   3,   1,   0, 140, 340, 180 },
    { "SASSY_BROW_ARCH",      MICRO_CAT_PLAYFUL,       1,  2,  13,  -2,   0,   0,   0,   6,   4,   2,  -2,   0,  -1, 120, 300, 170 },
    { "MISCHIEVOUS_SIDE_GLANCE", MICRO_CAT_PLAYFUL,   -1, -1,   6,  -2,   0,   0,   8,   6,   8,   4,   1,   3,   0, 130, 330, 180 },
    { "FROLIC_FLUTTER",       MICRO_CAT_PLAYFUL,       2,  1,   3,  -3,   0,   0,   4,   4,  10,  10,  -2,   0,  -1, 100, 260, 150 },
    { "SMUG_HALF_LID_L",      MICRO_CAT_PLAYFUL,       0, -1,   8,  -2,   0,   0,  16,   6,   8,   4,   2,  -1,   0, 150, 370, 200 },
    { "SMUG_HALF_LID_R",      MICRO_CAT_PLAYFUL,       0, -1,  -2,   8,   0,   0,   6,  16,   4,   8,   2,   1,   0, 150, 370, 200 },
    { "GLEE_SHIVER",          MICRO_CAT_PLAYFUL,       2,  2,   0,   0,   0,   0,   2,   2,  14,  14,  -3,   0,  -1, 100, 250, 150 },
    { "IMPISH_DART",          MICRO_CAT_PLAYFUL,       0,  0,   6,  -6,   0,   0,   6,   6,   6,   6,   0,  -3,   1, 110, 270, 160 },

    /* --- Category 3: Startle, Surprise & Alertness (48..63) --- */
    { "MICRO_STARTLE_DILATION", MICRO_CAT_STARTLE,     3,  4,  -6,   6,   0,   0,   0,   0,   0,   0,  -6,   0,  -1,  80, 220, 140 },
    { "FLEETING_SHOCK_RECOIL",  MICRO_CAT_STARTLE,     4,  5,  -8,   8,   0,   0,   0,   0,   0,   0,  -7,   0,  -2,  90, 240, 150 },
    { "INVOLUNTARY_WIDE_PERK",  MICRO_CAT_STARTLE,     2,  3,  -5,   5,   0,   0,   0,   0,   0,   0,  -5,   0,  -1, 100, 260, 150 },
    { "SUDDEN_DOUBLE_TAKE",     MICRO_CAT_STARTLE,     2,  3,  -4,   4,   0,   0,   0,   0,   0,   0,  -4,   2,  -1, 100, 280, 160 },
    { "HIGH_VIGILANCE_FLARE",   MICRO_CAT_STARTLE,     3,  4,  -7,   7,   0,   0,   0,   0,   0,   0,  -6,   0,   0,  90, 250, 150 },
    { "ALERT_DART_L",           MICRO_CAT_STARTLE,     2,  2,  -4,   4,   0,   0,   0,   0,   0,   0,  -3,  -3,   0, 100, 270, 160 },
    { "ALERT_DART_R",           MICRO_CAT_STARTLE,     2,  2,  -4,   4,   0,   0,   0,   0,   0,   0,  -3,   3,   0, 100, 270, 160 },
    { "FLEETING_BEWILDERMENT",  MICRO_CAT_STARTLE,     1,  2,   6,   6,   0,   0,   4,   4,   2,   2,  -2,   0,  -1, 120, 300, 170 },
    { "ASTONISHED_BREATH_HOLD", MICRO_CAT_STARTLE,     3,  4,  -6,   6,   0,   0,   0,   0,   0,   0,  -5,   0,  -1, 110, 360, 180 },
    { "MICRO_GASP_SPURT",       MICRO_CAT_STARTLE,     3,  4,  -5,   5,   0,   0,   0,   0,   0,   0,  -6,   0,  -2,  90, 230, 140 },
    { "FLABBERGASTED_WIDEN",    MICRO_CAT_STARTLE,     4,  5,  -8,   8,   0,   0,   0,   0,   0,   0,  -7,   0,   0, 100, 290, 160 },
    { "STARTLED_FREEZE",        MICRO_CAT_STARTLE,     3,  3,  -6,   6,   0,   0,   0,   0,   0,   0,  -5,   0,   0,  90, 380, 170 },
    { "STUNNED_BLINK_RECOVERY", MICRO_CAT_STARTLE,     2,  2,  -4,   4,   0,   0,   6,   6,   2,   2,  -3,   0,   1, 110, 280, 160 },
    { "INCREDULOUS_BROW_COCK",  MICRO_CAT_STARTLE,     2,  3,  12,  -4,   0,   0,   0,   4,   0,   2,  -3,  -1,  -1, 110, 300, 170 },
    { "FLEETING_DISBELIEF",     MICRO_CAT_STARTLE,     2,  2,   7,  -7,   0,   0,   2,   2,   2,   2,  -3,   0,   0, 120, 320, 170 },
    { "ELECTRIC_JOLT_TWITCH",   MICRO_CAT_STARTLE,     3,  4,  -5,   5,   0,   0,   0,   0,   0,   0,  -5,   1,  -1,  80, 200, 130 },

    /* --- Category 4: Skepticism, Scrutiny & Caution (64..79) --- */
    { "CRITICAL_BROW_SLANT",    MICRO_CAT_SKEPTICISM,  -2, -2,  11,  -3,   0,   0,  12,  16,   8,   8,   3,  -1,   0, 150, 380, 200 },
    { "SKEPTICAL_SQUINT_L",     MICRO_CAT_SKEPTICISM,  -2, -2,  10,  -2,   0,   0,  18,   8,  12,   6,   3,  -1,   0, 140, 360, 190 },
    { "SKEPTICAL_SQUINT_R",     MICRO_CAT_SKEPTICISM,  -2, -2,  -2,  10,   0,   0,   8,  18,   6,  12,   3,   1,   0, 140, 360, 190 },
    { "NARROWED_SCRUTINY",      MICRO_CAT_SKEPTICISM,  -3, -4,   8,  -8,   0,   0,  20,  20,  12,  12,   5,   0,   0, 160, 420, 220 },
    { "GUARDED_SIDE_EYE",       MICRO_CAT_SKEPTICISM,  -1, -2,   6,  -2,   0,   0,  14,  10,   8,   6,   2,   3,   0, 150, 370, 200 },
    { "DISAPPROVING_BROW_DROP", MICRO_CAT_SKEPTICISM,  -2, -3,  12, -12,   0,   0,  16,  16,   6,   6,   4,   0,   1, 160, 390, 210 },
    { "FLEETING_DISDAIN",       MICRO_CAT_SKEPTICISM,   0, -2,   8,  -4,   0,   0,  18,  10,   6,   6,   3,   1,  -1, 150, 350, 200 },
    { "HESITANT_SIDE_STEP",     MICRO_CAT_SKEPTICISM,  -1, -1,   4,  -4,   0,   0,  10,  10,   6,   6,   2,  -2,   1, 160, 380, 210 },
    { "WARY_GAZE_FREEZE",       MICRO_CAT_SKEPTICISM,  -2, -2,   6,  -6,   0,   0,  12,  12,   8,   8,   3,   0,   0, 140, 440, 210 },
    { "APPREHENSIVE_OUTER_DROOP", MICRO_CAT_SKEPTICISM, -1, -2, -7,   7,   0,   0,  12,  12,   6,   6,   2,   0,   1, 160, 380, 210 },
    { "CAUTIOUS_DISTANCE_SACCADE", MICRO_CAT_SKEPTICISM, -2, -2, 7,  -7,   0,   0,  14,  14,   8,   8,   3,   2,  -1, 150, 360, 200 },
    { "UNIMPRESSED_FLAT_LID",   MICRO_CAT_SKEPTICISM,   1, -3,   0,   0,   0,   0,  20,  20,   0,   0,   5,   0,   0, 170, 420, 220 },
    { "QUIZZICAL_EYE_ROLL",     MICRO_CAT_SKEPTICISM,   0, -1,   6,  -6,   0,   0,  10,  10,   4,   4,   2,   1,  -2, 170, 400, 220 },
    { "QUESTIONING_SQUINT",     MICRO_CAT_SKEPTICISM,  -2, -2,   9,  -4,   0,   0,  14,  12,  10,   8,   3,   0,   0, 150, 360, 190 },
    { "SUSPICIOUS_SCAN_L",      MICRO_CAT_SKEPTICISM,  -2, -3,   8,  -5,   0,   0,  16,  14,  10,  10,   4,  -3,   0, 150, 400, 210 },
    { "SUSPICIOUS_SCAN_R",      MICRO_CAT_SKEPTICISM,  -2, -3,  -5,   8,   0,   0,  14,  16,  10,  10,   4,   3,   0, 150, 400, 210 },

    /* --- Category 5: Vulnerability, Melancholy & Worry (80..95) --- */
    { "FLEETING_SORROW_DROOP",  MICRO_CAT_MELANCHOLY,  -1, -2, -10,  10,   0,   0,  14,  14,   4,   4,   2,   0,   2, 180, 440, 240 },
    { "TENDER_SYMPATHY",        MICRO_CAT_MELANCHOLY,   0, -1,  -7,   7,   0,   0,   8,   8,   8,   8,   0,   0,   1, 190, 480, 260 },
    { "ANXIOUS_MICRO_FLUTTER",  MICRO_CAT_MELANCHOLY,  -1, -1,  -6,   6,   0,   0,   8,   8,   4,   4,   1,   0,  -1, 110, 280, 160 },
    { "LONELY_DOWNWARD_AVERT",  MICRO_CAT_MELANCHOLY,  -2, -3,  -8,   8,   0,   0,  16,  16,   4,   4,   3,  -1,   3, 190, 480, 260 },
    { "HESITANT_UPWARD_GLANCE", MICRO_CAT_MELANCHOLY,  -1, -1,  -5,   5,   0,   0,   8,   8,   4,   4,   1,   0,  -2, 170, 400, 220 },
    { "SUPPRESSED_WINCE",       MICRO_CAT_MELANCHOLY,  -2, -3,  -7,   7,   0,   0,  20,  20,  12,  12,   4,   0,   1, 130, 310, 180 },
    { "SUBTLE_MELANCHOLIC_SIGH", MICRO_CAT_MELANCHOLY, -1, -2, -8,   8,   0,   0,  12,  12,   6,   6,   2,   0,   2, 200, 520, 280 },
    { "FRAGILE_GAZE_CAST",      MICRO_CAT_MELANCHOLY,  -1, -2,  -6,   6,   0,   0,  10,  10,   4,   4,   1,   1,   2, 180, 450, 240 },
    { "FORLORN_DISTANT_LOOK",   MICRO_CAT_MELANCHOLY,  -1, -2,  -7,   7,   0,   0,  14,  14,   4,   4,   2,   2,   1, 210, 540, 290 },
    { "REGRETFUL_LID_LOWER",    MICRO_CAT_MELANCHOLY,  -2, -3,  -9,   9,   0,   0,  18,  18,   4,   4,   3,   0,   2, 190, 460, 250 },
    { "TIMID_SHYNESS_PEEK",     MICRO_CAT_MELANCHOLY,  -1, -1,  -6,   6,   0,   0,  12,  12,   8,   8,   1,  -2,   2, 170, 420, 230 },
    { "SOFT_HEARTACHE_TREMOR",  MICRO_CAT_MELANCHOLY,  -1, -2,  -8,   8,   0,   0,  12,  12,   6,   6,   2,   0,   0, 120, 320, 180 },
    { "APOLOGETIC_BROW_SPLAY",  MICRO_CAT_MELANCHOLY,   0, -1,  -9,   9,   0,   0,   8,   8,   6,   6,   1,   0,   1, 180, 440, 240 },
    { "NEEDY_PUPIL_DILATION",   MICRO_CAT_MELANCHOLY,   2,  2,  -6,   6,   0,   0,   4,   4,   4,   4,  -3,   0,   0, 150, 390, 220 },
    { "FLEETING_MELANCHOLY_TILT", MICRO_CAT_MELANCHOLY, -1, -2, -9,   9,   0,   0,  12,  12,   6,   6,   2,  -1,   1, 180, 460, 250 },
    { "WISTFUL_CONTEMPLATION",  MICRO_CAT_MELANCHOLY,  -1, -1,  -5,   5,   0,   0,  10,  10,   6,   6,   1,   1,   1, 200, 500, 270 },

    /* --- Category 6: Irritation, Defiance & Determination (96..111) --- */
    { "STERN_BROW_FURROW",      MICRO_CAT_IRRITATION,  -2, -2,  13, -13,   0,   0,  12,  12,   6,   6,   3,   0,   0, 150, 380, 200 },
    { "STUBBORN_SQUINT",        MICRO_CAT_IRRITATION,  -3, -3,  12, -12,   0,   0,  16,  16,  10,  10,   4,   0,   0, 160, 420, 220 },
    { "IRRITATED_BROW_TWITCH",  MICRO_CAT_IRRITATION,  -1, -1,  14,  -6,   0,   0,   8,   8,   4,   4,   2,   0,   0, 100, 250, 150 },
    { "DEFIANT_NARROWING",      MICRO_CAT_IRRITATION,  -3, -4,  11, -11,   0,   0,  18,  18,  12,  12,   5,   0,   0, 150, 400, 210 },
    { "GRUMPY_LOWERED_GAZE",    MICRO_CAT_IRRITATION,  -2, -2,  10, -10,   0,   0,  14,  14,   6,   6,   3,   0,   2, 170, 420, 230 },
    { "FIERCE_FOCUS_LOCK",      MICRO_CAT_IRRITATION,  -2, -2,  12, -12,   0,   0,  12,  12,   8,   8,   3,   0,   0, 140, 450, 210 },
    { "IMPATIENT_MICRO_SACCADE", MICRO_CAT_IRRITATION, -1, -1,  10, -10,   0,   0,  10,  10,   6,   6,   2,   2,   0, 120, 280, 160 },
    { "SKEPTICAL_SIDE_GLARE",   MICRO_CAT_IRRITATION,  -2, -2,  12,  -6,   0,   0,  14,  12,   8,   6,   3,   2,   0, 150, 380, 200 },
    { "RESOLUTE_GAZE_FIXATION", MICRO_CAT_IRRITATION,  -1, -1,  10, -10,   0,   0,   8,   8,   6,   6,   2,   0,   0, 160, 440, 220 },
    { "SULLEN_EYE_CAST",        MICRO_CAT_IRRITATION,  -2, -2,   8,  -8,   0,   0,  16,  16,   6,   6,   3,  -1,   2, 180, 450, 240 },
    { "ANNOYED_PALPEBRAL_PINCH", MICRO_CAT_IRRITATION, -3, -3,  13, -13,   0,   0,  18,  18,  10,  10,   4,   0,   0, 130, 320, 180 },
    { "TENSE_INNER_BROW_NOTCH", MICRO_CAT_IRRITATION,  -2, -2,  14, -14,   0,   0,  10,  10,   6,   6,   3,   0,   0, 140, 360, 190 },
    { "PUFFED_CHEEK_TENSE",     MICRO_CAT_IRRITATION,   0, -1,   8,  -8,   0,   0,   6,   6,  14,  14,   2,   0,  -1, 150, 370, 200 },
    { "CHALLENGING_BROW_ARCH",  MICRO_CAT_IRRITATION,   1,  1,  14,  -2,   0,   0,   4,   8,   6,   4,  -1,   0,  -1, 140, 350, 190 },
    { "STOIC_GAZE_SETTLE",      MICRO_CAT_IRRITATION,  -1, -1,   8,  -8,   0,   0,  10,  10,   4,   4,   2,   0,   0, 180, 460, 240 },
    { "GRITTED_CONCENTRATION",  MICRO_CAT_IRRITATION,  -3, -3,  12, -12,   0,   0,  16,  16,   8,   8,   4,   0,   0, 160, 430, 220 },

    /* --- Category 7: Drowsiness, Relaxation & Biological Somnolence (112..127) --- */
    { "HEAVY_UPPER_LID_LAG",    MICRO_CAT_DROWSINESS,   1, -3,   0,   0,   0,   0,  24,  24,   4,   4,   3,   0,   1, 200, 480, 260 },
    { "DROWSY_SLIT_DRIFT",      MICRO_CAT_DROWSINESS,   1, -4,   0,   0,   0,   0,  28,  28,   6,   6,   4,   0,   2, 220, 520, 280 },
    { "CONTENTED_SLOW_FLUTTER", MICRO_CAT_DROWSINESS,   0, -2,  -3,   3,   0,   0,  14,  14,   8,   8,   1,   0,   1, 180, 420, 240 },
    { "PEACEFUL_SIGH_SOFTEN",   MICRO_CAT_DROWSINESS,   1, -1,  -4,   4,   0,   0,  10,  10,  10,  10,  -1,   0,   1, 210, 500, 270 },
    { "RELAXED_EYE_FLATTENING", MICRO_CAT_DROWSINESS,   2, -2,   0,   0,   0,   0,  12,  12,   4,   4,   2,   0,   0, 200, 460, 250 },
    { "LAZILY_LOWERED_BROW",    MICRO_CAT_DROWSINESS,   0, -2,   4,  -4,   0,   0,  16,  16,   4,   4,   2,   0,   1, 190, 450, 240 },
    { "DAYDREAMING_DISTANT_GAZE", MICRO_CAT_DROWSINESS, 0, -1,  -2,   2,   0,   0,   8,   8,   4,   4,   1,   2,   0, 220, 550, 300 },
    { "MICRO_YAWN_STRETCH",     MICRO_CAT_DROWSINESS,   3, -3,  -6,   6,   0,   0,  22,  22,  12,  12,   3,   0,  -1, 240, 580, 320 },
    { "SWEET_SLUMBER_SETTLE",   MICRO_CAT_DROWSINESS,   0, -3,  -4,   4,   0,   0,  22,  22,   6,   6,   3,   0,   2, 220, 520, 280 },
    { "COMFORTED_SOFT_SACCADE", MICRO_CAT_DROWSINESS,   0, -1,  -3,   3,   0,   0,  10,  10,   6,   6,   1,  -1,   1, 190, 440, 240 },
    { "SLEEPY_WOBBLE_DRIFT",    MICRO_CAT_DROWSINESS,   1, -2,   2,  -2,   0,   0,  18,  18,   6,   6,   2,   1,   2, 210, 480, 260 },
    { "HYPNAGOGIC_FLUTTER",     MICRO_CAT_DROWSINESS,  -1, -3,   0,   0,   0,   0,  26,  26,   8,   8,   3,   0,   1, 140, 340, 200 },
    { "EXHAUSTED_HEAVY_GLAZE",  MICRO_CAT_DROWSINESS,   1, -4,   2,  -2,   0,   0,  30,  30,   6,   6,   4,   0,   2, 230, 560, 300 },
    { "WARM_DROWSE_SPLAY",      MICRO_CAT_DROWSINESS,   1, -2,  -3,   3,   0,   0,  16,  16,   8,   8,   2,   0,   1, 210, 490, 270 },
    { "COZY_EYELID_SNUGGLE",    MICRO_CAT_DROWSINESS,   0, -2,  -5,   5,   0,   0,  18,  18,  12,  12,   2,   0,   1, 200, 470, 250 },
    { "RESTING_BLISS_SLIT",     MICRO_CAT_DROWSINESS,   1, -4,  -2,   2,   0,   0,  30,  30,   8,   8,   4,   0,   2, 240, 580, 310 }
};

/* Runtime Kinematic State */
static MicroExpressionId s_active_id = MICRO_EXPR_NONE;
static float s_active_intensity = 0.0f;
static float s_target_intensity = 1.0f;
static float s_elapsed_ms = 0.0f;
static float s_total_duration_ms = 0.0f;
static float s_onset_ms = 120.0f;
static float s_dwell_ms = 300.0f;
static float s_decay_ms = 180.0f;

/* Fractional Viscoelastic Spring State for Organic Relaxation Phase */
static float s_spring_pos = 0.0f;
static float s_spring_vel = 0.0f;

/* Autonomous Spontaneous Trigger Timer */
static float s_spontaneous_timer_s = 0.0f;
static float s_next_spontaneous_interval_s = 4.5f;

/* Cached active delta */
static MicroExpressionDelta s_current_delta = {0};

/* 5th-order minimum-jerk rise function (Flash & Hogan 1985) */
static inline float minimum_jerk_rise(float tau) {
    if (tau <= 0.0f) return 0.0f;
    if (tau >= 1.0f) return 1.0f;
    float tau3 = tau * tau * tau;
    return tau3 * (10.0f - 15.0f * tau + 6.0f * tau * tau);
}

void initMicroExpressionEngine(void) {
    s_active_id = MICRO_EXPR_NONE;
    s_active_intensity = 0.0f;
    s_target_intensity = 1.0f;
    s_elapsed_ms = 0.0f;
    s_total_duration_ms = 0.0f;
    s_spring_pos = 0.0f;
    s_spring_vel = 0.0f;
    s_spontaneous_timer_s = 0.0f;
    s_next_spontaneous_interval_s = 4.0f + (float)(micro_expr_random() % 3000) * 0.001f;
    memset(&s_current_delta, 0, sizeof(s_current_delta));
}

bool triggerMicroExpression(MicroExpressionId id, float intensity) {
    if ((int)id < 0 || (int)id >= NUM_MICRO_EXPRESSIONS) {
        return false;
    }

    if (intensity < 0.05f) intensity = 0.05f;
    if (intensity > 1.0f) intensity = 1.0f;

    const MicroExpressionDef& def = kMicroExpressions[(int)id];
    s_active_id = id;
    s_target_intensity = intensity;
    s_onset_ms = (float)def.onset_ms;
    s_dwell_ms = (float)def.dwell_ms;
    s_decay_ms = (float)def.decay_ms;
    s_total_duration_ms = s_onset_ms + s_dwell_ms + s_decay_ms;
    s_elapsed_ms = 0.0f;
    s_spring_pos = 1.0f;
    s_spring_vel = 0.0f;

    return true;
}

bool triggerMicroExpressionByName(const char* name, float intensity) {
    MicroExpressionId id = findMicroExpressionByName(name);
    if (id != MICRO_EXPR_NONE) {
        return triggerMicroExpression(id, intensity);
    }
    return false;
}

void stopMicroExpression(void) {
    s_active_id = MICRO_EXPR_NONE;
    s_active_intensity = 0.0f;
    s_elapsed_ms = 0.0f;
    memset(&s_current_delta, 0, sizeof(s_current_delta));
}

void updateMicroExpressionEngine(float dt_sec) {
    if (dt_sec <= 0.0005f) dt_sec = 0.01666f;
    if (dt_sec > 0.10f) dt_sec = 0.10f;

    if (s_active_id == MICRO_EXPR_NONE) {
        memset(&s_current_delta, 0, sizeof(s_current_delta));
        return;
    }

    float dt_ms = dt_sec * 1000.0f;
    s_elapsed_ms += dt_ms;

    if (s_elapsed_ms >= s_total_duration_ms) {
        stopMicroExpression();
        return;
    }

    /* 1. Calculate Continuous Physical Kinematic Envelope */
    float env = 0.0f;
    if (s_elapsed_ms < s_onset_ms) {
        /* Phase 1: Minimum-Jerk Motor Muscle Recruitment */
        float tau = s_elapsed_ms / s_onset_ms;
        env = minimum_jerk_rise(tau);
    } else if (s_elapsed_ms < (s_onset_ms + s_dwell_ms)) {
        /* Phase 2: Myogenic Apex Dwell with subtle respiratory micro-tremor (~3.5 Hz) */
        float t_dwell = (s_elapsed_ms - s_onset_ms) * 0.001f;
        float tremor = 0.025f * sinf(2.0f * kPi * 3.5f * t_dwell);
        env = 1.0f + tremor;
    } else {
        /* Phase 3: Fractional Viscoelastic Spring-Damper Relaxation
         * Harmonic parameters: omega_n = 18.0 rad/s, zeta = 0.85 (underdamped tissue compliance)
         * Solves: d2x/dt2 + 2*zeta*omega_n*dx/dt + omega_n^2 * x = 0
         */
        float t_decay = (s_elapsed_ms - (s_onset_ms + s_dwell_ms)) * 0.001f;
        const float omega_n = 18.0f;
        const float zeta = 0.85f;
        float omega_d = omega_n * sqrtf(1.0f - zeta * zeta);

        float exp_term = expf(-zeta * omega_n * t_decay);
        float spring = exp_term * (cosf(omega_d * t_decay) + (zeta / sqrtf(1.0f - zeta * zeta)) * sinf(omega_d * t_decay));
        env = spring;
        if (env < 0.0f) env = 0.0f;
    }

    if (env > 1.05f) env = 1.05f;
    if (env < 0.0f) env = 0.0f;

    s_active_intensity = env * s_target_intensity;

    /* 2. Retrieve Archetype Definition & Scale by Envelope */
    const MicroExpressionDef& def = kMicroExpressions[(int)s_active_id];

    /* Height deformation in pixels */
    float dh = (float)def.delta_h_scaled * 0.5f * s_active_intensity;

    /* Biological Incompressibility Law (Volume Conservation):
     * Sy = 1.0 + dh / H_canon; Sx = 1.0 / sqrt(Sy); dw = W_canon * (Sx - 1.0)
     */
    float sy = 1.0f + dh / SOMA_CANONICAL_EYE_HEIGHT_PX;
    if (sy < 0.5f) sy = 0.5f;
    float sx = 1.0f / sqrtf(sy);
    float tissue_dw = SOMA_CANONICAL_EYE_WIDTH_PX * (sx - 1.0f) * 0.5f;

    float dw = ((float)def.delta_w_scaled * 0.5f * s_active_intensity) + tissue_dw;

    /* Populate Active Somatic Deltas */
    s_current_delta.delta_w_l = dw;
    s_current_delta.delta_w_r = dw;
    s_current_delta.delta_h_l = dh;
    s_current_delta.delta_h_r = dh;

    s_current_delta.delta_brow_l = ((float)def.delta_brow_l_deg * kDegToRad) * s_active_intensity;
    s_current_delta.delta_brow_r = ((float)def.delta_brow_r_deg * kDegToRad) * s_active_intensity;

    s_current_delta.delta_cheek_l = ((float)def.delta_cheek_l_deg * kDegToRad) * s_active_intensity;
    s_current_delta.delta_cheek_r = ((float)def.delta_cheek_r_deg * kDegToRad) * s_active_intensity;

    s_current_delta.delta_upper_l = ((float)def.delta_upper_l_pct * 0.01f) * s_active_intensity;
    s_current_delta.delta_upper_r = ((float)def.delta_upper_r_pct * 0.01f) * s_active_intensity;

    s_current_delta.delta_lower_l = ((float)def.delta_lower_l_pct * 0.01f) * s_active_intensity;
    s_current_delta.delta_lower_r = ((float)def.delta_lower_r_pct * 0.01f) * s_active_intensity;

    s_current_delta.delta_n_l = ((float)def.delta_n_scaled * 0.1f) * s_active_intensity;
    s_current_delta.delta_n_r = ((float)def.delta_n_scaled * 0.1f) * s_active_intensity;

    s_current_delta.delta_gaze_x = ((float)def.delta_gaze_x_px) * s_active_intensity;
    s_current_delta.delta_gaze_y = ((float)def.delta_gaze_y_px) * s_active_intensity;

    s_current_delta.delta_tilt_l = 0.0f;
    s_current_delta.delta_tilt_r = 0.0f;

    s_current_delta.intensity = s_active_intensity;
    s_current_delta.active = true;
}

MicroExpressionDelta getActiveMicroExpressionDelta(void) {
    return s_current_delta;
}

bool isMicroExpressionActive(void) {
    return (s_active_id != MICRO_EXPR_NONE);
}

MicroExpressionId getActiveMicroExpressionId(void) {
    return s_active_id;
}

const char* getMicroExpressionName(MicroExpressionId id) {
    if ((int)id >= 0 && (int)id < NUM_MICRO_EXPRESSIONS) {
        return kMicroExpressions[(int)id].name;
    }
    return "NONE";
}

uint8_t getMicroExpressionCategory(MicroExpressionId id) {
    if ((int)id >= 0 && (int)id < NUM_MICRO_EXPRESSIONS) {
        return kMicroExpressions[(int)id].category;
    }
    return 0;
}

const char* getMicroCategoryName(uint8_t cat_id) {
    if (cat_id < NUM_MICRO_CATEGORIES) {
        return kCategoryNames[cat_id];
    }
    return "Unknown";
}

float getMicroExpressionProgress(void) {
    if (s_active_id == MICRO_EXPR_NONE || s_total_duration_ms <= 0.0f) return 0.0f;
    float p = s_elapsed_ms / s_total_duration_ms;
    if (p > 1.0f) p = 1.0f;
    return p;
}

static bool str_equals_case_insensitive(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = (char)toupper((unsigned char)*a);
        char cb = (char)toupper((unsigned char)*b);
        if (ca == '-' || ca == ' ') ca = '_';
        if (cb == '-' || cb == ' ') cb = '_';
        if (ca != cb) return false;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

MicroExpressionId findMicroExpressionByName(const char* name) {
    if (!name) return MICRO_EXPR_NONE;
    for (int i = 0; i < NUM_MICRO_EXPRESSIONS; i++) {
        if (str_equals_case_insensitive(kMicroExpressions[i].name, name)) {
            return (MicroExpressionId)i;
        }
    }
    return MICRO_EXPR_NONE;
}

void updateAutonomousMicroExpressions(float dt_sec, float valence, float arousal,
                                      float curiosity, float mischief, float social,
                                      float boredom, float fatigue, bool is_detected) {
    /* If already animating a micro-expression, allow it to complete */
    if (s_active_id != MICRO_EXPR_NONE) return;

    s_spontaneous_timer_s += dt_sec;
    if (s_spontaneous_timer_s < s_next_spontaneous_interval_s) return;

    /* Timer expired -> Select a biologically congruent micro-expression archetype */
    s_spontaneous_timer_s = 0.0f;
    /* Schedule next interval between 3.5 and 7.0 seconds */
    s_next_spontaneous_interval_s = 3.5f + (float)(micro_expr_random() % 3500) * 0.001f;

    MicroCategory chosen_cat = MICRO_CAT_COGNITIVE;

    /* Probabilistic selection driven by homeostatic and affective states */
    if (fatigue > 0.65f || (boredom > 0.60f && arousal < 0.20f)) {
        chosen_cat = MICRO_CAT_DROWSINESS;
    } else if (is_detected && social > 0.50f && valence > 0.15f) {
        chosen_cat = (mischief > 0.55f && (micro_expr_random() % 100 < 40)) ? MICRO_CAT_PLAYFUL : MICRO_CAT_AFFECTIONATE;
    } else if (arousal > 0.70f) {
        chosen_cat = (valence < -0.10f) ? MICRO_CAT_IRRITATION : MICRO_CAT_STARTLE;
    } else if (curiosity > 0.55f) {
        chosen_cat = (valence < -0.15f) ? MICRO_CAT_SKEPTICISM : MICRO_CAT_COGNITIVE;
    } else if (mischief > 0.55f) {
        chosen_cat = MICRO_CAT_PLAYFUL;
    } else if (valence < -0.30f) {
        chosen_cat = (arousal > 0.40f) ? MICRO_CAT_IRRITATION : MICRO_CAT_MELANCHOLY;
    } else {
        /* Baseline resting distribution */
        uint32_t roll = micro_expr_random() % 100;
        if (roll < 35) chosen_cat = MICRO_CAT_COGNITIVE;
        else if (roll < 55) chosen_cat = MICRO_CAT_AFFECTIONATE;
        else if (roll < 75) chosen_cat = MICRO_CAT_PLAYFUL;
        else if (roll < 88) chosen_cat = MICRO_CAT_SKEPTICISM;
        else chosen_cat = MICRO_CAT_DROWSINESS;
    }

    /* Pick one of the 16 nuances within the chosen category */
    int nuance_idx = (int)(micro_expr_random() % 16);
    int target_id = ((int)chosen_cat * 16) + nuance_idx;

    /* Subtle natural intensity: 0.70 to 1.00 */
    float intensity = 0.70f + (float)(micro_expr_random() % 300) * 0.001f;
    triggerMicroExpression((MicroExpressionId)target_id, intensity);
}
