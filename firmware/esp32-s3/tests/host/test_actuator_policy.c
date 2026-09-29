#include "actuator_policy.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned tests_run;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define RUN(test)                   \
    do {                            \
        test();                     \
        ++tests_run;                \
        printf("PASS %s\n", #test); \
    } while (0)

/* Deliberately synthetic host-test values. They are not release constants and
 * do not describe the car. */
static actuator_policy_config_t fixture_config(void)
{
    return (actuator_policy_config_t) {
        .command_timeout_ms = 500,
        .direction_neutral_ms = 150,
        .forward_speed_limit_mps = 1.0f,
        .reverse_speed_limit_mps = 0.5f,
        .forward_erpm_limit = 500,
        .reverse_erpm_limit = 300,
        .steering_left_limit_rad = 0.4f,
        .steering_right_limit_rad = -0.2f,
        .wheel_diameter_m = 0.1f,
        .gear_reduction = 1.0f,
        .motor_pole_pairs = 1,
        .forward_sign = 1,
        .steering_left_servo_deg = 65,
        .steering_center_servo_deg = 100,
        .steering_right_servo_deg = 137,
    };
}

static actuator_policy_t new_policy(void)
{
    actuator_policy_t policy;
    actuator_policy_config_t config = fixture_config();
    CHECK(actuator_policy_init(&policy, &config) == ACTUATOR_POLICY_OK);
    return policy;
}

static actuator_policy_command_t command(float speed, float steering, bool brake)
{
    return (actuator_policy_command_t) {
        .speed_mps = speed,
        .steering_angle_rad = steering,
        .brake = brake,
    };
}

static void release_and_submit(
    actuator_policy_t *policy,
    actuator_policy_command_t value,
    actuator_command_source_t source,
    uint64_t now_ms)
{
    actuator_policy_set_session_brake(policy, false);
    CHECK(actuator_policy_submit(policy, &value, source, now_ms) == ACTUATOR_POLICY_OK);
}

static void test_invalid_config_rejected(void)
{
    actuator_policy_t policy;
    actuator_policy_config_t config = fixture_config();
    config.forward_erpm_limit = 0;
    CHECK(!actuator_policy_config_valid(&config));
    CHECK(actuator_policy_init(&policy, &config) ==
          ACTUATOR_POLICY_ERROR_INVALID_CONFIG);
}

static void test_boot_is_session_inhibited(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_output_t output = actuator_policy_evaluate(&policy, 0);
    CHECK(output.reason == ACTUATOR_POLICY_REASON_SESSION_INHIBIT);
    CHECK(output.brake_requested);
    CHECK(output.vesc_erpm == 0);
    CHECK(output.steering_servo_deg == 100);
}

static void test_session_release_requires_new_command(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(0.5f, 0.0f, false);
    CHECK(actuator_policy_submit(&policy, &value,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 10) ==
          ACTUATOR_POLICY_OK);
    actuator_policy_set_session_brake(&policy, false);
    CHECK(actuator_policy_evaluate(&policy, 10).reason ==
          ACTUATOR_POLICY_REASON_COMMAND_STALE);
    CHECK(actuator_policy_submit(&policy, &value,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 11) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 11).reason ==
          ACTUATOR_POLICY_REASON_READY);
}

static void test_topic_and_service_enforcement_identical(void)
{
    actuator_policy_t topic = new_policy();
    actuator_policy_t service = new_policy();
    actuator_policy_command_t value = command(10.0f, -1.0f, false);
    release_and_submit(&topic, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 20);
    release_and_submit(&service, value, ACTUATOR_COMMAND_SOURCE_SERVICE, 20);
    actuator_policy_output_t a = actuator_policy_evaluate(&topic, 20);
    actuator_policy_output_t b = actuator_policy_evaluate(&service, 20);
    CHECK(a.reason == b.reason);
    CHECK(a.vesc_erpm == b.vesc_erpm);
    CHECK(a.steering_servo_deg == b.steering_servo_deg);
    CHECK(a.brake_requested == b.brake_requested);
    CHECK(a.speed_limited && b.speed_limited);
    CHECK(a.steering_limited && b.steering_limited);
    CHECK(a.source == ACTUATOR_COMMAND_SOURCE_TOPIC);
    CHECK(b.source == ACTUATOR_COMMAND_SOURCE_SERVICE);
}

static void test_explicit_command_brake(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(0.5f, 0.1f, true);
    release_and_submit(&policy, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 30);
    actuator_policy_output_t output = actuator_policy_evaluate(&policy, 30);
    CHECK(output.reason == ACTUATOR_POLICY_REASON_EXPLICIT_BRAKE);
    CHECK(output.brake_requested);
    CHECK(output.steering_servo_deg == 100);
}

static void test_session_brake_assertion(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(0.5f, 0.1f, false);
    release_and_submit(&policy, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 40);
    CHECK(actuator_policy_evaluate(&policy, 40).reason ==
          ACTUATOR_POLICY_REASON_READY);
    actuator_policy_set_session_brake(&policy, true);
    CHECK(actuator_policy_evaluate(&policy, 41).reason ==
          ACTUATOR_POLICY_REASON_SESSION_INHIBIT);
}

static void test_timeout_boundary_is_500_ms(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(0.5f, 0.0f, false);
    release_and_submit(&policy, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 100);
    CHECK(actuator_policy_evaluate(&policy, 600).reason ==
          ACTUATOR_POLICY_REASON_READY);
    CHECK(actuator_policy_evaluate(&policy, 601).reason ==
          ACTUATOR_POLICY_REASON_COMMAND_STALE);
}

static void test_fault_requires_new_command_after_clear(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(0.5f, 0.0f, false);
    release_and_submit(&policy, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    actuator_policy_set_fault(&policy, true);
    CHECK(actuator_policy_evaluate(&policy, 1).reason ==
          ACTUATOR_POLICY_REASON_FAULT);
    actuator_policy_set_fault(&policy, false);
    CHECK(actuator_policy_evaluate(&policy, 2).reason ==
          ACTUATOR_POLICY_REASON_COMMAND_STALE);
}

static void test_nonfinite_commands_rejected(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_set_session_brake(&policy, false);
    actuator_policy_command_t nan_command = command(NAN, 0.0f, false);
    CHECK(actuator_policy_submit(&policy, &nan_command,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 0) ==
          ACTUATOR_POLICY_ERROR_INVALID_COMMAND);
    CHECK(actuator_policy_evaluate(&policy, 0).reason ==
          ACTUATOR_POLICY_REASON_INVALID_COMMAND);
    actuator_policy_command_t inf_command = command(0.0f, INFINITY, false);
    CHECK(actuator_policy_submit(&policy, &inf_command,
                                 ACTUATOR_COMMAND_SOURCE_SERVICE, 1) ==
          ACTUATOR_POLICY_ERROR_INVALID_COMMAND);
    CHECK(actuator_policy_evaluate(&policy, 1).brake_requested);
}

static void test_brake_wins_even_when_fields_nonfinite(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_set_session_brake(&policy, false);
    actuator_policy_command_t value = command(NAN, INFINITY, true);
    CHECK(actuator_policy_submit(&policy, &value,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 0) ==
          ACTUATOR_POLICY_ERROR_INVALID_COMMAND);
    actuator_policy_output_t output = actuator_policy_evaluate(&policy, 0);
    CHECK(output.reason == ACTUATOR_POLICY_REASON_EXPLICIT_BRAKE);
    CHECK(output.brake_requested);
}

static void test_speed_limit_precedes_conversion(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t value = command(10.0f, 0.0f, false);
    release_and_submit(&policy, value, ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    actuator_policy_output_t output = actuator_policy_evaluate(&policy, 0);
    /* Fixture conversion of the clamped 1.0 m/s is 191 eRPM. Converting the
     * raw 10.0 m/s first would not produce this value. */
    CHECK(output.vesc_erpm == 191);
    CHECK(output.speed_limited);
}

static void test_independent_forward_reverse_erpm_limits(void)
{
    actuator_policy_config_t config = fixture_config();
    config.forward_speed_limit_mps = 10.0f;
    config.reverse_speed_limit_mps = 10.0f;
    config.forward_erpm_limit = 120;
    config.reverse_erpm_limit = 80;
    actuator_policy_t forward;
    actuator_policy_t reverse;
    CHECK(actuator_policy_init(&forward, &config) == ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_init(&reverse, &config) == ACTUATOR_POLICY_OK);
    release_and_submit(&forward, command(1.0f, 0.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    release_and_submit(&reverse, command(-1.0f, 0.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    CHECK(actuator_policy_evaluate(&forward, 0).vesc_erpm == 120);
    CHECK(actuator_policy_evaluate(&reverse, 0).vesc_erpm == -80);
}

static void test_asymmetric_steering_limits(void)
{
    actuator_policy_t left = new_policy();
    actuator_policy_t right = new_policy();
    release_and_submit(&left, command(0.0f, 1.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    release_and_submit(&right, command(0.0f, -1.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    actuator_policy_output_t left_out = actuator_policy_evaluate(&left, 0);
    actuator_policy_output_t right_out = actuator_policy_evaluate(&right, 0);
    CHECK(left_out.steering_servo_deg == 65);
    CHECK(right_out.steering_servo_deg == 137);
    CHECK(left_out.steering_limited && right_out.steering_limited);
}

static void test_direction_change_has_neutral_hold(void)
{
    actuator_policy_t policy = new_policy();
    release_and_submit(&policy, command(0.5f, 0.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    CHECK(actuator_policy_evaluate(&policy, 0).vesc_erpm > 0);
    CHECK(actuator_policy_submit(&policy, &(actuator_policy_command_t) {
                                     .speed_mps = -0.5f,
                                     .steering_angle_rad = 0.0f,
                                     .brake = false,
                                 },
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 10) ==
          ACTUATOR_POLICY_OK);
    actuator_policy_output_t neutral = actuator_policy_evaluate(&policy, 10);
    CHECK(neutral.reason == ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL);
    CHECK(neutral.vesc_erpm == 0);
    CHECK(!neutral.brake_requested);
    CHECK(actuator_policy_evaluate(&policy, 159).reason ==
          ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL);
    CHECK(actuator_policy_evaluate(&policy, 160).vesc_erpm < 0);
}

static void test_repeated_commands_do_not_extend_direction_hold(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_command_t reverse = command(-0.5f, 0.0f, false);
    release_and_submit(&policy, command(0.5f, 0.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    CHECK(actuator_policy_evaluate(&policy, 0).vesc_erpm > 0);
    CHECK(actuator_policy_submit(&policy, &reverse,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 10) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 10).reason ==
          ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL);
    CHECK(actuator_policy_submit(&policy, &reverse,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 100) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 100).reason ==
          ACTUATOR_POLICY_REASON_DIRECTION_NEUTRAL);
    CHECK(actuator_policy_submit(&policy, &reverse,
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 160) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 160).vesc_erpm < 0);
}

static void test_runtime_cap_can_only_lower_and_clear(void)
{
    actuator_policy_t policy = new_policy();
    CHECK(actuator_policy_set_runtime_erpm_limits(&policy, 100, 70) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_set_runtime_erpm_limits(&policy, 501, 70) ==
          ACTUATOR_POLICY_ERROR_INVALID_CONFIG);
    release_and_submit(&policy, command(1.0f, 0.0f, false),
                       ACTUATOR_COMMAND_SOURCE_TOPIC, 0);
    CHECK(actuator_policy_evaluate(&policy, 0).vesc_erpm == 100);
    actuator_policy_clear_runtime_erpm_limits(&policy);
    CHECK(actuator_policy_submit(&policy, &(actuator_policy_command_t) {
                                     .speed_mps = 1.0f,
                                     .steering_angle_rad = 0.0f,
                                     .brake = false,
                                 },
                                 ACTUATOR_COMMAND_SOURCE_TOPIC, 1) ==
          ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 1).vesc_erpm == 191);
}

static void test_routine_brake_release_cannot_clear_stop_latch(void)
{
    actuator_policy_t policy = new_policy();
    actuator_policy_latch_stop(&policy);
    actuator_policy_set_session_brake(&policy, false);
    CHECK(actuator_policy_evaluate(&policy, 0).reason ==
          ACTUATOR_POLICY_REASON_STOP_LATCHED);
    CHECK(actuator_policy_try_rearm_stop(&policy) ==
          ACTUATOR_POLICY_ERROR_STOP_LATCHED);
    actuator_policy_set_session_brake(&policy, true);
    CHECK(actuator_policy_try_rearm_stop(&policy) == ACTUATOR_POLICY_OK);
    CHECK(actuator_policy_evaluate(&policy, 0).reason ==
          ACTUATOR_POLICY_REASON_SESSION_INHIBIT);
}

int main(void)
{
    RUN(test_invalid_config_rejected);
    RUN(test_boot_is_session_inhibited);
    RUN(test_session_release_requires_new_command);
    RUN(test_topic_and_service_enforcement_identical);
    RUN(test_explicit_command_brake);
    RUN(test_session_brake_assertion);
    RUN(test_timeout_boundary_is_500_ms);
    RUN(test_fault_requires_new_command_after_clear);
    RUN(test_nonfinite_commands_rejected);
    RUN(test_brake_wins_even_when_fields_nonfinite);
    RUN(test_speed_limit_precedes_conversion);
    RUN(test_independent_forward_reverse_erpm_limits);
    RUN(test_asymmetric_steering_limits);
    RUN(test_direction_change_has_neutral_hold);
    RUN(test_repeated_commands_do_not_extend_direction_hold);
    RUN(test_runtime_cap_can_only_lower_and_clear);
    RUN(test_routine_brake_release_cannot_clear_stop_latch);
    printf("PASS %u actuator policy host tests (fixture values only)\n", tests_run);
    return 0;
}
