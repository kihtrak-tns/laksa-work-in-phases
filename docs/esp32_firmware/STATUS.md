# ESP32-S3 actuator firmware status

Updated: 2026-09-29 (Checkpoint 0)

## Current state

- Repository: `kihtrak-tns/laksa-work-in-phases`
- Branch: `codex/esp32-actuator-policy-20260929`
- Base: `a624d855d6b67b888c892d30eef08a9436cac74e`
- Installed firmware: `cba74e7-dirty` (unchanged; no flash)
- Hardware state for this work: `NO_FLASH`; no device or car operation
- Checkpoint 0: **REVIEW REQUIRED / IMPLEMENTATION STOPPED**
- Checkpoint 1 host tests/build: **NOT STARTED**

## Completed

- Fetched and confirmed the exact requested base before branching.
- Kept the working repository in the Ubuntu 22.04 native Linux filesystem at
  `/home/karsha-lab/laksa-work-in-phases`.
- Reviewed the phase firmware, interface definitions, P2 probe/analyzer, donor
  and production source revisions, installed flash inventory, and committed
  A1a/A1b/P2/P3/USB evidence.
- Recorded the deployed ROS field order and all identified motion entry points
  in `SOURCE_CONTRACT.md`.
- Recomputed the verified full-flash backup size/hash and documented the actual
  partition layout without adding the image or device identifiers to Git.
- Drafted app-only and full-image restore procedures; neither was executed.

## Findings that stop implementation

1. No committed source reproduces installed `cba74e7-dirty`. The deployed image
   combines donor-only brake/session behavior with production-only state
   behavior.
2. The installed firmware accepts `+/-2071` eRPM and rejects `4140` in P2, while
   both reviewed source lineages configure 900 eRPM. The exact installed and
   desired immutable ceiling is unknown.
3. Donor source invokes `VESC_BRAKE_CURRENT_A`, but that symbol has no definition
   in the donor revision. Deployed brake-current magnitude/action is unknown.
4. Reported asymmetric road-wheel endpoints need sign and servo-endpoint bench
   confirmation before becoming immutable firmware limits.
5. The operator has not yet confirmed the backup path as the intended restore
   asset, and security/eFuse state is absent from the backup metadata.

## Build gate

ESP-IDF 6.0.1 is not installed/exported in the Ubuntu 22.04 WSL2 environment:
`idf.py` is absent and no standard export script was found. No build was
attempted. This is a missing dependency to resolve only after Checkpoint 0
review authorizes implementation.

## Proposed next action

Review and either approve or revise the seven-step smallest reconciliation in
`SOURCE_CONTRACT.md`, especially:

- the source base and removal of the non-deployed `/cmd_vel` path;
- the explicit brake-current definition/action;
- conservative forward/reverse immutable eRPM ceilings;
- provisional versus final asymmetric steering bounds.

After that decision, implement one isolated, host-testable policy and its
decision-table tests. Do not flash or change the Orin integration.
