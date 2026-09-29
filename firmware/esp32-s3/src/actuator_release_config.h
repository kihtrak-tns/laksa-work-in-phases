#pragma once

/*
 * Deliberate release gate.
 *
 * These zero sentinels are not car values. A release reviewer must replace
 * every sentinel with an evidence-backed value and set the reviewed flag to 1.
 * Until then, the ESP-IDF firmware build fails and only the pure host policy
 * tests are supported.
 */
#define LAKSA_ACTUATOR_RELEASE_CONFIG_REVIEWED 0

#define LAKSA_REVIEWED_VESC_BRAKE_CURRENT_A 0.0f
#define LAKSA_REVIEWED_FORWARD_SPEED_LIMIT_MPS 0.0f
#define LAKSA_REVIEWED_REVERSE_SPEED_LIMIT_MPS 0.0f
#define LAKSA_REVIEWED_FORWARD_ERPM_LIMIT 0
#define LAKSA_REVIEWED_REVERSE_ERPM_LIMIT 0
#define LAKSA_REVIEWED_STEERING_LEFT_LIMIT_RAD 0.0f
#define LAKSA_REVIEWED_STEERING_RIGHT_LIMIT_RAD 0.0f
#define LAKSA_REVIEWED_WHEEL_DIAMETER_M 0.0f
#define LAKSA_REVIEWED_GEAR_REDUCTION 0.0f
#define LAKSA_REVIEWED_MOTOR_POLE_PAIRS 0
#define LAKSA_REVIEWED_FORWARD_SIGN 0
#define LAKSA_REVIEWED_STEERING_LEFT_SERVO_DEG 0
#define LAKSA_REVIEWED_STEERING_CENTER_SERVO_DEG 0
#define LAKSA_REVIEWED_STEERING_RIGHT_SERVO_DEG 0

#if !LAKSA_ACTUATOR_RELEASE_CONFIG_REVIEWED
#error "NON-RELEASABLE: review brake action/current, forward/reverse limits, conversion, and steering endpoints"
#endif

/* These integer-only checks become relevant after review is asserted.
 * actuator_policy_config_valid() checks the floating-point values because the
 * C preprocessor cannot evaluate them. */
#if LAKSA_ACTUATOR_RELEASE_CONFIG_REVIEWED && \
    (LAKSA_REVIEWED_FORWARD_ERPM_LIMIT <= 0 || \
     LAKSA_REVIEWED_REVERSE_ERPM_LIMIT <= 0 || \
     LAKSA_REVIEWED_MOTOR_POLE_PAIRS <= 0 || \
     (LAKSA_REVIEWED_FORWARD_SIGN != 1 && LAKSA_REVIEWED_FORWARD_SIGN != -1))
#error "Reviewed actuator release values must be nonzero and internally valid"
#endif
