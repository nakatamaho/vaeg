; ---------------------------------------------------------------------------
; CONFIG3_286.INC - NEON 3 80286 analog 16-colour edition
; ---------------------------------------------------------------------------
; Logical scene coordinates remain 640x400.  The default 640x200 video mode
; halves Y inside the GRCG primitives; /400 selects the full 640x400 benchmark
; path without changing scene geometry or timing.
; ---------------------------------------------------------------------------

%define SCREEN_W          640
%define SCREEN_H          400       ; logical scene height
%define VIDEO_H           200       ; default physical height
%define BYTES_PER_LINE     80
%define VRAM_B_SEG      0A800h
%define VRAM_R_SEG      0B000h
%define VRAM_G_SEG      0B800h
%define VRAM_E_SEG      0E000h
%define TEXT_CHAR_SEG   0A000h
%define TEXT_ATTR_SEG   0A200h

%define SCENE_COUNT           9
%define MUSIC_STEPS       1024
%define MUSIC_CHANNELS       9
%define MUSIC_BAR_COUNT      64
%define OPL_SCORE_CHANNELS    9
%define OPL_PERCUSSION_CHANNELS 6
%define OPL_PATCH_CHANNELS   15
%define MUSIC_DIVIDER        6
%define MUSIC_STEPS_PER_BAR  16
%define MUSIC_BAR_FRAMES     (MUSIC_STEPS_PER_BAR * MUSIC_DIVIDER)
%define TOTAL_FRAMES         (MUSIC_STEPS * MUSIC_DIVIDER)

%if TOTAL_FRAMES != 6144
    %error NEON3 score timing must remain 6144 logical frames
%endif

%define OPL_CANDIDATE_COUNT  15
%define OPNA_CANDIDATE_COUNT   4
%define OPN26_CANDIDATE_COUNT   2
%define OPN_DEVICE_NONE          0
%define OPN_DEVICE_OPNA          1
%define OPN_DEVICE_26K           2
%define SOUND_MODE_AUTO         0
%define SOUND_MODE_OPNA         1
%define SOUND_MODE_OPL          2
%define SOUND_MODE_BOTH         3
%define SOUND_MODE_26K          4
%define SOUND_MODE_NONE         5
%define CMD_TOKEN_MAX          31
%define FRAME_SKIP_MAX          10
%define ADAPTIVE_MAX_ELAPSED_FRAMES 12
%define REST              0FFh
%define HOLD              0FEh
%define NOTE(block, semi) (((block) << 4) | (semi))

; 80286 renderer limits.  The projection core is signed 16-bit, with 16x16
; multiplies producing DX:AX.  Scene-local rebasing keeps all active geometry
; safely inside this range.
%define CITY_NEAR_Z          48
%define CITY_FAR_Z        14000
%define CITY_FOCAL          300
%define CITY_SCREEN_CX      320
%define CITY_SCREEN_CY      200
%define CITY_PROJECT_CLAMP  6000
%define CITY_PALETTE_BLEND_FRAMES 128

; Keep NEON3 chapter-specific timing constants so the 286 choreography tracks
; the normal 16-colour edition at the same logical frame positions.
%define CITY_FINAL_ROTOZOOM_START       640
%define CITY_FINAL_ROTOZOOM_FOCUS_START 800
%define CITY_FINAL_ROTOZOOM_HALF        72
%define CITY_FINAL_ROTOZOOM_STEP        24
%define CITY_FINAL_ROTOZOOM_INSET       2
%define CITY_FINAL_ROTOZOOM_FOCUS_X     12
%define CITY_FINAL_ROTOZOOM_FOCUS_Y     12
%define CITY_FINAL_ROTOZOOM_SINGLE_TILE_START 840
%define CITY_FINAL_PALETTE_FADE_START   870
%define CITY_FINAL_COVER_REUSE_START    849
%define CITY_TUNNEL_WHITEOUT_START      608
%define CITY_RAIL_WHITEOUT_RECOVER_FRAMES 10
%define CITY_RAIL_PASS_SWERVE_START     308
%define CITY_RAIL_PASS_SWERVE_PEAK      324
%define CITY_RAIL_PASS_SWERVE_END       504
%define CITY_RAIL_CAMERA_ACCEL_START    576
%define CITY_RAIL_CAMERA_ACCEL_END      768
%define CITY_TERMINAL_TURN_START         32
%define CITY_TERMINAL_DECEL_FRAMES       64

; Faithful NEON3 route/model constants.  These mirror the 256/16-colour
; edition; only palette indices are mapped to the 16-colour hardware palette.
%define CITY_NEAR_CLIP_Z       (CITY_NEAR_Z + 4)
%define CITY_FAR_CLIP_Z        (CITY_FAR_Z - 4)
%define CITY_GROUND_SEGMENT_Z  1024
%define CITY_ROUTE_CRUISE_SPEED 20
; Top-level scene-route tuning constants.
; These are NASM preprocessor constants: they do not add runtime loads or
; branches.  Values below are the exact literals previously embedded in the
; scene routines unless explicitly noted as a new tuning value.
%define CITY_ROUTE_CENTER_X                 900
%define CITY_APPROACH_CAMERA_Z              -50
%define CITY_APPROACH_CAMERA_Y             1800
%define CITY_APPROACH_CAMERA_YAW            -24
%define CITY_APPROACH_CAMERA_PITCH          249
%define CITY_CANYON_CAMERA_Z                -50
%define CITY_CANYON_CAMERA_Y                145
%define CITY_CANYON_CAMERA_PITCH            254
%define CITY_BRIDGE_CAMERA_Y                155
%define CITY_BRIDGE_CAMERA_PITCH            253
%define CITY_PARK_ORBIT_CENTER_X           1250
%define CITY_PARK_ORBIT_CENTER_Z           7600
%define CITY_PARK_ORBIT_Y                   640
%define CITY_NEON_CAMERA_Z0                8200
%define CITY_NEON_CRUISE_Y_HIGH             350
%define CITY_NEON_CRUISE_Y_NORMAL           115
%define CITY_NEON_TUNNEL_DESCENT_START      448
%define CITY_NEON_TUNNEL_DESCENT_FRAMES     191
; Late commercial-district rear scenery.  Once each building/sign has
; completely passed the camera, move that individual object left while it is
; behind the viewer.  This preserves the accidental distant-city silhouette
; without letting it visibly "jump" sideways after it re-enters projection.
; 128 world-Z units is about 6.4 frames at the 20-unit cruise speed, so the
; move completes quickly while the object is normally hidden behind camera.
%define CITY_NEON_REAR_CITY_SHIFT_DISTANCE   128
%define CITY_NEON_REAR_CITY_SHIFT_X        -3600
; Independent cutoff for the intentional left-side rear-city silhouette.
; Tune this value so the scenery disappears exactly when its projection
; begins to overlap the tunnel.  It is deliberately independent of the
; tunnel portal cutoff below.
%define CITY_NEON_REAR_CITY_HIDE_FRAME        580
%define CITY_NEON_TUNNEL_ENTRY_FRAME         635
%define CITY_TUNNEL_CAMERA_Z0             15032
%define CITY_RAIL_CAMERA_Z0               17472
%define CITY_RAIL_CAMERA_Y                  135
%define CITY_RAIL_CAMERA_PITCH              254
%define CITY_TERMINAL_CAMERA_Z0           27600
%define CITY_TERMINAL_CAMERA_Y0             190
%define CITY_TERMINAL_CAMERA_PITCH          252
%define CITY_FINAL_CAMERA_Z_BASE          29159
%define CITY_FINAL_CAMERA_Y                2821
%define CITY_FINAL_CAMERA_PITCH             244

; Commercial-district billboard avoidance.  Entry timing and dodge peaks stay
; unchanged, but recovery is deliberately much slower.  A nominal recovery is
; four times the 28-frame entry.  If the next horizontal dodge starts sooner,
; that start wins and the previous recovery uses the whole available interval
; so it reaches the base route just as the next manoeuvre begins.  The third
; horizontal recovery can continue while the final vertical drop is active.
%define CITY_NEON_DODGE_ENTRY_FRAMES         28
%define CITY_NEON_DODGE_RETURN_FRAMES       (CITY_NEON_DODGE_ENTRY_FRAMES*4)
%define CITY_NEON_DODGE1_START               49
%define CITY_NEON_DODGE1_CENTER              77
%define CITY_NEON_DODGE2_START              127
%define CITY_NEON_DODGE2_CENTER             155
%define CITY_NEON_DODGE3_START              257
%define CITY_NEON_DODGE3_CENTER             285
%define CITY_NEON_DODGE4_START              335
%define CITY_NEON_DODGE4_END                363
%define CITY_NEON_DODGE1_RETURN_FRAMES      (CITY_NEON_DODGE2_START-CITY_NEON_DODGE1_CENTER)
%define CITY_NEON_DODGE2_RETURN_FRAMES      (CITY_NEON_DODGE3_START-CITY_NEON_DODGE2_CENTER)
%define CITY_NEON_DODGE3_RETURN_FRAMES       CITY_NEON_DODGE_RETURN_FRAMES
%define CITY_NEON_DODGE1_END                (CITY_NEON_DODGE1_CENTER+CITY_NEON_DODGE1_RETURN_FRAMES)
%define CITY_NEON_DODGE2_END                (CITY_NEON_DODGE2_CENTER+CITY_NEON_DODGE2_RETURN_FRAMES)
%define CITY_NEON_DODGE3_END                (CITY_NEON_DODGE3_CENTER+CITY_NEON_DODGE3_RETURN_FRAMES)
%define CITY_NEON_DODGE1_OVERSHOOT_LATERAL   240 ; stronger first RIGHT dodge peak
%define CITY_NEON_DODGE_LATERAL              180
%define CITY_NEON_DODGE2_LATERAL              90 ; second LEFT dodge: half lateral distance
%define CITY_NEON_DODGE_TILT_PHASE           512
%define CITY_NEON_DODGE_FINAL_DROP (CITY_NEON_CRUISE_Y_HIGH-CITY_NEON_CRUISE_Y_NORMAL)

; Terminal complex.  The station-side building is a composition of orthogonal
; boxes inspired by a stepped modern high-rise: broad podium, two side wings,
; a central tower, a setback upper stage and a small crown.  All dimensions are
; kept here so the silhouette can be tuned without hunting geometry literals.
%define CITY_TERMINAL_BUILDING_Z            29600
%define CITY_TERMINAL_PODIUM_X                100
%define CITY_TERMINAL_PODIUM_Y                  0
%define CITY_TERMINAL_PODIUM_W               1740
%define CITY_TERMINAL_PODIUM_H                260
%define CITY_TERMINAL_PODIUM_D                720
%define CITY_TERMINAL_LEFT_X                  140
%define CITY_TERMINAL_LEFT_Y                  260
%define CITY_TERMINAL_LEFT_Z                29640
%define CITY_TERMINAL_LEFT_W                  430
%define CITY_TERMINAL_LEFT_H                  390
%define CITY_TERMINAL_LEFT_D                  640
%define CITY_TERMINAL_CENTER_X                600
%define CITY_TERMINAL_CENTER_Y                260
%define CITY_TERMINAL_CENTER_Z              29620
%define CITY_TERMINAL_CENTER_W                650
%define CITY_TERMINAL_CENTER_H                650
%define CITY_TERMINAL_CENTER_D                680
%define CITY_TERMINAL_UPPER_X                 710
%define CITY_TERMINAL_UPPER_Y                 910
%define CITY_TERMINAL_UPPER_Z               29680
%define CITY_TERMINAL_UPPER_W                 430
%define CITY_TERMINAL_UPPER_H                 500
%define CITY_TERMINAL_UPPER_D                 560
%define CITY_TERMINAL_CROWN_X                 790
%define CITY_TERMINAL_CROWN_Y                1410
%define CITY_TERMINAL_CROWN_Z               29740
%define CITY_TERMINAL_CROWN_W                 270
%define CITY_TERMINAL_CROWN_H                 180
%define CITY_TERMINAL_CROWN_D                 440
%define CITY_TERMINAL_RIGHT_X                1280
%define CITY_TERMINAL_RIGHT_Y                 260
%define CITY_TERMINAL_RIGHT_Z               29640
%define CITY_TERMINAL_RIGHT_W                 500
%define CITY_TERMINAL_RIGHT_H                 430
%define CITY_TERMINAL_RIGHT_D                 640
%define CITY_TERMINAL_ROAD_Z0               28600
%define CITY_TERMINAL_ROAD_Z1               31600
%define CITY_TERMINAL_ROAD_HALF              1200
%define CITY_TERMINAL_ROAD_SPACING            256
; Right-rear landmark beside the terminal.
; Top-view layout (camera approaches from lower Z toward higher Z):
;
;   [ LOW front ]                 ; narrow stem behind SMALL
;   [ LOW rear spanning SMALL + MID + TALL ]
;   [ SMALL ] [ MID ] [ TALL ]
;
; SMALL/MID/TALL are arranged along the thin X/W axis and touch on side
; faces.  LOW starts behind SMALL and broadens only at the rear.  All masses
; stand on the same Y=0 base.
%define CITY_TERMINAL_LANDMARK_TALL_X         2220
%define CITY_TERMINAL_LANDMARK_TALL_Y            0
%define CITY_TERMINAL_LANDMARK_TALL_W          280 ; ~2/3 of v5 420, thin X axis
%define CITY_TERMINAL_LANDMARK_TALL_H         2170 ; ~1.2x v5 1810
%define CITY_TERMINAL_LANDMARK_TALL_D          680 ; retain original long Z depth

; Middle tower: directly LEFT of TALL, same thin-axis width.
%define CITY_TERMINAL_LANDMARK_MID_W  CITY_TERMINAL_LANDMARK_TALL_W
%define CITY_TERMINAL_LANDMARK_MID_X  (CITY_TERMINAL_LANDMARK_TALL_X - CITY_TERMINAL_LANDMARK_MID_W)
%define CITY_TERMINAL_LANDMARK_MID_Y             0
%define CITY_TERMINAL_LANDMARK_MID_D           700
%define CITY_TERMINAL_LANDMARK_MID_Z          30620
%define CITY_TERMINAL_LANDMARK_MID_H          1180

; TALL and MID share the same long-axis centre, so their X side faces read
; as one stepped slab without a front/back offset.
%define CITY_TERMINAL_LANDMARK_TALL_Z (CITY_TERMINAL_LANDMARK_MID_Z + ((CITY_TERMINAL_LANDMARK_MID_D - CITY_TERMINAL_LANDMARK_TALL_D) / 2))

; Small building: attached to the LEFT side of MID, opposite the TALL face.
; Its front Z line is aligned with MID's front Z line.
%define CITY_TERMINAL_LANDMARK_SMALL_W          140
%define CITY_TERMINAL_LANDMARK_SMALL_X (CITY_TERMINAL_LANDMARK_MID_X - CITY_TERMINAL_LANDMARK_SMALL_W)
%define CITY_TERMINAL_LANDMARK_SMALL_Y            0
%define CITY_TERMINAL_LANDMARK_SMALL_D          180
%define CITY_TERMINAL_LANDMARK_SMALL_Z CITY_TERMINAL_LANDMARK_MID_Z
%define CITY_TERMINAL_LANDMARK_SMALL_H          516

; Low-rise wing: two boxes form the requested stepped top-view footprint.
; FRONT begins directly behind SMALL.  Its left edge is exactly SMALL's left
; edge and it keeps SMALL's width.  REAR begins only after MID/TALL have ended
; in +Z, then broadens to cover the complete SMALL+MID+TALL X span.  The total
; +Z depth remains 896 (= 640 * 1.4) without intersecting MID or TALL.
%define CITY_TERMINAL_LANDMARK_LOW_REAR_H         260
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_H         345  ; fixed: keep prior height when SMALL_H changes
%define CITY_TERMINAL_LANDMARK_LOW_TOTAL_D        896 ; 640 * 1.4
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_X CITY_TERMINAL_LANDMARK_SMALL_X
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_Y          0
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_Z (CITY_TERMINAL_LANDMARK_SMALL_Z + CITY_TERMINAL_LANDMARK_SMALL_D)
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_W CITY_TERMINAL_LANDMARK_SMALL_W
%define CITY_TERMINAL_LANDMARK_LOW_REAR_Z (CITY_TERMINAL_LANDMARK_MID_Z + CITY_TERMINAL_LANDMARK_MID_D)
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_D (CITY_TERMINAL_LANDMARK_LOW_REAR_Z - CITY_TERMINAL_LANDMARK_LOW_FRONT_Z)
%define CITY_TERMINAL_LANDMARK_LOW_REAR_X CITY_TERMINAL_LANDMARK_SMALL_X
%define CITY_TERMINAL_LANDMARK_LOW_REAR_Y           0
%define CITY_TERMINAL_LANDMARK_LOW_REAR_W ((CITY_TERMINAL_LANDMARK_TALL_X + CITY_TERMINAL_LANDMARK_TALL_W) - CITY_TERMINAL_LANDMARK_SMALL_X)
%define CITY_TERMINAL_LANDMARK_LOW_REAR_D (CITY_TERMINAL_LANDMARK_LOW_TOTAL_D - CITY_TERMINAL_LANDMARK_LOW_FRONT_D)
; Low-saturation cool-blue logical palette slots used by the terminal scene.
; Keep every component of this landmark in the same subdued blue family.
%define CITY_TERMINAL_LANDMARK_COLOR_LOW       2
%define CITY_TERMINAL_LANDMARK_COLOR_SMALL     2
%define CITY_TERMINAL_LANDMARK_COLOR_MID       3
%define CITY_TERMINAL_LANDMARK_COLOR_TALL      3
%define CITY_WATER_PALETTE_INDEX 12

; MEGABRIDGE water-only raster wave.  The bridge/deck geometry is not
; scrolled: projected water-line X is displaced by a sine indexed by the
; logical raster Y plus global frame time.  No extra framebuffer is needed.
; Water-wave / grid tuning knobs.  Keep these compile-time constants so the
; bridge-water cost/coverage can be tested without touching the renderer.
%define CITY_WATER_RASTER_AMPLITUDE             20
%define CITY_WATER_RASTER_Y_DIV_SHIFT             1
%define CITY_WATER_RASTER_Y_PHASE_MUL             3
%define CITY_WATER_RASTER_FRAME_STEP              2
%define CITY_WATER_RASTER_DIAGNOSTIC_ZERO_SHIFT   0 ; 1 = coordinate diagnostic, no X wave

; Perspective-aware vertical wave for the transverse (horizontal) water-grid rows.
; Each row keeps its projected X/gap geometry but receives a small Y offset
; derived from its logical raster Y and frame time.  Set ENABLE=0 to disable
; the vertical wave while retaining the existing X raster wave.
%define CITY_WATER_CROSS_Y_WAVE_ENABLE             1
%define CITY_WATER_CROSS_Y_AMPLITUDE               8
%define CITY_WATER_CROSS_Y_DEPTH_REFERENCE        2048 ; full amplitude up to this camera-space Z, then ~1/Z
%define CITY_WATER_CROSS_Y_PHASE_MUL               2
%define CITY_WATER_CROSS_Y_FRAME_STEP              2
%define CITY_WATER_CROSS_Y_PHASE_BIAS             64

%define CITY_WATER_GRID_X_STEP                  800
; Finite transverse fallback/test footprint.  With CROSS_FULL_SCREEN=1,
; these +/-6400/count17 values are retained only for easy A/B testing.
%define CITY_WATER_GRID_FULL_X_MIN            -6400
%define CITY_WATER_GRID_FULL_X_COUNT             17
%define CITY_WATER_GRID_FULL_X_MAX (CITY_WATER_GRID_FULL_X_MIN + ((CITY_WATER_GRID_FULL_X_COUNT - 1) * CITY_WATER_GRID_X_STEP))
%define CITY_WATER_GRID_CROSS_FULL_SCREEN         1 ; 1 = transverse water lines use screen-edge spans
; Expensive per-raster longitudinal curves are intentionally truncated.
%define CITY_WATER_GRID_LONG_X_MIN            -4000
%define CITY_WATER_GRID_LONG_X_COUNT             11
%define CITY_WATER_GRID_LONG_X_MAX (CITY_WATER_GRID_LONG_X_MIN + ((CITY_WATER_GRID_LONG_X_COUNT - 1) * CITY_WATER_GRID_X_STEP))
%define CITY_WATER_GRID_LONG_SKIP_CENTER          1
%define CITY_WATER_GRID_LONG_SKIP_HALF CITY_WATER_GRID_X_STEP ; omit X=-800,0,+800 below the road

%define CITY_WATER_GRID_Z_STEP                  640
%define CITY_WATER_GRID_Z_START_OFFSET (CITY_WATER_GRID_Z_STEP * 1)
%define CITY_WATER_Y                            -400

; Manual world-X safety margin for the transverse water-line gap under the
; bridge road.  Increase this value if shifted water still intrudes into the
; road silhouette.  ROAD_BASE_HALF is visually tuned for the bridge/water overlap.
%define CITY_WATER_ROAD_BASE_HALF               70
%define CITY_WATER_ROAD_OCCLUDE_MARGIN           440
; Match the widened three-line longitudinal exclusion band by shortening each
; transverse span one additional water-grid cell toward the outside.
%define CITY_WATER_ROAD_OCCLUDE_GRID_EXTRA CITY_WATER_GRID_X_STEP
%define CITY_WATER_ROAD_OCCLUDE_HALF (CITY_WATER_ROAD_BASE_HALF + CITY_WATER_ROAD_OCCLUDE_MARGIN + CITY_WATER_ROAD_OCCLUDE_GRID_EXTRA)

; CENTRAL PARK entry: keep bridge structure, but hide only the water as soon
; as the entry yaw starts.  Set ENABLE=0 to keep water through the turn, or
; raise YAW_PHASE_MIN to delay the hand-off for testing.
%define CITY_WATER_PARK_HIDE_ON_TURN               1
%define CITY_WATER_PARK_HIDE_YAW_PHASE_MIN          1
%define CITY_WATER_PARK_TAIL_Z0                  5000
%define CITY_WATER_PARK_TAIL_Z1                  7108
%define CITY_WATER_PARK_TAIL_CROSS_Z0            5053
%define CITY_TUNNEL_EXIT_FLASH_GRAY 10
%define CITY_TUNNEL_EXIT_FLASH_WHITE 15
; Street-lamp pulse matches the 16-colour edition: fixed slot 14 is pale
; warm yellow and slot 15 is white, with about five pulses per 64 frames.
; Canyon street-lamp vertical geometry.  These values are shared by the
; pole/arm wireframe and the filled lamp face.  The current revision is
; reduced to 90% of the previous 370-unit profile.
%define CITY_CANYON_LAMP_POLE_Y        258
%define CITY_CANYON_LAMP_BEND_Y        302
%define CITY_CANYON_LAMP_TOP_Y         333
%define CITY_CANYON_LAMP_BODY_BOTTOM_Y (CITY_CANYON_LAMP_TOP_Y-8)
%define CITY_LAMP_STATIC_COLOR 14
%define CITY_LAMP_PULSE_LEVELS 14,14,14,14,14,14,15,15,15,15,14,14,14,14,14,14
%define CITY_NEON_SIGN_COLORS 8,9,11,15    ; fixed analog slots: vivid cyan, magenta, yellow, white
%define CITY_TUNNEL_EXIT_FORCE_WHITE 632
%define CITY_TUNNEL_CURVE_FRAME_START 320
%define CITY_TUNNEL_CURVE_Z     21432
%define CITY_TUNNEL_EXIT_Z      28000
%define CITY_TUNNEL_CURVE_DENOM 40000
%define CITY_TUNNEL_CURVE_EXIT_YAW 13
%define CITY_TUNNEL_CURVE_EXIT_ROLL 8
%define CITY_PARK_SCENE_Z_BIAS 4092    ; keep bridge->park camera Z continuous on 286
%define CITY_RAIL_SCENE_Z_BIAS -612
%define CITY_RAIL_TRAIN_DELAY 32
%define CITY_RAIL_TRAIN_SPEED 25
%define CITY_RAIL_ONCOMING_START 48
%define CITY_RAIL_ONCOMING_END 416
%define CITY_RAIL_ONCOMING_START_Z 15972
%define CITY_RAIL_ONCOMING_SPEED 28
%define CITY_RAIL_ONCOMING_CARS 8
%define CITY_RAIL_PASS_SWERVE_X 37
%define CITY_RAIL_PASS_ROLL 3
%define CITY_RAIL_CAMERA_FAST_SPEED 42
%define CITY_RAIL_MODEL_END 27400 ; stop near terminal far edge; avoids a long far-rail tail
%define CITY_TERMINAL_RAIL_Z_BIAS 4188 ; preserve far-rail position across terminal coordinate rebase
%define CITY_TERMINAL_TURN_LAST 351
%define CITY_FINAL_SCENE_REBASE -6000
%define CITY_NEON_SCENE_REBASE  -6000
