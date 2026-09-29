# Coordination-Repo Interface and File Notes

This snapshot was taken from `Project_LAKSA` at `1f0db4f`
(`feature/learned-driver`) with Samyak's uncommitted launcher and console
files layered on top (`ec7b788`).  The deployed Orin checkout differs from
what is pushed here.  This document records the gaps so that a reviewer
knows exactly what is missing, why, and how to obtain each piece.

## Deployed Orin Checkout

| item | value |
|---|---|
| Orin checkout SHA | `1f0db4f` (`feature/learned-driver`) |
| Installed firmware SHA | `cba74e7-dirty` (ESP32-S3, micro-ROS bridge) |
| Handoff branch | `autonomy-handoff-2026-09-02` commit `4787d05` |
| Built workspace | `~/laksa_ws` (colcon, sourced via `install/setup.bash`) |
| Third-party workspace | `~/third_party/third_party_ws` |
| ZED workspace | `~/zed_ws` (ZED SDK wrapper, optional) |

The Orin has local uncommitted changes on top of `1f0db4f`; the exact diff
was not captured.  The **installed `laksa_interfaces`** (built into
`~/laksa_ws`) come from the handoff branch (`4787d05`), not the production
mainline — they include the `brake` field in `DriveCommand.msg` and the
`brake_active`, `telemetry_sequence`, `telemetry_age_ms` fields in
`VescState.msg`.

### Interface reconciliation (done)

The pushed `.msg` files now match the deployed layout:

- **`DriveCommand.msg`**: 3 fields (`speed_mps`, `steering_angle_rad`,
  `brake`) — matches `4787d05` and the flashed firmware.
- **`VescState.msg`**: 28 fields including `brake_active`,
  `telemetry_sequence`, `telemetry_age_ms` — matches `4787d05` and the
  flashed firmware.

Both files carry header comments identifying which layout they match and
warning against the 2-field / 25-field production-mainline layouts.

## Missing Files

### `jetson/laksa_learned_driver/config/nav2_field_overrides.yaml`

**Referenced by**: `dryrun_bringup.sh` as `NAV_OVERRIDES`, loaded as the
second `--params-file` for `controller_server` after `nav2_ackermann.yaml`.

**What it does**: This file is Samyak's RPP (Regulated Pure Pursuit) field
override.  The base `nav2_ackermann.yaml` selects MPPI
(`nav2_mppi_controller::MPPIController`).  `nav2_field_overrides.yaml`
switches the controller plugin to `nav2_regulated_pure_pursuit_controller`
and sets its base tuning parameters.  `nav2_tonight_overrides.yaml` (the
P1 file in this repo) layers additional RPP parameters on top —
`desired_linear_vel`, `regulated_linear_scaling_min_speed`,
`min_approach_linear_velocity`, `use_collision_detection`,
`allow_reversing` — which are RPP-specific and would be ignored by MPPI.

**How to obtain**: Copy from the Orin at
`~/laksa_ws/src/laksa_learned_driver/config/nav2_field_overrides.yaml`
(or wherever Samyak's local checkout places it).  This file lives only on
the Orin and has never been committed to the official LAKSA repo.

**Controller answer**: Without `nav2_field_overrides.yaml`, the launcher
loads MPPI.  With it, the launcher loads RPP.  Tonight's overrides assume
RPP.  The branch is **not deployable** until this file is either
incorporated or the controller stack is explicitly chosen and documented.

### `jetson/setup/traction_bench.py`

**Referenced by**: `start_reliability.sh` at `${HERE}/../traction_bench.py`
(resolves to `jetson/setup/traction_bench.py`).

**What it does**: A single-speed bench script that drives one
brake→speed→brake step and writes a CSV of VESC telemetry.  Used by
`start_reliability.sh` to tabulate N sensorless start attempts.

**How to obtain**: Copy from the Orin at the path referenced above.  This
file was authored by Samyak and lives only on the Orin.

### Most of `laksa_learned_driver`

The pushed `laksa_learned_driver` package contains the console, hold
latch, perception, and config files that were added or modified for the
overnight session.  The bulk of the package (the learned-driver node
itself, perception pipelines, dashboard, etc.) was already installed on the
Orin from Samyak's `1f0db4f` checkout and is not duplicated here.  The
full package can be obtained from the official LAKSA repo at
`feature/learned-driver`.

## Sourced Workspace Paths (Orin)

The launcher (`dryrun_bringup.sh`) and shell scripts source in this order:

1. `/opt/ros/humble/setup.bash`
2. `~/zed_ws/install/setup.bash` (if present)
3. `~/third_party/third_party_ws/install/setup.bash`
4. `~/laksa_ws/install/setup.bash`

All `laksa_*` packages are built and installed into `~/laksa_ws`.

## Controller Stack Summary

| layer | file | controller |
|---|---|---|
| base | `nav2_ackermann.yaml` | MPPI (`nav2_mppi_controller::MPPIController`) |
| field override | `nav2_field_overrides.yaml` (NOT IN REPO) | switches to RPP |
| tonight override | `nav2_tonight_overrides.yaml` (in repo) | sets RPP-specific params |

The launch chain: `--params-file nav2_ackermann.yaml --params-file
nav2_field_overrides.yaml` (plus optionally `LAKSA_NAV_EXTRA` for the
tonight overrides).  Later files override earlier ones, so whichever
controller plugin `nav2_field_overrides.yaml` selects is what actually
runs.
