# ESP32-S3 actuator policy contract

Status: Checkpoint 1 host implementation. This is a newly reviewed source
candidate, not a reconstruction of installed `cba74e7-dirty`. No firmware was
flashed and no actuator behavior in this document is a physical PASS.

## Boundary and ownership

Every enabled ROS motion request enters `actuator_policy` before either
`vesc_uart_set_drive()` or `steering_control_set_target()`. The enabled
request paths are:

- `/laksa/command` using the deployed three-field `DriveCommand`;
- `/laksa/set_drive_command` using the same command and policy;
- `/laksa/brake` as the observed boot/session latch interface.

The legacy `/cmd_vel` subscription and direct HTTP actuator routes are
removed. The dashboard is read-only. Mission choice, sensor freshness, and
runtime operating policy remain Jetson responsibilities.

## States and transitions

The policy starts with `session_brake=true` and no accepted command. A
`/laksa/brake=false` transition releases only the boot/session latch and
invalidates any command received before release; a fresh motion request is
required. Asserting the latch, changing VESC fault state, or rearming the
independent stop latch also invalidates the prior command.

`stop_latched` is independent of the session latch and the per-command
`DriveCommand.brake` bit. Routine `/laksa/brake=false` cannot clear it.
Host policy rearm requires session brake asserted, no active fault, and no
accepted command. No GPIO, wireless receiver, or public rearm endpoint is
enabled in this candidate; physical-stop wiring and deliberate rearm remain
future reviewed work.

A finite non-braking command is clamped in speed and steering space before
conversion. A requested direction reversal holds zero eRPM for the configured
neutral interval; repeated commands do not restart the interval. Loss of a
lower runtime eRPM cap restores the immutable configured cap, never a higher
cap.

## Decision priority and outputs

| Priority | Condition | VESC request | Steering request | Notes |
| --- | --- | --- | --- | --- |
| 1 | physical stop | not implemented | not implemented | hardware path remains blocked and absent |
| 2 | internal stop latch | reviewed active brake | reviewed center | cannot be released by routine session message |
| 3 | boot/session inhibit | reviewed active brake | reviewed center | asserted on boot and agent teardown |
| 4 | VESC fault | reviewed active brake | reviewed center | fault transition invalidates the command |
| 5 | no command or age greater than 500 ms | reviewed active brake | reviewed center | independent of agent ping |
| 6 | per-command `brake=true` | reviewed active brake | reviewed center | transient command bit, not the stop latch |
| 7 | malformed/nonfinite command | reviewed active brake | reviewed center | rejected; brake bit still wins safely |
| 8 | valid bounded command | bounded eRPM | bounded asymmetric target | topic and service use identical evaluation |

“Reviewed active brake” deliberately has no value in this candidate.
`actuator_release_config.h` fails an ESP-IDF build until a reviewer supplies
and approves brake action/current, forward and reverse speed/eRPM limits,
conversion parameters, steering radians/endpoints, and direction sign. The
implementation calls `setBrakeCurrent()`; it does not silently replace the
unknown action with `setRPM(0)`.

## Evidence provenance

Observed installed behavior, from battery-unplugged A1 evidence:

- boot/session motion inhibition until `/laksa/brake=false`;
- `DriveCommand{speed_mps, steering_angle_rad, brake}` field order;
- `/laksa/brake`, topic/service endpoints, and ordered `VescState` layout;
- approximately 500 ms command-freshness loss and centered brake state.

Source-derived behavior:

- phase state/telemetry publishers, PCA9685 diagnostics, pins, and 500 ms
  watchdog;
- donor explicit-brake mechanism and session-latch shape, but not a defined
  brake-current value;
- the newly implemented independent latch, lower-only runtime cap, validation,
  asymmetric bounds, and one-policy routing.

Synthetic host-test behavior:

- all numeric fixture values in `tests/host/test_actuator_policy.c`;
- boundary, fault, stale, braking, invalid-value, direction, conversion-order,
  topic/service equivalence, and latch tests.

Unverified physical behavior:

- active brake current and sign, stopping impulse, steering motion on brake,
  loaded speed conversion, safe forward/reverse ceiling, and physical stop or
  rearm behavior.

## P3 restriction

P3 remains **NOT CLEAN**. Operator evidence says 0.25 m/s aborted on current in
8 of 10 starts, and the XT60 stop produced an abrupt backward kick with VESC
undervoltage fault 2. Those findings are not inputs from which to infer release
constants. Claude/operator resolution is required before any physical
validation proposal.
