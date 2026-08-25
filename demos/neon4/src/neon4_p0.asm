; ---------------------------------------------------------------------------
; Copyright (c) 2026 Nakata Maho
;
; This file is licensed under the BSD 2-Clause License.  See the repository
; license for the complete terms.  Ported By Maho Nakata.
; ---------------------------------------------------------------------------
; NEON RELAY 4 P0-1 extraction harness.
;
; This is deliberately not a VA payload.  It proves that the 80286 geometry,
; scene dispatcher, and scene data assemble as one flat binary before the VA
; drawing backend is introduced.
;
; The platform drawing entry points below are contract stubs.  The copied
; geometry file remains the source of the scene and projection implementation;
; hardware-specific rendering is introduced in a later milestone.

        cpu     286
        bits    16
        org     100h

%define NEON4_286 1
%define NEON4_P0 1

; Constants normally supplied by the discarded DOS/OPL and EGC modules.  They
; are compile-time compatibility values only; no P0 code performs device I/O.
%define OPL_PROBE_AUTO 0
%define OPL_DETECT_NONE 0
%define EGC_LENGTH_PORT 0
%define EGC_SHIFT_PORT 0

; ---------------------------------------------------------------------------
; P0 drawing contracts.  Inputs intentionally follow the original NEON4
; calling convention; no graphics memory or I/O is touched in this harness.
; ---------------------------------------------------------------------------

clear_graphics_frame16:
set_access_page:
low_dirty_span_frame_end:
text_update:
pixel_set:
hline_set:
hline_set_fast:
hline_set_same_colour_fast:
hline_set16_same_fast:
line_set:
hline_set_same_colour:
line_set_same_colour:
line_batch_begin:
fill_rect:
grcg16_prepare_color:
low_dirty_span_record_rect:
low_raster_track_rect16:
        ret

; The current low-colour source contains EGC/GRCG helpers around the scene
; code.  They are kept reachable for source extraction, but are inert in P0.
egc16_enable_vram_copy:
egc16_disable_to_grcg:
        ret

; ---------------------------------------------------------------------------
; Source extraction.  DATA4 includes the original music table; no sound or
; device code is linked by this harness.
; ---------------------------------------------------------------------------

%include "config4_286.inc"
%include "data4_p0.inc"
%include "low4_data.inc"
%include "geom4_low.inc"
%include "scene4_256.inc"
%include "frame_render4_low.inc"

; A real entry point is intentionally a harmless stop.  The probe below is
; retained so a listing proves that all eight scene entries are reachable from
; render_scene without running any platform code.
start:
        ret

neon4_p0_render_probe:
        mov     ax, [frame_counter]
        mov     [render_frame_counter], ax
        call    select_scene
        call    render_scene
        ret
