# ESP32-S3 source and deployed-interface contract

Status: Checkpoint 0 source reconciliation, 2026-09-29. This document is a
review artifact, not authorization to flash or operate the car.

## Claim vocabulary

- **OBSERVED_ON_CAR**: captured from the installed controller with the traction
  battery unplugged, unless a result says otherwise.
- **SOURCE_SUPPORTED**: present in a named committed source revision; this does
  not prove the installed dirty image was built from it.
- **OPERATOR_REPORTED**: a physical result in the Orin repository, not a host
  test or a firmware-source guarantee.
- **UNKNOWN**: no cited evidence establishes the fact.

No available commit is byte-identical to the installed firmware. The installed
identity is `cba74e7-dirty`, built with ESP-IDF 6.0.1 on 2026-09-05. The exact
16 MiB flash backup is therefore the only exact rollback image.

## Revisions and evidence reviewed

| Item | Exact revision | Role |
| --- | --- | --- |
| `kihtrak-tns/laksa-work-in-phases` | `a624d855d6b67b888c892d30eef08a9436cac74e` | feature base; current phase firmware |
| `Project-LAKSA/Project_LAKSA` donor | `4787d05d3c5bdfc0be1c190a45ab32f8da27667d` | brake-latch and explicit-brake source donor |
| `Project-LAKSA/Project_LAKSA` production | `1f0db4f027e2d3aa97d83d2c0a15216627efb5b1` | production command/state source lineage |
| `kihtrak-tns/orin-bringup` remote baseline | `854e75bbb95e47dd50a2d98be527cf6850371a75` | upstream reference |
| installed findings | `960087e16fba195e9f19f8fe6410862c45939124` | `docs/esp32_installed_firmware_findings.md` |
| A1a interface result | `c6765d3b7b8bd31620ac4ea4cc8058223d8989b1` | `results/A1a_20260928T022525Z.md` |
| A1b brake/session result | `90bdc73bab5bf9f392493a1e06dc02a33f8afb02` | `results/A1b_20260928T041406Z.md` |
| USB churn result | `3409cfcdb9c6273ac9b8ac0f8e11ad222b3d8958` | `results/churn_mechanism_20260928T154208Z.md` |
| P2 speed-ceiling result | `1653b2cd509f7774863001b6b8407e6f37b86999` | `results/P2_20260929T112420Z.md` |
| P3 operator result | `086644771ae38aced1288d8a7989abaed1c070cc` | `results/P3_20260929T160419Z.md` |

The local Orin checkout was read only and was not modified. Its P3 revision is
ahead of the recorded remote baseline, so P3 claims remain operator-reported.

## Lineage comparison

The phase-base and production files are byte-identical for both primary motion
files:

| File | Phase/production SHA-256 | Donor SHA-256 |
| --- | --- | --- |
| `src/micro_ros_bridge.c` | `6ef2be55101dbb8759705b75f5459889493fbddb0b48ed3e7687780a37a7f16c` | `84ff0a46c5442dd2d71ab9f6124105622b7e4b0b4ed3ca50ac10e3d0fcc3f6dc` |
| `src/vesc_uart.cpp` | `fd4425e91ff653f88c5abb8d9d7655ca16573408a50dc7f0196b46b4b391098c` | `a3156b9b7a9aa580d683ad021ca09891d6b6cb713a3181f9654dc33414c78ca8` |

The phase/production lineage contains the PCA9685 state publication and legacy
`/cmd_vel`, but lacks the deployed `/laksa/brake` latch and explicit brake
behavior. The donor contains the latter behavior and no `/cmd_vel`, but lacks
production-only state pieces. Flash strings and live discovery contain both
`/laksa/brake` and `/laksa/pca9685/state`, and contain no `/cmd_vel`. This proves
a mixed lineage, not the missing dirty-tree edits.

A further source defect blocks copying the donor wholesale: its
`src/vesc_uart.cpp` calls `VESC_BRAKE_CURRENT_A`, but that symbol is not defined
anywhere in donor revision `4787d05...`. The actual deployed brake current is
therefore **UNKNOWN**.

## Deployed ROS contract

Direction is relative to the ESP32. QoS is the observed/default source setting.

| Endpoint | Direction and type | QoS | Evidence and status |
| --- | --- | --- | --- |
| `/laksa/command` | subscription, `laksa_interfaces/msg/DriveCommand` | reliable, keep-last 1 | A1a live discovery: **OBSERVED_ON_CAR**; both lineages: **SOURCE_SUPPORTED** |
| `/laksa/brake` | subscription, `std_msgs/msg/Bool` | reliable default | A1a/A1b and flash strings: **OBSERVED_ON_CAR**; donor only: **SOURCE_SUPPORTED** |
| `/laksa/set_drive_command` | service, `laksa_interfaces/srv/SetDriveCommand` | service default | A1a: **OBSERVED_ON_CAR**; both lineages: **SOURCE_SUPPORTED** |
| `/laksa/get_state` | service, `laksa_interfaces/srv/GetVehicleState` | service default | A1a: **OBSERVED_ON_CAR**; both lineages: **SOURCE_SUPPORTED** |
| `/laksa/vesc/state` | publisher, `laksa_interfaces/msg/VescState` | best effort | A1a typed checks: **OBSERVED_ON_CAR**; source: **SOURCE_SUPPORTED** |
| `/laksa/state` | publisher, `laksa_interfaces/msg/VehicleState` | best effort | A1a: **OBSERVED_ON_CAR**; production/phase source: **SOURCE_SUPPORTED** |
| `/laksa/pca9685/state` | publisher | best effort | A1a and flash strings: **OBSERVED_ON_CAR**; production/phase only: **SOURCE_SUPPORTED** |
| `/laksa/imu/data` | publisher, IMU | best effort | A1a: **OBSERVED_ON_CAR**; source: **SOURCE_SUPPORTED** |
| `/laksa/imu/mag` | publisher, magnetic field | best effort | A1a: **OBSERVED_ON_CAR**; source: **SOURCE_SUPPORTED** |
| `/cmd_vel` | legacy subscription | source default | absent from live discovery and flash strings: not deployed; production/phase only: **SOURCE_SUPPORTED** |

`DriveCommand` field order and types, established by the deployed typed probe:

1. `float32 speed_mps`
2. `float32 steering_angle_rad`
3. `bool brake`

`SetDriveCommand` request contains one `DriveCommand command`; its response is
`bool accepted`, then `uint8 error_code`. `GetVehicleState` has an empty request
and one `VehicleState state` response.

The deployed `VescState` order is:

1. `builtin_interfaces/Time stamp`
2. `bool command_fresh`
3. `bool direction_change_pending`
4. `bool brake_active`
5. `bool telemetry_fresh`
6. `uint32 telemetry_sequence`
7. `uint32 telemetry_age_ms`
8. `int32 requested_erpm`
9. `int32 active_erpm`
10. `float32 measured_erpm`
11. `float32 motor_current_a`
12. `float32 input_current_a`
13. `float32 duty_cycle`
14. `float32 input_voltage_v`
15. `float32 amp_hours`
16. `float32 amp_hours_charged`
17. `float32 watt_hours`
18. `float32 watt_hours_charged`
19. `float32 temp_mosfet_c`
20. `float32 temp_motor_c`
21. `float32 pid_position`
22. `int32 tachometer`
23. `int32 tachometer_abs`
24. `uint8 controller_id`
25. `uint8 fault_code`
26. `float32 motor_angular_velocity_rad_s`
27. `float32 wheel_angular_velocity_rad_s`
28. `float32 vehicle_linear_velocity_mps`

The A1a result records 97/97 total checks, including 90 raw/typed interface
checks, with the traction battery unplugged.

## Motion entry points and actuator path

| Entry point | Installed | Current phase source | Required reconciliation |
| --- | --- | --- | --- |
| `/laksa/command` | yes | yes | route through one policy before steering/VESC output |
| `/laksa/set_drive_command` | yes | yes | route through the same policy; no service bypass |
| `/laksa/brake` | yes | no | restore session brake latch semantics without clearing an independent stop latch |
| `/cmd_vel` | no evidence | yes | remove/disable; it must not bypass policy |

In both source lineages, bridge callbacks convert speed/steering and then call
the steering and VESC modules. In the donor, the effective brake is the
session latch OR `DriveCommand.brake`; stale command, requested brake, or VESC
fault selects zero target and an explicit `setBrakeCurrent(...)` call. In the
production/phase lineage, timeout sends `setRPM(0)` and `/cmd_vel` is an
additional motion path. The future policy boundary must precede every call to
`vesc_uart_set_drive`, `vesc_uart_set_target_rpm`, or the steering target API.

## Pins, limits, and behavior

| Contract item | Value | Claim |
| --- | --- | --- |
| I2C SDA/SCL | GPIO 8 / GPIO 9 | both source lineages: **SOURCE_SUPPORTED** |
| VESC UART RX/TX | GPIO 18 / GPIO 17 | both source lineages: **SOURCE_SUPPORTED** |
| steering channel | PCA9685 channel 7 | both source lineages: **SOURCE_SUPPORTED** |
| steering center | 100 degrees in PCA mapping | both source lineages: **SOURCE_SUPPORTED**; A1b observes centering on brake |
| mechanical/source range | 63 to 139 degrees; software-safe 65 to 137 | **SOURCE_SUPPORTED**, not a measured road-wheel calibration |
| reported road-wheel bounds | left `+0.523 rad`, right `-0.288 rad` | **OPERATOR_REPORTED**; sign/endpoints require bench confirmation |
| ESP command timeout | 500 ms | flash strings/A1b timing and both sources: **OBSERVED_ON_CAR**, **SOURCE_SUPPORTED** |
| VESC UART timeout | 1000 ms, 0 A timeout brake | **OPERATOR_REPORTED/source configuration**, not changed here |
| source absolute eRPM cap | 900 | both reviewed sources: **SOURCE_SUPPORTED**, but contradicted by installed acceptance |
| installed acceptance samples | `+/-2071` accepted; `4140` rejected | P2 battery-unplugged: **OBSERVED_ON_CAR**; exact threshold **UNKNOWN** |
| donor explicit brake current | symbol used, value undefined in donor tree | mechanism **SOURCE_SUPPORTED**; value and deployed action **UNKNOWN** |

A1b observes that boot/session motion remains inhibited until
`/laksa/brake=false`, `brake=true` centers steering, and command freshness drops
after approximately 500 ms. It does not prove a hardware emergency stop.

P3 reports that a 0.22 m/s request produced 911 requested eRPM, approximately
981-1039 measured eRPM, and 10.4-11.6 A peaks with a 12 A bench limit. Two
starts were about 1.04 s by the bench clock. At 0.25 m/s, 8/10 runs aborted on
current. Stop case d included abrupt/backward-kick/undervoltage behavior. These
wheels-up/operator results do not establish loaded ground speed, braking
distance, or a safe immutable ceiling.

USB-C produced zero session teardowns in 35.7 minutes and restored about 40 Hz
IMU, while the hub path churned. This is limited, confounded evidence, not a
permanent-stability claim and not a reason to extend the independent 500 ms
command watchdog.

## Smallest proposed reconciliation (not implemented)

1. Retain the phase/production state and telemetry implementation as the base.
2. Restore the donor's deployed boot/session brake-latch semantics, but define
   and review the missing brake-current value rather than copying an undefined
   symbol.
3. Remove/disable legacy `/cmd_vel`, matching the installed endpoint inventory.
4. Place one host-testable policy below every enabled topic/service callback
   and above both VESC and steering writes.
5. Preserve the deployed three-field `DriveCommand` and ordered `VescState` ABI.
6. Keep the 500 ms command timeout for the first candidate. Do not change ping
   tolerance or VESC timeout in the same candidate.
7. Do not infer a final eRPM ceiling, conversion constants, brake current, or
   steering calibration from the incomplete dirty-image evidence.

## Checkpoint 0 decision

Implementation is intentionally stopped. The external ROS ABI is sufficiently
observed, but the exact actuator source path is not reproducible: the installed
dirty edits, absolute eRPM ceiling, explicit brake-current value, and final
steering mapping are unknown. Review must select/confirm the reconciliation
above and provide the actuator constants/evidence before policy implementation.
