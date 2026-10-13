; ===========================================================================
; DATA.INC
; Mutable state, geometry tables, sound patches, and sequence data.
; ===========================================================================

draw_page              db 1
frame_counter          dw 0        ; visual timeline (AFS or fixed-FS policy)
render_frame_counter   dw 0        ; immutable timestamp for the frame being drawn
frame_skip             db 0
frame_skip_count       db 0
speed_percent          db 100      ; /SPEED:0-200, 100 = one logical tick per display period
align 2
speed_phase            dw 0        ; visual /SPEED fractional accumulator
audio_speed_phase      dw 0        ; BGM /SPEED accumulator, independent of rendering
adaptive_skip_enabled  db 1       ; /AFS: render again as soon as prior frame is presented
adaptive_elapsed       db 1       ; legacy polling fallback only
adaptive_vsync_valid   db 0
adaptive_vsync_hooked  db 0
adaptive_vsync_owned   db 0       ; IRQ2 was masked before install, so we own its EOI
adaptive_pic_imr_before db 0
vsync_scheduler_active db 0       ; verified INT 0Ah scheduler is available
vsync_scheduler_running db 0      ; IRQ may run audio/time/presentation
vsync_visual_tick_due  db 0       ; fixed-FS: this VSYNC consumes one visual slot
vsync_frame_ready      db 0       ; main has completed a hidden page
vsync_pending_page     db 1       ; page to expose at the next VSYNC
vsync_present_scene_index db 0    ; immutable palette timestamp for pending page
align 2
vsync_present_scene_frame dw 0
vsync_pending_palette_grb times 8*3 db 0 ; slots 0..7, G/R/B nibbles
align 4
adaptive_old_int0a     dd 0
adaptive_vsync_count   dw 0       ; raw IRQ2/VSYNC count, modulo 65536
adaptive_last_present_vsync dw 0  ; legacy polling fallback
adaptive_pre_present_vsync  dw 0  ; legacy polling fallback
scene_index            db 0
scene_frame            dw 0
start_scene_request    db 0FFh     ; hidden /SCENE:n developer start, FF=normal

; Camera smoothing is a logical-timeline state, not a render-iteration state.
; AFS may advance frame_counter several times while one page is being drawn.
; These fields let FRAME_RENDER replay only the skipped camera updates.
city_camera_timeline_valid db 0
city_camera_replay         db 0
city_camera_hold_smoothing db 0
align 2
city_camera_last_frame     dw 0

; CPU feature state and NEON RELAY 2 geometry scratch.
cpu_cpuid_present      db 0
cpu_mmx_present        db 0
r2_particle_phase      db 0
r2_section_pulse       db 0
align 2
r2_center_x            dw 320
r2_center_y            dw 200
r2_radius_x            dw 0
r2_radius_y            dw 0
r2_prev_x              dw 0
r2_prev_y              dw 0
r2_current_x           dw 0
r2_current_y           dw 0
r2_mesh_base           dw 0
r2_ring_points         times 16*2 dw 0
r2_colour_cycle        db 1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2
                       db 3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4

; Revision 3 scene scratch.  Kept separate from the original tunnel/city state
; so the new entry corridor and restrained re-entry cannot accidentally share
; stale geometry with the previous implementations.
r3_quad_prev_valid      db 0
align 2
r3_entry_vanish_x       dw 320
r3_entry_vanish_y       dw 200
r3_gate_rx               dw 0
r3_gate_ry               dw 0
r3_gate_shear            dw 0
r3_gate_tilt             dw 0
r3_quad_current          times 8 dw 0
r3_quad_previous         times 8 dw 0
r3_route_ptr             dw 0
r3_route_prev_x          dw 0
r3_route_prev_y          dw 0
r3_packet_x              dw 0
r3_axis_point_count      dw 0
r3_axis_current          times 16 dw 0
r3_axis_previous         times 16 dw 0
r4_tunnel_sway_x         dw 0
r4_tunnel_sway_y         dw 0
r4_tunnel_motion_phase   dw 0
r4_tunnel_motion_local   dw 0
r4_tunnel_motion_env     dw 0
r4_tunnel_motion_active  db 0
r11_tunnel_dir_x          dw 0
r11_tunnel_dir_y          dw 0
r11_tunnel_curve_env      dw 0
r11_tunnel_curve_profile  dw 0
r12_tunnel_frame          dw 0
r12_tunnel_ring_limit     db 12
align 2
r4_aurora_offset            dw 0
r4_relay_bridge_remaining   dw 0

; Translation offsets for the eight RELAY SWARM particles at virtual relay
; frame -384.  The bridge evaluates the real RELAY SWARM orbit at virtual
; frames -384..-1 and adds offset * (remaining/256)^2.  Thus each current
; endpoint starts at the former off-screen launch coordinate, the complete
; streak keeps its normal direction, and both position and velocity converge
; to the unmodified RELAY SWARM orbit before the scene boundary.
r4_relay_entry_offset_x:
        dw -640,-367,476,245,-630,203,695,-291
r4_relay_entry_offset_y:
        dw -236,-444,-24,455,46,-543,192,553

draw_color             db 7
grcg_color_cache       db 0FFh

pixel_shift_table      db 0,1,2,3,4,5,6,7
pixel_input_x          dw 0
pixel_mask             db 0

hline_x0               dw 0
hline_x1               dw 0
hline_offset           dw 0
hline_start_mask       db 0
hline_end_mask         db 0
hline_same_byte        db 0
hline_middle_count     dw 0

line_x0                dw 0
line_y0                dw 0
line_x1                dw 0
line_y1                dw 0
line_dx                dw 0
line_dy                dw 0
line_sx                dw 0
line_sy                dw 0
line_err               dw 0
line_e2                dw 0
line_mask              db 0
line_all_visible        db 0

tri_x0                 dw 0
tri_y0                 dw 0
tri_x1                 dw 0
tri_y1                 dw 0
tri_x2                 dw 0
tri_y2                 dw 0
tri_color              db 0
tri_ycur               dw 0
tri_scan_xa            dw 0
tri_scan_xb            dw 0
align 4
tri_long_x             dd 0
tri_long_step          dd 0
tri_short_x            dd 0
tri_short_step         dd 0

edge_x0                dw 0
edge_y0                dw 0

oct_sin_x              dw 0
oct_cos_x              dw 0
oct_sin_y              dw 0
oct_cos_y              dw 0
align 4
oct_tmp_x              dd 0
oct_tmp_y              dd 0
oct_tmp_z1             dd 0
oct_tmp_z2             dd 0
oct_den                dd 0
oct_cross_a            dd 0
oct_num_abs            dd 0
oct_div_sign           dd 0
oct_projected          times 6*3 dw 0

oct_vertices:
        dw 0,-100,0
        dw 0,100,0
        dw -100,0,0
        dw 0,0,-100
        dw 100,0,0
        dw 0,0,100
oct_faces:
        db 0,2,3,5
        db 0,3,4,7
        db 0,4,5,6
        db 0,5,2,4
        db 1,3,2,3
        db 1,4,3,5
        db 1,5,4,2
        db 1,2,5,1
oct_edges:
        db 0,2, 0,3, 0,4, 0,5
        db 1,2, 1,3, 1,4, 1,5
        db 2,3, 3,4, 4,5, 5,2

tunnel_camera          dw 0
tunnel_base_z          dw 0
tunnel_depth           dw 0
tunnel_world           dw 0
tunnel_scale           dw 0
tunnel_center_x        dw 0
tunnel_center_y        dw 0
tunnel_prev_valid      db 0
tunnel_color           db 0
tunnel_roll_index      dw 0
tunnel_secondary       dw 0
tunnel_current         times 16*2 dw 0
tunnel_previous        times 16*2 dw 0

wave_prev_x            dw 0
wave_prev_y            dw 0
wave_y                 dw 0
diamond_r              dw 0
signal_bar_base         dw 0
bar_y                  dw 0
bar_width              dw 0
bar_x0                 dw 0
bar_x1                 dw 0
orb_x                   dw 0
orb_y                   dw 0
city_height            dw 0
city_x0                dw 0
city_x1                dw 0

circle_x:
        dw 256,237,181,98,0,-98,-181,-237
        dw -256,-237,-181,-98,0,98,181,237
circle_y:
        dw 0,98,181,237,256,237,181,98
        dw 0,-98,-181,-237,-256,-237,-181,-98
screen_border_x:
        dw 0,160,320,480,639,639,639,639
        dw 639,480,320,160,0,0,0,0
screen_border_y:
        dw 0,0,0,0,0,100,200,300
        dw 399,399,399,399,399,300,200,100

; Twelve evenly distributed phases for SIGNAL WAKE's moving bars.
; Successive gaps are 33 or 34 lines and the wrap gap is also 33 lines,
; so no pair becomes doubled at the top of the 400-line display.
signal_bar_phase:
        dw 0,33,67,100,133,167,200,233,267,300,333,367

sin_table:
        dw 0,402,804,1205,1606,2006,2404,2801
        dw 3196,3590,3981,4370,4756,5139,5520,5897
        dw 6270,6639,7005,7366,7723,8076,8423,8765
        dw 9102,9434,9760,10080,10394,10702,11003,11297
        dw 11585,11866,12140,12406,12665,12916,13160,13395
        dw 13623,13842,14053,14256,14449,14635,14811,14978
        dw 15137,15286,15426,15557,15679,15791,15893,15986
        dw 16069,16143,16207,16261,16305,16340,16364,16379
        dw 16384,16379,16364,16340,16305,16261,16207,16143
        dw 16069,15986,15893,15791,15679,15557,15426,15286
        dw 15137,14978,14811,14635,14449,14256,14053,13842
        dw 13623,13395,13160,12916,12665,12406,12140,11866
        dw 11585,11297,11003,10702,10394,10080,9760,9434
        dw 9102,8765,8423,8076,7723,7366,7005,6639
        dw 6270,5897,5520,5139,4756,4370,3981,3590
        dw 3196,2801,2404,2006,1606,1205,804,402
        dw 0,-402,-804,-1205,-1606,-2006,-2404,-2801
        dw -3196,-3590,-3981,-4370,-4756,-5139,-5520,-5897
        dw -6270,-6639,-7005,-7366,-7723,-8076,-8423,-8765
        dw -9102,-9434,-9760,-10080,-10394,-10702,-11003,-11297
        dw -11585,-11866,-12140,-12406,-12665,-12916,-13160,-13395
        dw -13623,-13842,-14053,-14256,-14449,-14635,-14811,-14978
        dw -15137,-15286,-15426,-15557,-15679,-15791,-15893,-15986
        dw -16069,-16143,-16207,-16261,-16305,-16340,-16364,-16379
        dw -16384,-16379,-16364,-16340,-16305,-16261,-16207,-16143
        dw -16069,-15986,-15893,-15791,-15679,-15557,-15426,-15286
        dw -15137,-14978,-14811,-14635,-14449,-14256,-14053,-13842
        dw -13623,-13395,-13160,-12916,-12665,-12406,-12140,-11866
        dw -11585,-11297,-11003,-10702,-10394,-10080,-9760,-9434
        dw -9102,-8765,-8423,-8076,-7723,-7366,-7005,-6639
        dw -6270,-5897,-5520,-5139,-4756,-4370,-3981,-3590
        dw -3196,-2801,-2404,-2006,-1606,-1205,-804,-402


; Exact lookup replacements for hot-loop integer division.
; Q16 reciprocal table for exact corrected perspective division, d=320..520.
persp_recip_q16:
        dw 204,204,203,202,202,201,201,200,199,199,198,197,197,196,196,195
        dw 195,194,193,193,192,192,191,191,190,189,189,188,188,187,187,186
        dw 186,185,185,184,184,183,183,182,182,181,181,180,180,179,179,178
        dw 178,177,177,176,176,175,175,174,174,173,173,172,172,172,171,171
        dw 170,170,169,169,168,168,168,167,167,166,166,165,165,165,164,164
        dw 163,163,163,162,162,161,161,161,160,160,159,159,159,158,158,157
        dw 157,157,156,156,156,155,155,154,154,154,153,153,153,152,152,152
        dw 151,151,151,150,150,149,149,149,148,148,148,147,147,147,146,146
        dw 146,145,145,145,144,144,144,144,143,143,143,142,142,142,141,141
        dw 141,140,140,140,140,139,139,139,138,138,138,137,137,137,137,136
        dw 136,136,135,135,135,135,134,134,134,134,133,133,133,132,132,132
        dw 132,131,131,131,131,130,130,130,130,129,129,129,129,128,128,128
        dw 128,127,127,127,127,126,126,126,126

tunnel_scale_table:
        dw 296,292,287,283,279,275,271,267,263,260,256,253,250,246,243,240
        dw 237,234,231,228,226,223,220,218,215,213,211,208,206,204,202,200
        dw 197,195,193,191,190,188,186,184,182,180,179,177,175,174,172,171
        dw 169,168,166,165,163,162,161,159,158,157,155,154,153,152,150,149
        dw 148,147,146,145,143,142,141,140,139,138,137,136,135,134,133,132
        dw 131,131,130,129,128,127,126,125,125,124,123,122,121,121,120,119
        dw 118,118,117,116,115,115,114,113,113,112,111,111,110,109,109,108
        dw 107,107,106,106,105,104,104,103,103,102,102,101,101,100,100,99
        dw 98,98,97,97,96,96,95,95,95,94,94,93,93,92,92,91
        dw 91,90,90,90,89,89,88,88,87,87,87,86,86,85,85,85
        dw 84,84,84,83,83,82,82,82,81,81,81,80,80,80,79,79
        dw 79,78,78,78,77,77,77,76,76,76,76,75,75,75,74,74
        dw 74,73,73,73,73,72,72,72,71,71,71,71,70,70,70,70
        dw 69,69,69,69,68,68,68,68,67,67,67,67,66,66,66,66
        dw 65,65,65,65,65,64,64,64,64,63,63,63,63,63,62,62
        dw 62,62,62,61,61,61,61,61,60,60,60,60,60,59,59,59
        dw 59,59,59,58,58,58,58,58,57,57,57,57,57,57,56,56
        dw 56,56,56,56,55,55,55,55,55,55,54,54,54,54,54,54
        dw 53,53,53,53,53,53,53,52,52,52,52,52,52,52,51,51
        dw 51,51,51,51,51,50,50,50,50,50,50,50,50,49,49,49
        dw 49,49,49,49,48,48,48,48,48,48,48,48,47,47,47,47
        dw 47,47,47,47,47,46,46,46,46,46,46,46,46,46,45,45
        dw 45,45,45,45,45,45,45,44,44,44,44,44,44,44,44,44
        dw 43,43,43,43,43,43,43,43,43,43,42,42,42,42,42,42
        dw 42,42,42,42,42,41,41,41,41,41,41,41,41,41,41,41
        dw 40,40,40,40,40,40,40,40,40,40,40,40,39,39,39,39
        dw 39,39,39,39,39,39,39,39,38,38,38,38,38,38,38,38
        dw 38,38,38,38,38,37,37,37,37,37,37,37,37,37,37,37
        dw 37,37,36,36,36,36,36,36,36,36,36,36,36,36,36,36
        dw 35,35,35,35,35,35,35,35,35,35,35,35,35,35,35,34
        dw 34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,33
        dw 33,33,33,33,33,33,33,33,33,33,33,33,33,33,33,33

mod6_table:
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5

; Text overlay data
text_last_scene         db 0FFh
ticker_phase            dw 0
hex_digits              db '0123456789ABCDEF'
hex_buffer              db '0000',0
text_top                db ' NEON RELAY 3 :: MEGACITY FLIGHT                               FRAME=',0
text_status_left        db 'PEGC 256-COLOR / FIXED-POINT 3D / PAGE FLIP',0
text_status_plane       db 'PEGC PLANE / 386 FIXED-POINT 3D / PAGE FLIP',0
text_status_packed      db 'PEGC PACKED LITE / 32K BANKS / PAGE FLIP',0
text_status_packed_full db 'PEGC PACKED FULL / 386 FIXED-POINT 3D',0
text_status_cpu_mmx     db 'CPU: 386 FIXED-POINT 3D',0
text_status_cpu_scalar  db 'CPU: 386+ FIXED-POINT CAMERA / INTEGER PERSPECTIVE',0
text_status_both        db 'OPNA+OPL3',0
text_status_26k_both    db '26K+OPL3 ',0
text_status_opna6       db 'OPNA6F+3S',0
text_status_opna3       db 'OPNA3F+3S',0
text_status_26k         db '26K 3F+3S',0
text_status_opl3        db 'OPL3 9CH ',0
text_status_silent      db 'NO SOUND ',0
scene_title_ptrs        dw scene_title_0,scene_title_1,scene_title_2
                        dw scene_title_3,scene_title_4,scene_title_5
                        dw scene_title_6,scene_title_7,scene_title_8
scene_title_0           db '[00] CITY DESCENT       / BAR 01  SKYLINE TO STREET LEVEL',0
scene_title_1           db '[01] TOWER CANYON       / BAR 09  BRIDGE AHEAD',0
scene_title_2           db '[02] MEGABRIDGE         / BAR 17  PARK BESIDE EXIT ROAD',0
scene_title_3           db '[03] CENTRAL PARK       / BAR 23  FULL ORBIT TO DOWNTOWN',0
scene_title_4           db '[04] NEON DISTRICT      / BAR 31  DESCENT INTO UNDERCITY',0
scene_title_5           db '[05] SUBTERRANEAN ROAD  / BAR 39  TUNNEL EXIT TO RAIL',0
scene_title_6           db '[06] RAIL TO TERMINAL   / BAR 47  TRAIN OVERTAKE RUN',0
scene_title_7           db '[07] AERIAL TERMINAL    / BAR 55  FORWARD CLIMB AND TURN',0
scene_title_8           db '[08] CITY ORBIT         / BAR 55  PANORAMA / ROTO COVER',0
text_console_clear_sequence db 1Bh,'*','$'
ticker_text             db ' NEON RELAY 3 // CITY DESCENT // TOWER CANYON // MEGABRIDGE // CENTRAL PARK // NEON DISTRICT // SUBTERRANEAN ROAD // RAIL TO TERMINAL // AERIAL TERMINAL // CITY ORBIT // ',0
ticker_text_end:

; Common sound state
sound_present           db 0
sound_mode              db SOUND_MODE_AUTO
show_help               db 0
bad_option              db 0
opna_detected           db 0
opna_device_type        db OPN_DEVICE_NONE
opl_detected            db 0
opna_volume             db 115     ; OPNA-only: modest FM+SSG boost over patch level
opna_fmwait_count        db 0       ; /FMWAIT: 0=off; bare /FMWAIT selects 2
opna_ssg_trim           db 100     ; /SV relative SSG level; 100 preserves the authored balance
opna_rhythm_volume      db 100
opl_volume              db 80      ; keep OPL3 behind OPNA in the default mix
opna_volume_explicit    db 0
opl_volume_explicit     db 0
opna_tl_adjust          db 0       ; /NV:100 default = patch-table level
opl_tl_adjust           db 6       ; /LV:80 default = about +6 TL attenuation
cmd_token               times CMD_TOKEN_MAX+1 db 0
music_index             dw 0
music_score_index       dw 0
music_divider           db 1
music_section           db 0FFh

; OPNA/YM2608 state
opna_present            db 0
opna_channel_count      db 0
opna_base               dw 0188h
opna_control_port       dw 0
opna_control_saved      db 0
opna_control_changed    db 0
opna_psg_saved          times 14 db 0
opna_psg_mixer          db 3Fh
opna_channel_tmp        db 0
opna_bank_tmp           db 0
opna_local_tmp          db 0
opna_note_tmp           db 0
opna_fnum_tmp           dw 0
opna_ssg_channel_tmp    db 0
opna_ssg_note_tmp       db 0
opna_ssg_period_tmp     dw 0
opna_scene_atten_tmp    db 0

opna_port_candidates    dw 0088h,0188h,0288h,0388h
opn26_port_candidates   dw 0088h,0188h
opn26_probe_result      db 0
opna_operator_offsets   db 0,4,8,12
opna_rhythm_voice_setup db 0D8h,0D4h,098h,0D2h,094h,054h

; Direct rhythm_steps data is stored with the common score.

; Carrier flags in YM register-slot order OP1,OP3,OP2,OP4.
; /NV affects audible carriers.  Per-bar offsets are then applied to all four
; operators, matching the authored OPNA Draft 8 dynamics.
opna_carrier_flags      db 0,0,1,1, 0,1,1,1, 0,0,0,1
                        db 0,0,1,1, 0,1,1,1, 1,1,1,1

; Six four-operator OPNA patches from METROPOLITAN IGNITION.
opna_mul:
        db 01h,04h,02h,01h, 01h,02h,03h,01h, 01h,01h,02h,01h
        db 01h,01h,02h,02h, 01h,03h,02h,01h, 01h,02h,04h,01h
opna_tl:
        db 20h,2Ah,12h,08h, 18h,24h,2Eh,08h, 10h,22h,28h,04h
        db 24h,20h,16h,14h, 22h,28h,14h,0Eh, 24h,2Ah,2Eh,12h
opna_ar:
        db 1Fh,1Fh,1Fh,1Fh, 1Fh,1Fh,1Fh,1Fh, 1Fh,1Fh,1Eh,1Fh
        db 18h,17h,1Ah,19h, 1Fh,1Fh,1Eh,1Fh, 1Eh,1Fh,1Dh,1Fh
opna_dr:
        db 18h,1Ah,10h,0Eh, 17h,16h,18h,14h, 12h,15h,18h,10h
        db 07h,08h,09h,08h, 14h,16h,10h,0Eh, 0Ch,10h,12h,0Ah
opna_sr:
        db 0Ch,0Eh,05h,04h, 0Ah,09h,0Bh,08h, 05h,08h,0Ah,04h
        db 03h,03h,04h,03h, 08h,0Ah,05h,04h, 04h,06h,07h,03h
opna_slrr:
        db 67h,77h,57h,46h, 75h,75h,85h,64h, 55h,66h,76h,45h
        db 78h,78h,67h,67h, 66h,77h,57h,46h, 68h,79h,79h,57h
opna_alg             db 1Ch,1Dh,19h,14h,1Dh,0Fh
opna_pan             db 0C0h,080h,0C0h,040h,080h,040h
opna_fnum_table      dw 026Ah,028Fh,02B6h,02DFh,030Bh,0339h
                     dw 036Ah,039Eh,03D5h,0410h,044Eh,048Fh

; SSG periods use the same pitch convention as FM: block 4 is C5.
opna_ssg_period_c5   dw 239,225,212,200,189,178
                     dw 168,159,150,142,134,126

; Per-bar authored dynamics. FM entries are signed TL offsets; SSG and rhythm
; entries are absolute source levels before /NV or /RV scaling.
opna_bar_fm_atten:
        db 02h,04h,03h,08h,09h,0Ah ; bar 01
        db 01h,03h,02h,07h,08h,09h ; bar 02
        db 00h,02h,01h,06h,07h,08h ; bar 03
        db 0FFh,01h,00h,05h,06h,07h ; bar 04
        db 0FFh,00h,00h,05h,06h,08h ; bar 05
        db 0FFh,00h,00h,05h,06h,08h ; bar 06
        db 0FFh,00h,00h,05h,06h,08h ; bar 07
        db 0FFh,00h,00h,05h,06h,08h ; bar 08
        db 0FFh,00h,00h,05h,06h,08h ; bar 09
        db 0FFh,00h,00h,05h,06h,08h ; bar 10
        db 0FFh,00h,00h,05h,06h,08h ; bar 11
        db 0FFh,00h,00h,05h,06h,08h ; bar 12
        db 0FFh,00h,00h,05h,06h,08h ; bar 13
        db 0FFh,00h,00h,05h,06h,08h ; bar 14
        db 0FFh,00h,00h,05h,06h,08h ; bar 15
        db 0FFh,00h,00h,05h,06h,08h ; bar 16
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 17
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 18
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 19
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 20
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 21
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 22
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 23
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 24
        db 00h,03h,02h,07h,08h,0Ah ; bar 25
        db 00h,03h,02h,07h,08h,0Ah ; bar 26
        db 00h,03h,02h,07h,08h,0Ah ; bar 27
        db 00h,03h,02h,07h,08h,0Ah ; bar 28
        db 0FFh,02h,01h,06h,07h,09h ; bar 29
        db 0FFh,02h,01h,06h,07h,09h ; bar 30
        db 0FFh,02h,01h,06h,07h,09h ; bar 31
        db 0FFh,02h,01h,06h,07h,09h ; bar 32
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 33
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 34
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 35
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 36
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 37
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 38
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 39
        db 02h,05h,04h,09h,0Ah,0Ch ; bar 40
        db 00h,03h,02h,07h,08h,0Ah ; bar 41
        db 00h,03h,02h,07h,08h,0Ah ; bar 42
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 43
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 44
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 45
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 46
        db 0FFh,02h,01h,06h,07h,09h ; bar 47
        db 0FFh,00h,00h,05h,06h,08h ; bar 48
        db 0FEh,0FFh,0FFh,04h,05h,07h ; bar 49
        db 0FEh,0FEh,0FEh,03h,04h,06h ; bar 50
        db 0FEh,0FEh,0FEh,03h,04h,06h ; bar 51
        db 0FEh,0FEh,0FEh,03h,04h,06h ; bar 52
        db 0FEh,0FEh,0FEh,03h,04h,06h ; bar 53
        db 0FEh,0FEh,0FEh,03h,04h,06h ; bar 54
        db 0FDh,0FDh,0FDh,02h,02h,04h ; bar 55
        db 0FDh,0FDh,0FDh,02h,02h,04h ; bar 56
        db 0FDh,0FDh,0FDh,02h,02h,04h ; bar 57
        db 0FDh,0FDh,0FDh,02h,02h,04h ; bar 58
        db 0FCh,0FCh,0FBh,01h,01h,02h ; bar 59
        db 0FCh,0FCh,0FBh,01h,01h,02h ; bar 60
        db 0FCh,0FCh,0FBh,01h,01h,02h ; bar 61
        db 0FCh,0FCh,0FBh,01h,01h,02h ; bar 62
        db 0FCh,0FCh,0FBh,01h,01h,02h ; bar 63
        db 0FDh,0FDh,0FDh,02h,02h,04h ; bar 64
opna_bar_ssg_volume:
        db 06h,06h,06h ; bar 01
        db 07h,07h,07h ; bar 02
        db 08h,08h,08h ; bar 03
        db 09h,09h,08h ; bar 04
        db 0Ah,09h,09h ; bar 05
        db 0Ah,09h,09h ; bar 06
        db 0Ah,09h,09h ; bar 07
        db 0Ah,09h,09h ; bar 08
        db 0Ah,09h,09h ; bar 09
        db 0Ah,09h,09h ; bar 10
        db 0Ah,09h,09h ; bar 11
        db 0Ah,09h,09h ; bar 12
        db 0Ah,09h,09h ; bar 13
        db 0Ah,09h,09h ; bar 14
        db 0Ah,09h,09h ; bar 15
        db 0Ah,09h,09h ; bar 16
        db 0Ah,0Ah,09h ; bar 17
        db 0Ah,0Ah,09h ; bar 18
        db 0Ah,0Ah,09h ; bar 19
        db 0Ah,0Ah,09h ; bar 20
        db 06h,06h,06h ; bar 21
        db 06h,06h,06h ; bar 22
        db 06h,06h,06h ; bar 23
        db 06h,06h,06h ; bar 24
        db 08h,08h,07h ; bar 25
        db 08h,08h,07h ; bar 26
        db 08h,08h,07h ; bar 27
        db 08h,08h,07h ; bar 28
        db 09h,08h,08h ; bar 29
        db 09h,08h,08h ; bar 30
        db 09h,08h,08h ; bar 31
        db 09h,08h,08h ; bar 32
        db 0Ah,0Ah,09h ; bar 33
        db 0Ah,0Ah,09h ; bar 34
        db 0Ah,0Ah,09h ; bar 35
        db 0Ah,0Ah,09h ; bar 36
        db 06h,06h,06h ; bar 37
        db 06h,06h,06h ; bar 38
        db 06h,06h,06h ; bar 39
        db 06h,06h,06h ; bar 40
        db 08h,08h,07h ; bar 41
        db 08h,08h,07h ; bar 42
        db 0Ah,0Ah,09h ; bar 43
        db 0Ah,0Ah,09h ; bar 44
        db 0Ah,0Ah,09h ; bar 45
        db 0Ah,0Ah,09h ; bar 46
        db 09h,08h,08h ; bar 47
        db 0Ah,09h,09h ; bar 48
        db 0Ah,0Ah,09h ; bar 49
        db 0Bh,0Ah,0Ah ; bar 50
        db 0Bh,0Ah,0Ah ; bar 51
        db 0Bh,0Ah,0Ah ; bar 52
        db 0Bh,0Ah,0Ah ; bar 53
        db 0Bh,0Ah,0Ah ; bar 54
        db 0Bh,0Bh,0Ah ; bar 55
        db 0Bh,0Bh,0Ah ; bar 56
        db 0Bh,0Bh,0Ah ; bar 57
        db 0Bh,0Bh,0Ah ; bar 58
        db 0Ch,0Ch,0Bh ; bar 59
        db 0Ch,0Ch,0Bh ; bar 60
        db 0Ch,0Ch,0Bh ; bar 61
        db 0Ch,0Ch,0Bh ; bar 62
        db 0Ch,0Ch,0Bh ; bar 63
        db 0Bh,0Bh,0Ah ; bar 64
opna_bar_rhythm_level:
        db 00h,28h,2Ch,30h,32h,32h,32h,32h,32h,32h,32h,32h,34h,34h,34h,34h ; bars 01
        db 34h,34h,34h,34h,2Ah,2Ah,2Ah,2Ah,2Eh,2Eh,2Eh,2Eh,36h,36h,36h,36h ; bars 02
        db 36h,36h,36h,36h,2Bh,2Bh,2Bh,2Bh,30h,30h,37h,37h,37h,37h,30h,34h ; bars 03
        db 38h,3Ah,3Bh,3Bh,3Bh,3Bh,3Dh,3Dh,3Dh,3Dh,3Fh,3Fh,3Fh,3Fh,3Fh,3Bh ; bars 04


; OPL3 port state
opl_present             db 0
opl_probe_mode          db OPL_PROBE_AUTO
opl_detect_source       db OPL_DETECT_NONE
opl_ymf_signature    db 0
opl_addr0               dw 1488h
opl_data0               dw 1489h
opl_addr1               dw 148Ah
opl_data1               dw 148Bh
opl_118_id_port         dw 0000h
opl_118_id_value        db 0FFh
opl_ymf297_active       db 0
opl_percussion_active   db 0
opl_bank_tmp            db 0
opl_local_tmp           db 0
opl_mod_tmp             db 0
opl_car_tmp             db 0
opl_note_tmp            db 0
opl_block_tmp           db 0
opl_fnum_tmp            dw 0
opl_scene_atten_tmp     db 0

; Optional board-level OPL mixer settings.  0FFh means leave the system mixer
; untouched.  Explicit values are percentages 0..100 and are restored on exit.
sb_mixer_volume         db 0FFh
cs_mixer_volume         db 0FFh
sb_mixer_active         db 0
cs_mixer_active         db 0
sb_mixer_addr_port      dw 0
sb_mixer_data_port      dw 0
sb_mixer_saved_index    db 0
sb_mixer_saved_left     db 0
sb_mixer_saved_right    db 0
cs_mixer_saved_index    db 0
cs_mixer_saved_left     db 0
cs_mixer_saved_right    db 0

; Saved YMF7xx WSS AUX2 mixer state.  AUX2 carries the integrated OPL3
; output on OPL3-SA hardware; only bit 7 is temporarily cleared.
ymf_aux2_active         db 0
ymf_aux2_saved_index    db 0
ymf_aux2_saved_left     db 0
ymf_aux2_saved_right    db 0

opl_port_candidates:
        ; Sound Blaster 16(98): base I/O can be D2h,D4h,...,DEh.
        ; These exact board mappings are checked before any 1488h fallback.
        dw 20D2h,21D2h,22D2h,23D2h
        dw 20D4h,21D4h,22D4h,23D4h
        dw 20D6h,21D6h,22D6h,23D6h
        dw 20D8h,21D8h,22D8h,23D8h
        dw 20DAh,21DAh,22DAh,23DAh
        dw 20DCh,21DCh,22DCh,23DCh
        dw 20DEh,21DEh,22DEh,23DEh

        ; SB16(98) alternate C8xxh-CBxxh aliases for the same base choices.
        dw 0C8D2h,0C9D2h,0CAD2h,0CBD2h
        dw 0C8D4h,0C9D4h,0CAD4h,0CBD4h
        dw 0C8D6h,0C9D6h,0CAD6h,0CBD6h
        dw 0C8D8h,0C9D8h,0CAD8h,0CBD8h
        dw 0C8DAh,0C9DAh,0CADAh,0CBDAh
        dw 0C8DCh,0C9DCh,0CADCh,0CBDCh
        dw 0C8DEh,0C9DEh,0CADEh,0CBDEh

        ; PC-9801-118 / 86+118 and YMF OPL3-SA use 1488h-148Bh.
        ; This is intentionally last so WSS-only hardware cannot hide SB16.
        dw 1488h,1489h,148Ah,148Bh

opl_mod_offsets         db 0,1,2,8,9,10,16,17,18
opl_car_offsets         db 3,4,5,11,12,13,19,20,21

; Approximate standard OPL F-numbers C..B.
opl_fnum_table          dw 0157h,016Bh,0181h,0198h,01B0h,01CAh
                        dw 01E5h,0202h,0220h,0241h,0263h,0287h

; Fifteen two-operator OPL3 patches, 11 bytes each:
; mod20,car20,mod40,car40,mod60,car60,mod80,car80,modE0,carE0,C0.
; CH0..8 carry the common tonal score. CH9..14 are one-shot percussion voices.
opl_patch_table:
        db 22h,21h,16h,06h,0F4h,0F3h,45h,25h,00h,00h,34h ; OPL3 urban pluck lead
        db 21h,22h,0Ch,08h,0E5h,0F4h,35h,34h,01h,00h,31h ; OPL3 city additive stab
        db 01h,21h,15h,05h,0F2h,0F3h,54h,34h,00h,00h,36h ; OPL3 solid drive bass
        db 21h,22h,0Fh,0Ch,0B5h,0C5h,56h,56h,00h,00h,31h ; OPL3 broad chord organ
        db 23h,21h,1Bh,08h,0E4h,0F3h,45h,25h,02h,00h,32h ; OPL3 chrome response lead
        db 24h,21h,1Dh,0Ah,0F5h,0F4h,36h,25h,01h,00h,34h ; OPL3 skyline glass accent
        db 23h,21h,18h,0Ah,0E8h,0F6h,46h,36h,01h,00h,34h ; OPL3 prismatic electric harp
        db 21h,21h,18h,10h,0B5h,0C6h,57h,57h,00h,00h,31h ; OPL3 soft glass organ counterline
        db 22h,21h,1Bh,0Ch,0F5h,0F5h,84h,64h,00h,00h,32h ; OPL3 muted metro marimba pulse
        db 01h,01h,0Ah,04h,0F9h,0F7h,0A8h,97h,00h,00h,36h ; OPL3 compact urban kick
        db 0Bh,01h,16h,0Dh,0FCh,0FAh,0D9h,0C9h,02h,00h,34h ; OPL3 short electronic snare
        db 0Bh,07h,24h,1Ch,0F8h,0F8h,0B8h,0A8h,01h,03h,31h ; OPL3 restrained metal crash
        db 09h,07h,2Ah,22h,0FDh,0FDh,0E9h,0E9h,01h,02h,31h ; OPL3 dry closed hat
        db 02h,01h,12h,09h,0FAh,0F8h,0B8h,0A8h,00h,00h,34h ; OPL3 rounded electro tom
        db 04h,01h,26h,1Ch,0FDh,0FCh,0E9h,0D9h,01h,00h,31h ; OPL3 soft wood rim click

opl_percussion_bits     db 01h,02h,04h,08h,10h,20h
opl_percussion_notes    db 10h,32h,56h,5Bh,19h,47h

; Per-bar OPL3 TL offsets. Tonal rows contain CH0..8; percussion uses one
; common offset for CH9..14. Signed tonal FM offsets retain the OPNA dynamics.
opl_bar_tonal_atten:
        db 02h,04h,03h,08h,09h,0Ah,08h,08h,08h ; bar 01
        db 01h,03h,02h,07h,08h,09h,07h,07h,07h ; bar 02
        db 00h,02h,01h,06h,07h,08h,06h,06h,06h ; bar 03
        db 0FFh,01h,00h,05h,06h,07h,05h,05h,06h ; bar 04
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 05
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 06
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 07
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 08
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 09
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 10
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 11
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 12
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 13
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 14
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 15
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 16
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 17
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 18
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 19
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 20
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 21
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 22
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 23
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 24
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 25
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 26
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 27
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 28
        db 0FFh,02h,01h,06h,07h,09h,05h,07h,07h ; bar 29
        db 0FFh,02h,01h,06h,07h,09h,05h,07h,07h ; bar 30
        db 0FFh,02h,01h,06h,07h,09h,05h,07h,07h ; bar 31
        db 0FFh,02h,01h,06h,07h,09h,05h,07h,07h ; bar 32
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 33
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 34
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 35
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 36
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 37
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 38
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 39
        db 02h,05h,04h,09h,0Ah,0Ch,0Bh,0Bh,0Bh ; bar 40
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 41
        db 00h,03h,02h,07h,08h,0Ah,07h,07h,09h ; bar 42
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 43
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 44
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 45
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 46
        db 0FFh,02h,01h,06h,07h,09h,05h,07h,07h ; bar 47
        db 0FFh,00h,00h,05h,06h,08h,03h,05h,05h ; bar 48
        db 0FEh,0FFh,0FFh,04h,05h,07h,03h,03h,05h ; bar 49
        db 0FEh,0FEh,0FEh,03h,04h,06h,01h,03h,03h ; bar 50
        db 0FEh,0FEh,0FEh,03h,04h,06h,01h,03h,03h ; bar 51
        db 0FEh,0FEh,0FEh,03h,04h,06h,01h,03h,03h ; bar 52
        db 0FEh,0FEh,0FEh,03h,04h,06h,01h,03h,03h ; bar 53
        db 0FEh,0FEh,0FEh,03h,04h,06h,01h,03h,03h ; bar 54
        db 0FDh,0FDh,0FDh,02h,02h,04h,01h,01h,03h ; bar 55
        db 0FDh,0FDh,0FDh,02h,02h,04h,01h,01h,03h ; bar 56
        db 0FDh,0FDh,0FDh,02h,02h,04h,01h,01h,03h ; bar 57
        db 0FDh,0FDh,0FDh,02h,02h,04h,01h,01h,03h ; bar 58
        db 0FCh,0FCh,0FBh,01h,01h,02h,00h,00h,01h ; bar 59
        db 0FCh,0FCh,0FBh,01h,01h,02h,00h,00h,01h ; bar 60
        db 0FCh,0FCh,0FBh,01h,01h,02h,00h,00h,01h ; bar 61
        db 0FCh,0FCh,0FBh,01h,01h,02h,00h,00h,01h ; bar 62
        db 0FCh,0FCh,0FBh,01h,01h,02h,00h,00h,01h ; bar 63
        db 0FDh,0FDh,0FDh,02h,02h,04h,01h,01h,03h ; bar 64
opl_bar_percussion_atten:
        db 3Fh,0Ah,08h,07h,06h,06h,06h,06h,06h,06h,06h,06h,05h,05h,05h,05h ; bars 01
        db 05h,05h,05h,05h,09h,09h,09h,09h,08h,08h,08h,08h,04h,04h,04h,04h ; bars 02
        db 04h,04h,04h,04h,09h,09h,09h,09h,07h,07h,04h,04h,04h,04h,07h,05h ; bars 03
        db 03h,02h,02h,02h,02h,02h,01h,01h,01h,01h,00h,00h,00h,00h,00h,02h ; bars 04

; One common score drives OPN, OPNA and OPL3.
%include "MUSIC_URBAN_D8.INC"



; Final-scene rotozoom overlay state (Q8 affine coefficients).
city_roto_angle          db 0
align 2
city_roto_phase          dw 0
city_roto_zoom_q8        dw 0
city_roto_sin_q8         dw 0
city_roto_cos_q8         dw 0
city_roto_center_x       dw 0
city_roto_center_y       dw 0
city_roto_focus_center_x dw 0
city_roto_focus_center_y dw 0
city_roto_focus_ease     dw 0
city_roto_cell_x         dw 0
city_roto_cell_y         dw 0
city_roto_p0_x           dw 0
city_roto_p0_y           dw 0
city_roto_p1_x           dw 0
city_roto_p1_y           dw 0
city_roto_p2_x           dw 0
city_roto_p2_y           dw 0
city_roto_p3_x           dw 0
city_roto_p3_y           dw 0
city_roto_bbox_min_x     dw 0
city_roto_bbox_max_x     dw 0
city_roto_bbox_min_y     dw 0
city_roto_bbox_max_y     dw 0
city_roto_row            db 0
city_roto_col            db 0
align 2

; ---------------------------------------------------------------------------
; NEON 3 fixed-point city renderer state
; ---------------------------------------------------------------------------
align 4
city_cam_x              dd 0       ; per-scene target camera
city_cam_y              dd 0
city_cam_z              dd 0
city_view_x_q4          dd 0       ; persistent signed 28.4 camera
city_view_y_q4          dd 0
city_view_z_q4          dd 0
city_view_yaw_phase     dw 0       ; interpolated 8.8 table phase
city_view_pitch_phase   dw 0
city_view_roll_phase    dw 0
city_camera_initialized db 0
city_camera_direct      db 0
city_camera_z_direct    db 0
city_camera_phase_direct db 0
city_camera_view_q4_direct db 0
align 2
city_tunnel_curve_frame      dw 0
city_tunnel_curve_x0         dw 0
city_tunnel_curve_offset_now dw 0
city_tunnel_curve_s          dd 0
city_rail_preview_x0        dw 0
city_rail_preview_z0        dw 0
city_rail_preview_world_x   dw 0
city_rail_preview_world_z   dw 0
city_roto_cover_page_mask   db 0
align 2
city_cam_yaw_phase_target   dw 0
city_cam_pitch_phase_target dw 0
city_cam_roll_phase_target  dw 0
align 4
city_world_z_bias       dd 0
align 2
city_global_frame       dw 0
city_route_travel       dw 0
city_route_center_x     dw 0
city_route_model_x      dw 0
city_route_model_z      dw 0
city_transition_seg_z0  dw 0
city_transition_seg_z1  dw 0
city_transition_seg_half dw 0
city_transition_seg_y   dw 0
city_transition_seg_spacing dw 0
city_transition_cursor_z dw 0
city_transition_ease    dw 0
align 4
city_bezier_t            dd 0
city_bezier_p0           dd 0
city_bezier_p1           dd 0
city_bezier_p2           dd 0
city_bezier_p3           dd 0
city_bezier_a            dd 0
city_bezier_b            dd 0
city_bezier_c            dd 0
city_bezier_d            dd 0
city_bezier_e            dd 0
city_delta_x            dd 0
city_delta_y            dd 0
city_delta_z            dd 0
city_rot_x              dd 0
city_rot_y              dd 0
city_rot_z              dd 0
city_depth32            dd 0
city_screen_space_x     dd 0
city_screen_space_y     dd 0
city_cam_yaw            db 0
city_cam_pitch          db 0
city_cam_roll           db 0
align 2
city_sin_yaw            dw 0
city_cos_yaw            dw 16384
city_sin_pitch          dw 0
city_cos_pitch          dw 16384
city_sin_roll           dw 0
city_cos_roll           dw 16384
city_project_x          dw 0
city_project_y          dw 0
city_project_depth      dw 0
city_project_valid      db 0
align 2

; Line-level clipping scratch.  Long road/ground lines may cross the near
; plane or screen boundary even though one endpoint is not directly visible.
city_line_depth0        dd 0
city_line_depth1        dd 0
city_clip_boundary      dd 0
city_clip_num           dd 0
city_clip_den           dd 0
city_clip_temp_x        dw 0
city_clip_temp_y        dw 0
city_clip_temp_z        dw 0
city_clip_endpoint      db 0
city_clip_code0         db 0
city_clip_code1         db 0
align 2
city_clip_x0            dw 0
city_clip_y0            dw 0
city_clip_x1            dw 0
city_clip_y1            dw 0

city_line_x0            dw 0
city_line_y0            dw 0
city_line_z0            dw 0
city_line_x1            dw 0
city_line_y1            dw 0
city_line_z1            dw 0
city_line_sx0           dw 0
city_line_sy0           dw 0
city_ground_x           dw 0
city_ground_y           dw 0
city_ground_z0          dw 0
city_ground_z1          dw 0
city_ground_cursor_z    dw 0

city_obj_x              dw 0
city_obj_y              dw 0
city_obj_z              dw 0
city_obj_w              dw 0
city_obj_h              dw 0
city_obj_d              dw 0
city_obj_skew_q8        dw 0       ; X shift per +256 Z for road-aligned boxes
city_obj_skew_dx        dw 0       ; cached shift across city_obj_d
city_obj_antenna        dw 120
city_obj_flags          db 0
align 2
city_box_projected      times 8*2 dw 0
city_box_valid          times 8 db 0
city_edge_sx            dw 0
city_edge_sy            dw 0
city_edge_v0            db 0
city_edge_v1            db 0
city_band_y             dw 0
city_box_edges:
        db 0,1, 1,3, 3,2, 2,0
        db 4,5, 5,7, 7,6, 6,4
        db 0,4, 1,5, 2,6, 3,7

city_panel_x0           dw 0
city_panel_x1           dw 0
city_panel_y0           dw 0
city_panel_y1           dw 0
city_panel_z            dw 0

city_tree_x             dw 0
city_tree_z             dw 0
city_tree_x0            dw 0
city_tree_x1            dw 0
city_tree_z0            dw 0
city_tree_z1            dw 0

city_road_half          dw 0
city_road_y             dw 0
city_road_far           dw 0
city_road_spacing       dw 0
city_road_phase         dw 0
city_road_cycle         dw 0
city_road_cross_count   db 0
align 2
city_road_skew_q8       dw 0
city_grid_index         dw 0
city_grid_z             dw 0
city_lamp_pulse_color  db 0
city_lamp_pulse_table  db CITY_LAMP_PULSE_LEVELS
city_lamp_phase_fast_table:
        db 0,1,2,3,5,6,7,8,10,11,12,13,15,0,1,2
        db 4,5,6,7,9,10,11,12,14,15,0,1,3,4,5,6
        db 8,9,10,11,13,14,15,0,2,3,4,5,7,8,9,10
        db 12,13,14,15,1,2,3,4,6,7,8,9,11,12,13,14
align 2

city_row_left_x         dw 0
city_row_right_x        dw 0
city_row_y              dw 0
city_row_width          dw 0
city_row_depth          dw 0
city_row_height_bias    dw 0
city_row_spacing        dw 0
city_row_near           dw 0
city_row_z_limit        dw 0
city_row_phase          dw 0
city_row_cycle          dw 0
city_row_count          db 0
city_row_color_base     db 0
city_row_index          dw 0
city_row_cell_base      dw 0
city_row_remainder      dw 0
city_row_identity       dw 0
city_row_skew_q8        dw 0
city_row_skew_offset    dw 0

city_sway_amount        dw 0
city_sway_speed         db 0
city_orbit_angle        db 0
align 2
city_orbit_phase        dw 0
city_bridge_phase       dw 0
city_bridge_z           dw 0
city_bridge_visible_z_min dw 0 ; scene-2 model-level near cull; keeps repeat phase unchanged
city_tunnel_phase       dw 0
city_rail_phase         dw 0
city_train_phase        dw 0
city_oncoming_train_phase dw 0
city_rail_pass_swerve_q4   dd 0
city_rail_pass_roll_phase dw 0
; View-only commercial-sign avoidance pulse (scene 4).
city_neon_dodge_x       dw 0
city_neon_dodge_y       dw 0
city_neon_dodge_roll_phase  dw 0
city_neon_dodge_pitch_phase dw 0
city_neon_dodge_tilt_q14  dw 0       ; sine envelope for smooth bank/pitch
city_neon_scenery_x_bias dw 0   ; per-object rear-city side shift
city_terminal_phase     dw 0
city_terminal_z         dw 0

city_curve_prev_valid   dw 0
city_curve_index        dw 0
city_curve_angle        db 0
align 2
city_curve_outer_x      dw 0
city_curve_outer_z      dw 0
city_curve_inner_x      dw 0
city_curve_inner_z      dw 0
city_curve_y            dw 0
city_curve_prev_outer_x dw 0
city_curve_prev_outer_z dw 0
city_curve_prev_inner_x dw 0
city_curve_prev_inner_z dw 0
city_curve_prev_y       dw 0

; Procedural building variation. Values stay deliberately coarse so distant
; towers retain stable silhouettes and do not shimmer from frame to frame.
city_height_table:
        dw 260,420,620,340,780,500,920,380
        dw 700,300,560,860,450,1040,640,360
city_jitter_table:
        dw -70,20,55,-30,75,-55,10,40
        dw -20,65,-75,30,0,50,-40,80

city_park_tree_x:
        dw -430,-210,40,310,470,-350,-80,210,410,-240,80,300
city_park_tree_z:
        dw -410,-470,-390,-430,-180,-120,-80,-130,90,250,330,260
city_park_building_x:
        ; Move perimeter block #1 farther left so its 260-unit footprint does
        ; not intrude into the X=670..1130 neon road during the park exit.
        dw -1150,-900,-350,760,960,650,-360,-980
city_park_building_z:
        dw -650,760,900,760,-420,-900,-1050,250

city_rail_x:
        ; Original railway geometry: two track pairs to the left of the X=900
        ; camera centre.  Keep this internal relationship unchanged; scene
        ; transitions must move/rebase the whole scene, not individual rails.
        dw -360,-300,-170,-110
city_rail_track_only      db 0
city_interchange_support_x:
        dw -280,190,-280,190
city_interchange_support_z:
        dw 1170,1170,1640,1640

; ---------------------------------------------------------------------------
; NEON3 80286 private state
; ---------------------------------------------------------------------------
video_400_mode    db 0
align 2
video_height      dw VIDEO_H
video_last_line   dw VIDEO_H-1
video_clear_words dw (BYTES_PER_LINE * VIDEO_H) / 2

text286_top db ' NEON RELAY 3 :: MEGACITY FLIGHT                               FRAME=',0
text286_ticker db ' NEON RELAY 3 // CITY DESCENT // TOWER CANYON // MEGABRIDGE // CENTRAL PARK // NEON DISTRICT // SUBTERRANEAN ROAD // RAIL TO TERMINAL // AERIAL TERMINAL // CITY ORBIT // ',0
text286_ticker_end:
text16_video_status_200 db 'ANALOG 16-COLOR / 640x200 / GRCG 4-PLANE / PAGE FLIP',0
text16_video_status_400 db 'ANALOG 16-COLOR / 640x400 BENCHMARK / GRCG 4-PLANE',0
text16_cpu_status_286   db '80286 / 16-BIT FIXED-POINT 3D / SHARED NEON3 64-BAR SCORE',0

; 16-bit camera and projection scratch.  All world coordinates are scene-local.
city286_cam_x       dw 0
city286_cam_y       dw 0
city286_cam_z       dw 0
city286_yaw         db 0
city286_pitch       db 0
city286_roll        db 0
align 2
city286_sin_yaw     dw 0
city286_cos_yaw     dw 16384
city286_sin_pitch   dw 0
city286_cos_pitch   dw 16384
city286_sin_roll    dw 0
city286_cos_roll    dw 16384
city286_dx          dw 0
city286_dy          dw 0
city286_dz          dw 0
city286_rx          dw 0
city286_ry          dw 0
city286_rz          dw 0
city286_tx          dw 0
city286_ty          dw 0
city286_tz          dw 0
city286_project_x   dw 0
city286_project_y   dw 0
city286_project_z   dw 0
city286_project_ok  db 0
align 2
city286_line_x0     dw 0
city286_line_y0     dw 0
city286_line_z0     dw 0
city286_line_x1     dw 0
city286_line_y1     dw 0
city286_line_z1     dw 0
city286_screen_x0   dw 0
city286_screen_y0   dw 0
city286_screen_x1   dw 0
city286_screen_y1   dw 0
city286_tmp0        dw 0
city286_tmp1        dw 0
city286_tmp2        dw 0
city286_tmp3        dw 0
city286_tmp4        dw 0
city286_tmp5        dw 0
city286_loop_z      dw 0
city286_loop_x      dw 0
city286_phase       dw 0
city286_angle       db 0
city286_color       db 0
align 2
city286_train_step  dw -500

; Procedural object descriptor used by the reduced-cost 286 box renderer.
city286_obj_x       dw 0
city286_obj_y       dw 0
city286_obj_z       dw 0
city286_obj_w       dw 0
city286_obj_h       dw 0
city286_obj_d       dw 0
city286_obj_color   db 0
city286_obj_flags   db 0

; Projected box corners: top/front/back, bottom/front/back.
city286_box_xy      times 8*2 dw 0
city286_box_valid   times 8 db 0

; Lightweight final affine panel state.
city286_roto_phase   dw 0
city286_roto_angle   db 0
align 2
city286_roto_zoom    dw 0
city286_roto_sin     dw 0
city286_roto_cos     dw 0
city286_roto_cx      dw 0
city286_roto_cy      dw 0
city286_roto_x0      dw 0
city286_roto_y0      dw 0
city286_roto_x1      dw 0
city286_roto_y1      dw 0

; Small deterministic height/jitter tables keep the skyline stable while the
; camera moves.  Values are deliberately 16-bit and division-free in hot loops.
city286_height_table:
        dw 520,880,1320,700,1580,1040,1900,760
        dw 1440,960,1740,620,1180,2020,820,1500
city286_width_table:
        dw 260,340,220,300,280,360,240,320
city286_sign_color:
        db 4,6,3,5,2,7,4,6,5,3,7,2,6,4,5,7

; ---------------------------------------------------------------------------
; Faithful-port 80286 renderer state
; ---------------------------------------------------------------------------
city286f_scene_z_bias    dw 0
city286f_model_z_bias    dw 0
city286f_t0x             dw 0
city286f_t0y             dw 0
city286f_t0z             dw 0
city286f_t1x             dw 0
city286f_t1y             dw 0
city286f_t1z             dw 0
city286f_sx0             dw 0
city286f_sy0             dw 0
city286f_sx1             dw 0
city286f_sy1             dw 0
city286f_out0            db 0
city286f_out1            db 0
city_water_raster_active db 0
align 2
city_water_raster_table_frame dw 0FFFFh
city_water_warp_x0      dw 0
city_water_warp_y0      dw 0
city_water_warp_x1      dw 0
city_water_warp_y1      dw 0
city_water_warp_dx      dw 0
city_water_warp_dy      dw 0
city_water_warp_sx      dw 0
city_water_warp_sy      dw 0
city_water_warp_err     dw 0
city_water_warp_e2      dw 0
city_water_raster_shift_table times SCREEN_H db 0
align 2
city286f_box_x0          dw 0
city286f_box_x1          dw 0
city286f_box_y0          dw 0
city286f_box_y1          dw 0
city286f_box_z0          dw 0
city286f_box_z1          dw 0
city286f_bezier_t        dw 0       ; Q8 0..256
city286f_bezier_p0       dw 0
city286f_bezier_p1       dw 0
city286f_bezier_p2       dw 0
city286f_bezier_p3       dw 0
city286f_lerp_a          dw 0
city286f_lerp_b          dw 0
city286f_lerp_c          dw 0
city286f_lerp_d          dw 0
city286f_orbit_phase     dw 0       ; 8.8 turn phase
city286f_curve_s         dw 0
city286f_curve_offset    dw 0
city286f_rail_window0    dw 0
city286f_rail_window1    dw 0
city286f_screen_quad_x0  dw 0
city286f_screen_quad_y0  dw 0
city286f_screen_quad_x1  dw 0
city286f_screen_quad_y1  dw 0
city286f_screen_quad_x2  dw 0
city286f_screen_quad_y2  dw 0
city286f_screen_quad_x3  dw 0
city286f_screen_quad_y3  dw 0

; Persistent camera state used by the fidelity-first 286 renderer.
; Position is stored as signed integer world units plus a 1/16-unit remainder,
; which reproduces the source camera's 28.4 easing without 386 registers.
city286f_camera_z_direct     db 0
city286f_camera_phase_direct db 0
city286f_camera_view_direct  db 0
align 2
city286f_view_x              dw 0
city286f_view_y              dw 0
city286f_view_z              dw 0
city286f_view_yaw_phase      dw 0
city286f_view_pitch_phase    dw 0
city286f_view_roll_phase     dw 0
city286f_view_x_frac         db 0
city286f_view_y_frac         db 0
city286f_view_z_frac         db 0
align 2
city286f_pressure_x           dw 0
city286f_pressure_roll        dw 0
; 286 scanline triangle / faithful rotozoom state.
city286f_tri_y             dw 0
city286f_tri_xa            dw 0
city286f_tri_xb            dw 0
city286f_roto_focus_x      dw 0
city286f_roto_focus_y      dw 0
city286f_roto_focus_ease   dw 0
city286f_roto_cell_x       dw 0
city286f_roto_cell_y       dw 0
city286f_roto_row          db 0
city286f_roto_col          db 0
align 2
city286f_roto_p0_x         dw 0
city286f_roto_p0_y         dw 0
city286f_roto_p1_x         dw 0
city286f_roto_p1_y         dw 0
city286f_roto_p2_x         dw 0
city286f_roto_p2_y         dw 0
city286f_roto_p3_x         dw 0
city286f_roto_p3_y         dw 0
city286f_bbox_min_x        dw 0
city286f_bbox_max_x        dw 0
city286f_bbox_min_y        dw 0
city286f_bbox_max_y        dw 0
