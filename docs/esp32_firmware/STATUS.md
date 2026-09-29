# ESP32-S3 actuator firmware status

Updated: 2026-09-29 (Checkpoint 1)

## Current state

- Repository: `kihtrak-tns/laksa-work-in-phases`
- Branch: `codex/esp32-actuator-policy-20260929`
- Feature base: `a624d855d6b67b888c892d30eef08a9436cac74e`
- Reviewed Checkpoint 0: `bb54f7b80ad655474e7cdbb412561f2fbcad543d`
- Policy implementation: `eada9fef56bbe61dc2c2a325d5e036942a3901e3`
- Clean-build source/dependency pin: `8b6d5997f5182b5d78ff7054c45da0ade5c3846f`
- Installed firmware: `cba74e7-dirty` (unchanged; no flash)
- Candidate identity: newly reviewed phase-source reconciliation, **not** a
  reproduction of installed `cba74e7-dirty`
- Hardware state: **BLOCKED / NO FLASH / NO DEVICE OR CAR OPERATION**
- Host policy and interface gate: **PASS**
- ESP-IDF build gate: **NON-RELEASABLE BY DESIGN**; compile stopped at the
  unresolved actuator configuration guard and produced no application image

## Checkpoint 1 — host candidate, not a release

Decision: **READY FOR SOURCE REVIEW; NOT READY TO FLASH OR PHYSICALLY TEST**.

The reviewed seven-step reconciliation is implemented. The phase
state/telemetry path is retained, the observed boot/session `/laksa/brake`
interface is restored, `/cmd_vel` and direct HTTP motion routes are removed,
and topic/service commands share one policy before steering or VESC writes.
The 500 ms watchdog is unchanged. The stop latch, session latch, and
per-command brake bit are independent.

No release constants were inferred. `actuator_release_config.h` contains only
explicit zero sentinels and a compile-time error. A flashable image cannot be
created until brake action/current, forward and reverse limits, conversion,
steering endpoints, direction sign, and direction-neutral timing receive
evidence-backed review.

## Host tests and audits

Tested source: `8b6d5997f5182b5d78ff7054c45da0ade5c3846f`.
The later `.gitmodules` metadata commit does not change compiled inputs.

```text
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Ifirmware/esp32-s3/src \
  firmware/esp32-s3/src/actuator_policy.c \
  firmware/esp32-s3/tests/host/test_actuator_policy.c \
  -lm -o /tmp/laksa_actuator_policy_tests
/tmp/laksa_actuator_policy_tests
```

Result: **17/17 PASS** using test-only fixture values. Coverage includes invalid
configuration, boot inhibit, explicit session release with fresh-command
requirement, identical topic/service enforcement, command and session brakes,
the 500 ms boundary, faults, nonfinite inputs, clamp-before-conversion,
independent forward/reverse eRPM limits, asymmetric steering, direction-change
neutral hold (including repeated commands), lower-only runtime caps, and stop
latch independence.

`python3 firmware/esp32-s3/tests/host/check_ros_interface_layout.py` result:
**PASS**, preserving `DriveCommand` (3 entries), `VescState` (28),
`SetDriveCommand` (4 including separator), and `GetVehicleState` (2).

The firmware source audit found no `/cmd_vel`, `/api/motor`, `/api/steering`,
`VESC_MAX_ABS_RPM`, or legacy direct-RPM setter. `git diff --check` passed.

## Clean build result

Ubuntu 22.04 WSL2 now has ESP-IDF v6.0.1 at exact revision
`8c19b156084a0753687347cca1f5355782893533`, xtensa toolchain 15.2.0,
Python 3.10.12, and the exact micro-ROS revisions in `DEPENDENCY_LOCK.md`.

From empty ESP-IDF and micro-ROS output trees, `idf.py build` regenerated the
pinned Humble stack and the project typesupport. The build compiled through
step 1482 and stopped at step 1483/1491 with:

```text
actuator_release_config.h:29:2: error:
#error "NON-RELEASABLE: review brake action/current, forward/reverse limits,
conversion, and steering endpoints"
```

This is the required release gate, not a successful firmware build. No
`sample_project.bin` exists and the candidate image SHA-256 is `NONE`.
Third-party ESP-IDF/micro-ROS warnings are recorded in the build log; none was
converted into a release claim.

## Evidence provenance

- Observed installed A1 tests support the boot/session interface, deployed ROS
  field order, and approximately 500 ms freshness behavior.
- Donor source supports the explicit-brake mechanism and session-latch shape,
  but does not define its brake-current value.
- Phase source supplies the retained state/telemetry path.
- Newly implemented behavior includes the unified policy, independent stop
  latch, validation, asymmetric bounds, lower-only runtime cap, and
  direction-neutral state machine. It is host-tested only.
- All numeric policy-test fixtures are synthetic and are not car settings.

## Remaining blockers

1. Review and approve the actual active-brake action/current.
2. Review forward and reverse speed/eRPM limits and the speed-to-eRPM
   conversion; P2 samples and wheels-up eRPM are not calibration evidence.
3. Review steering direction and asymmetric servo endpoints.
4. Review the direction-change neutral interval and deliberate stop-latch rearm
   path before exposing either to a physical workflow.
5. Resolve P3 with the operator/Claude. P3 is **NOT CLEAN**: 0.25 m/s aborted
   on current in 8/10 starts, and XT60 stop caused an abrupt backward kick with
   VESC undervoltage fault 2.
6. Confirm the rollback asset and read-only security/eFuse state before any
   separately approved human-run flash gate.

## Next human-run gate

The next gate is **review and configuration resolution**, not flashing:
operator/Claude must resolve the P3 current/kick findings and approve every
actuator release setting above. Only then should a new source checkpoint be
reviewed and allowed to produce a candidate image. USB/device access, XT60
operation, flashing, and car motion remain prohibited for Codex.
