/**
 * @file micro_expression_engine.h
 * @brief Continuous biomechanical micro-expression and FACS palpebral perturbation engine for LoRe.
 * @details Implements 128 parametric micro-expressions across 8 affective categories
 *          governed by 5th-order minimum-jerk kinematics, fractional viscoelastic
 *          relaxation, and volume-conserving biological tissue incompressibility.
 */

#ifndef LORE_MICRO_EXPRESSION_ENGINE_H
#define LORE_MICRO_EXPRESSION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_MICRO_EXPRESSIONS 128
#define NUM_MICRO_CATEGORIES 8

/**
 * @enum MicroCategory
 * @brief High-level affective and biological categories for micro-expressions.
 */
typedef enum {
    MICRO_CAT_COGNITIVE = 0,    /* Intellectual focus, curious inquiry, perplexity (0..15) */
    MICRO_CAT_AFFECTIONATE = 1, /* Social bonding, warmth, bashful peeks, gratitude (16..31) */
    MICRO_CAT_PLAYFUL = 2,      /* Mischief, sly smirks, teasing squints, cheeky twitches (32..47) */
    MICRO_CAT_STARTLE = 3,      /* Sudden alerts, wide-eyed perks, startle recoil (48..63) */
    MICRO_CAT_SKEPTICISM = 4,   /* Critical appraisal, narrowed scrutiny, wary side-eye (64..79) */
    MICRO_CAT_MELANCHOLY = 5,   /* Fleeting sorrow, gentle sigh droop, vulnerability (80..95) */
    MICRO_CAT_IRRITATION = 6,   /* Determined furrow, stubborn squint, annoyed pinch (96..111) */
    MICRO_CAT_DROWSINESS = 7    /* Upper lid lag, sleepy wobble, daydreaming glaze (112..127) */
} MicroCategory;

/**
 * @enum MicroExpressionId
 * @brief Complete taxonomy of 128 discrete biological micro-expressions.
 */
typedef enum {
    /* Category 0: Cognitive & Attentive Nuances (0..15) */
    MICRO_EXPR_INQUISITIVE_BROW_L = 0,
    MICRO_EXPR_INQUISITIVE_BROW_R,
    MICRO_EXPR_PERPLEXED_FURROW,
    MICRO_EXPR_DEEP_CONCENTRATION,
    MICRO_EXPR_PENSIVE_GAZE_DRIFT,
    MICRO_EXPR_SUDDEN_REALIZATION,
    MICRO_EXPR_ANALYTICAL_SCAN,
    MICRO_EXPR_THOUGHTFUL_SQUINT,
    MICRO_EXPR_FLEETING_HESITATION,
    MICRO_EXPR_FLEETING_CONFUSION,
    MICRO_EXPR_CURIOUS_PEER_UP,
    MICRO_EXPR_ATTENTIVE_PERK,
    MICRO_EXPR_MILD_DISTRACTION,
    MICRO_EXPR_PONDERING_LEFT,
    MICRO_EXPR_PONDERING_RIGHT,
    MICRO_EXPR_MEMORY_RECALL_DRIFT,

    /* Category 1: Affectionate, Social & Friendly Nuances (16..31) */
    MICRO_EXPR_SHY_FOND_PEEK = 16,
    MICRO_EXPR_SUBTLE_AFFECTION_SOFTEN,
    MICRO_EXPR_FLEETING_DUCHENNE_SMILE,
    MICRO_EXPR_WARM_GAZE_LINGER,
    MICRO_EXPR_BASHFUL_OUTER_DROOP,
    MICRO_EXPR_PLAYFUL_WINK_L,
    MICRO_EXPR_PLAYFUL_WINK_R,
    MICRO_EXPR_SUPPRESSED_GIGGLE,
    MICRO_EXPR_MODEST_DOWNCAST,
    MICRO_EXPR_GENTLE_GRATITUDE,
    MICRO_EXPR_LOVING_SQUINT,
    MICRO_EXPR_WELCOMING_PERK,
    MICRO_EXPR_SYMPATHETIC_TILT,
    MICRO_EXPR_SOFT_REASSURANCE,
    MICRO_EXPR_COY_LATERAL_GLANCE,
    MICRO_EXPR_CHEERFUL_SHIMMER,

    /* Category 2: Playful, Cheeky & Mischievous Nuances (32..47) */
    MICRO_EXPR_SLY_SMIRK_L = 32,
    MICRO_EXPR_SLY_SMIRK_R,
    MICRO_EXPR_CHEEKY_BROW_TWITCH,
    MICRO_EXPR_TEASING_ASYM_SQUINT,
    MICRO_EXPR_SCHEMING_NARROW,
    MICRO_EXPR_PLAYFUL_STARTLE_BOUNCE,
    MICRO_EXPR_FEIGNED_INNOCENCE,
    MICRO_EXPR_CONSPIRATORIAL_WINK,
    MICRO_EXPR_MOCK_SUSPICION,
    MICRO_EXPR_SASSY_BROW_ARCH,
    MICRO_EXPR_MISCHIEVOUS_SIDE_GLANCE,
    MICRO_EXPR_FROLIC_FLUTTER,
    MICRO_EXPR_SMUG_HALF_LID_L,
    MICRO_EXPR_SMUG_HALF_LID_R,
    MICRO_EXPR_GLEE_SHIVER,
    MICRO_EXPR_IMPISH_DART,

    /* Category 3: Startle, Surprise & Alertness Nuances (48..63) */
    MICRO_EXPR_MICRO_STARTLE_DILATION = 48,
    MICRO_EXPR_FLEETING_SHOCK_RECOIL,
    MICRO_EXPR_INVOLUNTARY_WIDE_PERK,
    MICRO_EXPR_SUDDEN_DOUBLE_TAKE,
    MICRO_EXPR_HIGH_VIGILANCE_FLARE,
    MICRO_EXPR_ALERT_DART_L,
    MICRO_EXPR_ALERT_DART_R,
    MICRO_EXPR_FLEETING_BEWILDERMENT,
    MICRO_EXPR_ASTONISHED_BREATH_HOLD,
    MICRO_EXPR_MICRO_GASP_SPURT,
    MICRO_EXPR_FLABBERGASTED_WIDEN,
    MICRO_EXPR_STARTLED_FREEZE,
    MICRO_EXPR_STUNNED_BLINK_RECOVERY,
    MICRO_EXPR_INCREDULOUS_BROW_COCK,
    MICRO_EXPR_FLEETING_DISBELIEF,
    MICRO_EXPR_ELECTRIC_JOLT_TWITCH,

    /* Category 4: Skepticism, Scrutiny & Caution Nuances (64..79) */
    MICRO_EXPR_CRITICAL_BROW_SLANT = 64,
    MICRO_EXPR_SKEPTICAL_SQUINT_L,
    MICRO_EXPR_SKEPTICAL_SQUINT_R,
    MICRO_EXPR_NARROWED_SCRUTINY,
    MICRO_EXPR_GUARDED_SIDE_EYE,
    MICRO_EXPR_DISAPPROVING_BROW_DROP,
    MICRO_EXPR_FLEETING_DISDAIN,
    MICRO_EXPR_HESITANT_SIDE_STEP,
    MICRO_EXPR_WARY_GAZE_FREEZE,
    MICRO_EXPR_APPREHENSIVE_OUTER_DROOP,
    MICRO_EXPR_CAUTIOUS_DISTANCE_SACCADE,
    MICRO_EXPR_UNIMPRESSED_FLAT_LID,
    MICRO_EXPR_QUIZZICAL_EYE_ROLL,
    MICRO_EXPR_QUESTIONING_SQUINT,
    MICRO_EXPR_SUSPICIOUS_SCAN_L,
    MICRO_EXPR_SUSPICIOUS_SCAN_R,

    /* Category 5: Vulnerability, Melancholy & Worry Nuances (80..95) */
    MICRO_EXPR_FLEETING_SORROW_DROOP = 80,
    MICRO_EXPR_TENDER_SYMPATHY,
    MICRO_EXPR_ANXIOUS_MICRO_FLUTTER,
    MICRO_EXPR_LONELY_DOWNWARD_AVERT,
    MICRO_EXPR_HESITANT_UPWARD_GLANCE,
    MICRO_EXPR_SUPPRESSED_WINCE,
    MICRO_EXPR_SUBTLE_MELANCHOLIC_SIGH,
    MICRO_EXPR_FRAGILE_GAZE_CAST,
    MICRO_EXPR_FORLORN_DISTANT_LOOK,
    MICRO_EXPR_REGRETFUL_LID_LOWER,
    MICRO_EXPR_TIMID_SHYNESS_PEEK,
    MICRO_EXPR_SOFT_HEARTACHE_TREMOR,
    MICRO_EXPR_APOLOGETIC_BROW_SPLAY,
    MICRO_EXPR_NEEDY_PUPIL_DILATION,
    MICRO_EXPR_FLEETING_MELANCHOLY_TILT,
    MICRO_EXPR_WISTFUL_CONTEMPLATION,

    /* Category 6: Irritation, Defiance & Determination Nuances (96..111) */
    MICRO_EXPR_STERN_BROW_FURROW = 96,
    MICRO_EXPR_STUBBORN_SQUINT,
    MICRO_EXPR_IRRITATED_BROW_TWITCH,
    MICRO_EXPR_DEFIANT_NARROWING,
    MICRO_EXPR_GRUMPY_LOWERED_GAZE,
    MICRO_EXPR_FIERCE_FOCUS_LOCK,
    MICRO_EXPR_IMPATIENT_MICRO_SACCADE,
    MICRO_EXPR_SKEPTICAL_SIDE_GLARE,
    MICRO_EXPR_RESOLUTE_GAZE_FIXATION,
    MICRO_EXPR_SULLEN_EYE_CAST,
    MICRO_EXPR_ANNOYED_PALPEBRAL_PINCH,
    MICRO_EXPR_TENSE_INNER_BROW_NOTCH,
    MICRO_EXPR_PUFFED_CHEEK_TENSE,
    MICRO_EXPR_CHALLENGING_BROW_ARCH,
    MICRO_EXPR_STOIC_GAZE_SETTLE,
    MICRO_EXPR_GRITTED_CONCENTRATION,

    /* Category 7: Drowsiness, Relaxation & Biological Somnolence Nuances (112..127) */
    MICRO_EXPR_HEAVY_UPPER_LID_LAG = 112,
    MICRO_EXPR_DROWSY_SLIT_DRIFT,
    MICRO_EXPR_CONTENTED_SLOW_FLUTTER,
    MICRO_EXPR_PEACEFUL_SIGH_SOFTEN,
    MICRO_EXPR_RELAXED_EYE_FLATTENING,
    MICRO_EXPR_LAZILY_LOWERED_BROW,
    MICRO_EXPR_DAYDREAMING_DISTANT_GAZE,
    MICRO_EXPR_MICRO_YAWN_STRETCH,
    MICRO_EXPR_SWEET_SLUMBER_SETTLE,
    MICRO_EXPR_COMFORTED_SOFT_SACCADE,
    MICRO_EXPR_SLEEPY_WOBBLE_DRIFT,
    MICRO_EXPR_HYPNAGOGIC_FLUTTER,
    MICRO_EXPR_EXHAUSTED_HEAVY_GLAZE,
    MICRO_EXPR_WARM_DROWSE_SPLAY,
    MICRO_EXPR_COZY_EYELID_SNUGGLE,
    MICRO_EXPR_RESTING_BLISS_SLIT = 127
} MicroExpressionId;

#define MICRO_EXPR_NONE ((MicroExpressionId)-1)

/**
 * @struct MicroExpressionDelta
 * @brief Continuous physical somatic offsets computed at 60 FPS.
 */
typedef struct {
    float delta_w_l, delta_w_r;         /* Width offsets in pixels */
    float delta_h_l, delta_h_r;         /* Height offsets in pixels */
    float delta_n_l, delta_n_r;         /* Squircle Lamé exponent offsets */
    float delta_tilt_l, delta_tilt_r;   /* Ocular roll torsion in radians */
    float delta_brow_l, delta_brow_r;   /* Brow slant angle offsets in radians */
    float delta_cheek_l, delta_cheek_r; /* Cheek slant angle offsets in radians */
    float delta_upper_l, delta_upper_r; /* Upper eyelid droop progress offsets [0.0..1.0] */
    float delta_lower_l, delta_lower_r; /* Lower eyelid push-up progress offsets [0.0..1.0] */
    float delta_gaze_x, delta_gaze_y;   /* Micro-saccade fixational glance offsets in pixels */
    float intensity;                    /* Current physical envelope value [0.0..1.0] */
    bool active;                        /* Whether a micro-expression is currently animating */
} MicroExpressionDelta;

/**
 * @struct MicroExpressionDef
 * @brief Compact ROM descriptor for each of the 128 micro-expression archetypes.
 */
typedef struct {
    const char* name;          /* Unique identifier name */
    uint8_t category;          /* MicroCategory index (0..7) */
    int8_t delta_w_scaled;     /* Width delta in units of 0.5px [-10..+10] */
    int8_t delta_h_scaled;     /* Height delta in units of 0.5px [-10..+10] */
    int8_t delta_brow_l_deg;   /* Left brow tilt delta in degrees [-20..+20] */
    int8_t delta_brow_r_deg;   /* Right brow tilt delta in degrees [-20..+20] */
    int8_t delta_cheek_l_deg;  /* Left cheek tilt delta in degrees [-15..+15] */
    int8_t delta_cheek_r_deg;  /* Right cheek tilt delta in degrees [-15..+15] */
    int8_t delta_upper_l_pct;  /* Left upper lid droop in % [0..35] */
    int8_t delta_upper_r_pct;  /* Right upper lid droop in % [0..35] */
    int8_t delta_lower_l_pct;  /* Left lower lid push in % [0..35] */
    int8_t delta_lower_r_pct;  /* Right lower lid push in % [0..35] */
    int8_t delta_n_scaled;     /* Lamé exponent delta in units of 0.1 [-10..+10] */
    int8_t delta_gaze_x_px;    /* Micro-saccade horizontal gaze shift in px [-4..+4] */
    int8_t delta_gaze_y_px;    /* Micro-saccade vertical gaze shift in px [-3..+3] */
    uint16_t onset_ms;         /* Minimum-jerk motor recruitment rise time in ms */
    uint16_t dwell_ms;         /* Myogenic apex dwell time in ms */
    uint16_t decay_ms;         /* Viscoelastic tissue relaxation time in ms */
} MicroExpressionDef;

/**
 * @brief Initializes the micro-expression kinematic and generative state.
 */
void initMicroExpressionEngine(void);

/**
 * @brief Advances the continuous 60 FPS minimum-jerk and viscoelastic spring dynamics.
 * @param dt_sec Delta time in seconds (~0.0166s).
 */
void updateMicroExpressionEngine(float dt_sec);

/**
 * @brief Triggers a micro-expression by discrete enumeration ID.
 * @param id MicroExpressionId (0..127).
 * @param intensity Amplitude scaling [0.0..1.0].
 * @return True if successfully initiated, false if rejected or invalid.
 */
bool triggerMicroExpression(MicroExpressionId id, float intensity);

/**
 * @brief Triggers a micro-expression by canonical string name.
 * @param name Null-terminated identifier string (case-insensitive).
 * @param intensity Amplitude scaling [0.0..1.0].
 * @return True if found and triggered.
 */
bool triggerMicroExpressionByName(const char* name, float intensity);

/**
 * @brief Immediately halts and resets active micro-expression.
 */
void stopMicroExpression(void);

/**
 * @brief Retrieves the active continuous physical somatic offsets for facial integration.
 * @return MicroExpressionDelta struct.
 */
MicroExpressionDelta getActiveMicroExpressionDelta(void);

/**
 * @brief Checks if a micro-expression is currently animating.
 */
bool isMicroExpressionActive(void);

/**
 * @brief Retrieves the currently active micro-expression ID, or MICRO_EXPR_NONE.
 */
MicroExpressionId getActiveMicroExpressionId(void);

/**
 * @brief Retrieves the canonical name string of a micro-expression.
 */
const char* getMicroExpressionName(MicroExpressionId id);

/**
 * @brief Retrieves the category index (0..7) of a micro-expression.
 */
uint8_t getMicroExpressionCategory(MicroExpressionId id);

/**
 * @brief Retrieves the human-readable category name.
 */
const char* getMicroCategoryName(uint8_t cat_id);

/**
 * @brief Retrieves normalized execution progress [0.0..1.0] of current micro-expression.
 */
float getMicroExpressionProgress(void);

/**
 * @brief Looks up a micro-expression ID by canonical name.
 * @return MicroExpressionId or MICRO_EXPR_NONE if not found.
 */
MicroExpressionId findMicroExpressionByName(const char* name);

/**
 * @brief Evaluates autonomous spontaneous micro-expression generation.
 * @param dt_sec Delta time in seconds.
 * @param valence Current affective valence [-1.0..1.0].
 * @param arousal Current affective arousal [0.0..1.0].
 * @param curiosity Homeostatic curiosity drive [0.0..1.0].
 * @param mischief Homeostatic mischief drive [0.0..1.0].
 * @param social Homeostatic social drive [0.0..1.0].
 * @param boredom Homeostatic boredom drive [0.0..1.0].
 * @param fatigue Homeostatic fatigue drive [0.0..1.0].
 * @param is_detected Human companion detected flag.
 */
void updateAutonomousMicroExpressions(float dt_sec, float valence, float arousal,
                                      float curiosity, float mischief, float social,
                                      float boredom, float fatigue, bool is_detected);

#ifdef __cplusplus
}
#endif

#endif /* LORE_MICRO_EXPRESSION_ENGINE_H */
