#include "actuator_policy.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define ACTUATOR_POLICY_PI 3.14159265358979323846

static bool source_valid(actuator_command_source_t source)
{
    return source == ACTUATOR_COMMAND_SOURCE_TOPIC ||
           source == ACTUATOR_COMMAND_SOURCE_SERVICE;
}

static int sign_i32(int32_t value)
{
    return (value > 0) - (value < 0);
}

static float clamp_float(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

bool actuator_policy_config_valid(const actuator_policy_config_t *config)
{
    return config != NULL &&
           config->command_timeout_ms > 0U &&
           config->direction_neutral_ms > 0U &&
           isfinite(config->forward_speed_limit_mps) &&
           config->forward_speed_limit_mps > 0.0f &&
           isfinite(config->reverse_speed_limit_mps) &&
           config->reverse_speed_limit_mps > 0.0f &&
           config->forward_erpm_limit > 0 &&
           config->reverse_erpm_limit > 0 &&
           isfinite(config->steering_left_limit_rad) &&
           config->steering_left_limit_rad > 0.0f &&
           isfinite(config->steering_right_limit_rad) &&
           config->steering_right_limit_rad < 0.0f &&
           isfinite(config->wheel_diameter_m) &&
           config->wheel_diameter_m > 0.0f &&
           isfinite(config->gear_reduction) &&
           config->gear_reduction > 0.0f &&
           config->motor_pole_pairs > 0U &&
           (config->forward_sign == 1 || config->forward_sign == -1) &&
           config->steering_left_servo_deg < config->steering_center_servo_deg &&
           config->steering_center_servo_deg < config->steering_right_servo_deg;
}

actuator_policy_error_t actuator_policy_init(
    actuator_policy_t *policy,
    const actuator_policy_config_t *config)
{
    if (policy == NULL || !actuator_policy_config_valid(config)) {
        return ACTUATOR_POLICY_ERROR_INVALID_CONFIG;
    }

    memset(policy, 0, sizeof(*policy));
    policy->config = *config;
    policy->runtime_forward_erpm_limit = config->forward_erpm_limit;
    policy->runtime_reverse_erpm_limit = config->reverse_erpm_limit;
    policy->session_brake = true;
    policy->command_source = ACTUATOR_COMMAND_SOURCE_TOPIC;
    policy->initialized = true;
    return ACTUATOR_POLICY_OK;
}

actuator_policy_error_t actuator_policy_submit(
    actuator_policy_t *policy,
    const actuator_policy_command_t *command,
    actuator_command_source_t source,
    uint64_t now_ms)
{
    if (policy == NULL || !policy->initialized || command == NULL ||
        !source_valid(source)) {
        return ACTUATOR_POLICY_ERROR_INVALID_COMMAND;
    }

    policy->command_source = source;

    bool finite = isfinite(command->speed_mps) &&
                  isfinite(command->steering_angle_rad);
    if (command->brake) {
        policy->command = (actuator_policy_command_t) {
            .speed_mps = 0.0f,
            .steering_angle_rad = 0.0f,
            .brake = true,
        };
        policy->direction_change_pending = false;
        policy->command_time_ms = now_ms;
        policy->has_command = true;
        policy->invalid_command_seen = !finite;
        return finite ? ACTUATOR_POLICY_OK : ACTUATOR_POLICY_ERROR_INVALID_COMMAND;
    }

    if (!finite) {
        policy->has_command = false;
        policy->direction_change_pending = false;
        policy->invalid_command_seen = true;
        policy->last_drive_erpm = 0;
        return ACTUATOR_POLICY_ERROR_INVALID_COMMAND;
    }

    policy->command = *command;
    policy->command_time_ms = now_ms;
    policy->has_command = true;
    policy->invalid_command_seen = false;
    return ACTUATOR_POLICY_OK;
}

void actuator_policy_set_session_brake(actuator_policy_t *policy, bool asserted)
{
    if (policy == NULL || !policy->initialized) {
        return;
    }
    if (policy->session_brake != asserted) {
        policy->has_command = false;
        policy->invalid_command_seen = false;
        policy->direction_change_pending = false;
        policy->last_drive_erpm = 0;
    }
    policy->session_brake = asserted;
}

void actuator_policy_set_fault(actuator_policy_t *policy, bool asserted)
{
    if (policy == NULL || !policy->initialized) {
        return;
    }
    if (policy->fault_active != asserted) {
        policy->has_command = false;
        policy->invalid_command_seen = false;
        policy->direction_change_pending = false;
        policy->last_drive_erpm = 0;
    }
    policy->fault_active = asserted;
}

void actuator_policy_latch_stop(actuator_policy_t *policy)
{
    if (policy == NULL || !policy->initialized) {
        return;
    }
    policy->stop_latched = true;
    policy->has_command = false;
    policy->direction_change_pending = false;
    policy->last_drive_erpm = 0;
}

actuator_policy_error_t actuator_policy_try_rearm_stop(actuator_policy_t *policy)
{
    if (policy == NULL || !policy->initialized) {
        return ACTUATOR_POLICY_ERROR_INVALID_CONFIG;
    }
    if (!policy->stop_latched) {
        return ACTUATOR_POLICY_OK;
    }
    if (!policy->session_brake || policy->fault_active || policy->has_command) {
        return ACTUATOR_POLICY_ERROR_STOP_LATCHED;
    }
    policy->stop_latched = false;
    return ACTUATOR_POLICY_OK;
}

actuator_policy_error_t actuator_policy_set_runtime_erpm_limits(
    actuator_policy_t *policy,
    int32_t forward_limit,
    int32_t reverse_limit)
{
    if (policy == NULL || !policy->initialized || forward_limit <= 0 ||
        reverse_limit <= 0 || forward_limit > policy->config.forward_erpm_limit ||
        reverse_limit > policy->config.reverse_erpm_limit) {
        return ACTUATOR_POLICY_ERROR_INVALID_CONFIG;
    }
    policy->runtime_forward_erpm_limit = forward_limit;
    policy->runtime_reverse_erpm_limit = reverse_limit;
    return ACTUATOR_POLICY_OK;
}

void actuator_policy_clear_runtime_erpm_limits(actuator_policy_t *policy)
{
    if (policy == NULL || !policy->initialized) {
        return;
    }
    policy->runtime_forward_erpm_limit = policy->config.forward_erpm_limit;
    policy->runtime_reverse_erpm_limit = policy->config.reverse_erpm_limit;
}

static int32_t speed_to_erpm(const actuator_policy_config_t *config, float speed_mps)
{
    double circumference = ACTUATOR_POLICY_PI * (double)config->wheel_diameter_m;
    double erpm = (double)speed_mps * 60.0 * (double)config->gear_reduction *
                  (double)config->motor_pole_pairs / circumference *
                  (double)config->forward_sign;
    if (!isfinite(erpm) || erpm > (double)INT32_MAX || erpm < (double)INT32_MIN) {
        return 0;
    }
    return (int32_t)lround(erpm);
}

static uint8_t steering_to_servo(
    const actuator_policy_config_t *config,
    float steering_rad)
{
    double servo;
    if (steering_rad >= 0.0f) {
        double ratio = (double)steering_rad /
                       (double)config->steering_left_limit_rad;
        servo = (double)config->steering_center_servo_deg -
                ratio * (double)(config->steering_center_servo_deg -
                                 config->steering_left_servo_deg);
    } else {
        double ratio = (double)steering_rad /
                       (double)config->steering_right_limit_rad;
        servo = (double)config->steering_center_servo_deg +
                ratio * (double)(config->steering_right_servo_deg -
                                 config->steering_center_servo_deg);
    }
    return (uint8_t)lround(servo);
}

static actuator_policy_output_t inhibited_output(
    const actuator_policy_t *policy,
    actuator_policy_reason_t reason)
{
    return (actuator_policy_output_t) {
        .reason = reason,
        .vesc_erpm = 0,
        .steering_servo_deg = policy->config.steering_center_servo_deg,
        .brake_requested = true,
        .speed_limited = false,
        .steering_limited = false,
        .source = policy->command_source,
    };
}

actuator_policy_output_t actuator_policy_evaluate(
    actuator_policy_t *policy,
    uint64_t now_ms)
{
    if (policy == NULL || !policy->initialized) {
        actuator_policy_output_t invalid = {0};
        invalid.reason = ACTUATOR_POLICY_REASON_INVALID_COMMAND;
        invalid.brake_requested = true;
        return invalid;
    }
    if (policy->stop_latched) {
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_STOP_LATCHED);
    }
    if (policy->session_brake) {
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_SESSION_INHIBIT);
    }
    if (policy->fault_active) {
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_FAULT);
    }
    if (policy->invalid_command_seen && !policy->has_command) {
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_INVALID_COMMAND);
    }
    if (!policy->has_command || now_ms < policy->command_time_ms ||
        now_ms - policy->command_time_ms > policy->config.command_timeout_ms) {
        policy->last_drive_erpm = 0;
        policy->direction_change_pending = false;
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_COMMAND_STALE);
    }
    if (policy->command.brake) {
        policy->last_drive_erpm = 0;
        policy->direction_change_pending = false;
        return inhibited_output(policy, ACTUATOR_POLICY_REASON_EXPLICIT_BRAKE);
    }

    float speed = clamp_float(policy->command.speed_mps,
                              -policy->config.reverse_speed_limit_mps,
                              policy->config.forward_speed_limit_mps);
    float steering = clamp_float(policy->command.steering_angle_rad,
                                 policy->config.steering_right_limit_rad,
                                 policy->config.steering_left_limit_rad);
    int32_t erpm = speed_to_erpm(&policy->config, speed);
    bool speed_limited = speed != policy->command.speed_mps;
    bool steering_limited = steering != policy->command.steering_angle_rad;

    int32_t signed_forward = erpm * policy->config.forward_sign;
    if (signed_forward > policy->runtime_forward_erpm_limit) {
        erpm = policy->runtime_forward_erpm_limit * policy->config.forward_sign;
        speed_limited = true;
    } else if (signed_forward < -policy->runtime_reverse_erpm_limit) {
        erpm = -policy->runtime_reverse_erpm_limit * policy->config.forward_sign;
        speed_limited = true;
    }

    actuator_policy_output_t output = {
        .reason = ACTUATOR_POLICY_REASON_READY,
        .vesc_erpm = erpm,
        .steering_servo_deg = steering_to_servo(&policy->config, steering),
        .brake_requested = false,
        .speed_limited = speed_limited,
        .steering_limited = steering_limited,
        .source = policy->command_source,
    };

    if (policy->direction_change_pending) {
        if (erpm == 0 || sign_i32(policy->last_drive_erpm) == sign_i32(erpm)) {
            policy->direction_change_pending = false;
        } else if (now_ms < policy->direction_change_time_ms ||
                   now_ms - policy->direction_change_time_ms <
                       policy->config.direction_neutral_ms) {
            output.reason = ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL;
            output.vesc_erpm = 0;
            return output;
        }
        policy->direction_change_pending = false;
    } else if (policy->last_drive_erpm != 0 && erpm != 0 &&
               sign_i32(policy->last_drive_erpm) != sign_i32(erpm)) {
        policy->direction_change_pending = true;
        policy->direction_change_time_ms = now_ms;
        output.reason = ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL;
        output.vesc_erpm = 0;
        return output;
    }

    policy->last_drive_erpm = erpm;
    return output;
}

const char *actuator_policy_reason_name(actuator_policy_reason_t reason)
{
    switch (reason) {
    case ACTUATOR_POLICY_REASON_STOP_LATCHED:
        return "stop_latched";
    case ACTUATOR_POLICY_REASON_SESSION_INHIBIT:
        return "session_inhibit";
    case ACTUATOR_POLICY_REASON_FAULT:
        return "fault";
    case ACTUATOR_POLICY_REASON_COMMAND_STALE:
        return "command_stale";
    case ACTUATOR_POLICY_REASON_EXPLICIT_BRAKE:
        return "explicit_brake";
    case ACTUATOR_POLICY_REASON_INVALID_COMMAND:
        return "invalid_command";
    case ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL:
        return "direction_neutral";
    case ACTUATOR_POLICY_REASON_READY:
        return "ready";
    default:
        return "unknown";
    }
}
