; ==========================================================================
; CITY3D286_FAITHFUL.INC
; Faithful 80286 port of the NEON3 256/16-colour scene choreography.
;
; Policy of this revision:
;   * preserve original NEON3 geometry, frame boundaries and camera targets;
;   * use scene-local Z rebasing only where signed 16-bit coordinates require it;
;   * do NOT reduce object counts merely for 286 performance;
;   * clip in camera depth and logical 640x400 screen space before Bresenham.
; ==========================================================================

; --------------------------------------------------------------------------
; 16-bit math helpers
; --------------------------------------------------------------------------
; AX=signed a, BX=signed b, CX=signed divisor -> AX=(a*b)/divisor.
;
; The railway overtake brings box edges very close to the near plane.  The
; depth/screen clippers can therefore ask for a signed 32/16 interpolation
; whose *intermediate* quotient is outside the IDIV r16 range even though the
; final line will immediately be rejected by the next clip stage.  A raw IDIV
; would raise INT 0 on a real 80286.  Normalize to unsigned magnitudes first,
; compare against abs(divisor)*32767, and saturate instead of trapping.  A
; zero denominator is a degenerate clip edge; return zero displacement.
city286f_muldiv_s16:
        push    bx
        push    cx
        push    dx
        push    si
        push    di

        mov     si, cx                  ; signed divisor
        imul    bx                      ; DX:AX = signed 32-bit numerator

        mov     bx, ax                  ; preserve numerator low
        mov     cx, dx                  ; preserve numerator high
        mov     ax, bx
        or      ax, cx
        jnz     .nonzero_num
        xor     ax, ax
        jmp     .done
.nonzero_num:
        test    si, si
        jnz     .have_divisor
        xor     ax, ax                  ; degenerate clip: no displacement
        jmp     .done
.have_divisor:
        xor     di, di                  ; bit0 = result negative
        test    cx, 8000h
        jz      .num_abs_ready
        not     bx
        not     cx
        add     bx, 1
        adc     cx, 0                   ; CX:BX = abs(numerator)
        xor     di, 1
.num_abs_ready:
        test    si, si
        jns     .div_abs_ready
        neg     si                      ; -32768 becomes unsigned 8000h
        xor     di, 1
.div_abs_ready:

        ; limit = abs(divisor) * 32767.
        mov     ax, si
        mov     dx, 32767
        mul     dx                      ; DX:AX = positive limit
        cmp     cx, dx
        ja      .saturate
        jb      .divide
        cmp     bx, ax
        ja      .saturate
.divide:
        mov     ax, bx
        mov     dx, cx
        div     si                      ; quotient is now guaranteed <=32767
        jmp     .apply_sign
.saturate:
        mov     ax, 32767
.apply_sign:
        test    di, 1
        jz      .done
        neg     ax
.done:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; AX=a, BX=b, CX=t(Q8 0..256) -> AX=lerp(a,b,t).
;
; Do not form (b-a) in 16 bits.  Bridge/park camera phases use wrapped signed
; 8.8 values, and a future control-point pair can legitimately straddle the
; signed boundary.  The weighted form keeps the complete signed 32-bit sum and
; only shifts after both products have been added.
city286f_lerp_q8:
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp

        mov     si, ax                  ; a
        mov     di, bx                  ; b
        mov     bp, cx                  ; t

        mov     bx, 256
        sub     bx, bp                  ; 256-t
        mov     ax, si
        imul    bx                      ; DX:AX = a*(256-t)
        push    dx
        push    ax

        mov     ax, di
        mov     bx, bp
        imul    bx                      ; DX:AX = b*t
        pop     bx                      ; first product low
        pop     cx                      ; first product high
        add     ax, bx
        adc     dx, cx                  ; DX:AX = weighted sum

        ; Signed 32-bit >> 8, 80286-safe (no SHRD).
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1
        sar     dx, 1
        rcr     ax, 1

        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; AX=elapsed, BX=last elapsed -> AX=0..256 smoothstep in Q8.
city286f_smoothstep:
        push    bx
        push    cx
        push    dx
        push    si
        mov     cx, bx
        mov     bx, 256
        mul     bx                      ; DX:AX = elapsed*256
        div     cx
        cmp     ax, 256
        jbe     .t_ok
        mov     ax, 256
.t_ok:
        mov     si, ax                  ; t
        mul     si                      ; t*t
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx                  ; BX=t^2 Q8
        mov     cx, si
        shl     cx, 1
        mov     ax, 768
        sub     ax, cx                  ; 3-2t, Q8
        mov     cx, ax
        mov     ax, bx
        mul     cx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     ax, bx
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; AX=elapsed, BX=last -> AX=(elapsed/last)^4 in Q8.
city286f_accel4:
        ; Match the 256 edition: calculate t^2 in Q12, then square to Q8.
        ; All intermediates fit the 80286's native DX:AX 32-bit product.
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     si, bx                  ; last
        mov     cx, ax                  ; elapsed
        mul     cx                      ; DX:AX = elapsed^2
        mov     di, 12
.shift_q12:
        shl     ax, 1
        rcl     dx, 1
        dec     di
        jnz     .shift_q12
        mov     [city286f_t0x], ax
        mov     [city286f_t0y], dx
        mov     ax, si
        mul     si                      ; last^2 (fits 16 bits here)
        mov     bx, ax
        mov     ax, [city286f_t0x]
        mov     dx, [city286f_t0y]
        div     bx                      ; AX=t^2, Q12 (0..4096)
        mov     bx, ax
        mul     bx                      ; Q24
        mov     ax, dx                  ; >>16 => Q8 t^4
        cmp     ax, 256
        jbe     .ready
        mov     ax, 256
.ready:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; Cubic Bezier using signed 16-bit control points and Q8 t.
; city286f_bezier_p0..p3 / city286f_bezier_t -> AX result.
city286f_bezier16:
        push    bx
        push    cx
        push    dx
        mov     cx, [city286f_bezier_t]
        mov     ax, [city286f_bezier_p0]
        mov     bx, [city286f_bezier_p1]
        call    city286f_lerp_q8
        mov     [city286f_lerp_a], ax
        mov     ax, [city286f_bezier_p1]
        mov     bx, [city286f_bezier_p2]
        call    city286f_lerp_q8
        mov     [city286f_lerp_b], ax
        mov     ax, [city286f_bezier_p2]
        mov     bx, [city286f_bezier_p3]
        call    city286f_lerp_q8
        mov     [city286f_lerp_c], ax
        mov     ax, [city286f_lerp_a]
        mov     bx, [city286f_lerp_b]
        call    city286f_lerp_q8
        mov     [city286f_lerp_d], ax
        mov     ax, [city286f_lerp_b]
        mov     bx, [city286f_lerp_c]
        call    city286f_lerp_q8
        mov     bx, ax
        mov     ax, [city286f_lerp_d]
        call    city286f_lerp_q8
        pop     dx
        pop     cx
        pop     bx
        ret

; AX=8.8 phase -> AX=interpolated Q2.14 sine.
; Real interpolated phase helper. Kept separate to avoid hidden 32-bit state.
; AX=phase 8.8 -> AX=sine Q14.
city286f_sin_phase:
        push    bx
        push    cx
        push    dx
        push    si
        mov     cx, ax
        mov     al, ch
        call    city286_get_sin
        mov     si, ax
        mov     al, ch
        inc     al
        call    city286_get_sin
        sub     ax, si
        xor     bx, bx
        mov     bl, cl
        call    city286_mul_q8
        add     ax, si
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

city286f_cos_phase:
        add     ax, (64 << 8)
        jmp     city286f_sin_phase

; Use the original edition's 8.8 angular phases without quantising them to
; whole table entries.  This is used by the park/bridge hand-off, where a
; one-degree truncation is visible as a scene seam.
city286f_update_matrix_phase:
        push    ax
        mov     ax, [city_cam_yaw_phase_target]
        mov     [city286f_t0x], ax
        mov     [city286_yaw], ah
        call    city286f_sin_phase
        mov     [city286_sin_yaw], ax
        mov     ax, [city286f_t0x]
        call    city286f_cos_phase
        mov     [city286_cos_yaw], ax

        mov     ax, [city_cam_pitch_phase_target]
        mov     [city286f_t0x], ax
        mov     [city286_pitch], ah
        call    city286f_sin_phase
        mov     [city286_sin_pitch], ax
        mov     ax, [city286f_t0x]
        call    city286f_cos_phase
        mov     [city286_cos_pitch], ax

        mov     ax, [city_cam_roll_phase_target]
        mov     [city286f_t0x], ax
        mov     [city286_roll], ah
        call    city286f_sin_phase
        mov     [city286_sin_roll], ax
        mov     ax, [city286f_t0x]
        call    city286f_cos_phase
        mov     [city286_cos_roll], ax
        pop     ax
        ret

; Build one signed horizontal displacement for every logical raster row.
; This is deliberately 80286-safe: the table uses only 16-bit arithmetic and
; signed-byte entries.  It is regenerated once per frame and then reused by all
; water pixels, avoiding sine/multiply work in the inner raster loop.
city286f_water_raster_prepare_table:
%if CITY_WATER_RASTER_DIAGNOSTIC_ZERO_SHIFT
        ret
%endif
        pusha
        mov     ax, [city_global_frame]
        cmp     ax, [city_water_raster_table_frame]
        je      .done
        mov     [city_water_raster_table_frame], ax
        xor     si, si
.loop:
        mov     ax, si
        imul    ax, ax, CITY_WATER_RASTER_Y_PHASE_MUL
        sar     ax, CITY_WATER_RASTER_Y_DIV_SHIFT
        mov     cx, [city_global_frame]
        imul    cx, cx, CITY_WATER_RASTER_FRAME_STEP
        add     ax, cx
        mov     bx, CITY_WATER_RASTER_AMPLITUDE
        call    city286_get_sin_scaled
        mov     [city_water_raster_shift_table + si], al
        inc     si
        cmp     si, SCREEN_H
        jb      .loop
.done:
        popa
        ret

; BX=logical raster Y -> AX=signed X displacement.
city286f_water_raster_get_shift:
%if CITY_WATER_RASTER_DIAGNOSTIC_ZERO_SHIFT
        xor     ax, ax
        ret
%endif
        push    si
        cmp     bx, 0
        jl      .zero
        cmp     bx, SCREEN_H
        jge     .zero
        mov     si, bx
        mov     al, [city_water_raster_shift_table + si]
        cbw
        pop     si
        ret
.zero:
        xor     ax, ax
        pop     si
        ret

; BX=unwarped logical raster Y, DX=camera-space depth Z
; -> AX=signed Y displacement for transverse water rows.  80286-safe.
; Match perspective by attenuating the apparent wave height approximately as
; 1/Z beyond CITY_WATER_CROSS_Y_DEPTH_REFERENCE.
city286f_water_cross_get_y_shift:
%if CITY_WATER_CROSS_Y_WAVE_ENABLE
        push    bx
        push    cx
        push    dx
        push    si

        mov     ax, bx
        imul    ax, ax, CITY_WATER_CROSS_Y_PHASE_MUL
        mov     cx, [city_global_frame]
        imul    cx, cx, CITY_WATER_CROSS_Y_FRAME_STEP
        add     ax, cx
        add     ax, CITY_WATER_CROSS_Y_PHASE_BIAS
        mov     si, ax

        mov     cx, dx
        or      cx, cx
        jle     .zero
        cmp     cx, CITY_WATER_CROSS_Y_DEPTH_REFERENCE
        jbe     .full_amplitude
        mov     ax, (CITY_WATER_CROSS_Y_AMPLITUDE * CITY_WATER_CROSS_Y_DEPTH_REFERENCE)
        xor     dx, dx
        div     cx
        mov     bx, ax
        jmp     .amplitude_ready
.full_amplitude:
        mov     bx, CITY_WATER_CROSS_Y_AMPLITUDE
.amplitude_ready:
        or      bx, bx
        jz      .zero
        mov     ax, si
        call    city286_get_sin_scaled
        jmp     .done
.zero:
        xor     ax, ax
.done:
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
%else
        xor     ax, ax
        ret
%endif

; 80286 counterpart of the Y-only longitudinal-water clip.  X is deliberately
; left unclipped until after the raster displacement, otherwise clipping the
; unwarped line to x=0/639 moves its base position before the wave is applied.
; Operates on city_water_warp_{x0,y0,x1,y1}; CF=1 if vertically invisible.
city286f_water_raster_clip_y_only:
        pusha
        mov     ax, [city_water_warp_y0]
        mov     bx, [city_water_warp_y1]
        cmp     ax, 0
        jge     .not_both_top
        cmp     bx, 0
        jl      .reject
.not_both_top:
        cmp     ax, SCREEN_H-1
        jle     .not_both_bottom
        cmp     bx, SCREEN_H-1
        jg      .reject
.not_both_bottom:

        mov     ax, [city_water_warp_y0]
        cmp     ax, 0
        jl      .clip0_top
        cmp     ax, SCREEN_H-1
        jg      .clip0_bottom
        jmp     .clip1_test
.clip0_top:
        xor     si, si
        jmp     .clip0
.clip0_bottom:
        mov     si, SCREEN_H-1
.clip0:
        mov     ax, [city_water_warp_x1]
        sub     ax, [city_water_warp_x0]
        mov     bx, si
        sub     bx, [city_water_warp_y0]
        mov     cx, [city_water_warp_y1]
        sub     cx, [city_water_warp_y0]
        call    city286f_muldiv_s16
        add     ax, [city_water_warp_x0]
        mov     [city_water_warp_x0], ax
        mov     [city_water_warp_y0], si

.clip1_test:
        mov     ax, [city_water_warp_y1]
        cmp     ax, 0
        jl      .clip1_top
        cmp     ax, SCREEN_H-1
        jg      .clip1_bottom
        jmp     .accept
.clip1_top:
        xor     si, si
        jmp     .clip1
.clip1_bottom:
        mov     si, SCREEN_H-1
.clip1:
        mov     ax, [city_water_warp_x0]
        sub     ax, [city_water_warp_x1]
        mov     bx, si
        sub     bx, [city_water_warp_y1]
        mov     cx, [city_water_warp_y0]
        sub     cx, [city_water_warp_y1]
        call    city286f_muldiv_s16
        add     ax, [city_water_warp_x1]
        mov     [city_water_warp_x1], ax
        mov     [city_water_warp_y1], si
.accept:
        popa
        clc
        ret
.reject:
        popa
        stc
        ret

; AX=x0, BX=y0, CX=x1, DX=y1 in unwarped projected screen coordinates.
; Horizontal constant-Z lines take one common shift.  Longitudinal lines are
; rasterized once and every emitted pixel receives the X shift for its logical
; raster row, so the wave is continuous without world-Z chord subdivision.
city286f_water_raster_line_set:
        pusha
        call    city286f_water_raster_prepare_table
        mov     [city286f_sx0], ax
        mov     [city286f_sy0], bx
        mov     [city286f_sx1], cx
        mov     [city286f_sy1], dx

        cmp     bx, dx
        jne     .curve
        call    city286f_water_raster_get_shift
        add     [city286f_sx0], ax
        add     [city286f_sx1], ax
        mov     dx, [city286f_t1z]
        call    city286f_water_cross_get_y_shift
        add     [city286f_sy0], ax
        add     [city286f_sy1], ax
        call    city286f_clip_screen
        jc      .done
        mov     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        mov     cx, [city286f_sx1]
        mov     dx, [city286f_sy1]
        call    line_batch_begin
        call    line_set_same_colour
        jmp     .done

.curve:
        mov     ax, [city286f_sx0]
        mov     [city_water_warp_x0], ax
        mov     ax, [city286f_sy0]
        mov     [city_water_warp_y0], ax
        mov     ax, [city286f_sx1]
        mov     [city_water_warp_x1], ax
        mov     ax, [city286f_sy1]
        mov     [city_water_warp_y1], ax
        call    city286f_water_raster_clip_y_only
        jc      .done

        mov     ax, [city_water_warp_x1]
        sub     ax, [city_water_warp_x0]
        jns     .dx_positive
        neg     ax
        mov     word [city_water_warp_sx], -1
        jmp     .dx_ready
.dx_positive:
        mov     word [city_water_warp_sx], 1
.dx_ready:
        mov     [city_water_warp_dx], ax

        mov     ax, [city_water_warp_y1]
        sub     ax, [city_water_warp_y0]
        jns     .dy_positive
        neg     ax
        mov     word [city_water_warp_sy], -1
        jmp     .dy_abs_ready
.dy_positive:
        mov     word [city_water_warp_sy], 1
.dy_abs_ready:
        neg     ax
        mov     [city_water_warp_dy], ax
        mov     ax, [city_water_warp_dx]
        add     ax, [city_water_warp_dy]
        mov     [city_water_warp_err], ax

        call    line_batch_begin
.plot_loop:
        mov     bx, [city_water_warp_y0]
        call    city286f_water_raster_get_shift
        add     ax, [city_water_warp_x0]
        mov     bx, [city_water_warp_y0]
        call    pixel_set_same_colour_fast

        mov     ax, [city_water_warp_x0]
        cmp     ax, [city_water_warp_x1]
        jne     .advance
        mov     ax, [city_water_warp_y0]
        cmp     ax, [city_water_warp_y1]
        je      .done
.advance:
        mov     ax, [city_water_warp_err]
        add     ax, ax
        mov     [city_water_warp_e2], ax
        cmp     ax, [city_water_warp_dy]
        jl      .skip_x
        mov     ax, [city_water_warp_err]
        add     ax, [city_water_warp_dy]
        mov     [city_water_warp_err], ax
        mov     ax, [city_water_warp_sx]
        add     [city_water_warp_x0], ax
.skip_x:
        mov     ax, [city_water_warp_e2]
        cmp     ax, [city_water_warp_dx]
        jg      .plot_loop
        mov     ax, [city_water_warp_err]
        add     ax, [city_water_warp_dx]
        mov     [city_water_warp_err], ax
        mov     ax, [city_water_warp_sy]
        add     [city_water_warp_y0], ax
        jmp     .plot_loop
.done:
        popa
        ret

; CX=model/world Z before faithful scene/model biases.  As on the 386 path,
; transverse water rows are intrinsically horizontal in the bridge view.
; Project only the two road-mask edges and extend the visible parts directly
; to x=0 and x=SCREEN_W-1.  This removes the large +/-6400 endpoint transforms
; and keeps the road gap stationary instead of applying a redundant X shift.
city286f_draw_water_cross_screenwide:
        pusha
        mov     di, cx
        add     di, [city286f_scene_z_bias]
        add     di, [city286f_model_z_bias]

        mov     ax, -CITY_WATER_ROAD_OCCLUDE_HALF
        mov     bx, CITY_WATER_Y
        mov     cx, di
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        je      .done
        mov     si, [city286_project_x]
        mov     bp, [city286_project_y]

        mov     ax, CITY_WATER_ROAD_OCCLUDE_HALF
        mov     bx, CITY_WATER_Y
        mov     cx, di
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        je      .done
        mov     di, [city286_project_x]

        ; Apply the perspective-aware vertical wave to the transverse water raster row.
        mov     bx, bp
        mov     dx, [city286_project_z]
        call    city286f_water_cross_get_y_shift
        add     bp, ax

        cmp     bp, 0
        jl      .done
        cmp     bp, SCREEN_H-1
        jg      .done
        cmp     si, di
        jle     .gap_sorted
        xchg    si, di
.gap_sorted:
        call    line_batch_begin

        cmp     si, 0
        jl      .right_span
        mov     cx, si
        cmp     cx, SCREEN_W-1
        jle     .left_x_ready
        mov     cx, SCREEN_W-1
.left_x_ready:
        xor     ax, ax
        mov     bx, bp
        mov     dx, bp
        call    line_set_same_colour

.right_span:
        cmp     di, SCREEN_W-1
        jg      .done
        mov     ax, di
        cmp     ax, 0
        jge     .right_x_ready
        xor     ax, ax
.right_x_ready:
        mov     bx, bp
        mov     cx, SCREEN_W-1
        mov     dx, bp
        call    line_set_same_colour
.done:
        popa
        ret

; --------------------------------------------------------------------------
; Robust 286 line projection / clipping
; --------------------------------------------------------------------------
; Clamp AX to a safe logical screen-intermediate range so all later 16-bit
; Cohen-Sutherland deltas remain representable.
city286f_clamp_screen_coord:
        cmp     ax, -8192
        jge     .low_ok
        mov     ax, -8192
.low_ok:
        cmp     ax, 8191
        jle     .done
        mov     ax, 8191
.done:
        ret

; Project already camera-rotated T0/T1 point. SI selects 0 or 1.
city286f_project_t0:
        mov     ax, [city286f_t0x]
        mov     bx, [city286f_t0z]
        call    city286_project_component_safe
        add     ax, CITY_SCREEN_CX
        call    city286f_clamp_screen_coord
        mov     [city286f_sx0], ax
        mov     ax, [city286f_t0y]
        mov     bx, [city286f_t0z]
        call    city286_project_component_safe
        neg     ax
        add     ax, CITY_SCREEN_CY
        call    city286f_clamp_screen_coord
        mov     [city286f_sy0], ax
        ret

city286f_project_t1:
        mov     ax, [city286f_t1x]
        mov     bx, [city286f_t1z]
        call    city286_project_component_safe
        add     ax, CITY_SCREEN_CX
        call    city286f_clamp_screen_coord
        mov     [city286f_sx1], ax
        mov     ax, [city286f_t1y]
        mov     bx, [city286f_t1z]
        call    city286_project_component_safe
        neg     ax
        add     ax, CITY_SCREEN_CY
        call    city286f_clamp_screen_coord
        mov     [city286f_sy1], ax
        ret

; AX=depth boundary, clip transformed endpoint 0 toward endpoint 1.
city286f_clip_t0_depth:
        mov     [city_clip_temp_z], ax
        mov     cx, [city286f_t1z]
        sub     cx, [city286f_t0z]       ; denominator
        mov     bx, [city_clip_temp_z]
        sub     bx, [city286f_t0z]       ; numerator
        mov     [city286f_lerp_c], bx
        mov     [city286f_lerp_d], cx
        mov     ax, [city286f_t1x]
        sub     ax, [city286f_t0x]
        mov     bx, [city286f_lerp_c]
        mov     cx, [city286f_lerp_d]
        call    city286f_muldiv_s16
        add     [city286f_t0x], ax
        mov     ax, [city286f_t1y]
        sub     ax, [city286f_t0y]
        mov     bx, [city286f_lerp_c]
        mov     cx, [city286f_lerp_d]
        call    city286f_muldiv_s16
        add     [city286f_t0y], ax
        mov     ax, [city_clip_temp_z]
        mov     [city286f_t0z], ax
        ret

; AX=depth boundary, clip transformed endpoint 1 toward endpoint 0.
city286f_clip_t1_depth:
        mov     [city_clip_temp_z], ax
        mov     cx, [city286f_t0z]
        sub     cx, [city286f_t1z]
        mov     bx, [city_clip_temp_z]
        sub     bx, [city286f_t1z]
        mov     [city286f_lerp_c], bx
        mov     [city286f_lerp_d], cx
        mov     ax, [city286f_t0x]
        sub     ax, [city286f_t1x]
        mov     bx, [city286f_lerp_c]
        mov     cx, [city286f_lerp_d]
        call    city286f_muldiv_s16
        add     [city286f_t1x], ax
        mov     ax, [city286f_t0y]
        sub     ax, [city286f_t1y]
        mov     bx, [city286f_lerp_c]
        mov     cx, [city286f_lerp_d]
        call    city286f_muldiv_s16
        add     [city286f_t1y], ax
        mov     ax, [city_clip_temp_z]
        mov     [city286f_t1z], ax
        ret

city286f_screen_outcode:
        ; AX=x, BX=y -> AL code
        push    dx
        xor     dl, dl
        cmp     ax, 0
        jge     .not_left
        or      dl, 1
.not_left:
        cmp     ax, SCREEN_W-1
        jle     .not_right
        or      dl, 2
.not_right:
        cmp     bx, 0
        jge     .not_top
        or      dl, 4
.not_top:
        cmp     bx, SCREEN_H-1
        jle     .done
        or      dl, 8
.done:
        mov     al, dl
        pop     dx
        ret

; Clip city286f_sx0/sy0/sx1/sy1 to logical 640x400. CF=1 rejected.
city286f_clip_screen:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     si, 8
.loop:
        mov     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        call    city286f_screen_outcode
        mov     [city286f_out0], al
        mov     ax, [city286f_sx1]
        mov     bx, [city286f_sy1]
        call    city286f_screen_outcode
        mov     [city286f_out1], al
        mov     al, [city286f_out0]
        mov     ah, [city286f_out1]
        mov     dl, al
        or      dl, ah
        jnz     short .__jcc286_skip_001
        jmp     near .accept
.__jcc286_skip_001:
        mov     dl, al
        and     dl, ah
        jz      short .__jcc286_skip_002
        jmp     near .reject
.__jcc286_skip_002:
        cmp     byte [city286f_out0], 0
        jne     .use0
        mov     al, [city286f_out1]
        mov     byte [city_clip_endpoint], 1
        jmp     .code
.use0:
        mov     al, [city286f_out0]
        mov     byte [city_clip_endpoint], 0
.code:
        test    al, 4
        jz      short .__jcc286_skip_003
        jmp     near .top
.__jcc286_skip_003:
        test    al, 8
        jz      short .__jcc286_skip_004
        jmp     near .bottom
.__jcc286_skip_004:
        test    al, 2
        jz      short .__jcc286_skip_072
        jmp     near .right
.__jcc286_skip_072:
        ; left x=0
        mov     ax, [city286f_sy1]
        sub     ax, [city286f_sy0]
        mov     bx, [city286f_sx0]
        neg     bx
        mov     cx, [city286f_sx1]
        sub     cx, [city286f_sx0]
        call    city286f_muldiv_s16
        add     ax, [city286f_sy0]
        mov     [city_clip_temp_y], ax
        mov     word [city_clip_temp_x], 0
        jmp     .store
.right:
        mov     ax, [city286f_sy1]
        sub     ax, [city286f_sy0]
        mov     bx, SCREEN_W-1
        sub     bx, [city286f_sx0]
        mov     cx, [city286f_sx1]
        sub     cx, [city286f_sx0]
        call    city286f_muldiv_s16
        add     ax, [city286f_sy0]
        mov     [city_clip_temp_y], ax
        mov     word [city_clip_temp_x], SCREEN_W-1
        jmp     .store
.top:
        mov     ax, [city286f_sx1]
        sub     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        neg     bx
        mov     cx, [city286f_sy1]
        sub     cx, [city286f_sy0]
        call    city286f_muldiv_s16
        add     ax, [city286f_sx0]
        mov     [city_clip_temp_x], ax
        mov     word [city_clip_temp_y], 0
        jmp     .store
.bottom:
        mov     ax, [city286f_sx1]
        sub     ax, [city286f_sx0]
        mov     bx, SCREEN_H-1
        sub     bx, [city286f_sy0]
        mov     cx, [city286f_sy1]
        sub     cx, [city286f_sy0]
        call    city286f_muldiv_s16
        add     ax, [city286f_sx0]
        mov     [city_clip_temp_x], ax
        mov     word [city_clip_temp_y], SCREEN_H-1
.store:
        cmp     byte [city_clip_endpoint], 0
        jne     .store1
        mov     ax, [city_clip_temp_x]
        mov     [city286f_sx0], ax
        mov     ax, [city_clip_temp_y]
        mov     [city286f_sy0], ax
        jmp     .next
.store1:
        mov     ax, [city_clip_temp_x]
        mov     [city286f_sx1], ax
        mov     ax, [city_clip_temp_y]
        mov     [city286f_sy1], ax
.next:
        dec     si
        jz      short .__jcc286_skip_005
        jmp     near .loop
.__jcc286_skip_005:
.reject:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        stc
        ret
.accept:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        clc
        ret

; AX/BX/CX -> DX/SI/DI world line, in original NEON3 coordinates.
; Scene/model Z bias is applied before projection.
city286f_draw_world_line:
        pusha
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        add     di, [city286f_scene_z_bias]
        add     di, [city286f_model_z_bias]

        ; Transform endpoint 0. city286_project_point leaves TX/TY/TZ even when
        ; its own visibility test rejects the point.
        call    city286_project_point
        mov     ax, [city286_tx]
        mov     [city286f_t0x], ax
        mov     ax, [city286_ty]
        mov     [city286f_t0y], ax
        mov     ax, [city286_tz]
        mov     [city286f_t0z], ax

        mov     ax, dx
        mov     bx, si
        mov     cx, di
        call    city286_project_point
        mov     ax, [city286_tx]
        mov     [city286f_t1x], ax
        mov     ax, [city286_ty]
        mov     [city286f_t1y], ax
        mov     ax, [city286_tz]
        mov     [city286f_t1z], ax

        ; Near clip.
        mov     ax, [city286f_t0z]
        cmp     ax, CITY_NEAR_CLIP_Z
        jge     .near0_ok
        mov     ax, [city286f_t1z]
        cmp     ax, CITY_NEAR_CLIP_Z
        jge     short .__jcc286_skip_006
        jmp     near .reject
.__jcc286_skip_006:
        mov     ax, CITY_NEAR_CLIP_Z
        call    city286f_clip_t0_depth
.near0_ok:
        mov     ax, [city286f_t1z]
        cmp     ax, CITY_NEAR_CLIP_Z
        jge     .far
        mov     ax, CITY_NEAR_CLIP_Z
        call    city286f_clip_t1_depth
.far:
        mov     ax, [city286f_t0z]
        cmp     ax, CITY_FAR_CLIP_Z
        jle     .far0_ok
        mov     ax, [city286f_t1z]
        cmp     ax, CITY_FAR_CLIP_Z
        jle     short .__jcc286_skip_007
        jmp     near .reject
.__jcc286_skip_007:
        mov     ax, CITY_FAR_CLIP_Z
        call    city286f_clip_t0_depth
.far0_ok:
        mov     ax, [city286f_t1z]
        cmp     ax, CITY_FAR_CLIP_Z
        jle     .project
        mov     ax, CITY_FAR_CLIP_Z
        call    city286f_clip_t1_depth
.project:
        call    city286f_project_t0
        call    city286f_project_t1
        cmp     byte [city_water_raster_active], 0
        je      .ordinary_line
        mov     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        mov     cx, [city286f_sx1]
        mov     dx, [city286f_sy1]
        call    city286f_water_raster_line_set
        jmp     .reject
.ordinary_line:
        call    city286f_clip_screen
        jc      .reject
        mov     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        mov     cx, [city286f_sx1]
        mov     dx, [city286f_sy1]
        call    line_set
.reject:
        popa
        ret

; Segment a long constant-X/Z world line exactly as the 256 edition does.
; AX=X, BX=Y, CX=z0, DI=z1.
city286f_draw_world_line_z_chunks:
        pusha
        mov     [city_ground_x], ax
        mov     [city_ground_y], bx
        mov     [city_ground_cursor_z], cx
        mov     [city_ground_z1], di
        cmp     cx, di
        jl      .forward
        jle     short .__jcc286_skip_008
        jmp     near .backward
.__jcc286_skip_008:
        jmp     .done
.forward:
        mov     cx, [city_ground_cursor_z]
        cmp     cx, [city_ground_z1]
        jl      short .__jcc286_skip_009
        jmp     near .done
.__jcc286_skip_009:
        mov     di, cx
        add     di, CITY_GROUND_SEGMENT_Z
        cmp     di, [city_ground_z1]
        jle     .forward_end
        mov     di, [city_ground_z1]
.forward_end:
        mov     ax, [city_ground_x]
        mov     bx, [city_ground_y]
        mov     dx, ax
        mov     si, bx
        call    city286f_draw_world_line
        mov     [city_ground_cursor_z], di
        jmp     near .forward
.backward:
        mov     cx, [city_ground_cursor_z]
        cmp     cx, [city_ground_z1]
        jg      short .__jcc286_skip_073
        jmp     near .done
.__jcc286_skip_073:
        mov     di, cx
        sub     di, CITY_GROUND_SEGMENT_Z
        cmp     di, [city_ground_z1]
        jge     .backward_end
        mov     di, [city_ground_z1]
.backward_end:
        mov     ax, [city_ground_x]
        mov     bx, [city_ground_y]
        mov     dx, ax
        mov     si, bx
        call    city286f_draw_world_line
        mov     [city_ground_cursor_z], di
        jmp     near .backward
.done:
        popa
        ret

; --------------------------------------------------------------------------
; Faithful box/frame/tree primitives (same object semantics as CITY3D256)
; --------------------------------------------------------------------------
; BL=vertex 0..7 -> AX/BX/CX world position, before Z bias.
; --------------------------------------------------------------------------
; 80286 scanline triangle rasterizer
; --------------------------------------------------------------------------
; AX=x0, BX=x1, CX=y0, DX=y1, SI=query y -> AX=x(query y).
; Caller only requests Y inside the edge, so the signed quotient cannot exceed
; the original signed X delta and is safe for 16-bit IDIV.
city286f_edge_x_at_y:
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     di, ax
        sub     bx, ax                  ; dx
        sub     si, cx                  ; y-y0
        sub     dx, cx                  ; dy
        mov     ax, bx
        mov     bx, si
        mov     cx, dx
        call    city286f_muldiv_s16
        add     ax, di
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; Fill tri_x0/y0, tri_x1/y1, tri_x2/y2 with current draw_color.  This is a
; deliberately division-heavy fidelity path; the 286 edition now prioritises
; identical geometry over speed.  Horizontal spans are clipped by VIDEO3_286.
city286f_fill_triangle:
        pusha
        ; sort 0/1
        mov     ax, [tri_y0]
        cmp     ax, [tri_y1]
        jle     .sort12
        mov     bx, [tri_y1]
        mov     [tri_y0], bx
        mov     [tri_y1], ax
        mov     ax, [tri_x0]
        mov     bx, [tri_x1]
        mov     [tri_x0], bx
        mov     [tri_x1], ax
.sort12:
        mov     ax, [tri_y1]
        cmp     ax, [tri_y2]
        jle     .sort01again
        mov     bx, [tri_y2]
        mov     [tri_y1], bx
        mov     [tri_y2], ax
        mov     ax, [tri_x1]
        mov     bx, [tri_x2]
        mov     [tri_x1], bx
        mov     [tri_x2], ax
.sort01again:
        mov     ax, [tri_y0]
        cmp     ax, [tri_y1]
        jle     .sorted
        mov     bx, [tri_y1]
        mov     [tri_y0], bx
        mov     [tri_y1], ax
        mov     ax, [tri_x0]
        mov     bx, [tri_x1]
        mov     [tri_x0], bx
        mov     [tri_x1], ax
.sorted:
        cmp     word [tri_y2], 0
        jge     short .__jcc286_skip_010
        jmp     near .done
.__jcc286_skip_010:
        cmp     word [tri_y0], SCREEN_H-1
        jle     short .__jcc286_skip_011
        jmp     near .done
.__jcc286_skip_011:
        mov     ax, [tri_y2]
        sub     ax, [tri_y0]
        jnz     short .__jcc286_skip_012
        jmp     near .done
.__jcc286_skip_012:
        mov     ax, [tri_y0]
        cmp     ax, 0
        jge     .ystart_ok
        xor     ax, ax
.ystart_ok:
        mov     [city286f_tri_y], ax
        mov     dx, [tri_y2]
        cmp     dx, SCREEN_H-1
        jle     .yend_ok
        mov     dx, SCREEN_H-1
.yend_ok:
        mov     bp, dx                  ; inclusive end y
        call    line_batch_begin
.scan:
        mov     si, [city286f_tri_y]
        ; Long edge V0->V2.
        mov     ax, [tri_x0]
        mov     bx, [tri_x2]
        mov     cx, [tri_y0]
        mov     dx, [tri_y2]
        call    city286f_edge_x_at_y
        mov     [city286f_tri_xa], ax

        mov     si, [city286f_tri_y]
        cmp     si, [tri_y1]
        jg      short .__jcc286_skip_074
        jmp     near .upper
.__jcc286_skip_074:
        ; Lower edge V1->V2; skip a zero-height edge at the bottom.
        mov     ax, [tri_y2]
        cmp     ax, [tri_y1]
        jne     short .__jcc286_skip_013
        jmp     near .done_scanline
.__jcc286_skip_013:
        mov     ax, [tri_x1]
        mov     bx, [tri_x2]
        mov     cx, [tri_y1]
        mov     dx, [tri_y2]
        call    city286f_edge_x_at_y
        jmp     .short_ready
.upper:
        mov     ax, [tri_y1]
        cmp     ax, [tri_y0]
        je      .lower_fallback
        mov     ax, [tri_x0]
        mov     bx, [tri_x1]
        mov     cx, [tri_y0]
        mov     dx, [tri_y1]
        call    city286f_edge_x_at_y
        jmp     .short_ready
.lower_fallback:
        mov     ax, [tri_x1]
        mov     bx, [tri_x2]
        mov     cx, [tri_y1]
        mov     dx, [tri_y2]
        cmp     dx, cx
        je      .done_scanline
        call    city286f_edge_x_at_y
.short_ready:
        mov     [city286f_tri_xb], ax
        mov     ax, [city286f_tri_xa]
        mov     cx, [city286f_tri_xb]
        mov     bx, [city286f_tri_y]
        call    hline_set_same_colour
.done_scanline:
        mov     ax, [city286f_tri_y]
        cmp     ax, bp
        jge     .done
        inc     word [city286f_tri_y]
        jmp     .scan
.done:
        popa
        ret

city286f_box_get_vertex:
        push    dx
        mov     dl, bl
        mov     ax, [city_obj_x]
        test    dl, 1
        jz      .x
        add     ax, [city_obj_w]
.x:
        mov     bx, [city_obj_y]
        test    dl, 4
        jz      .y
        add     bx, [city_obj_h]
.y:
        mov     cx, [city_obj_z]
        test    dl, 2
        jz      .z
        add     cx, [city_obj_d]
        add     ax, [city_obj_skew_dx]
.z:
        pop     dx
        ret

city286f_draw_box_wire:
        pusha
        mov     ax, [city_obj_d]
        mov     bx, [city_obj_skew_q8]
        call    city286_mul_q8
        mov     [city_obj_skew_dx], ax
        xor     si, si
        mov     bp, 12
.edge:
        mov     bl, [city_box_edges + si]
        call    city286f_box_get_vertex
        mov     [city_line_x0], ax
        mov     [city_line_y0], bx
        mov     [city_line_z0], cx
        mov     bl, [city_box_edges + si + 1]
        call    city286f_box_get_vertex
        mov     dx, ax
        mov     si, bx
        mov     di, cx
        push    si
        mov     ax, [city_line_x0]
        mov     bx, [city_line_y0]
        mov     cx, [city_line_z0]
        pop     si
        call    city286f_draw_world_line
        ; Restore edge-table cursor from the loop counter stored separately.
        ; SI was endpoint Y, so rebuild it from remaining edge count.
        mov     ax, 12
        sub     ax, bp
        shl     ax, 1
        add     ax, 2
        mov     si, ax
        dec     bp
        jz      short .__jcc286_skip_014
        jmp     near .edge
.__jcc286_skip_014:

        test    byte [city_obj_flags], 1
        jz      .no_band
        call    city286f_draw_box_mid_band
.no_band:
        test    byte [city_obj_flags], 2
        jnz     short .__jcc286_skip_075
        jmp     near .done
.__jcc286_skip_075:
        mov     ax, [city_obj_w]
        sar     ax, 1
        add     ax, [city_obj_x]
        mov     dx, [city_obj_skew_dx]
        sar     dx, 1
        add     ax, dx
        mov     dx, ax
        mov     bx, [city_obj_y]
        add     bx, [city_obj_h]
        mov     si, bx
        add     si, [city_obj_antenna]
        mov     cx, [city_obj_d]
        sar     cx, 1
        add     cx, [city_obj_z]
        mov     di, cx
        call    city286f_draw_world_line
.done:
        popa
        ret

city286f_draw_box_mid_band:
        pusha
        mov     bx, [city_obj_h]
        sar     bx, 1
        add     bx, [city_obj_y]
        mov     [city_band_y], bx
        mov     ax, [city_obj_x]
        mov     cx, [city_obj_z]
        mov     dx, ax
        add     dx, [city_obj_w]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_obj_x]
        add     ax, [city_obj_w]
        mov     cx, [city_obj_z]
        mov     dx, ax
        add     dx, [city_obj_skew_dx]
        mov     si, bx
        mov     di, cx
        add     di, [city_obj_d]
        call    city286f_draw_world_line
        mov     ax, [city_obj_x]
        add     ax, [city_obj_w]
        add     ax, [city_obj_skew_dx]
        mov     cx, [city_obj_z]
        add     cx, [city_obj_d]
        mov     dx, [city_obj_x]
        add     dx, [city_obj_skew_dx]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_obj_x]
        add     ax, [city_obj_skew_dx]
        mov     cx, [city_obj_z]
        add     cx, [city_obj_d]
        mov     dx, [city_obj_x]
        mov     si, bx
        mov     di, [city_obj_z]
        call    city286f_draw_world_line
        popa
        ret

city286f_draw_frame:
        pusha
        mov     ax, [city_panel_x0]
        mov     bx, [city_panel_y0]
        mov     cx, [city_panel_z]
        mov     dx, [city_panel_x1]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_panel_x1]
        mov     bx, [city_panel_y0]
        mov     cx, [city_panel_z]
        mov     dx, ax
        mov     si, [city_panel_y1]
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_panel_x1]
        mov     bx, [city_panel_y1]
        mov     cx, [city_panel_z]
        mov     dx, [city_panel_x0]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_panel_x0]
        mov     bx, [city_panel_y1]
        mov     cx, [city_panel_z]
        mov     dx, ax
        mov     si, [city_panel_y0]
        mov     di, cx
        call    city286f_draw_world_line
        popa
        ret

city286f_draw_panel:
        call    city286f_draw_frame
        pusha
        mov     ax, [city_panel_x0]
        mov     bx, [city_panel_y0]
        mov     cx, [city_panel_z]
        mov     dx, [city_panel_x1]
        mov     si, [city_panel_y1]
        mov     di, cx
        call    city286f_draw_world_line
        popa
        ret

; Exact NEON3 tree: 32x32x90 trunk plus 136x136 pyramid crown with apex Y=235.
city286f_draw_tree:
        pusha
        mov     ax, [city_tree_x]
        sub     ax, 16
        mov     [city_obj_x], ax
        mov     word [city_obj_y], 0
        mov     ax, [city_tree_z]
        sub     ax, 16
        mov     [city_obj_z], ax
        mov     word [city_obj_w], 32
        mov     word [city_obj_d], 32
        mov     word [city_obj_h], 90
        mov     byte [city_obj_flags], 0
        mov     byte [draw_color], 2
        call    city286f_draw_box_wire
        mov     ax, [city_tree_x]
        sub     ax, 68
        mov     [city_tree_x0], ax
        mov     ax, [city_tree_x]
        add     ax, 68
        mov     [city_tree_x1], ax
        mov     ax, [city_tree_z]
        sub     ax, 68
        mov     [city_tree_z0], ax
        mov     ax, [city_tree_z]
        add     ax, 68
        mov     [city_tree_z1], ax
        mov     byte [draw_color], 5
        ; crown base
        mov     ax, [city_tree_x0]
        mov     bx, 72
        mov     cx, [city_tree_z0]
        mov     dx, [city_tree_x1]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_tree_x1]
        mov     cx, [city_tree_z0]
        mov     dx, ax
        mov     si, bx
        mov     di, [city_tree_z1]
        call    city286f_draw_world_line
        mov     ax, [city_tree_x1]
        mov     cx, [city_tree_z1]
        mov     dx, [city_tree_x0]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, [city_tree_x0]
        mov     cx, [city_tree_z1]
        mov     dx, ax
        mov     si, bx
        mov     di, [city_tree_z0]
        call    city286f_draw_world_line
        ; four sides to apex
        mov     dx, [city_tree_x]
        mov     si, 235
        mov     di, [city_tree_z]
        mov     ax, [city_tree_x0]
        mov     bx, 72
        mov     cx, [city_tree_z0]
        call    city286f_draw_world_line
        mov     ax, [city_tree_x1]
        mov     cx, [city_tree_z0]
        call    city286f_draw_world_line
        mov     ax, [city_tree_x1]
        mov     cx, [city_tree_z1]
        call    city286f_draw_world_line
        mov     ax, [city_tree_x0]
        mov     cx, [city_tree_z1]
        call    city286f_draw_world_line
        popa
        ret

; --------------------------------------------------------------------------
; Reusable exact NEON3 environment models
; --------------------------------------------------------------------------
city286f_phase_mod:
        xor     dx, dx
        div     bx
        mov     ax, dx
        ret

city286f_draw_road_grid:
        pusha
        mov     byte [draw_color], 1
        mov     word [city_grid_index], 0
        mov     bp, 5
.long:
        mov     si, [city_grid_index]
        mov     ax, [city_road_half]
        neg     ax
        push    ax
        mov     ax, [city_road_half]
        sar     ax, 1
        imul    si
        mov     dx, ax
        pop     ax
        add     ax, dx
        mov     bx, [city_road_y]
        mov     cx, CITY_NEAR_Z+8
        mov     di, [city_road_far]
        call    city286f_draw_world_line_z_chunks
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_076
        jmp     near .long
.__jcc286_skip_076:
        mov     word [city_grid_index], 0
        xor     bx, bx
        mov     bl, [city_road_cross_count]
        mov     bp, bx
.cross:
        mov     ax, [city_grid_index]
        imul    word [city_road_spacing]
        add     ax, CITY_NEAR_Z+48
        sub     ax, [city_road_phase]
        cmp     ax, CITY_NEAR_Z+8
        jge     .z
        add     ax, [city_road_cycle]
.z:
        mov     cx, ax
        mov     di, ax
        mov     ax, [city_road_half]
        neg     ax
        mov     dx, [city_road_half]
        mov     bx, [city_road_y]
        mov     si, bx
        call    city286f_draw_world_line
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_015
        jmp     near .cross
.__jcc286_skip_015:
        popa
        ret

city286f_draw_street_rows:
        pusha
        mov     ax, [city_row_skew_q8]
        mov     [city_obj_skew_q8], ax
        mov     ax, [city_row_phase]
        xor     dx, dx
        div     word [city_row_spacing]
        mov     [city_row_cell_base], ax
        mov     [city_row_remainder], dx
        mov     word [city_row_index], -2
        xor     bx, bx
        mov     bl, [city_row_count]
        add     bx, 2
        mov     bp, bx
.row:
        mov     si, [city_row_index]
        mov     ax, si
        imul    word [city_row_spacing]
        add     ax, [city_row_near]
        sub     ax, [city_row_remainder]
        mov     [city_obj_z], ax
        mov     ax, [city_row_z_limit]
        test    ax, ax
        jz      .range
        cmp     [city_obj_z], ax
        jl      short .__jcc286_skip_016
        jmp     near .next
.__jcc286_skip_016:
.range:
        mov     ax, [city_row_y]
        mov     [city_obj_y], ax
        mov     ax, [city_row_width]
        mov     [city_obj_w], ax
        mov     ax, [city_row_depth]
        mov     [city_obj_d], ax
        mov     bx, [city_row_cell_base]
        add     bx, si
        and     bx, 15
        mov     [city_row_identity], bx
        shl     bx, 1
        mov     ax, [city_height_table + bx]
        add     ax, [city_row_height_bias]
        mov     [city_obj_h], ax
        mov     bx, [city_row_identity]
        shl     bx, 1
        mov     dx, [city_jitter_table + bx]
        mov     word [city_row_skew_offset], 0
        cmp     word [city_row_skew_q8], 0
        je      .skew
        push    dx
        mov     ax, [city_obj_z]
        mov     bx, [city_row_skew_q8]
        call    city286_mul_q8
        mov     [city_row_skew_offset], ax
        pop     dx
.skew:
        mov     ax, [city_row_left_x]
        add     ax, [city_row_skew_offset]
        add     ax, dx
        mov     [city_obj_x], ax
        mov     al, [city_row_color_base]
        mov     bx, [city_row_identity]
        and     bl, 3
        add     al, bl
        cmp     al, 7
        jbe     .lc
        sub     al, 6
.lc:
        mov     [draw_color], al
        mov     byte [city_obj_flags], 0
        mov     bx, [city_row_identity]
        and     bx, 7
        cmp     bx, 3
        jae     .lf
        mov     byte [city_obj_flags], 1
        test    bl, 1
        jnz     .lf
        or      byte [city_obj_flags], 2
.lf:
        call    city286f_draw_box_wire
        mov     ax, [city_row_right_x]
        add     ax, [city_row_skew_offset]
        sub     ax, dx
        mov     [city_obj_x], ax
        mov     al, [city_row_color_base]
        mov     bx, [city_row_identity]
        inc     bx
        and     bl, 3
        add     al, bl
        cmp     al, 7
        jbe     .rc
        sub     al, 6
.rc:
        mov     [draw_color], al
        call    city286f_draw_box_wire
.next:
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_017
        jmp     near .row
.__jcc286_skip_017:
        mov     word [city_obj_skew_q8], 0
        popa
        ret

city286f_draw_flat_road:
        pusha
        mov     byte [draw_color], 1
        mov     word [city_grid_index], 0
        mov     bp, 5
.long:
        mov     ax, [city_transition_seg_half]
        neg     ax
        add     ax, [city_route_center_x]
        push    ax
        mov     ax, [city_transition_seg_half]
        sar     ax, 1
        imul    word [city_grid_index]
        mov     dx, ax
        pop     ax
        add     ax, dx
        mov     bx, [city_transition_seg_y]
        mov     cx, [city_transition_seg_z0]
        mov     di, [city_transition_seg_z1]
        call    city286f_draw_world_line_z_chunks
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_077
        jmp     near .long
.__jcc286_skip_077:
        mov     ax, [city_transition_seg_z0]
        mov     [city_transition_cursor_z], ax
.cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, [city_transition_seg_z1]
        jle     short .__jcc286_skip_078
        jmp     near .done
.__jcc286_skip_078:
        mov     di, cx
        mov     ax, [city_route_center_x]
        sub     ax, [city_transition_seg_half]
        mov     dx, [city_route_center_x]
        add     dx, [city_transition_seg_half]
        mov     bx, [city_transition_seg_y]
        mov     si, bx
        call    city286f_draw_world_line
        mov     ax, [city_transition_seg_spacing]
        add     [city_transition_cursor_z], ax
        jmp     .cross
.done:
        popa
        ret

; Canyon street lamps matching the 256/16 editions.  Vertical dimensions
; come from CITY_CANYON_LAMP_* in CONFIG3_286.INC.  The current top is 370
; world units (308 * 1.2, rounded), while the lamp body remains 8 units high.
; The small trapezoidal lamp face is filled and pulses between bright warm
; yellow and white; there is no light cone.
city286f_draw_canyon_lamps_static:
        pusha
        mov     byte [city_lamp_pulse_color], CITY_LAMP_STATIC_COLOR
        jmp     near city286f_draw_canyon_lamps_geometry

city286f_draw_canyon_lamps:
        pusha
        ; Five colour pulses per 64 logical frames (~12.8 frames/pulse).
        ; The phase LUT avoids MUL/DIV on the 286 path.
        mov     ax, [scene_frame]
        and     ax, 003Fh
        mov     si, ax
        mov     al, [city_lamp_phase_fast_table + si]
        xor     ah, ah
        mov     si, ax
        mov     al, [city_lamp_pulse_table + si]
        mov     [city_lamp_pulse_color], al
city286f_draw_canyon_lamps_geometry:

        mov     ax, [city_route_travel]
        mov     bx, 3840
        call    city286f_phase_mod
        mov     [city_road_phase], ax
        mov     word [city_grid_index], 0
        mov     bp, 4
.lamp_loop:
        mov     ax, [city_grid_index]
        mov     bx, 960
        mul     bx
        add     ax, CITY_NEAR_Z+192
        sub     ax, [city_road_phase]
        cmp     ax, CITY_NEAR_Z+48
        jge     .z_ready
        add     ax, 3840
.z_ready:
        mov     [city_grid_z], ax
        mov     dx, [city_row_z_limit]
        test    dx, dx
        jz      .draw_pair
        cmp     ax, dx
        jge     .next_pair
.draw_pair:
        mov     byte [draw_color], 7

        ; Left lamp.
        mov     ax, -330
        mov     bx, 0
        mov     cx, [city_grid_z]
        mov     dx, -330
        mov     si, CITY_CANYON_LAMP_POLE_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, -330
        mov     bx, CITY_CANYON_LAMP_POLE_Y
        mov     cx, [city_grid_z]
        mov     dx, -315
        mov     si, CITY_CANYON_LAMP_BEND_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, -315
        mov     bx, CITY_CANYON_LAMP_BEND_Y
        mov     cx, [city_grid_z]
        mov     dx, -282
        mov     si, CITY_CANYON_LAMP_TOP_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, -282
        mov     bx, CITY_CANYON_LAMP_TOP_Y
        mov     cx, [city_grid_z]
        mov     dx, -220
        mov     si, CITY_CANYON_LAMP_TOP_Y
        mov     di, cx
        call    city286f_draw_world_line
        xor     al, al
        call    city286f_fill_canyon_lamp_face

        ; Right mirror.
        mov     byte [draw_color], 7
        mov     ax, 330
        mov     bx, 0
        mov     cx, [city_grid_z]
        mov     dx, 330
        mov     si, CITY_CANYON_LAMP_POLE_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, 330
        mov     bx, CITY_CANYON_LAMP_POLE_Y
        mov     cx, [city_grid_z]
        mov     dx, 315
        mov     si, CITY_CANYON_LAMP_BEND_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, 315
        mov     bx, CITY_CANYON_LAMP_BEND_Y
        mov     cx, [city_grid_z]
        mov     dx, 282
        mov     si, CITY_CANYON_LAMP_TOP_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, 282
        mov     bx, CITY_CANYON_LAMP_TOP_Y
        mov     cx, [city_grid_z]
        mov     dx, 220
        mov     si, CITY_CANYON_LAMP_TOP_Y
        mov     di, cx
        call    city286f_draw_world_line
        mov     al, 1
        call    city286f_fill_canyon_lamp_face
.next_pair:
        inc     word [city_grid_index]
        dec     bp
        jz      .done
        jmp     near .lamp_loop
.done:
        popa
        ret

; AL=0 left, AL!=0 right.  Project the four lamp corners once and reuse the
; existing two-triangle 286 fill path.  Only the tiny 8-unit-high luminaire is
; rasterised, keeping the added cost bounded.
city286f_fill_canyon_lamp_face:
        pusha
        xor     ah, ah
        mov     bp, ax

        mov     ax, -220
        test    bp, bp
        jz      .p0_x
        neg     ax
.p0_x:
        mov     bx, CITY_CANYON_LAMP_TOP_Y
        mov     cx, [city_grid_z]
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .p0_ok
        jmp     near .done
.p0_ok:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y0], ax

        mov     ax, -184
        test    bp, bp
        jz      .p1_x
        neg     ax
.p1_x:
        mov     bx, CITY_CANYON_LAMP_TOP_Y
        mov     cx, [city_grid_z]
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .p1_ok
        jmp     near .done
.p1_ok:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x1], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y1], ax

        mov     ax, -190
        test    bp, bp
        jz      .p2_x
        neg     ax
.p2_x:
        mov     bx, CITY_CANYON_LAMP_BODY_BOTTOM_Y
        mov     cx, [city_grid_z]
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .p2_ok
        jmp     near .done
.p2_ok:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x2], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y2], ax

        mov     ax, -216
        test    bp, bp
        jz      .p3_x
        neg     ax
.p3_x:
        mov     bx, CITY_CANYON_LAMP_BODY_BOTTOM_Y
        mov     cx, [city_grid_z]
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .p3_ok
        jmp     near .done
.p3_ok:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x3], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y3], ax

        mov     al, [city_lamp_pulse_color]
        mov     [draw_color], al
        mov     ax, [city286f_screen_quad_x0]
        mov     [tri_x0], ax
        mov     ax, [city286f_screen_quad_y0]
        mov     [tri_y0], ax
        mov     ax, [city286f_screen_quad_x1]
        mov     [tri_x1], ax
        mov     ax, [city286f_screen_quad_y1]
        mov     [tri_y1], ax
        mov     ax, [city286f_screen_quad_x2]
        mov     [tri_x2], ax
        mov     ax, [city286f_screen_quad_y2]
        mov     [tri_y2], ax
        call    city286f_fill_triangle

        mov     ax, [city286f_screen_quad_x0]
        mov     [tri_x0], ax
        mov     ax, [city286f_screen_quad_y0]
        mov     [tri_y0], ax
        mov     ax, [city286f_screen_quad_x2]
        mov     [tri_x1], ax
        mov     ax, [city286f_screen_quad_y2]
        mov     [tri_y1], ax
        mov     ax, [city286f_screen_quad_x3]
        mov     [tri_x2], ax
        mov     ax, [city286f_screen_quad_y3]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
.done:
        popa
        ret

city286f_draw_bridge_fixed:
        pusha
        ; Preserve the original repeat phase, but in scene 2 explicitly reject
        ; bridge geometry already behind the near plane before projection.  This
        ; avoids late-scene re-entry/wrap artefacts without shifting tower/water
        ; spacing or changing the canyon->bridge transition.
        mov     ax, [city_transition_seg_z0]
        mov     [city_bridge_visible_z_min], ax
        cmp     byte [scene_index], 2
        jne     .visible_min_ready
        mov     ax, [city286_cam_z]
        add     ax, CITY_NEAR_CLIP_Z
        cmp     ax, [city_bridge_visible_z_min]
        jle     .visible_min_ready
        mov     [city_bridge_visible_z_min], ax
.visible_min_ready:
        mov     byte [draw_color], CITY_WATER_PALETTE_INDEX
        mov     byte [city_water_raster_active], 1
        mov     word [city_grid_index], 0
        mov     bp, CITY_WATER_GRID_LONG_X_COUNT
.river_long:
        mov     ax, [city_grid_index]
        mov     bx, CITY_WATER_GRID_X_STEP
        imul    bx
        add     ax, CITY_WATER_GRID_LONG_X_MIN
%if CITY_WATER_GRID_LONG_SKIP_CENTER
        ; Leave a three-grid-line opening below the bridge road: omit the
        ; centre longitudinal line and its immediate neighbours (X=-800/0/+800).
        cmp     ax, -CITY_WATER_GRID_LONG_SKIP_HALF
        jl      .river_long_draw
        cmp     ax, CITY_WATER_GRID_LONG_SKIP_HALF
        jle     .river_long_advance
.river_long_draw:
%endif
        mov     bx, CITY_WATER_Y
        mov     cx, [city_transition_seg_z0]
        add     cx, CITY_WATER_GRID_Z_START_OFFSET
        cmp     cx, [city_bridge_visible_z_min]
        jge     .river_long_z_ready
        mov     cx, [city_bridge_visible_z_min]
.river_long_z_ready:
        mov     di, [city_transition_seg_z1]
        call    city286f_draw_world_line_z_chunks
.river_long_advance:
        inc     word [city_grid_index]
        dec     bp
        jnz     .river_long
        mov     ax, [city_transition_seg_z0]
        add     ax, CITY_WATER_GRID_Z_START_OFFSET
        mov     [city_transition_cursor_z], ax
.river_cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, [city_transition_seg_z1]
        jg      .deck
        cmp     cx, [city_bridge_visible_z_min]
        jl      .river_cross_advance
%if CITY_WATER_GRID_CROSS_FULL_SCREEN
        call    city286f_draw_water_cross_screenwide
%else
        mov     di, cx
        ; Fallback/test path: finite transverse world-space segments.
        mov     ax, CITY_WATER_GRID_FULL_X_MIN
        mov     dx, -CITY_WATER_ROAD_OCCLUDE_HALF
        mov     bx, CITY_WATER_Y
        mov     si, bx
        call    city286f_draw_world_line
        mov     cx, [city_transition_cursor_z]
        mov     di, cx
        mov     ax, CITY_WATER_ROAD_OCCLUDE_HALF
        mov     dx, CITY_WATER_GRID_FULL_X_MAX
        mov     bx, CITY_WATER_Y
        mov     si, bx
        call    city286f_draw_world_line
%endif
.river_cross_advance:
        add     word [city_transition_cursor_z], CITY_WATER_GRID_Z_STEP
        jmp     .river_cross
.deck:
        mov     byte [city_water_raster_active], 0
        mov     byte [draw_color], 7
        mov     ax, -310
        mov     bx, 20
        mov     cx, [city_transition_seg_z0]
        cmp     cx, [city_bridge_visible_z_min]
        jge     .deck_long_z_ready
        mov     cx, [city_bridge_visible_z_min]
.deck_long_z_ready:
        mov     di, [city_transition_seg_z1]
        call    city286f_draw_world_line_z_chunks
        mov     ax, 310
        call    city286f_draw_world_line_z_chunks
        mov     byte [draw_color], 1
        mov     ax, -95
        call    city286f_draw_world_line_z_chunks
        mov     ax, 95
        call    city286f_draw_world_line_z_chunks
        mov     ax, [city_transition_seg_z0]
        add     ax, 96
        mov     [city_transition_cursor_z], ax
.deck_cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, [city_transition_seg_z1]
        jg      .towers
        cmp     cx, [city_bridge_visible_z_min]
        jl      .deck_cross_advance
        mov     di, cx
        mov     ax, -300
        mov     dx, 300
        mov     bx, 20
        mov     si, bx
        mov     byte [draw_color], 6
        call    city286f_draw_world_line
.deck_cross_advance:
        add     word [city_transition_cursor_z], 192
        jmp     .deck_cross
.towers:
        mov     ax, [city_transition_seg_z0]
        add     ax, 420
        mov     [city_transition_cursor_z], ax
.tower:
        mov     ax, [city_transition_cursor_z]
        cmp     ax, [city_transition_seg_z1]
        jle     short .__jcc286_skip_018
        jmp     near .done
.__jcc286_skip_018:
        mov     dx, ax
        add     dx, 650                 ; farthest forward stay/tower extent
        cmp     dx, [city_bridge_visible_z_min]
        jge     .tower_visible
        jmp     near .tower_advance
.tower_visible:
        mov     [city_bridge_z], ax
        mov     word [city_obj_x], -390
        mov     word [city_obj_y], 20
        mov     [city_obj_z], ax
        mov     word [city_obj_w], 70
        mov     word [city_obj_h], 650
        mov     word [city_obj_d], 120
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], 6
        call    city286f_draw_box_wire
        mov     word [city_obj_x], 320
        call    city286f_draw_box_wire

        ; Keep the lower-pier silhouette out from under the road projection.
        ; A short outward shoulder begins at the tower's outside deck corner,
        ; then the visible leg drops only to Y=-30.  Going all the way to the
        ; water projects the line back across the road on this camera path.
        mov     byte [draw_color], 6
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, -390
        mov     bx, 20
        mov     dx, -420
        mov     si, 0
        call    city286f_draw_world_line
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, 390
        mov     bx, 20
        mov     dx, 420
        mov     si, 0
        call    city286f_draw_world_line

        ; Keep the visible lower-pier segment shallow in Y.  Although the
        ; endpoints remain below the deck, extending it to the water projects
        ; the line back across the road in this camera path.
        mov     ax, [city_transition_seg_z0]
        add     ax, 420
        cmp     [city_bridge_z], ax
        je      .lower_piers_done
        mov     ax, [city_transition_seg_z1]
        sub     ax, 512
        cmp     [city_bridge_z], ax
        jge     .lower_piers_done
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, -420
        mov     bx, 0
        mov     dx, -420
        mov     si, -30
        call    city286f_draw_world_line
        mov     ax, 420
        mov     bx, 0
        mov     dx, 420
        mov     si, -30
        call    city286f_draw_world_line
.lower_piers_done:

        mov     byte [draw_color], 7
        mov     ax, -320
        mov     bx, 670
        mov     cx, [city_bridge_z]
        add     cx, 60
        mov     dx, 320
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     ax, -355
        mov     bx, 670
        mov     cx, [city_bridge_z]
        add     cx, 60
        mov     dx, -280
        mov     si, 20
        mov     di, cx
        sub     di, 470
        call    city286f_draw_world_line
        mov     di, cx
        add     di, 590
        call    city286f_draw_world_line
        mov     ax, 355
        mov     dx, 280
        mov     di, cx
        sub     di, 470
        call    city286f_draw_world_line
        mov     di, cx
        add     di, 590
        call    city286f_draw_world_line
.tower_advance:
        add     word [city_transition_cursor_z], 1024
        jmp     .tower
.done:
        popa
        ret

city286f_draw_bridge_tail:
        ; Exact phase-preserving tail used by the original park entry.
        ; Do not call the full bridge helper here: the river cross-lines, deck
        ; braces and towers each have an independent phase carried over from
        ; the preceding megabridge scene.
        pusha
        ; Preserve the bridge STRUCTURE throughout the park-entry turn, but
        ; hand the water off independently when the camera starts yawing.
%if CITY_WATER_PARK_HIDE_ON_TURN
        mov     ax, [city_cam_yaw_phase_target]
        cmp     ax, CITY_WATER_PARK_HIDE_YAW_PHASE_MIN
        jae     .deck
%endif
        mov     byte [draw_color], CITY_WATER_PALETTE_INDEX
        mov     byte [city_water_raster_active], 1
        mov     word [city_grid_index], 0
        mov     bp, CITY_WATER_GRID_LONG_X_COUNT
.river_long:
        mov     ax, [city_grid_index]
        mov     bx, CITY_WATER_GRID_X_STEP
        imul    bx
        add     ax, CITY_WATER_GRID_LONG_X_MIN
%if CITY_WATER_GRID_LONG_SKIP_CENTER
        ; Keep the same three-line opening in the phase-preserving bridge tail.
        cmp     ax, -CITY_WATER_GRID_LONG_SKIP_HALF
        jl      .river_long_draw
        cmp     ax, CITY_WATER_GRID_LONG_SKIP_HALF
        jle     .river_long_advance
.river_long_draw:
%endif
        mov     bx, CITY_WATER_Y
        mov     cx, CITY_WATER_PARK_TAIL_Z0
        mov     di, CITY_WATER_PARK_TAIL_Z1
        call    city286f_draw_world_line_z_chunks
.river_long_advance:
        inc     word [city_grid_index]
        dec     bp
        jz      .river_long_done
        jmp     near .river_long
.river_long_done:
        mov     word [city_transition_cursor_z], CITY_WATER_PARK_TAIL_CROSS_Z0
.river_cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, CITY_WATER_PARK_TAIL_Z1
        jg      .deck
%if CITY_WATER_GRID_CROSS_FULL_SCREEN
        call    city286f_draw_water_cross_screenwide
%else
        mov     di, cx
        mov     ax, CITY_WATER_GRID_FULL_X_MIN
        mov     dx, -CITY_WATER_ROAD_OCCLUDE_HALF
        mov     bx, CITY_WATER_Y
        mov     si, bx
        call    city286f_draw_world_line
        mov     cx, [city_transition_cursor_z]
        mov     di, cx
        mov     ax, CITY_WATER_ROAD_OCCLUDE_HALF
        mov     dx, CITY_WATER_GRID_FULL_X_MAX
        mov     bx, CITY_WATER_Y
        mov     si, bx
        call    city286f_draw_world_line
%endif
        add     word [city_transition_cursor_z], CITY_WATER_GRID_Z_STEP
        jmp     near .river_cross

.deck:
        mov     byte [city_water_raster_active], 0
        mov     byte [draw_color], 7
        mov     ax, -310
        mov     bx, 20
        mov     cx, 5000
        mov     di, 7108
        call    city286f_draw_world_line_z_chunks
        mov     ax, 310
        call    city286f_draw_world_line_z_chunks
        mov     byte [draw_color], 1
        mov     ax, -95
        call    city286f_draw_world_line_z_chunks
        mov     ax, 95
        call    city286f_draw_world_line_z_chunks
        mov     word [city_transition_cursor_z], 5021
.deck_cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, 7108
        jg      .towers
        mov     di, cx
        mov     ax, -300
        mov     dx, 300
        mov     bx, 20
        mov     si, bx
        mov     byte [draw_color], 6
        call    city286f_draw_world_line
        add     word [city_transition_cursor_z], 192
        jmp     near .deck_cross

.towers:
        mov     word [city_transition_cursor_z], 5729
.tower_loop:
        mov     ax, [city_transition_cursor_z]
        cmp     ax, 7108
        jle     short .__jcc286_skip_019
        jmp     near .done
.__jcc286_skip_019:
        mov     [city_bridge_z], ax
        mov     word [city_obj_x], -390
        mov     word [city_obj_y], 20
        mov     [city_obj_z], ax
        mov     word [city_obj_w], 70
        mov     word [city_obj_h], 650
        mov     word [city_obj_d], 120
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], 6
        call    city286f_draw_box_wire
        mov     word [city_obj_x], 320
        call    city286f_draw_box_wire

        ; Keep the lower-pier silhouette out from under the road projection.
        ; A short outward shoulder begins at the tower's outside deck corner,
        ; then the visible leg drops only to Y=-30.  Going all the way to the
        ; water projects the line back across the road on this camera path.
        mov     byte [draw_color], 6
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, -390
        mov     bx, 20
        mov     dx, -420
        mov     si, 0
        call    city286f_draw_world_line
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, 390
        mov     bx, 20
        mov     dx, 420
        mov     si, 0
        call    city286f_draw_world_line

        ; Same corrected lower-pier depth in rebased bridge-tail coordinates.
        cmp     word [city_bridge_z], (7108-512)
        jge     .lower_piers_done
        mov     cx, [city_bridge_z]
        mov     di, cx
        mov     ax, -420
        mov     bx, 0
        mov     dx, -420
        mov     si, -30
        call    city286f_draw_world_line
        mov     ax, 420
        mov     bx, 0
        mov     dx, 420
        mov     si, -30
        call    city286f_draw_world_line
.lower_piers_done:

        mov     byte [draw_color], 7
        mov     ax, -320
        mov     bx, 670
        mov     cx, [city_bridge_z]
        add     cx, 60
        mov     dx, 320
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line

        mov     ax, -355
        mov     bx, 670
        mov     cx, [city_bridge_z]
        add     cx, 60
        mov     dx, -280
        mov     si, 20
        mov     di, cx
        sub     di, 470
        call    city286f_draw_world_line
        mov     di, cx
        add     di, 590
        call    city286f_draw_world_line
        mov     ax, 355
        mov     dx, 280
        mov     di, cx
        sub     di, 470
        call    city286f_draw_world_line
        mov     di, cx
        add     di, 590
        call    city286f_draw_world_line

        add     word [city_transition_cursor_z], 1024
        jmp     near .tower_loop
.done:
        popa
        ret

city286f_draw_park_fixed:
        pusha
        mov     byte [draw_color], 2
        mov     word [city_grid_index], 0
        mov     bp, 9
.xlines:
        mov     ax, [city_grid_index]
        mov     bx, 150
        imul    bx
        sub     ax, 600
        add     ax, [city_route_model_x]
        mov     bx, 0
        mov     cx, [city_route_model_z]
        sub     cx, 600
        mov     di, [city_route_model_z]
        add     di, 600
        call    city286f_draw_world_line_z_chunks
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_079
        jmp     near .xlines
.__jcc286_skip_079:
        mov     word [city_grid_index], 0
        mov     bp, 9
.zlines:
        mov     cx, [city_grid_index]
        mov     ax, cx
        mov     bx, 150
        imul    bx
        mov     cx, ax
        sub     cx, 600
        add     cx, [city_route_model_z]
        mov     di, cx
        mov     ax, [city_route_model_x]
        sub     ax, 600
        mov     dx, [city_route_model_x]
        add     dx, 600
        xor     bx, bx
        xor     si, si
        call    city286f_draw_world_line
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_020
        jmp     near .zlines
.__jcc286_skip_020:
        mov     word [city_grid_index], 0
        mov     bp, 12
.trees:
        mov     bx, [city_grid_index]
        shl     bx, 1
        mov     ax, [city_park_tree_x + bx]
        add     ax, [city_route_model_x]
        mov     [city_tree_x], ax
        mov     ax, [city_park_tree_z + bx]
        add     ax, [city_route_model_z]
        mov     [city_tree_z], ax
        call    city286f_draw_tree
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_080
        jmp     near .trees
.__jcc286_skip_080:
        mov     word [city_row_index], 0
        mov     bp, 8
.blocks:
        cmp     word [city_row_index], 0
        jne     short .__jcc286_skip_021
        jmp     near .nextb
.__jcc286_skip_021:
        cmp     word [city_row_index], 2
        jne     short .__jcc286_skip_022
        jmp     near .nextb
.__jcc286_skip_022:
        cmp     word [city_row_index], 7
        jne     short .__jcc286_skip_023
        jmp     near .nextb
.__jcc286_skip_023:
        mov     bx, [city_row_index]
        shl     bx, 1
        mov     ax, [city_park_building_x + bx]
        add     ax, [city_route_model_x]
        mov     [city_obj_x], ax
        mov     ax, [city_park_building_z + bx]
        add     ax, [city_route_model_z]
        mov     [city_obj_z], ax
        mov     word [city_obj_y], 0
        mov     word [city_obj_w], 260
        mov     word [city_obj_d], 240
        mov     ax, [city_row_index]
        and     ax, 3
        mov     bx, 80
        imul    bx
        add     ax, 260
        mov     [city_obj_h], ax
        mov     byte [city_obj_flags], 1
        mov     al, [city_row_index]
        and     al, 3
        add     al, 3
        mov     [draw_color], al
        call    city286f_draw_box_wire
.nextb:
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_024
        jmp     near .blocks
.__jcc286_skip_024:
        popa
        ret

; Commercial entrance used during the final park turn.
city286f_draw_neon_entry_fixed:
        pusha
        ; Near the end of the park turn use the exact live neon model so the
        ; chapter switch cannot replace visibly different geometry.
        cmp     word [scene_frame], 848
        jb      .entry_geometry
        call    city286f_draw_neon_fixed
        popa
        ret
.entry_geometry:
        mov     word [city_row_index], 0
        mov     bp, 8
.buildings:
        mov     ax, [city_row_index]
        mov     bx, 360
        imul    bx
        add     ax, 8400
        mov     [city_obj_z], ax
        mov     word [city_obj_y], 0
        mov     word [city_obj_w], 260
        mov     word [city_obj_d], 190
        mov     bx, [city_row_index]
        and     bx, 15
        shl     bx, 1
        mov     ax, [city_height_table + bx]
        add     ax, 220
        mov     [city_obj_h], ax
        mov     byte [city_obj_flags], 1
        mov     word [city_obj_x], 290
        mov     al, [city_row_index]
        and     al, 3
        add     al, 2
        mov     [draw_color], al
        call    city286f_draw_box_wire
        mov     word [city_obj_x], 1230
        inc     byte [draw_color]
        cmp     byte [draw_color], 7
        jbe     .rc
        mov     byte [draw_color], 2
.rc:
        call    city286f_draw_box_wire
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_025
        jmp     near .buildings
.__jcc286_skip_025:
        mov     word [city_row_index], 0
        mov     bp, 4
.signs:
        mov     ax, [city_row_index]
        mov     bx, 520
        imul    bx
        add     ax, 8700
        mov     [city_panel_z], ax
        mov     al, [city_row_index]
        and     al, 7
        cmp     al, 2
        je      .sleft_overhang
        test    byte [city_row_index], 1
        jnz     .sright
        mov     word [city_panel_x0], 555
        mov     word [city_panel_x1], 650
        jmp     .snormal_height
.sright:
        mov     word [city_panel_x0], 1150
        mov     word [city_panel_x1], 1245
.snormal_height:
        mov     word [city_panel_y0], 110
        mov     word [city_panel_y1], 350
        jmp     .sready
.sleft_overhang:
        mov     word [city_panel_x0], 555
        mov     word [city_panel_x1], 800
        mov     word [city_panel_y0], 286
        mov     word [city_panel_y1], 436
.sready:
        mov     ax, [scene_frame]
        mov     cl, 3
        shr     ax, cl
        add     al, [city_row_index]
        and     al, 3
        xor     ah, ah
        mov     bx, ax
        mov     al, [city286_neon_sign_colours + bx]
        mov     [draw_color], al
        call    city286f_draw_panel
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_entry_signs_done
        jmp     near .signs
.__jcc286_entry_signs_done:

.done:
        popa
        ret

; AX = rear-most world Z of one commercial object.
; Return AX = per-object X bias after that object has completely passed camera.
city286f_neon_rear_x_bias:
        push    bx
        push    cx
        push    dx
        cmp     byte [scene_index], 4
        jne     .zero
        mov     cx, ax
        mov     ax, [city_cam_z]
        cmp     ax, cx
        jbe     .zero
        sub     ax, cx
        cmp     ax, CITY_NEON_REAR_CITY_SHIFT_DISTANCE
        jae     .full
        mov     bx, CITY_NEON_REAR_CITY_SHIFT_DISTANCE
        call    city286f_smoothstep
        mov     bx, ax
        mov     ax, CITY_NEON_REAR_CITY_SHIFT_X
        call    city286_mul_q8
        jmp     .done
.full:
        mov     ax, CITY_NEON_REAR_CITY_SHIFT_X
        jmp     .done
.zero:
        xor     ax, ax
.done:
        pop     dx
        pop     cx
        pop     bx
        ret

city286f_draw_neon_fixed:
        pusha
        mov     word [city_route_center_x], 900
        mov     word [city_transition_seg_z0], 8280
        mov     word [city_transition_seg_z1], 18448
        mov     word [city_transition_seg_half], 230
        mov     word [city_transition_seg_y], 0
        mov     word [city_transition_seg_spacing], 192
        call    city286f_draw_flat_road

        ; descending ramp 18448..20948
        mov     word [city_grid_index], 0
        mov     bp, 5
.ramp_long:
        mov     ax, [city_grid_index]
        mov     dx, 115
        imul    dx
        add     ax, 670
        mov     dx, ax
        xor     bx, bx
        mov     si, -175
        mov     cx, 18448
        mov     di, 20948
        mov     byte [draw_color], 1
        call    city286f_draw_world_line
        inc     word [city_grid_index]
        dec     bp
        jz      short .__jcc286_skip_081
        jmp     near .ramp_long
.__jcc286_skip_081:
        mov     word [city_transition_cursor_z], 18448
.ramp_cross:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, 20948
        jle     short .__jcc286_skip_082
        jmp     near .buildings
.__jcc286_skip_082:
        mov     ax, cx
        sub     ax, 18448
        mov     bx, 175
        mul     bx
        mov     bx, 2500
        div     bx
        neg     ax
        mov     bx, ax
        mov     si, ax
        mov     ax, 670
        mov     dx, 1130
        mov     di, cx
        call    city286f_draw_world_line
        add     word [city_transition_cursor_z], 160
        jmp     .ramp_cross
.buildings:
        ; Match the normal renderer: rear-city motion is now per object and
        ; begins when that object's rear edge has crossed behind the camera.
        ; This avoids starting the sideways move only after phantom geometry is
        ; already visible during the tunnel-entry pitch.
        mov     word [city_neon_scenery_x_bias], 0
        cmp     byte [scene_index], 4
        jne     .draw_b
        cmp     word [scene_frame], CITY_NEON_REAR_CITY_HIDE_FRAME
        jb      .draw_b
        jmp     near .done
.draw_b:
        mov     word [city_row_index], 0
        mov     bp, 28
.bloop:
        mov     ax, [city_row_index]
        mov     bx, 360
        imul    bx
        add     ax, 8400
        mov     [city_obj_z], ax
        mov     word [city_obj_y], 0
        mov     word [city_obj_w], 260
        mov     word [city_obj_d], 190
        mov     ax, [city_obj_z]
        add     ax, [city_obj_d]          ; shift only after the rear edge passes
        call    city286f_neon_rear_x_bias
        mov     [city_neon_scenery_x_bias], ax
        mov     bx, [city_row_index]
        and     bx, 15
        shl     bx, 1
        mov     ax, [city_height_table + bx]
        add     ax, 220
        mov     [city_obj_h], ax
        mov     byte [city_obj_flags], 1
        mov     ax, 290
        add     ax, [city_neon_scenery_x_bias]
        mov     [city_obj_x], ax
        mov     al, [city_row_index]
        and     al, 3
        add     al, 2
        mov     [draw_color], al
        call    city286f_draw_box_wire
        mov     ax, 1230
        add     ax, [city_neon_scenery_x_bias]
        mov     [city_obj_x], ax
        inc     byte [draw_color]
        cmp     byte [draw_color], 7
        jbe     .brc
        mov     byte [draw_color], 2
.brc:
        call    city286f_draw_box_wire
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_028
        jmp     near .bloop
.__jcc286_skip_028:
        mov     word [city_row_index], 0
        mov     bp, 18
.sloop:
        mov     ax, [city_row_index]
        mov     bx, 520
        imul    bx
        add     ax, 8700
        mov     [city_panel_z], ax
        call    city286f_neon_rear_x_bias
        mov     [city_neon_scenery_x_bias], ax
        ; A few signs project over the road edge as cantilevered neon boards.
        mov     al, [city_row_index]
        and     al, 7
        cmp     al, 2
        je      .sleft_overhang
        cmp     al, 5
        je      .sright_overhang
        test    byte [city_row_index], 1
        jnz     .sright
        mov     word [city_panel_x0], 555
        mov     word [city_panel_x1], 650
        jmp     .snormal_height
.sright:
        mov     word [city_panel_x0], 1150
        mov     word [city_panel_x1], 1245
.snormal_height:
        mov     word [city_panel_y0], 110
        mov     word [city_panel_y1], 350
        jmp     .sready
.sleft_overhang:
        mov     word [city_panel_x0], 555
        mov     word [city_panel_x1], 800
        mov     word [city_panel_y0], 286
        mov     word [city_panel_y1], 436
        jmp     .sready
.sright_overhang:
        mov     word [city_panel_x0], 1000
        mov     word [city_panel_x1], 1245
        mov     word [city_panel_y0], 286
        mov     word [city_panel_y1], 436
.sready:
        mov     ax, [city_neon_scenery_x_bias]
        add     [city_panel_x0], ax
        add     [city_panel_x1], ax
        ; Same direct high-saturation neon cycle as the 386/256 renderer.
        mov     ax, [scene_frame]
        mov     cl, 3
        shr     ax, cl
        add     al, [city_row_index]
        and     al, 3
        xor     ah, ah
        mov     bx, ax
        mov     al, [city286_neon_sign_colours + bx]
        mov     [draw_color], al
        call    city286f_draw_panel
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_029
        jmp     near .sloop
.__jcc286_skip_029:
.done:
        popa
        ret

city286_neon_sign_colours:
        db CITY_NEON_SIGN_COLORS

city286f_draw_tunnel_portal:
        pusha
        mov     word [city_panel_z], 15000
        mov     word [city_panel_x0], 470
        mov     word [city_panel_x1], 1330
        mov     word [city_panel_y0], -175
        mov     word [city_panel_y1], 235
        mov     byte [draw_color], 7
        call    city286f_draw_frame
        popa
        ret

; AX=tunnel-local Z -> AX=rightward curve offset.
city286f_tunnel_curve_offset:
        push    bx
        push    dx
        cmp     ax, CITY_TUNNEL_CURVE_Z
        jbe     .zero
        sub     ax, CITY_TUNNEL_CURVE_Z
        mov     bx, ax
        mul     bx
        mov     bx, CITY_TUNNEL_CURVE_DENOM
        div     bx
        jmp     .done
.zero:
        xor     ax, ax
.done:
        pop     dx
        pop     bx
        ret

; AX=base X, BX=Y, CX=z0, DI=z1 along the tunnel curve.
city286f_draw_world_line_tunnel_curve:
        pusha
        mov     [city_ground_x], ax
        mov     [city_ground_y], bx
        mov     [city_ground_cursor_z], cx
        mov     [city_ground_z1], di
.loop:
        mov     cx, [city_ground_cursor_z]
        cmp     cx, [city_ground_z1]
        jl      short .__jcc286_skip_030
        jmp     near .done
.__jcc286_skip_030:
        mov     di, cx
        add     di, 384
        cmp     di, [city_ground_z1]
        jle     .end
        mov     di, [city_ground_z1]
.end:
        mov     ax, cx
        call    city286f_tunnel_curve_offset
        add     ax, [city_ground_x]
        mov     [city_line_x0], ax
        mov     ax, di
        call    city286f_tunnel_curve_offset
        add     ax, [city_ground_x]
        mov     dx, ax
        mov     ax, [city_line_x0]
        mov     bx, [city_ground_y]
        mov     si, bx
        call    city286f_draw_world_line
        mov     [city_ground_cursor_z], di
        jmp     .loop
.done:
        popa
        ret

; Filled exit glare.  Match the normal edition: project the aperture quad
; and fill it as two triangles, so yaw/bank changes the visible silhouette.
city286f_draw_tunnel_exit_white:
        pusha
        mov     ax, CITY_TUNNEL_EXIT_Z
        call    city286f_tunnel_curve_offset
        mov     [city286f_curve_offset], ax
        ; P0 upper-left
        mov     ax, 438
        add     ax, [city286f_curve_offset]
        mov     bx, 267
        mov     cx, CITY_TUNNEL_EXIT_Z
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .__jcc286_skip_031
        jmp     near .done
.__jcc286_skip_031:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y0], ax
        ; P1 upper-right
        mov     ax, 1362
        add     ax, [city286f_curve_offset]
        mov     bx, 267
        mov     cx, CITY_TUNNEL_EXIT_Z
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .__jcc286_skip_032
        jmp     near .done
.__jcc286_skip_032:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x1], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y1], ax
        ; P2 lower-right
        mov     ax, 1362
        add     ax, [city286f_curve_offset]
        mov     bx, -207
        mov     cx, CITY_TUNNEL_EXIT_Z
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .__jcc286_skip_033
        jmp     near .done
.__jcc286_skip_033:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x2], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y2], ax
        ; P3 lower-left
        mov     ax, 438
        add     ax, [city286f_curve_offset]
        mov     bx, -207
        mov     cx, CITY_TUNNEL_EXIT_Z
        add     cx, [city286f_scene_z_bias]
        add     cx, [city286f_model_z_bias]
        call    city286_project_point
        cmp     byte [city286_project_ok], 0
        jne     short .__jcc286_skip_034
        jmp     near .done
.__jcc286_skip_034:
        mov     ax, [city286_project_x]
        mov     [city286f_screen_quad_x3], ax
        mov     ax, [city286_project_y]
        mov     [city286f_screen_quad_y3], ax
        ; Use the same two-triangle projected aperture as the normal edition.
        ; The previous 286 bounding-box fill visibly changed shape during bank.
        mov     byte [draw_color], CITY_TUNNEL_EXIT_FLASH_WHITE
        cmp     word [scene_frame], CITY_TUNNEL_EXIT_FORCE_WHITE
        jae     .colour
        test    byte [scene_frame], 4
        jz      .colour
        mov     byte [draw_color], CITY_TUNNEL_EXIT_FLASH_GRAY
.colour:
        mov     ax, [city286f_screen_quad_x0]
        mov     [tri_x0], ax
        mov     ax, [city286f_screen_quad_y0]
        mov     [tri_y0], ax
        mov     ax, [city286f_screen_quad_x1]
        mov     [tri_x1], ax
        mov     ax, [city286f_screen_quad_y1]
        mov     [tri_y1], ax
        mov     ax, [city286f_screen_quad_x2]
        mov     [tri_x2], ax
        mov     ax, [city286f_screen_quad_y2]
        mov     [tri_y2], ax
        call    city286f_fill_triangle

        mov     ax, [city286f_screen_quad_x0]
        mov     [tri_x0], ax
        mov     ax, [city286f_screen_quad_y0]
        mov     [tri_y0], ax
        mov     ax, [city286f_screen_quad_x2]
        mov     [tri_x1], ax
        mov     ax, [city286f_screen_quad_y2]
        mov     [tri_y1], ax
        mov     ax, [city286f_screen_quad_x3]
        mov     [tri_x2], ax
        mov     ax, [city286f_screen_quad_y3]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
.done:
        popa
        ret

city286f_draw_tunnel_curved:
        pusha
        ; start rib at ceil(max(camera+near,15000)/256)
        mov     ax, [city286_cam_z]
        sub     ax, [city286f_scene_z_bias]
        sub     ax, [city286f_model_z_bias]
        add     ax, CITY_NEAR_Z
        cmp     ax, 15000
        jge     .start
        mov     ax, 15000
.start:
        cmp     ax, CITY_TUNNEL_EXIT_Z
        jle     short .__jcc286_skip_035
        jmp     near .done
.__jcc286_skip_035:
        sub     ax, 15000
        add     ax, 255
        and     ax, 0FF00h
        add     ax, 15000
        mov     [city_transition_cursor_z], ax
        mov     dx, ax
        sub     dx, 15000
        shr     dx, 8
        mov     [city_row_index], dx
.rings:
        mov     ax, [city_transition_cursor_z]
        cmp     ax, CITY_TUNNEL_EXIT_Z
        jle     short .__jcc286_skip_036
        jmp     near .rails
.__jcc286_skip_036:
        push    ax
        call    city286f_tunnel_curve_offset
        mov     [city_tunnel_curve_offset_now], ax
        pop     ax
        mov     [city_panel_z], ax
        mov     bx, [city_tunnel_curve_offset_now]
        mov     ax, 470
        add     ax, bx
        mov     [city_panel_x0], ax
        mov     ax, 1330
        add     ax, bx
        mov     [city_panel_x1], ax
        mov     word [city_panel_y0], -175
        mov     word [city_panel_y1], 235
        mov     byte [draw_color], 3
        call    city286f_draw_frame
        test    byte [city_row_index], 1
        jnz     .next
        mov     byte [draw_color], 7
        mov     ax, 830
        add     ax, [city_tunnel_curve_offset_now]
        mov     bx, 225
        mov     cx, [city_transition_cursor_z]
        mov     dx, 970
        add     dx, [city_tunnel_curve_offset_now]
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
.next:
        add     word [city_transition_cursor_z], 256
        inc     word [city_row_index]
        jmp     .rings
.rails:
        mov     ax, [city286_cam_z]
        sub     ax, [city286f_scene_z_bias]
        sub     ax, [city286f_model_z_bias]
        add     ax, CITY_NEAR_Z
        cmp     ax, 15000
        jge     .rstart
        mov     ax, 15000
.rstart:
        cmp     ax, CITY_TUNNEL_EXIT_Z
        jl      short .__jcc286_skip_037
        jmp     near .done
.__jcc286_skip_037:
        mov     cx, ax
        mov     di, CITY_TUNNEL_EXIT_Z
        mov     byte [draw_color], 6
        mov     ax, 470
        mov     bx, -175
        call    city286f_draw_world_line_tunnel_curve
        mov     ax, 1330
        call    city286f_draw_world_line_tunnel_curve
        mov     ax, 470
        mov     bx, 235
        call    city286f_draw_world_line_tunnel_curve
        mov     ax, 1330
        call    city286f_draw_world_line_tunnel_curve
        mov     byte [draw_color], 1
        mov     ax, 780
        mov     bx, -174
        call    city286f_draw_world_line_tunnel_curve
        mov     ax, 1020
        call    city286f_draw_world_line_tunnel_curve
.done:
        popa
        ret

; --------------------------------------------------------------------------
; Railway / terminal / final-city models, preserving the original dimensions
; --------------------------------------------------------------------------
city286f_draw_rail_fixed:
        pusha
        ; Convert the rebased camera back to the model's original local Z.
        mov     ax, [city286_cam_z]
        sub     ax, [city286f_scene_z_bias]
        sub     ax, [city286f_model_z_bias]
        mov     bx, ax
        sub     ax, 512
        jns     .w0_nonneg
        xor     ax, ax
.w0_nonneg:
        cmp     ax, CITY_RAIL_MODEL_END
        jbe     .w0
        mov     ax, CITY_RAIL_MODEL_END
.w0:
        mov     [city286f_rail_window0], ax
        ; End = min(local camera + 14000, model end), avoiding signed overflow.
        mov     ax, bx
        cmp     ax, CITY_RAIL_MODEL_END-14000
        jbe     .w1_add
        mov     ax, CITY_RAIL_MODEL_END
        jmp     .w1
.w1_add:
        add     ax, 14000
        jns     .w1_nonneg
        xor     ax, ax
.w1_nonneg:
        cmp     ax, CITY_RAIL_MODEL_END
        jbe     .w1
        mov     ax, CITY_RAIL_MODEL_END
.w1:
        mov     [city286f_rail_window1], ax
        cmp     ax, [city286f_rail_window0]
        jae     short .__jcc286_skip_038
        jmp     near .done
.__jcc286_skip_038:

        mov     byte [draw_color], 6
        mov     word [city_grid_index], 0
        mov     bp, 4
.rails:
        mov     bx, [city_grid_index]
        shl     bx, 1
        mov     ax, [city_rail_x + bx]
        add     ax, 900
        xor     bx, bx
        mov     cx, [city286f_rail_window0]
        mov     di, [city286f_rail_window1]
        call    city286f_draw_world_line_z_chunks
        inc     word [city_grid_index]
        dec     bp
        jnz     .rails

        mov     ax, [city286f_rail_window0]
        xor     dx, dx
        mov     bx, 192
        div     bx
        test    dx, dx
        jz      .salign
        inc     ax
.salign:
        mov     bx, 192
        mul     bx
        mov     [city_transition_cursor_z], ax
.sleepers:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, [city286f_rail_window1]
        jg      .poles
        mov     di, cx
        mov     ax, 500
        mov     dx, 820
        xor     bx, bx
        xor     si, si
        mov     byte [draw_color], 2
        call    city286f_draw_world_line
        add     word [city_transition_cursor_z], 192
        jmp     .sleepers

.poles:
        mov     ax, 100
.pseek:
        cmp     ax, [city286f_rail_window0]
        jge     .palign
        add     ax, 640
        jmp     .pseek
.palign:
        mov     [city_transition_cursor_z], ax
.ploop:
        mov     cx, [city_transition_cursor_z]
        cmp     cx, [city286f_rail_window1]
        jle     short .__jcc286_skip_083
        jmp     near .wall
.__jcc286_skip_083:
        mov     di, cx
        mov     byte [draw_color], 5
        mov     ax, 470
        xor     bx, bx
        mov     dx, ax
        mov     si, 360
        call    city286f_draw_world_line
        mov     ax, 470
        mov     bx, 330
        mov     dx, 860
        mov     si, bx
        call    city286f_draw_world_line
        add     word [city_transition_cursor_z], 640
        jmp     .ploop

.wall:
        cmp     byte [city_rail_track_only], 0
        je      short .__jcc286_skip_039
        jmp     near .done
.__jcc286_skip_039:
        mov     word [city_row_index], 0
        mov     bp, 20
.wloop:
        mov     ax, [city_row_index]
        mov     bx, 1100
        mul     bx
        add     ax, 600
        cmp     ax, [city286f_rail_window0]
        jge     short .__jcc286_skip_040
        jmp     near .wnext
.__jcc286_skip_040:
        cmp     ax, [city286f_rail_window1]
        jle     short .__jcc286_skip_041
        jmp     near .done
.__jcc286_skip_041:
        mov     [city_obj_z], ax
        mov     word [city_obj_x], 1220
        mov     word [city_obj_y], 0
        mov     word [city_obj_w], 360
        mov     word [city_obj_d], 300
        mov     bx, [city_row_index]
        and     bx, 15
        shl     bx, 1
        mov     ax, [city_height_table + bx]
        add     ax, 180
        mov     [city_obj_h], ax
        mov     byte [city_obj_flags], 1
        mov     al, [city_row_index]
        and     al, 3
        add     al, 2
        mov     [draw_color], al
        call    city286f_draw_box_wire
.wnext:
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_042
        jmp     near .wloop
.__jcc286_skip_042:
.done:
        popa
        ret

city286f_draw_train_fixed:
        pusha
        mov     word [city_row_index], 0
        mov     bp, 10
.loop:
        mov     ax, [city_row_index]
        mov     bx, 300
        mul     bx
        neg     ax
        add     ax, [city_train_phase]
        mov     [city_obj_z], ax
        mov     word [city_obj_x], 500
        mov     word [city_obj_y], 8
        mov     word [city_obj_w], 140
        mov     word [city_obj_h], 175
        mov     word [city_obj_d], 285
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], 7
        call    city286f_draw_box_wire
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_084
        jmp     near .loop
.__jcc286_skip_084:
        popa
        ret

city286f_draw_oncoming_train_fixed:
        pusha
        mov     word [city_row_index], 0
        mov     bp, CITY_RAIL_ONCOMING_CARS
.loop:
        mov     ax, [city_row_index]
        mov     bx, 300
        mul     bx
        add     ax, [city_oncoming_train_phase]
        mov     [city_obj_z], ax
        mov     word [city_obj_x], 690
        mov     word [city_obj_y], 8
        mov     word [city_obj_w], 140
        mov     word [city_obj_h], 175
        mov     word [city_obj_d], 285
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], 6
        call    city286f_draw_box_wire
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_085
        jmp     near .loop
.__jcc286_skip_085:
        popa
        ret

city286f_draw_terminal_fixed:
        pusha
        mov     word [city_obj_x], 140
        mov     word [city_obj_y], 0
        mov     word [city_obj_z], 29600
        mov     word [city_obj_w], 1520
        mov     word [city_obj_h], 330
        mov     word [city_obj_d], 720
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], 5
        call    city286f_draw_box_wire
        mov     word [city_obj_x], -90
        mov     word [city_obj_w], 150
        mov     word [city_obj_h], 820
        mov     byte [city_obj_flags], 3
        mov     word [city_obj_antenna], 180
        mov     byte [draw_color], 6
        call    city286f_draw_box_wire
        mov     word [city_obj_x], 1740
        call    city286f_draw_box_wire
        mov     byte [draw_color], 7
        mov     ax, -150
        mov     bx, 430
        mov     cx, 29840
        mov     dx, 1950
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
        mov     bx, 470
        mov     si, bx
        call    city286f_draw_world_line
        mov     word [city_route_center_x], 900
        mov     word [city_transition_seg_z0], 28600
        mov     word [city_transition_seg_z1], 31600
        mov     word [city_transition_seg_half], 1200
        mov     word [city_transition_seg_y], 0
        mov     word [city_transition_seg_spacing], 256
        call    city286f_draw_flat_road
        popa
        ret

; Two-stage terminal landmark matching the 386/256 renderer.  Move it
; toward the railway so the gap to the outer catenary/track envelope is about
; half the previous value, while retaining the same Z clearance from terminal.
city286f_draw_terminal_landmark:
        pusha
        ; Low-rise FRONT: directly behind SMALL, same thin-axis footprint.
        mov     word [city_obj_x], CITY_TERMINAL_LANDMARK_LOW_FRONT_X
        mov     word [city_obj_y], CITY_TERMINAL_LANDMARK_LOW_FRONT_Y
        mov     word [city_obj_z], CITY_TERMINAL_LANDMARK_LOW_FRONT_Z
        mov     word [city_obj_w], CITY_TERMINAL_LANDMARK_LOW_FRONT_W
        mov     word [city_obj_h], CITY_TERMINAL_LANDMARK_LOW_FRONT_H
        mov     word [city_obj_d], CITY_TERMINAL_LANDMARK_LOW_FRONT_D
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], CITY_TERMINAL_LANDMARK_COLOR_LOW
        call    city286f_draw_box_wire

        ; Low-rise REAR: after MID/TALL in +Z, broaden across the complete
        ; SMALL+MID+TALL width.  This produces the stepped/L-like footprint.
        mov     word [city_obj_x], CITY_TERMINAL_LANDMARK_LOW_REAR_X
        mov     word [city_obj_y], CITY_TERMINAL_LANDMARK_LOW_REAR_Y
        mov     word [city_obj_z], CITY_TERMINAL_LANDMARK_LOW_REAR_Z
        mov     word [city_obj_w], CITY_TERMINAL_LANDMARK_LOW_REAR_W
        mov     word [city_obj_h], CITY_TERMINAL_LANDMARK_LOW_REAR_H
        mov     word [city_obj_d], CITY_TERMINAL_LANDMARK_LOW_REAR_D
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], CITY_TERMINAL_LANDMARK_COLOR_LOW
        call    city286f_draw_box_wire

        ; Small building on MID's side opposite TALL.
        mov     word [city_obj_x], CITY_TERMINAL_LANDMARK_SMALL_X
        mov     word [city_obj_y], CITY_TERMINAL_LANDMARK_SMALL_Y
        mov     word [city_obj_z], CITY_TERMINAL_LANDMARK_SMALL_Z
        mov     word [city_obj_w], CITY_TERMINAL_LANDMARK_SMALL_W
        mov     word [city_obj_h], CITY_TERMINAL_LANDMARK_SMALL_H
        mov     word [city_obj_d], CITY_TERMINAL_LANDMARK_SMALL_D
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], CITY_TERMINAL_LANDMARK_COLOR_SMALL
        call    city286f_draw_box_wire

        ; Main middle volume, directly between SMALL and TALL.
        mov     word [city_obj_x], CITY_TERMINAL_LANDMARK_MID_X
        mov     word [city_obj_y], CITY_TERMINAL_LANDMARK_MID_Y
        mov     word [city_obj_z], CITY_TERMINAL_LANDMARK_MID_Z
        mov     word [city_obj_w], CITY_TERMINAL_LANDMARK_MID_W
        mov     word [city_obj_h], CITY_TERMINAL_LANDMARK_MID_H
        mov     word [city_obj_d], CITY_TERMINAL_LANDMARK_MID_D
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], CITY_TERMINAL_LANDMARK_COLOR_MID
        call    city286f_draw_box_wire

        ; Tall right tower; X side face touches MID and Z centre is aligned.
        mov     word [city_obj_x], CITY_TERMINAL_LANDMARK_TALL_X
        mov     word [city_obj_y], CITY_TERMINAL_LANDMARK_TALL_Y
        mov     word [city_obj_z], CITY_TERMINAL_LANDMARK_TALL_Z
        mov     word [city_obj_w], CITY_TERMINAL_LANDMARK_TALL_W
        mov     word [city_obj_h], CITY_TERMINAL_LANDMARK_TALL_H
        mov     word [city_obj_d], CITY_TERMINAL_LANDMARK_TALL_D
        mov     byte [city_obj_flags], 1
        mov     byte [draw_color], CITY_TERMINAL_LANDMARK_COLOR_TALL
        call    city286f_draw_box_wire
        popa
        ret

; Low-cost tapered lattice tower on the left side of the final panorama.
; The nearest foot is X=-1250, leaving about half the former gap to the
; terminal canopy.  A short rectangular neighbour sits just outside it.
city286f_draw_final_left_tower:
        pusha
        mov     byte [draw_color], 6
        mov     ax, -1650
        mov     bx, 0
        mov     cx, 29800
        mov     dx, -1450
        mov     si, 1180
        mov     di, 29800
        call    city286f_draw_world_line
        mov     ax, -1250
        mov     bx, 0
        mov     cx, 29800
        mov     dx, -1450
        mov     si, 1180
        mov     di, 29800
        call    city286f_draw_world_line
        mov     ax, -1650
        mov     bx, 0
        mov     cx, 30040
        mov     dx, -1450
        mov     si, 1180
        mov     di, 30040
        call    city286f_draw_world_line
        mov     ax, -1250
        mov     bx, 0
        mov     cx, 30040
        mov     dx, -1450
        mov     si, 1180
        mov     di, 30040
        call    city286f_draw_world_line
        mov     ax, -1650
        mov     bx, 0
        mov     cx, 29800
        mov     dx, -1650
        mov     si, 0
        mov     di, 30040
        call    city286f_draw_world_line
        mov     ax, -1250
        mov     bx, 0
        mov     cx, 29800
        mov     dx, -1250
        mov     si, 0
        mov     di, 30040
        call    city286f_draw_world_line
        mov     ax, -1450
        mov     bx, 1180
        mov     cx, 29800
        mov     dx, -1450
        mov     si, 1180
        mov     di, 30040
        call    city286f_draw_world_line
        mov     byte [draw_color], 7
        mov     ax, -1560
        mov     bx, 760
        mov     cx, 29800
        mov     dx, -1340
        mov     si, 760
        mov     di, 29800
        call    city286f_draw_world_line
        mov     ax, -1560
        mov     bx, 760
        mov     cx, 30040
        mov     dx, -1340
        mov     si, 760
        mov     di, 30040
        call    city286f_draw_world_line
        mov     ax, -1450
        mov     bx, 1180
        mov     cx, 29920
        mov     dx, -1450
        mov     si, 1580
        mov     di, 29920
        call    city286f_draw_world_line
        ; Plain low neighbour: one wireframe box, no extra band or antenna.
        mov     word [city_obj_x], -2120
        mov     word [city_obj_y], 0
        mov     word [city_obj_z], 29820
        mov     word [city_obj_w], 420
        mov     word [city_obj_h], 420
        mov     word [city_obj_d], 400
        mov     byte [city_obj_flags], 0
        mov     byte [draw_color], 5
        call    city286f_draw_box_wire
        popa
        ret

city286f_draw_final_city:
        pusha
        mov     word [city_route_center_x], 0
        mov     word [city_transition_seg_z0], 26000
        mov     word [city_transition_seg_z1], 31600
        mov     word [city_transition_seg_half], 2200
        mov     word [city_transition_seg_y], 0
        mov     word [city_transition_seg_spacing], 500
        call    city286f_draw_flat_road
        mov     word [city_row_index], 0
        mov     bp, 12
.loop:
        mov     ax, [city_row_index]
        and     ax, 3
        mov     bx, 1200
        mul     bx
        sub     ax, 1800
        mov     [city_obj_x], ax
        mov     ax, [city_row_index]
        shr     ax, 2
        mov     bx, 700
        mul     bx
        add     ax, 26600
        mov     [city_obj_z], ax
        mov     word [city_obj_y], 0
        mov     word [city_obj_w], 420
        mov     word [city_obj_d], 380
        mov     bx, [city_row_index]
        and     bx, 15
        shl     bx, 1
        mov     ax, [city_height_table + bx]
        add     ax, 260
        mov     [city_obj_h], ax
        mov     byte [city_obj_flags], 1
        test    byte [city_row_index], 3
        jnz     .flags
        or      byte [city_obj_flags], 2
.flags:
        mov     al, [city_row_index]
        and     al, 5
        add     al, 2
        cmp     al, 7
        jbe     .color
        sub     al, 6
.color:
        mov     [draw_color], al
        call    city286f_draw_box_wire
        inc     word [city_row_index]
        dec     bp
        jz      short .__jcc286_skip_043
        jmp     near .loop
.__jcc286_skip_043:
        call    city286f_draw_terminal_landmark
        call    city286f_draw_final_left_tower
        popa
        ret

; --------------------------------------------------------------------------
; Camera setup helpers
; --------------------------------------------------------------------------
; --------------------------------------------------------------------------
; Persistent camera emulation
; --------------------------------------------------------------------------
; AX=target integer position, SI -> view integer, DI -> 1/16 remainder byte.
; This implements the source camera update: view_q4 += (target_q4-view_q4)>>4
; without requiring 32-bit registers.  Large discontinuities are copied
; directly; legitimate smoothed paths in NEON3 never approach that threshold.
city286f_smooth_position_q4:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bx, [si]
        mov     cx, ax
        sub     cx, bx
        cmp     cx, 2047
        jle     short .__jcc286_skip_044
        jmp     near .copy
.__jcc286_skip_044:
        cmp     cx, -2047
        jge     short .__jcc286_skip_086
        jmp     near .copy
.__jcc286_skip_086:
        shl     cx, 4
        xor     dx, dx
        mov     dl, [di]
        sub     cx, dx
        sar     cx, 4                  ; signed Q4 update
        mov     ax, cx
        xor     dx, dx
        mov     dl, [di]
        add     ax, dx
        mov     dx, ax
        and     dx, 000fh
        mov     [di], dl
        sar     ax, 4
        add     [si], ax
        jmp     .done
.copy:
        mov     [si], ax               ; AX is still the original target
        mov     byte [di], 0
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Copy target camera position/orientation into the persistent rendered view.
city286f_camera_copy_direct:
        mov     ax, [city286_cam_x]
        mov     [city286f_view_x], ax
        mov     byte [city286f_view_x_frac], 0
        mov     ax, [city286_cam_y]
        mov     [city286f_view_y], ax
        mov     byte [city286f_view_y_frac], 0
        mov     ax, [city286_cam_z]
        mov     [city286f_view_z], ax
        mov     byte [city286f_view_z_frac], 0
        xor     ax, ax
        mov     al, [city286_yaw]
        shl     ax, 8
        mov     [city286f_view_yaw_phase], ax
        xor     ax, ax
        mov     al, [city286_pitch]
        shl     ax, 8
        mov     [city286f_view_pitch_phase], ax
        xor     ax, ax
        mov     al, [city286_roll]
        shl     ax, 8
        mov     [city286f_view_roll_phase], ax
        mov     byte [city_camera_initialized], 1
        ret

; Reproduce CITY3D256's persistent camera smoother in 286-safe arithmetic.
; Target values are in city286_cam_*.  On return those variables contain the
; rendered view used by projection, while the persistent phase/view state is
; retained for the next frame.
city286f_update_camera_matrix:
        pusha
        cmp     byte [city286f_camera_view_direct], 0
        jne     .copy_phase_direct
        cmp     byte [city286f_camera_phase_direct], 0
        jne     .copy_phase_direct
        cmp     byte [city_camera_initialized], 0
        je      short .__jcc286_skip_045
        jmp     near .smooth
.__jcc286_skip_045:
        call    city286f_camera_copy_direct
        jmp     .matrix

.copy_phase_direct:
        mov     ax, [city286_cam_x]
        mov     [city286f_view_x], ax
        mov     byte [city286f_view_x_frac], 0
        mov     ax, [city286_cam_y]
        mov     [city286f_view_y], ax
        mov     byte [city286f_view_y_frac], 0
        mov     ax, [city286_cam_z]
        mov     [city286f_view_z], ax
        mov     byte [city286f_view_z_frac], 0
        mov     ax, [city_cam_yaw_phase_target]
        mov     [city286f_view_yaw_phase], ax
        mov     ax, [city_cam_pitch_phase_target]
        mov     [city286f_view_pitch_phase], ax
        mov     ax, [city_cam_roll_phase_target]
        mov     [city286f_view_roll_phase], ax
        mov     byte [city_camera_initialized], 1
        jmp     .matrix

.smooth:
        cmp     byte [city_camera_hold_smoothing], 0
        jne     .matrix
        mov     ax, [city286_cam_x]
        mov     si, city286f_view_x
        mov     di, city286f_view_x_frac
        call    city286f_smooth_position_q4
        mov     ax, [city286_cam_y]
        mov     si, city286f_view_y
        mov     di, city286f_view_y_frac
        call    city286f_smooth_position_q4
        cmp     byte [city286f_camera_z_direct], 0
        je      .smooth_z
        mov     ax, [city286_cam_z]
        mov     [city286f_view_z], ax
        mov     byte [city286f_view_z_frac], 0
        jmp     .angles
.smooth_z:
        mov     ax, [city286_cam_z]
        mov     si, city286f_view_z
        mov     di, city286f_view_z_frac
        call    city286f_smooth_position_q4
.angles:
        ; 8.8 turn phases intentionally wrap in 16 bits, exactly like source.
        xor     ax, ax
        mov     al, [city286_yaw]
        shl     ax, 8
        sub     ax, [city286f_view_yaw_phase]
        sar     ax, 4
        add     [city286f_view_yaw_phase], ax
        xor     ax, ax
        mov     al, [city286_pitch]
        shl     ax, 8
        sub     ax, [city286f_view_pitch_phase]
        sar     ax, 4
        add     [city286f_view_pitch_phase], ax
        xor     ax, ax
        mov     al, [city286_roll]
        shl     ax, 8
        sub     ax, [city286f_view_roll_phase]
        sar     ax, 4
        add     [city286f_view_roll_phase], ax

.matrix:
        mov     ax, [city286f_view_x]
        mov     [city286_cam_x], ax
        mov     ax, [city286f_view_y]
        mov     [city286_cam_y], ax
        mov     ax, [city286f_view_z]
        mov     [city286_cam_z], ax
        mov     ax, [city286f_view_yaw_phase]
        mov     [city_cam_yaw_phase_target], ax
        mov     ax, [city286f_view_pitch_phase]
        mov     [city_cam_pitch_phase_target], ax
        mov     ax, [city286f_view_roll_phase]
        mov     [city_cam_roll_phase_target], ax
        call    city286f_update_matrix_phase
        popa
        ret

city286f_camera_begin:
        call    city286_camera_clear
        mov     byte [city286f_camera_z_direct], 0
        mov     byte [city286f_camera_phase_direct], 0
        mov     byte [city286f_camera_view_direct], 0
        mov     word [city286f_model_z_bias], 0
        mov     word [city286f_scene_z_bias], 0
        ret

; AX=original scene Z, apply current scene rebase to camera Z.
city286f_set_camera_z:
        add     ax, [city286f_scene_z_bias]
        mov     [city286_cam_z], ax
        ret

; Global approach+canyon travel = global frame * 20.
city286f_calc_route_travel:
        mov     ax, [city_global_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        mov     [city_route_travel], ax
        ret

; Straight tunnel model used while entering the portal from the neon district.
city286f_draw_tunnel_fixed:
        pusha
        mov     ax, [city286_cam_z]
        sub     ax, [city286f_scene_z_bias]
        sub     ax, [city286f_model_z_bias]
        add     ax, CITY_NEAR_Z
        cmp     ax, 15000
        jge     .start
        mov     ax, 15000
.start:
        cmp     ax, 27000
        jle     short .__jcc286_skip_046
        jmp     near .done
.__jcc286_skip_046:
        sub     ax, 15000
        add     ax, 255
        and     ax, 0FF00h
        add     ax, 15000
        mov     [city_transition_cursor_z], ax
        mov     dx, ax
        sub     dx, 15000
        shr     dx, 8
        mov     [city_row_index], dx
.rings:
        mov     ax, [city_transition_cursor_z]
        cmp     ax, 27000
        jle     short .__jcc286_skip_047
        jmp     near .rails
.__jcc286_skip_047:
        mov     [city_panel_z], ax
        mov     word [city_panel_x0], 470
        mov     word [city_panel_x1], 1330
        mov     word [city_panel_y0], -175
        mov     word [city_panel_y1], 235
        mov     byte [draw_color], 3
        call    city286f_draw_frame
        test    byte [city_row_index], 1
        jnz     .next
        mov     byte [draw_color], 7
        mov     ax, 830
        mov     bx, 225
        mov     cx, [city_transition_cursor_z]
        mov     dx, 970
        mov     si, bx
        mov     di, cx
        call    city286f_draw_world_line
.next:
        add     word [city_transition_cursor_z], 256
        inc     word [city_row_index]
        jmp     .rings
.rails:
        mov     ax, [city286_cam_z]
        sub     ax, [city286f_scene_z_bias]
        sub     ax, [city286f_model_z_bias]
        add     ax, CITY_NEAR_Z
        cmp     ax, 15000
        jge     .rs
        mov     ax, 15000
.rs:
        cmp     ax, 27000
        jl      short .__jcc286_skip_048
        jmp     near .done
.__jcc286_skip_048:
        mov     cx, ax
        mov     di, 27000
        mov     byte [draw_color], 6
        mov     ax, 470
        mov     bx, -175
        call    city286f_draw_world_line_z_chunks
        mov     ax, 1330
        call    city286f_draw_world_line_z_chunks
        mov     ax, 470
        mov     bx, 235
        call    city286f_draw_world_line_z_chunks
        mov     ax, 1330
        call    city286f_draw_world_line_z_chunks
        mov     byte [draw_color], 1
        mov     ax, 780
        mov     bx, -174
        call    city286f_draw_world_line_z_chunks
        mov     ax, 1020
        call    city286f_draw_world_line_z_chunks
.done:
        popa
        ret

; --------------------------------------------------------------------------
; Faithful final rotozoom (geometry/timing port of CITY3D256)
; --------------------------------------------------------------------------
; AX,BX,CX,DX endpoints -> clipped same-colour line.
city286f_roto_draw_clipped_line:
        pusha
        mov     [city286f_sx0], ax
        mov     [city286f_sy0], bx
        mov     [city286f_sx1], cx
        mov     [city286f_sy1], dx
        call    city286f_clip_screen
        jc      .done
        mov     ax, [city286f_sx0]
        mov     bx, [city286f_sy0]
        mov     cx, [city286f_sx1]
        mov     dx, [city286f_sy1]
        call    line_set_same_colour
.done:
        popa
        ret

; Transform and fill the selected source cell (0..24,0..24), inset by 2.
city286f_roto_fill_focused_tile:
        pusha
        mov     ax, CITY_FINAL_ROTOZOOM_INSET
        mov     bx, CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p0_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p0_y], ax
        mov     ax, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        mov     bx, CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p1_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p1_y], ax
        mov     ax, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        mov     bx, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p2_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p2_y], ax
        mov     ax, CITY_FINAL_ROTOZOOM_INSET
        mov     bx, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p3_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p3_y], ax
        mov     byte [draw_color], 4
        mov     ax, [city286f_roto_p0_x]
        mov     [tri_x0], ax
        mov     ax, [city286f_roto_p0_y]
        mov     [tri_y0], ax
        mov     ax, [city286f_roto_p1_x]
        mov     [tri_x1], ax
        mov     ax, [city286f_roto_p1_y]
        mov     [tri_y1], ax
        mov     ax, [city286f_roto_p2_x]
        mov     [tri_x2], ax
        mov     ax, [city286f_roto_p2_y]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
        mov     ax, [city286f_roto_p0_x]
        mov     [tri_x0], ax
        mov     ax, [city286f_roto_p0_y]
        mov     [tri_y0], ax
        mov     ax, [city286f_roto_p2_x]
        mov     [tri_x1], ax
        mov     ax, [city286f_roto_p2_y]
        mov     [tri_y1], ax
        mov     ax, [city286f_roto_p3_x]
        mov     [tri_x2], ax
        mov     ax, [city286f_roto_p3_y]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
        popa
        ret

city286f_roto_fill_cells:
        pusha
        mov     word [city286f_roto_cell_y], -CITY_FINAL_ROTOZOOM_HALF
        mov     byte [city286f_roto_row], 0
.row:
        mov     word [city286f_roto_cell_x], -CITY_FINAL_ROTOZOOM_HALF
        mov     byte [city286f_roto_col], 0
.col:
        mov     al, [city286f_roto_row]
        xor     al, [city286f_roto_col]
        test    al, 1
        jz      short .__jcc286_skip_049
        jmp     near .next
.__jcc286_skip_049:
        ; Four inset transformed corners.
        mov     ax, [city286f_roto_cell_x]
        add     ax, CITY_FINAL_ROTOZOOM_INSET
        mov     bx, [city286f_roto_cell_y]
        add     bx, CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p0_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p0_y], ax

        mov     ax, [city286f_roto_cell_x]
        add     ax, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        mov     bx, [city286f_roto_cell_y]
        add     bx, CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p1_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p1_y], ax

        mov     ax, [city286f_roto_cell_x]
        add     ax, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        mov     bx, [city286f_roto_cell_y]
        add     bx, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p2_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p2_y], ax

        mov     ax, [city286f_roto_cell_x]
        add     ax, CITY_FINAL_ROTOZOOM_INSET
        mov     bx, [city286f_roto_cell_y]
        add     bx, CITY_FINAL_ROTOZOOM_STEP-CITY_FINAL_ROTOZOOM_INSET
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286f_roto_p3_x], ax
        mov     ax, [city286_project_y]
        mov     [city286f_roto_p3_y], ax

        ; Bounding-box reject before scanline divisions.
        mov     ax, [city286f_roto_p0_x]
        mov     [city286f_bbox_min_x], ax
        mov     [city286f_bbox_max_x], ax
        mov     ax, [city286f_roto_p0_y]
        mov     [city286f_bbox_min_y], ax
        mov     [city286f_bbox_max_y], ax
        mov     si, city286f_roto_p1_x
        mov     bp, 3
.bbox:
        mov     ax, [si]
        cmp     ax, [city286f_bbox_min_x]
        jge     .minx
        mov     [city286f_bbox_min_x], ax
.minx:
        cmp     ax, [city286f_bbox_max_x]
        jle     .maxx
        mov     [city286f_bbox_max_x], ax
.maxx:
        mov     ax, [si+2]
        cmp     ax, [city286f_bbox_min_y]
        jge     .miny
        mov     [city286f_bbox_min_y], ax
.miny:
        cmp     ax, [city286f_bbox_max_y]
        jle     .maxy
        mov     [city286f_bbox_max_y], ax
.maxy:
        add     si, 4
        dec     bp
        jz      short .__jcc286_skip_050
        jmp     near .bbox
.__jcc286_skip_050:
        cmp     word [city286f_bbox_max_x], 0
        jge     short .__jcc286_skip_051
        jmp     near .next
.__jcc286_skip_051:
        cmp     word [city286f_bbox_min_x], SCREEN_W-1
        jle     short .__jcc286_skip_052
        jmp     near .next
.__jcc286_skip_052:
        cmp     word [city286f_bbox_max_y], 0
        jge     short .__jcc286_skip_053
        jmp     near .next
.__jcc286_skip_053:
        cmp     word [city286f_bbox_min_y], SCREEN_H-1
        jle     short .__jcc286_skip_054
        jmp     near .next
.__jcc286_skip_054:

        mov     al, [city286f_roto_row]
        and     al, 1
        add     al, 3
        mov     [draw_color], al
        mov     ax, [city286f_roto_p0_x]
        mov     [tri_x0], ax
        mov     ax, [city286f_roto_p0_y]
        mov     [tri_y0], ax
        mov     ax, [city286f_roto_p1_x]
        mov     [tri_x1], ax
        mov     ax, [city286f_roto_p1_y]
        mov     [tri_y1], ax
        mov     ax, [city286f_roto_p2_x]
        mov     [tri_x2], ax
        mov     ax, [city286f_roto_p2_y]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
        mov     ax, [city286f_roto_p0_x]
        mov     [tri_x0], ax
        mov     ax, [city286f_roto_p0_y]
        mov     [tri_y0], ax
        mov     ax, [city286f_roto_p2_x]
        mov     [tri_x1], ax
        mov     ax, [city286f_roto_p2_y]
        mov     [tri_y1], ax
        mov     ax, [city286f_roto_p3_x]
        mov     [tri_x2], ax
        mov     ax, [city286f_roto_p3_y]
        mov     [tri_y2], ax
        call    city286f_fill_triangle
.next:
        add     word [city286f_roto_cell_x], CITY_FINAL_ROTOZOOM_STEP
        inc     byte [city286f_roto_col]
        cmp     word [city286f_roto_cell_x], CITY_FINAL_ROTOZOOM_HALF
        jge     short .__jcc286_skip_055
        jmp     near .col
.__jcc286_skip_055:
        add     word [city286f_roto_cell_y], CITY_FINAL_ROTOZOOM_STEP
        inc     byte [city286f_roto_row]
        cmp     word [city286f_roto_cell_y], CITY_FINAL_ROTOZOOM_HALF
        jge     short .__jcc286_skip_056
        jmp     near .row
.__jcc286_skip_056:
        popa
        ret

city286_draw_final_rotozoom:
        pusha
        mov     ax, [scene_frame]
        cmp     ax, CITY_FINAL_ROTOZOOM_START
        jae     short .__jcc286_skip_057
        jmp     near .done
.__jcc286_skip_057:
        cmp     ax, CITY_FINAL_COVER_REUSE_START
        jb      .setup
        mov     byte [draw_color], 4
        xor     ax, ax
        xor     bx, bx
        mov     cx, SCREEN_W-1
        mov     si, SCREEN_H-1
        call    fill_rect
        jmp     .done
.setup:
        sub     ax, CITY_FINAL_ROTOZOOM_START
        mov     [city286_roto_phase], ax
        mov     dl, al
        shl     al, 1
        add     al, dl
        mov     [city286_roto_angle], al
        mov     ax, [city286_roto_phase]
        cmp     ax, (CITY_FINAL_ROTOZOOM_FOCUS_START-CITY_FINAL_ROTOZOOM_START)
        jae     .focus_zoom
        shl     ax, 1
        add     ax, 96
        jmp     .zoom_ready
.focus_zoom:
        sub     ax, (CITY_FINAL_ROTOZOOM_FOCUS_START-CITY_FINAL_ROTOZOOM_START)
        mov     bx, ax
        mul     bx
        mov     bx, 5
        mul     bx
        add     ax, 416
.zoom_ready:
        mov     [city286_roto_zoom], ax
        mov     al, [city286_roto_angle]
        call    city286_get_sin
        mov     bx, [city286_roto_zoom]
        call    city286_mul_q14
        mov     [city286_roto_sin], ax
        mov     al, [city286_roto_angle]
        call    city286_get_cos
        mov     bx, [city286_roto_zoom]
        call    city286_mul_q14
        mov     [city286_roto_cos], ax

        ; Exact source fly-through centre: x=760-phase*9/5, y=300-phase/2.
        mov     ax, [city286_roto_phase]
        mov     bx, 9
        mul     bx
        mov     bx, 5
        div     bx
        mov     bx, 760
        sub     bx, ax
        mov     [city286_roto_cx], bx
        mov     ax, [city286_roto_phase]
        shr     ax, 1
        mov     bx, 300
        sub     bx, ax
        mov     [city286_roto_cy], bx

        cmp     word [city286_roto_phase], (CITY_FINAL_ROTOZOOM_FOCUS_START-CITY_FINAL_ROTOZOOM_START)
        jae     short .__jcc286_skip_058
        jmp     near .centre_ready
.__jcc286_skip_058:
        mov     ax, [city286_roto_cos]
        sub     ax, [city286_roto_sin]
        mov     bx, CITY_FINAL_ROTOZOOM_FOCUS_X
        imul    bx
        ; signed >>8 of DX:AX
        mov     cx, ax
        shr     cx, 8
        shl     dx, 8
        or      cx, dx
        mov     ax, CITY_SCREEN_CX
        sub     ax, cx
        mov     [city286f_roto_focus_x], ax
        mov     ax, [city286_roto_sin]
        add     ax, [city286_roto_cos]
        mov     bx, CITY_FINAL_ROTOZOOM_FOCUS_Y
        imul    bx
        mov     cx, ax
        shr     cx, 8
        shl     dx, 8
        or      cx, dx
        mov     ax, CITY_SCREEN_CY
        sub     ax, cx
        mov     [city286f_roto_focus_y], ax
        mov     ax, [city286_roto_phase]
        sub     ax, (CITY_FINAL_ROTOZOOM_FOCUS_START-CITY_FINAL_ROTOZOOM_START)
        mov     bx, 63
        call    city286f_smoothstep
        mov     [city286f_roto_focus_ease], ax
        mov     ax, [city286_roto_cx]
        mov     bx, [city286f_roto_focus_x]
        mov     cx, [city286f_roto_focus_ease]
        call    city286f_lerp_q8
        mov     [city286_roto_cx], ax
        mov     ax, [city286_roto_cy]
        mov     bx, [city286f_roto_focus_y]
        mov     cx, [city286f_roto_focus_ease]
        call    city286f_lerp_q8
        mov     [city286_roto_cy], ax
.centre_ready:
        cmp     word [scene_frame], CITY_FINAL_ROTOZOOM_SINGLE_TILE_START
        jb      .panel
        call    city286f_roto_fill_focused_tile
        jmp     .done
.panel:
        call    city286f_roto_fill_cells
        mov     byte [draw_color], 6
        call    line_batch_begin
        mov     si, -CITY_FINAL_ROTOZOOM_HALF
.vloop:
        mov     ax, si
        mov     bx, -CITY_FINAL_ROTOZOOM_HALF
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286_roto_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286_roto_y0], ax
        mov     ax, si
        mov     bx, CITY_FINAL_ROTOZOOM_HALF
        call    city286_roto_transform
        mov     ax, [city286_roto_x0]
        mov     bx, [city286_roto_y0]
        mov     cx, [city286_project_x]
        mov     dx, [city286_project_y]
        call    city286f_roto_draw_clipped_line
        add     si, CITY_FINAL_ROTOZOOM_STEP
        cmp     si, CITY_FINAL_ROTOZOOM_HALF
        jg      short .__jcc286_skip_059
        jmp     near .vloop
.__jcc286_skip_059:
        mov     byte [draw_color], 5
        call    line_batch_begin
        mov     si, -CITY_FINAL_ROTOZOOM_HALF
.hloop:
        mov     ax, -CITY_FINAL_ROTOZOOM_HALF
        mov     bx, si
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286_roto_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286_roto_y0], ax
        mov     ax, CITY_FINAL_ROTOZOOM_HALF
        mov     bx, si
        call    city286_roto_transform
        mov     ax, [city286_roto_x0]
        mov     bx, [city286_roto_y0]
        mov     cx, [city286_project_x]
        mov     dx, [city286_project_y]
        call    city286f_roto_draw_clipped_line
        add     si, CITY_FINAL_ROTOZOOM_STEP
        cmp     si, CITY_FINAL_ROTOZOOM_HALF
        jg      short .__jcc286_skip_060
        jmp     near .hloop
.__jcc286_skip_060:
        ; Centre axes, as in source.
        mov     byte [draw_color], 7
        call    line_batch_begin
        xor     ax, ax
        mov     bx, -CITY_FINAL_ROTOZOOM_HALF
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286_roto_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286_roto_y0], ax
        xor     ax, ax
        mov     bx, CITY_FINAL_ROTOZOOM_HALF
        call    city286_roto_transform
        mov     ax, [city286_roto_x0]
        mov     bx, [city286_roto_y0]
        mov     cx, [city286_project_x]
        mov     dx, [city286_project_y]
        call    city286f_roto_draw_clipped_line
        mov     ax, -CITY_FINAL_ROTOZOOM_HALF
        xor     bx, bx
        call    city286_roto_transform
        mov     ax, [city286_project_x]
        mov     [city286_roto_x0], ax
        mov     ax, [city286_project_y]
        mov     [city286_roto_y0], ax
        mov     ax, CITY_FINAL_ROTOZOOM_HALF
        xor     bx, bx
        call    city286_roto_transform
        mov     ax, [city286_roto_x0]
        mov     bx, [city286_roto_y0]
        mov     cx, [city286_project_x]
        mov     dx, [city286_project_y]
        call    city286f_roto_draw_clipped_line
.done:
        popa
        ret

; --------------------------------------------------------------------------
; Faithful scene routes
; --------------------------------------------------------------------------
scene_route_city_approach:
        call    city286f_camera_begin
        ; The final-orbit -> opening camera rebase is armed in select_scene
        ; when the selected chapter changes to scene 0.  Do not depend on
        ; scene_frame==0 here: AFS can skip that exact frame under load.
        call    city286f_calc_route_travel
        mov     ax, CITY_APPROACH_CAMERA_Z
        call    city286f_set_camera_z
        cmp     word [scene_frame], 448
        jb      short .__jcc286_skip_087
        jmp     near .turn
.__jcc286_skip_087:
        ; x 1400 -> 500 over frame 0..447
        mov     ax, [scene_frame]
        mov     bx, 900
        mul     bx
        mov     bx, 447
        div     bx
        mov     dx, 1400
        sub     dx, ax
        mov     [city286_cam_x], dx
        mov     word [city286_cam_y], CITY_APPROACH_CAMERA_Y
        mov     byte [city286_yaw], CITY_APPROACH_CAMERA_YAW
        mov     byte [city286_pitch], CITY_APPROACH_CAMERA_PITCH
        mov     byte [city286_roll], 0
        jmp     .matrix
.turn:
        mov     ax, [scene_frame]
        sub     ax, 448
        mov     bx, 191
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     bx, 500
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     ax, 500
        sub     ax, bx
        mov     [city286_cam_x], ax
        cmp     word [scene_frame], 512
        jae     .dive
        mov     word [city286_cam_y], CITY_APPROACH_CAMERA_Y
        jmp     .height
.dive:
        mov     ax, [scene_frame]
        sub     ax, 512
        mov     bx, 127
        call    city286f_smoothstep
        mov     bx, 1655
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     ax, 1800
        sub     ax, bx
        mov     [city286_cam_y], ax
.height:
        mov     ax, [city_transition_ease]
        mov     bx, 24
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     ax, bx
        sub     al, 24
        mov     [city286_yaw], al
        cmp     word [scene_frame], 512
        jae     short .__jcc286_skip_061
        jmp     near .pitch_pre
.__jcc286_skip_061:
        cmp     word [scene_frame], 576
        jb      short .__jcc286_skip_088
        jmp     near .pitch_rec
.__jcc286_skip_088:
        mov     ax, [scene_frame]
        sub     ax, 512
        mov     bx, 63
        call    city286f_smoothstep
        mov     bx, 17
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     al, 249
        sub     al, bl
        mov     [city286_pitch], al
        jmp     .bank
.pitch_rec:
        mov     ax, [scene_frame]
        sub     ax, 576
        mov     bx, 63
        call    city286f_smoothstep
        mov     bx, 22
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     al, 232
        add     al, bl
        mov     [city286_pitch], al
        jmp     .bank
.pitch_pre:
        mov     ax, [city_transition_ease]
        mov     bx, 2
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     al, 249
        add     al, bl
        mov     [city286_pitch], al
.bank:
        mov     ax, [scene_frame]
        sub     ax, 448
        cmp     ax, 96
        jae     .bank_rec
        mov     bx, 28
        mul     bx
        mov     bx, 96
        div     bx
        neg     al
        mov     [city286_roll], al
        jmp     .matrix
.bank_rec:
        sub     ax, 96
        mov     bx, 18
        mul     bx
        mov     bx, 95
        div     bx
        sub     al, 28
        mov     [city286_roll], al
.matrix:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        mov     word [city_road_half], 260
        mov     word [city_road_y], 0
        mov     word [city_road_far], 4600
        mov     word [city_road_spacing], 256
        mov     word [city_road_cycle], 3584
        mov     byte [city_road_cross_count], 14
        mov     ax, [city_route_travel]
        mov     bx, 3584
        call    city286f_phase_mod
        mov     [city_road_phase], ax
        call    city286f_draw_road_grid
        mov     word [city_row_left_x], -650
        mov     word [city_row_right_x], 350
        mov     word [city_row_y], 0
        mov     word [city_row_width], 280
        mov     word [city_row_depth], 220
        mov     word [city_row_height_bias], 260
        mov     word [city_row_spacing], 300
        mov     word [city_row_near], 170
        mov     byte [city_row_count], 12
        mov     byte [city_row_color_base], 2
        mov     ax, [city_route_travel]
        mov     [city_row_phase], ax
        mov     word [city_row_z_limit], 0
        call    city286f_draw_street_rows
        ; Match the 386 renderer: introduce the lamps halfway through the
        ; large 448..639 approach camera move, where the bank/dive masks pop-in.
        cmp     word [scene_frame], 544
        jb      .approach_lamps_done
        call    city286f_draw_canyon_lamps_static
.approach_lamps_done:
        ret

scene_route_tower_canyon:
        call    city286f_camera_begin
        cmp     word [scene_frame], 480
        jb      short .__jcc286_skip_062
        jmp     near .align
.__jcc286_skip_062:
        mov     ax, CITY_CANYON_CAMERA_Z
        call    city286f_set_camera_z
        mov     word [city286_cam_y], CITY_CANYON_CAMERA_Y
        mov     word [city286_cam_x], 0
        mov     byte [city286_yaw], 0
        mov     byte [city286_pitch], CITY_CANYON_CAMERA_PITCH
        cmp     word [scene_frame], 96
        jae     .level
        mov     ax, [scene_frame]
        mov     bx, 10
        mul     bx
        mov     bx, 95
        div     bx
        sub     al, 10
        mov     [city286_roll], al
        jmp     .matrix
.level:
        mov     byte [city286_roll], 0
        jmp     .matrix
.align:
        mov     byte [city286f_camera_z_direct], 1
        mov     ax, [scene_frame]
        sub     ax, 480
        mov     bx, 159
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     word [city286_cam_x], 0
        mov     ax, CITY_CANYON_CAMERA_Z
        call    city286f_set_camera_z
        mov     byte [city286_yaw], 0
        mov     byte [city286_roll], 0
        mov     ax, [city_transition_ease]
        mov     bx, 10
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        add     bx, 145
        mov     [city286_cam_y], bx
        mov     ax, [city_transition_ease]
        shr     ax, 8
        mov     dl, 254
        sub     dl, al
        mov     [city286_pitch], dl
.matrix:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        call    city286f_calc_route_travel
        mov     word [city_road_half], 260
        mov     word [city_road_y], 0
        mov     word [city_road_far], 4600
        mov     word [city_road_spacing], 256
        mov     word [city_road_cycle], 3584
        mov     byte [city_road_cross_count], 14
        mov     ax, [city_route_travel]
        mov     bx, 3584
        call    city286f_phase_mod
        mov     [city_road_phase], ax
        call    city286f_draw_road_grid
        mov     word [city_row_left_x], -650
        mov     word [city_row_right_x], 350
        mov     word [city_row_y], 0
        mov     word [city_row_width], 280
        mov     word [city_row_depth], 220
        mov     word [city_row_height_bias], 260
        mov     word [city_row_spacing], 300
        mov     word [city_row_near], 170
        mov     byte [city_row_count], 12
        mov     byte [city_row_color_base], 2
        mov     ax, [city_route_travel]
        mov     [city_row_phase], ax
        mov     word [city_row_z_limit], 0
        cmp     word [scene_frame], 480
        jb      .rows
        mov     ax, [scene_frame]
        sub     ax, 480
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        neg     ax
        add     ax, 3365
        mov     [city_row_z_limit], ax
.rows:
        call    city286f_draw_street_rows
        call    city286f_draw_canyon_lamps
        mov     word [city_row_z_limit], 0
        cmp     word [scene_frame], 480
        jb      .done
        mov     ax, [scene_frame]
        sub     ax, 480
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        neg     ax
        add     ax, 3365
        mov     [city_transition_seg_z0], ax
        add     ax, 11015
        mov     [city_transition_seg_z1], ax
        call    city286f_draw_bridge_fixed
.done:
        ret

scene_route_megabridge:
        call    city286f_camera_begin
        mov     byte [city286f_camera_z_direct], 1
        mov     ax, [scene_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        sub     ax, 50
        call    city286f_set_camera_z
        mov     word [city286_cam_x], 0
        mov     word [city286_cam_y], CITY_BRIDGE_CAMERA_Y
        mov     byte [city286_yaw], 0
        mov     byte [city286_roll], 0
        mov     byte [city286_pitch], CITY_BRIDGE_CAMERA_PITCH
        cmp     word [scene_frame], 384
        jae     short .__jcc286_skip_063
        jmp     near .matrix
.__jcc286_skip_063:
        mov     ax, [scene_frame]
        sub     ax, 384
        mov     bx, 127
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     bx, 65
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        add     bx, 155
        mov     [city286_cam_y], bx
        mov     ax, [city_transition_ease]
        mov     bx, 2
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     al, 253
        sub     al, bl
        mov     [city286_pitch], al
.matrix:
        cmp     word [scene_frame], 384
        jb      .matrix_integer
        mov     word [city_cam_yaw_phase_target], 0
        mov     word [city_cam_roll_phase_target], 0
        mov     ax, [city_transition_ease]
        mov     bx, 2
        mul     bx                      ; Q8 ease * 2 => 8.8 phase delta
        mov     dx, (253 << 8)
        sub     dx, ax
        mov     [city_cam_pitch_phase_target], dx
        mov     byte [city286f_camera_phase_direct], 1
        jmp     .matrix_apply
.matrix_integer:
        ; Before frame 384, source uses the persistent angle smoother.
.matrix_apply:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
.matrix_done:
        cmp     word [scene_frame], 32
        jb      short .__jcc286_skip_064
        jmp     near .tail_done
.__jcc286_skip_064:
        mov     word [city_row_left_x], -650
        mov     word [city_row_right_x], 350
        mov     word [city_row_y], 0
        mov     word [city_row_width], 280
        mov     word [city_row_depth], 220
        mov     word [city_row_height_bias], 260
        mov     word [city_row_spacing], 300
        mov     word [city_row_near], 170
        mov     byte [city_row_count], 12
        mov     byte [city_row_color_base], 2
        mov     ax, [city_global_frame]
        sub     ax, [scene_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        mov     [city_row_phase], ax
        mov     word [city_row_z_limit], 185
        call    city286f_draw_street_rows
        mov     word [city_row_z_limit], 0
.tail_done:
        mov     word [city_transition_seg_z0], 185
        mov     word [city_transition_seg_z1], 11200
        call    city286f_draw_bridge_fixed
        cmp     word [scene_frame], 320
        jb      .done
        mov     word [city_route_center_x], 0
        mov     word [city_transition_seg_z0], 11200
        mov     word [city_transition_seg_z1], 13000
        mov     word [city_transition_seg_half], 310
        mov     word [city_transition_seg_y], 20
        mov     word [city_transition_seg_spacing], 256
        call    city286f_draw_flat_road
        mov     word [city_route_model_x], 1250
        mov     word [city_route_model_z], 11692
        call    city286f_draw_park_fixed
.done:
        ret

scene_route_central_park:
        call    city286f_camera_begin
        ; Keep the 286 camera in the megabridge coordinate frame instead of
        ; jumping Z by -4092 at the scene boundary.  Every park-local model and
        ; camera Z receives the same bias in city286f_draw_world_line /
        ; city286f_set_camera_z, so relative geometry is bit-for-bit unchanged.
        ; Bridge frame 511: Z=10170.  Park frame 0: 6078+4092=10170.
        mov     word [city286f_scene_z_bias], CITY_PARK_SCENE_Z_BIAS
        mov     byte [city286f_camera_view_direct], 1
        cmp     word [scene_frame], 128
        jb      short .__jcc286_skip_065
        jmp     near .orbit_test
.__jcc286_skip_065:
        mov     ax, [scene_frame]
        mov     bx, 256
        mul     bx
        mov     bx, 127
        div     bx
        mov     [city286f_bezier_t], ax
        ; X 0,0,200,200
        mov     word [city286f_bezier_p0], 0
        mov     word [city286f_bezier_p1], 0
        mov     word [city286f_bezier_p2], 200
        mov     word [city286f_bezier_p3], 200
        call    city286f_bezier16
        mov     [city286_cam_x], ax
        ; Z 6078,6925,7163,7600
        mov     word [city286f_bezier_p0], 6078
        mov     word [city286f_bezier_p1], 6925
        mov     word [city286f_bezier_p2], 7163
        mov     word [city286f_bezier_p3], 7600
        call    city286f_bezier16
        call    city286f_set_camera_z
        ; Y 220,220,640,640
        mov     word [city286f_bezier_p0], 220
        mov     word [city286f_bezier_p1], 220
        mov     word [city286f_bezier_p2], 640
        mov     word [city286f_bezier_p3], 640
        call    city286f_bezier16
        mov     [city286_cam_y], ax
        ; yaw phase 0,0,12043,64<<8
        mov     word [city286f_bezier_p0], 0
        mov     word [city286f_bezier_p1], 0
        mov     word [city286f_bezier_p2], 12043
        mov     word [city286f_bezier_p3], (64 << 8)
        call    city286f_bezier16
        mov     [city_cam_yaw_phase_target], ax
        ; pitch phase 251 -> 249 (signed representation is intentional)
        mov     word [city286f_bezier_p0], (251 << 8)
        mov     word [city286f_bezier_p1], (251 << 8)
        mov     word [city286f_bezier_p2], (249 << 8)
        mov     word [city286f_bezier_p3], (249 << 8)
        call    city286f_bezier16
        mov     [city_cam_pitch_phase_target], ax
        mov     word [city_cam_roll_phase_target], 0
        jmp     .matrix
.orbit_test:
        cmp     word [scene_frame], 768
        jb      short .__jcc286_skip_066
        jmp     near .exit
.__jcc286_skip_066:
        mov     ax, [scene_frame]
        sub     ax, 128
        cmp     ax, 639
        je      .orbit_exact_end
        ; quotient = elapsed*65536/639 using DX:AX as the 32-bit dividend.
        mov     dx, ax
        xor     ax, ax
        mov     bx, 639
        div     bx
        mov     dx, 0C000h
        sub     dx, ax
        jmp     .orbit_phase_ready
.orbit_exact_end:
        mov     dx, 0C000h
.orbit_phase_ready:
        mov     [city286f_orbit_phase], dx
        mov     ax, dx
        call    city286f_sin_phase
        mov     bx, 1050
        call    city286_mul_q14
        add     ax, CITY_PARK_ORBIT_CENTER_X
        mov     [city286_cam_x], ax
        mov     ax, [city286f_orbit_phase]
        call    city286f_cos_phase
        mov     bx, -1050
        call    city286_mul_q14
        add     ax, CITY_PARK_ORBIT_CENTER_Z
        call    city286f_set_camera_z
        mov     word [city286_cam_y], CITY_PARK_ORBIT_Y
        mov     ax, [city286f_orbit_phase]
        neg     ax
        mov     [city_cam_yaw_phase_target], ax
        mov     word [city_cam_pitch_phase_target], (249 << 8)
        mov     word [city_cam_roll_phase_target], 0
        jmp     .matrix
.exit:
        mov     ax, [scene_frame]
        sub     ax, 768
        mov     bx, 127
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     bx, 700
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        add     bx, 200
        mov     [city286_cam_x], bx
        mov     ax, [scene_frame]
        sub     ax, 768
        mov     bx, 127
        call    city286f_accel4
        mov     bx, 600
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        add     bx, CITY_PARK_ORBIT_CENTER_Z
        mov     ax, bx
        call    city286f_set_camera_z
        ; End the park exit at Y=350 so scene 4 begins at exactly the same
        ; high cruise altitude instead of stepping upward at the boundary.
        mov     ax, [city_transition_ease]
        mov     bx, (CITY_PARK_ORBIT_Y-CITY_NEON_CRUISE_Y_HIGH)
        mul     bx
        mov     bx, ax
        shr     bx, 8
        shl     dx, 8
        or      bx, dx
        mov     ax, CITY_PARK_ORBIT_Y
        sub     ax, bx
        mov     [city286_cam_y], ax
        mov     ax, [city_transition_ease]
        mov     bx, 64
        mul     bx                      ; ease(Q8) * 64 = yaw phase delta
        mov     dx, (64 << 8)
        sub     dx, ax
        mov     [city_cam_yaw_phase_target], dx
        mov     ax, [city_transition_ease]
        mov     bx, 6
        mul     bx                      ; ease(Q8) * 6 = pitch phase delta
        add     ax, (249 << 8)
        mov     [city_cam_pitch_phase_target], ax
        mov     word [city_cam_roll_phase_target], 0
.matrix:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        cmp     word [scene_frame], 128
        jae     .tail
        call    city286f_draw_bridge_tail
.tail:
        mov     word [city_route_center_x], 0
        mov     word [city_transition_seg_z0], 7108
        mov     word [city_transition_seg_z1], 8908
        mov     word [city_transition_seg_half], 310
        mov     word [city_transition_seg_y], 20
        mov     word [city_transition_seg_spacing], 256
        call    city286f_draw_flat_road
        mov     word [city_route_model_x], 1250
        mov     word [city_route_model_z], 7600
        call    city286f_draw_park_fixed
        cmp     word [scene_frame], 768
        jb      .done
        call    city286f_draw_neon_entry_fixed
.done:
        ret

; --------------------------------------------------------------------------
; Remaining faithful scene routes (4..8)
; --------------------------------------------------------------------------

; 286 counterpart of the commercial-district avoidance path.  Entry timing,
; peaks and the right -> left -> right -> down(keep) order match the 386/16/256
; renderer exactly.  Horizontal recovery is deliberately much slower.  The
; nominal target is four times the 28-frame entry; if the following horizontal
; dodge starts first, that start frame wins and the preceding recovery is
; stretched across all available time.  Dodge 3 can continue recovering while
; the final vertical drop runs.
;
; AX=elapsed (0..BX), BX=segment duration. Return AX=cosine ease Q14 and store
; the positive sine attitude envelope Q14 in city_neon_dodge_tilt_q14.
city286f_neon_dodge_curve:
        push    bx
        push    cx
        push    dx
        push    si
        mov     cx, bx
        mov     bx, 8000h
        mul     bx
        div     cx
        cmp     ax, 8000h
        jbe     .phase_ready
        mov     ax, 8000h
.phase_ready:
        mov     si, ax
        call    city286f_sin_phase
        mov     [city_neon_dodge_tilt_q14], ax
        mov     ax, si
        call    city286f_cos_phase
        mov     dx, 16384
        sub     dx, ax
        shr     dx, 1
        mov     ax, dx
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

city286f_prepare_neon_dodge:
        pusha
        mov     word [city_neon_dodge_x], 0
        mov     word [city_neon_dodge_y], 0
        mov     word [city_neon_dodge_roll_phase], 0
        mov     word [city_neon_dodge_pitch_phase], 0
        mov     word [city_neon_dodge_tilt_q14], 0

        ; Horizontal manoeuvres.  Preserve all original dodge start frames;
        ; recovery is what expands into the free time between them.
        mov     ax, [scene_frame]
        cmp     ax, CITY_NEON_DODGE1_START
        jae     .check_left_start
        jmp     near .after_horizontal
.check_left_start:
        cmp     ax, CITY_NEON_DODGE2_START
        jae     .check_right_start
        jmp     near .first_right
.check_right_start:
        cmp     ax, CITY_NEON_DODGE3_START
        jae     .check_right_end
        jmp     near .left_dodge
.check_right_end:
        cmp     ax, CITY_NEON_DODGE3_END
        jbe     .right_dodge
        jmp     near .after_horizontal

.first_right:
        cmp     ax, CITY_NEON_DODGE1_CENTER
        jbe     .first_right_entry
        mov     dx, CITY_NEON_DODGE1_END
        sub     dx, ax
        mov     ax, dx
        mov     bx, CITY_NEON_DODGE1_RETURN_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        mov     [city_neon_dodge_roll_phase], ax
        jmp     .set_first_right_x
.first_right_entry:
        sub     ax, CITY_NEON_DODGE1_START
        mov     bx, CITY_NEON_DODGE_ENTRY_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_roll_phase], ax
.set_first_right_x:
        mov     ax, CITY_NEON_DODGE1_OVERSHOOT_LATERAL
        mov     bx, si
        call    city286_mul_q14
        mov     [city_neon_dodge_x], ax
        jmp     near .after_horizontal

.left_dodge:
        cmp     ax, CITY_NEON_DODGE2_CENTER
        jbe     .left_entry
        mov     dx, CITY_NEON_DODGE2_END
        sub     dx, ax
        mov     ax, dx
        mov     bx, CITY_NEON_DODGE2_RETURN_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_roll_phase], ax
        jmp     .set_left_x
.left_entry:
        sub     ax, CITY_NEON_DODGE2_START
        mov     bx, CITY_NEON_DODGE_ENTRY_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        mov     [city_neon_dodge_roll_phase], ax
.set_left_x:
        mov     ax, CITY_NEON_DODGE2_LATERAL
        mov     bx, si
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_x], ax
        jmp     near .after_horizontal

.right_dodge:
        cmp     ax, CITY_NEON_DODGE3_CENTER
        jbe     .right_entry
        mov     dx, CITY_NEON_DODGE3_END
        sub     dx, ax
        mov     ax, dx
        mov     bx, CITY_NEON_DODGE3_RETURN_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        mov     [city_neon_dodge_roll_phase], ax
        jmp     .set_right_x
.right_entry:
        sub     ax, CITY_NEON_DODGE3_START
        mov     bx, CITY_NEON_DODGE_ENTRY_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_roll_phase], ax
.set_right_x:
        mov     ax, CITY_NEON_DODGE_LATERAL
        mov     bx, si
        call    city286_mul_q14
        mov     [city_neon_dodge_x], ax

.after_horizontal:
        ; The final dodge is vertical and may overlap dodge 3 recovery.
        mov     ax, [scene_frame]
        cmp     ax, CITY_NEON_DODGE4_START
        jae     .check_final_end
        jmp     near .done
.check_final_end:
        cmp     ax, CITY_NEON_DODGE4_END
        jbe     .final_down
        jmp     near .done
.final_down:
        sub     ax, CITY_NEON_DODGE4_START
        mov     bx, CITY_NEON_DODGE_ENTRY_FRAMES
        call    city286f_neon_dodge_curve
        mov     si, ax
        mov     ax, CITY_NEON_DODGE_TILT_PHASE
        mov     bx, [city_neon_dodge_tilt_q14]
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_pitch_phase], ax
        mov     ax, CITY_NEON_DODGE_FINAL_DROP
        mov     bx, si
        call    city286_mul_q14
        neg     ax
        mov     [city_neon_dodge_y], ax
.done:
        popa
        ret

scene_route_neon_district:
        call    city286f_camera_begin
        mov     byte [city286f_camera_z_direct], 1
        mov     word [city286f_scene_z_bias], CITY_NEON_SCENE_REBASE
        mov     word [city286_cam_x], CITY_ROUTE_CENTER_X
        mov     ax, [scene_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        add     ax, CITY_NEON_CAMERA_Z0
        call    city286f_set_camera_z
        ; High sign-intersection cruise first.  After the final dodge, make the
        ; original 115-unit route height the real target through frame 447 and
        ; synchronize the persistent Y view, so skipped AFS render frames cannot
        ; expose the former 350-unit target.  Frame 448 resumes the old descent.
        mov     word [city286_cam_y], CITY_NEON_CRUISE_Y_HIGH
        cmp     word [scene_frame], (CITY_NEON_DODGE4_END+1)
        jb      .angles
        cmp     word [scene_frame], CITY_NEON_TUNNEL_DESCENT_START
        jae     .tunnel_descent
        mov     word [city286_cam_y], CITY_NEON_CRUISE_Y_NORMAL
        mov     word [city286f_view_y], CITY_NEON_CRUISE_Y_NORMAL
        mov     byte [city286f_view_y_frac], 0
        jmp     .angles
.tunnel_descent:
        mov     ax, [scene_frame]
        sub     ax, CITY_NEON_TUNNEL_DESCENT_START
        mov     bx, CITY_NEON_CRUISE_Y_NORMAL
        mul     bx
        mov     bx, CITY_NEON_TUNNEL_DESCENT_FRAMES
        div     bx
        neg     ax
        add     ax, CITY_NEON_CRUISE_Y_NORMAL
        mov     [city286_cam_y], ax
.angles:
        mov     byte [city286_yaw], 0
        mov     byte [city286_roll], 0
        mov     byte [city286_pitch], 255
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        call    city286f_prepare_neon_dodge
        mov     ax, [city_neon_dodge_x]
        add     [city286_cam_x], ax
        mov     ax, [city_neon_dodge_y]
        add     [city286_cam_y], ax
        mov     ax, [city_neon_dodge_pitch_phase]
        add     [city_cam_pitch_phase_target], ax
        mov     ax, [city_neon_dodge_roll_phase]
        add     [city_cam_roll_phase_target], ax
        call    city286f_update_matrix_phase

        call    city286f_draw_neon_fixed
        cmp     word [scene_frame], CITY_NEON_TUNNEL_DESCENT_START
        jae     short .__jcc286_skip_089
        jmp     near .done
.__jcc286_skip_089:
        ; Shared tunnel is shifted by +5948 in the normal edition.  Keep the
        ; scene-wide -6000 rebase, so the effective 286 model shift is -52.
        mov     word [city286f_model_z_bias], 5948
        cmp     word [scene_frame], CITY_NEON_TUNNEL_ENTRY_FRAME
        jae     .portal_done
        call    city286f_draw_tunnel_portal
.portal_done:
        cmp     word [scene_frame], 512
        jb      .reset_bias
        call    city286f_draw_tunnel_fixed
.reset_bias:
        mov     word [city286f_model_z_bias], 0
.done:
        ; Do not feed the temporary avoidance displacement back into the 286
        ; persistent smoother.
        mov     ax, [city_neon_dodge_x]
        sub     [city286_cam_x], ax
        mov     ax, [city_neon_dodge_y]
        sub     [city286_cam_y], ax
        mov     ax, [city_neon_dodge_pitch_phase]
        sub     [city_cam_pitch_phase_target], ax
        mov     ax, [city_neon_dodge_roll_phase]
        sub     [city_cam_roll_phase_target], ax
        ret

scene_route_subterranean:
        call    city286f_camera_begin
        mov     byte [city286f_camera_z_direct], 1
        ; Straight start, then the same broad right-hand bend as the source.
        mov     ax, [scene_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        add     ax, CITY_TUNNEL_CAMERA_Z0
        call    city286f_set_camera_z
        mov     word [city286_cam_x], CITY_ROUTE_CENTER_X
        mov     byte [city286_yaw], 0
        mov     byte [city286_roll], 0
        mov     byte [city286_pitch], 0

        cmp     word [scene_frame], CITY_TUNNEL_CURVE_FRAME_START
        jae     short .__jcc286_skip_067
        jmp     near .vertical
.__jcc286_skip_067:
        mov     ax, [scene_frame]
        sub     ax, CITY_TUNNEL_CURVE_FRAME_START
        mov     [city_tunnel_curve_frame], ax
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        mov     [city286f_curve_s], ax

        ; x = 900 + s^2/40000.  s <= 6380, so 16x16->32 is sufficient.
        mov     bx, ax
        mul     bx
        mov     bx, CITY_TUNNEL_CURVE_DENOM
        div     bx
        add     ax, 900
        mov     [city286_cam_x], ax

        mov     ax, [city_tunnel_curve_frame]
        mov     bx, CITY_TUNNEL_CURVE_EXIT_YAW
        mul     bx
        mov     bx, (639-CITY_TUNNEL_CURVE_FRAME_START)
        div     bx
        mov     [city286_yaw], al

        mov     ax, [city_tunnel_curve_frame]
        cmp     ax, 160
        jae     .bank_recover
        mov     bx, CITY_TUNNEL_CURVE_EXIT_ROLL
        mul     bx
        mov     bx, 160
        div     bx
        neg     al
        mov     [city286_roll], al
        jmp     .vertical
.bank_recover:
        sub     ax, 160
        mov     bx, CITY_TUNNEL_CURVE_EXIT_ROLL
        mul     bx
        mov     bx, (319-160)
        div     bx
        mov     dl, CITY_TUNNEL_CURVE_EXIT_ROLL
        sub     dl, al
        neg     dl
        mov     [city286_roll], dl

.vertical:
        mov     word [city286_cam_y], 0
        cmp     word [scene_frame], 544
        jb      .camera_ready
        mov     ax, [scene_frame]
        sub     ax, 544
        mov     bx, 135
        mul     bx
        mov     bx, 95
        div     bx
        mov     [city286_cam_y], ax
.camera_ready:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        ; Draw the glare first, then the near ribs over it, exactly as source.
        call    city286f_draw_tunnel_exit_white
        call    city286f_draw_tunnel_curved
        ret

scene_route_rail_terminal:
        call    city286f_camera_begin
        mov     byte [city286f_camera_z_direct], 1
        mov     word [city286_cam_x], CITY_ROUTE_CENTER_X

        cmp     word [scene_frame], CITY_RAIL_CAMERA_ACCEL_START
        jae     short .__jcc286_skip_090
        jmp     near .rail_cruise
.__jcc286_skip_090:
        cmp     word [scene_frame], CITY_RAIL_CAMERA_ACCEL_END
        jb      short .__jcc286_skip_068
        jmp     near .rail_fast
.__jcc286_skip_068:
        mov     ax, [scene_frame]
        sub     ax, CITY_RAIL_CAMERA_ACCEL_START
        mov     bx, 256
        mul     bx
        mov     bx, CITY_RAIL_CAMERA_ACCEL_END-CITY_RAIL_CAMERA_ACCEL_START
        div     bx
        mov     [city286f_bezier_t], ax
        mov     word [city286f_bezier_p0], 11520
        mov     word [city286f_bezier_p1], 12800
        mov     word [city286f_bezier_p2], 14784
        mov     word [city286f_bezier_p3], 17472
        call    city286f_bezier16
        call    city286f_set_camera_z
        jmp     .height
.rail_cruise:
        mov     ax, [scene_frame]
        mov     bx, CITY_ROUTE_CRUISE_SPEED
        mul     bx
        call    city286f_set_camera_z
        jmp     .height
.rail_fast:
        mov     ax, [scene_frame]
        sub     ax, CITY_RAIL_CAMERA_ACCEL_END
        mov     bx, CITY_RAIL_CAMERA_FAST_SPEED
        mul     bx
        add     ax, CITY_RAIL_CAMERA_Z0
        call    city286f_set_camera_z
.height:
        mov     word [city286_cam_y], CITY_RAIL_CAMERA_Y
        mov     byte [city286_pitch], CITY_RAIL_CAMERA_PITCH
        cmp     word [scene_frame], 768
        jae     .terminal_alignment
        mov     byte [city286_yaw], 0
        mov     byte [city286_roll], 0
        jmp     .pressure
.terminal_alignment:
        mov     ax, [scene_frame]
        sub     ax, 768
        mov     bx, 127
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     bx, 55
        call    city286_mul_q8
        add     ax, 135
        mov     [city286_cam_y], ax
        mov     ax, [city_transition_ease]
        mov     bx, 3
        call    city286_mul_q8
        mov     dl, 3
        sub     dl, al
        mov     [city286_yaw], dl
        mov     byte [city286_roll], 0
        mov     ax, [city_transition_ease]
        mov     bx, 2
        call    city286_mul_q8
        mov     dl, 254
        sub     dl, al
        mov     [city286_pitch], dl

.pressure:
        ; One-shot opposing-train pressure pulse.  The source applies this only
        ; to the rendered view; direct 286 camera state is rebuilt every frame,
        ; so adding it here is equivalent without contaminating later frames.
        mov     word [city286f_pressure_x], 0      ; view-only X pulse
        mov     word [city286f_pressure_roll], 0      ; view-only roll magnitude
        mov     ax, [scene_frame]
        cmp     ax, CITY_RAIL_PASS_SWERVE_START
        jae     short .__jcc286_skip_069
        jmp     near .pressure_ready
.__jcc286_skip_069:
        cmp     ax, CITY_RAIL_PASS_SWERVE_END
        jbe     short .__jcc286_skip_070
        jmp     near .pressure_ready
.__jcc286_skip_070:
        cmp     ax, CITY_RAIL_PASS_SWERVE_PEAK
        ja      .pressure_fall
        sub     ax, CITY_RAIL_PASS_SWERVE_START
        mov     bx, CITY_RAIL_PASS_SWERVE_PEAK-CITY_RAIL_PASS_SWERVE_START
        jmp     .pressure_ease
.pressure_fall:
        mov     dx, CITY_RAIL_PASS_SWERVE_END
        sub     dx, ax
        mov     ax, dx
        mov     bx, CITY_RAIL_PASS_SWERVE_END-CITY_RAIL_PASS_SWERVE_PEAK
.pressure_ease:
        call    city286f_smoothstep
        mov     [city_transition_ease], ax
        mov     bx, CITY_RAIL_PASS_SWERVE_X
        call    city286_mul_q8
        mov     [city286f_pressure_x], ax
        mov     ax, [city_transition_ease]
        mov     bx, CITY_RAIL_PASS_ROLL
        call    city286_mul_q8
        mov     [city286f_pressure_roll], ax
.pressure_ready:
        ; Smooth only the normal camera target.  The wind-pressure displacement
        ; and bank are then added to the rendered view, so the persistent
        ; smoother never learns the temporary kick (same as CITY3D256).
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        mov     ax, [city286f_pressure_x]
        add     [city286_cam_x], ax
        mov     ax, [city286f_view_yaw_phase]
        mov     [city_cam_yaw_phase_target], ax
        mov     ax, [city286f_view_pitch_phase]
        mov     [city_cam_pitch_phase_target], ax
        mov     ax, [city286f_pressure_roll]
        shl     ax, 8
        mov     dx, [city286f_view_roll_phase]
        sub     dx, ax                  ; opposite bank direction, view only
        mov     [city_cam_roll_phase_target], dx
        call    city286f_update_matrix_phase

        mov     word [city286f_model_z_bias], CITY_RAIL_SCENE_Z_BIAS
        call    city286f_draw_rail_fixed

        cmp     word [scene_frame], CITY_RAIL_ONCOMING_START
        jb      .oncoming_done
        cmp     word [scene_frame], CITY_RAIL_ONCOMING_END
        jae     .oncoming_done
        mov     ax, [scene_frame]
        mov     bx, CITY_RAIL_ONCOMING_SPEED
        mul     bx
        neg     ax
        add     ax, CITY_RAIL_ONCOMING_START_Z
        mov     [city_oncoming_train_phase], ax
        call    city286f_draw_oncoming_train_fixed
.oncoming_done:
        cmp     word [scene_frame], CITY_RAIL_TRAIN_DELAY
        jb      .terminal_test
        mov     ax, [scene_frame]
        sub     ax, CITY_RAIL_TRAIN_DELAY
        mov     bx, CITY_RAIL_TRAIN_SPEED
        mul     bx
        add     ax, 900
        mov     [city_train_phase], ax
        call    city286f_draw_train_fixed
.terminal_test:
        cmp     word [scene_frame], 704
        jb      .reset_bias
        mov     word [city286f_model_z_bias], -4800
        call    city286f_draw_terminal_fixed
        ; Carry the landmark into the railway -> terminal preview too.
        call    city286f_draw_terminal_landmark
.reset_bias:
        mov     word [city286f_model_z_bias], 0
        ret

scene_route_terminal_ascent:
        call    city286f_camera_begin
        mov     byte [city286f_camera_z_direct], 1
        mov     word [city286f_scene_z_bias], CITY_FINAL_SCENE_REBASE
        cmp     word [scene_frame], CITY_TERMINAL_TURN_START
        jb      short .__jcc286_skip_091
        jmp     near .launch
.__jcc286_skip_091:
        mov     word [city286_cam_x], CITY_ROUTE_CENTER_X
        mov     ax, [scene_frame]
        mov     bx, CITY_RAIL_CAMERA_FAST_SPEED
        mul     bx
        add     ax, CITY_TERMINAL_CAMERA_Z0
        call    city286f_set_camera_z
        mov     ax, [scene_frame]
        shr     ax, 1
        add     ax, CITY_TERMINAL_CAMERA_Y0
        mov     [city286_cam_y], ax
        mov     byte [city286_yaw], 0
        mov     byte [city286_pitch], CITY_TERMINAL_CAMERA_PITCH
        mov     byte [city286_roll], 0
        jmp     .matrix
.launch:
        mov     ax, [scene_frame]
        sub     ax, CITY_TERMINAL_TURN_START
        cmp     ax, CITY_TERMINAL_DECEL_FRAMES
        jb      short .__jcc286_skip_092
        jmp     near .launch_z_cruise
.__jcc286_skip_092:
        mov     bx, 256
        mul     bx
        mov     bx, CITY_TERMINAL_DECEL_FRAMES
        div     bx
        mov     [city286f_bezier_t], ax
        mov     word [city286f_bezier_p0], 28944
        mov     word [city286f_bezier_p1], 29840
        mov     word [city286f_bezier_p2], 30384
        mov     word [city286f_bezier_p3], 30576
        call    city286f_bezier16
        call    city286f_set_camera_z
        jmp     .launch_z_ready
.launch_z_cruise:
        sub     ax, CITY_TERMINAL_DECEL_FRAMES
        mov     bx, 9
        mul     bx
        add     ax, 30576
        call    city286f_set_camera_z
.launch_z_ready:
        mov     ax, [scene_frame]
        sub     ax, CITY_TERMINAL_TURN_START
        mov     bx, 2615
        mul     bx
        mov     bx, CITY_TERMINAL_TURN_LAST
        div     bx
        add     ax, 206
        mov     [city286_cam_y], ax

        mov     ax, [scene_frame]
        sub     ax, CITY_TERMINAL_TURN_START
        mov     bx, 1200
        mul     bx
        mov     bx, CITY_TERMINAL_TURN_LAST
        div     bx
        neg     ax
        add     ax, 900
        mov     [city286_cam_x], ax

        mov     ax, [scene_frame]
        sub     ax, CITY_TERMINAL_TURN_START
        mov     bx, 128
        mul     bx
        mov     bx, CITY_TERMINAL_TURN_LAST
        div     bx
        mov     [city286_yaw], al

        mov     ax, [scene_frame]
        sub     ax, CITY_TERMINAL_TURN_START
        mov     bx, 8
        mul     bx
        mov     bx, CITY_TERMINAL_TURN_LAST
        div     bx
        mov     dl, 252
        sub     dl, al
        mov     [city286_pitch], dl

        mov     al, [city286_yaw]
        mov     bx, 10
        call    city286_get_sin_scaled
        mov     [city286_roll], al
.matrix:
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        ; Keep the terminal-length rails continuous across the railway -> terminal rebase.
        ; The terminal model shifts by 4800 units relative to the railway
        ; chapter; without the matching +4188 model bias the far track appears
        ; to be cut off suddenly at the scene boundary.
        mov     byte [city_rail_track_only], 1
        mov     word [city286f_model_z_bias], CITY_TERMINAL_RAIL_Z_BIAS
        call    city286f_draw_rail_fixed
        mov     word [city286f_model_z_bias], 0
        mov     byte [city_rail_track_only], 0
        call    city286f_draw_terminal_fixed
        cmp     word [scene_frame], CITY_TERMINAL_TURN_START
        jae     .draw_final_city
        ; Keep the landmark present from the beginning of the terminal chapter.
        call    city286f_draw_terminal_landmark
        jmp     .done
.draw_final_city:
        ; The final-city routine includes the same landmark, avoiding a model
        ; change when the turn begins.
        call    city286f_draw_final_city
.done:
        ret

scene_route_city_overlook:
        cmp     word [scene_frame], CITY_FINAL_COVER_REUSE_START
        jb      short .__jcc286_skip_071
        jmp     near .covered_surface_only
.__jcc286_skip_071:
        call    city286f_camera_begin
        mov     word [city286f_scene_z_bias], CITY_FINAL_SCENE_REBASE

        mov     ax, [scene_frame]
        mov     bx, 256
        mul     bx
        mov     bx, 896
        div     bx
        add     al, 128
        mov     [city_orbit_angle], al

        call    city286_get_sin
        mov     bx, 4000
        call    city286_mul_q14
        sub     ax, 300
        mov     [city286_cam_x], ax

        mov     al, [city_orbit_angle]
        call    city286_get_cos
        mov     bx, -4000
        call    city286_mul_q14
        add     ax, CITY_FINAL_CAMERA_Z_BASE
        call    city286f_set_camera_z

        mov     word [city286_cam_y], CITY_FINAL_CAMERA_Y
        mov     al, [city_orbit_angle]
        neg     al
        mov     [city286_yaw], al
        mov     byte [city286_pitch], CITY_FINAL_CAMERA_PITCH
        mov     byte [city286_roll], 0
        call    city286f_update_camera_matrix
        cmp     byte [city_camera_replay], 0
        je      .camera_replay_continue
        ret
.camera_replay_continue:
        call    city286f_draw_terminal_fixed
        call    city286f_draw_final_city
        call    city286_draw_final_rotozoom
        ret
.covered_surface_only:
        cmp     byte [city_camera_replay], 0
        jne     .covered_replay_done
        call    city286_draw_final_rotozoom
.covered_replay_done:
        ret
