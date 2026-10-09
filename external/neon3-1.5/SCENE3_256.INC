; ---------------------------------------------------------------------------
; SCENE3_256.INC - NEON 3 continuous city-flight chapter selection
; ---------------------------------------------------------------------------

select_scene:
        pusha
        mov     [city_global_frame], ax
        mov     [scene_frame], ax
        xor     bx, bx
        mov     si, scene_length_table
.select_loop:
        cmp     bx, SCENE_COUNT-1
        jae     .selected
        cmp     ax, [si]
        jb      .selected
        sub     ax, [si]
        mov     [scene_frame], ax
        add     si, 2
        inc     bx
        jmp     .select_loop
.selected:
        ; The subterranean -> railway boundary must rebase the persistent
        ; camera exactly once when the selected chapter changes.  Do not key
        ; this to scene_frame==0: PackedFull can be expensive enough for AFS
        ; to advance across the boundary and make the first rendered railway
        ; frame 1..N instead of 0.  In that case the old frame-0-only reset
        ; left the tunnel view in the camera smoother and recreated the
        ; abolished cross-frame slide behind the whiteout.
        cmp     bl, [scene_index]
        je      .scene_state_ready

        ; Persistent camera smoothing must be rebased at scene-local
        ; coordinate discontinuities by detecting the chapter transition,
        ; never by testing scene_frame==0.  AFS may advance several logical
        ; frames while one page is being rendered, so the first rendered
        ; frame after a boundary can legitimately be 1..N.
        cmp     bl, 0                   ; final city orbit -> city approach
        je      .rebase_camera
        cmp     bl, 6                   ; subterranean -> railway
        jne     .scene_state_ready
.rebase_camera:
        mov     byte [city_camera_initialized], 0
.scene_state_ready:
        mov     [scene_index], bl
        popa
        ret

render_scene:
        pusha
        xor     bx, bx
        mov     bl, [scene_index]
        shl     bx, 1
        call    word [scene_routines + bx]
        popa
        ret

; Keep the persistent camera smoother tied to logical frames rather than to
; the number of pages that happened to be rendered.  /AFS can advance the
; visual timeline many frames while a slow page is being built.  Replaying
; just the camera portion of those omitted frames reproduces the same camera
; state that /FS:0 would have reached without paying any geometry cost.
;
; For an exceptionally long stall, replaying hundreds/thousands of camera
; steps would itself create a catch-up spiral.  At that point no intermediate
; frame can be visible anyway, so rebase the smoother on the current target.
%define CITY_CAMERA_CATCHUP_MAX 64

city_camera_catch_up:
        pushf
        pusha
        mov     byte [city_camera_replay], 0
        mov     byte [city_camera_hold_smoothing], 0

        mov     ax, [render_frame_counter]
        cmp     byte [city_camera_timeline_valid], 0
        jne     .have_previous
        mov     [city_camera_last_frame], ax
        mov     byte [city_camera_timeline_valid], 1
        jmp     .done

.have_previous:
        ; CX = forward logical distance, including wrap at TOTAL_FRAMES.
        mov     cx, ax
        sub     cx, [city_camera_last_frame]
        jnc     .delta_ready
        add     cx, TOTAL_FRAMES
.delta_ready:
        or      cx, cx
        jnz     .advanced

        ; /SPEED below 100 can present another page at the same logical frame.
        ; Do not let that extra render iteration advance the smoother again.
        mov     byte [city_camera_hold_smoothing], 1
        jmp     .store_current

.advanced:
        cmp     cx, CITY_CAMERA_CATCHUP_MAX
        jbe     .replay_missing

        ; Very large AFS jump: snap the persistent filter to the current
        ; logical camera on the real render instead of spending another long
        ; interval replaying invisible frames.
        mov     byte [city_camera_initialized], 0
        jmp     .store_current

.replay_missing:
        ; The current frame itself will be processed by the normal render.
        ; Replay only last+1 .. current-1 with drawing suppressed.
        dec     cx
        jz      .store_current
        mov     dx, [city_camera_last_frame]
        mov     byte [city_camera_replay], 1
.replay_loop:
        inc     dx
        cmp     dx, TOTAL_FRAMES
        jb      .frame_ready
        xor     dx, dx
.frame_ready:
        mov     ax, dx
        call    select_scene
        call    render_scene
        loop    .replay_loop
        mov     byte [city_camera_replay], 0

.store_current:
        mov     ax, [render_frame_counter]
        mov     [city_camera_last_frame], ax
.done:
        mov     byte [city_camera_replay], 0
        popa
        popf
        ret

; Nine continuous chapters.  Removed ascent, aerial-crossing and interchange
; chapters are absorbed into the surrounding flight path.  Total remains 6144.
scene_length_table:
        dw      640,640,512,896,640,640,896,384,896

scene_routines:
        dw      scene_route_city_approach
        dw      scene_route_tower_canyon
        dw      scene_route_megabridge
        dw      scene_route_central_park
        dw      scene_route_neon_district
        dw      scene_route_subterranean
        dw      scene_route_rail_terminal
        dw      scene_route_terminal_ascent
        dw      scene_route_city_overlook
