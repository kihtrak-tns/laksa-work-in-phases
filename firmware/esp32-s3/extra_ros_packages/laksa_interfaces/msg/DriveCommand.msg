# SI-unit drive command. Positive steering is left, following REP-103.
# Layout matches the handoff branch (autonomy-handoff-2026-09-02, 4787d05)
# and the installed ESP32 firmware (cba74e7-dirty). The production mainline
# (1f0db4f) has only speed_mps and steering_angle_rad — that 2-field layout
# does NOT match the flashed build and must not be used.
float32 speed_mps
float32 steering_angle_rad
bool brake
