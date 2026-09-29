#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACTUATOR_COMMAND_SOURCE_TOPIC = 0,
    ACTUATOR_COMMAND_SOURCE_SERVICE = 1,
} actuator_command_source_t;

typedef enum {
    ACTUATOR_POLICY_OK = 0,
    ACTUATOR_POLICY_ERROR_INVALID_COMMAND = 1,
    ACTUATOR_POLICY_ERROR_INVALID_CONFIG = 2,
    ACTUATOR_POLICY_ERROR_STOP_LATCHED = 3,
} actuator_policy_error_t;

typedef enum {
    ACTUATOR_POLICY_REASON_STOP_LATCHED,
    ACTUATOR_POLICY_REASON_SESSION_INHIBIT,
    ACTUATOR_POLICY_REASON_FAULT,
    ACTUATOR_POLICY_REASON_COMMAND_STALE,
    ACTUATOR_POLICY_REASON_EXPLICIT_BRAKE,
    ACTUATOR_POLICY_REASON_INVALID_COMMAND,
    ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL,
    ACTUATOR_POLICY_REASON_READY,
} actuator_policy_reason_t;

typedef struct {
    uint32_t command_timeout_ms;
    uint32_t direction_neutral_ms;
    float forward_speed_limit_mps;
    float reverse_speed_limit_mps;
    int32_t forward_erpm_limit;
    int32_t reverse_erpm_limit;
    float steering_left_limit_rad;
    float steering_right_limit_rad;
    float wheel_diameter_m;
    float gear_reduction;
    uint16_t motor_pole_pairs;
    int8_t forward_sign;
    uint8_t steering_left_servo_deg;
    uint8_t steering_center_servo_deg;
    uint8_t steering_right_servo_deg;
} actuator_policy_config_t;

typedef struct {
    float speed_mps;
    float steering_angle_rad;
    bool brake;
} actuator_policy_command_t;

typedef struct {
    actuator_policy_reason_t reason;
    int32_t vesc_erpm;
    uint8_t steering_servo_deg;
    bool brake_requested;
    bool speed_limited;
    bool steering_limited;
    actuator_command_source_t source;
} actuator_policy_output_t;

typedef struct {
    actuator_policy_config_t config;
    actuator_policy_command_t command;
    uint64_t command_time_ms;
    uint64_t direction_change_time_ms;
    int32_t last_drive_erpm;
    int32_t runtime_forward_erpm_limit;
    int32_t runtime_reverse_erpm_limit;
    actuator_command_source_t command_source;
    bool initialized;
    bool session_brake;
    bool stop_latched;
    bool fault_active;
    bool has_command;
    bool invalid_command_seen;
    bool direction_change_pending;
} actuator_policy_t;

bool actuator_policy_config_valid(const actuator_policy_config_t *config);

actuator_policy_error_t actuator_policy_init(
    actuator_policy_t *policy,
    const actuator_policy_config_t *config);

actuator_policy_error_t actuator_policy_submit(
    actuator_policy_t *policy,
    const actuator_policy_command_t *command,
    actuator_command_source_t source,
    uint64_t now_ms);

void actuator_policy_set_session_brake(
    actuator_policy_t *policy,
    bool asserted);

void actuator_policy_set_fault(
    actuator_policy_t *policy,
    bool asserted);

void actuator_policy_latch_stop(actuator_policy_t *policy);

actuator_policy_error_t actuator_policy_try_rearm_stop(
    actuator_policy_t *policy);

actuator_policy_error_t actuator_policy_set_runtime_erpm_limits(
    actuator_policy_t *policy,
    int32_t forward_limit,
    int32_t reverse_limit);

void actuator_policy_clear_runtime_erpm_limits(actuator_policy_t *policy);

actuator_policy_output_t actuator_policy_evaluate(
    actuator_policy_t *policy,
    uint64_t now_ms);

const char *actuator_policy_reason_name(actuator_policy_reason_t reason);

#ifdef __cplusplus
}
#endif
