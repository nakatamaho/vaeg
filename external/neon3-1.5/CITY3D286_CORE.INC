; ===========================================================================
; CITY3D286.INC - NEON 3 scene renderer for the Intel 80286
; ===========================================================================
;
; This is not a mechanical downgrade of CITY3D256.INC.  The 386 renderer uses
; 32-bit camera state throughout, while this file deliberately keeps every
; active scene in a rebased signed-16-bit world.  16x16 IMUL produces DX:AX;
; Q2.14 and Q8 products are then reduced with shifts available on the 80286.
;
; Logical screen space is always 640x400.  VIDEO3_286.INC performs the final
; Y/2 conversion only when the physical mode is 640x200.
; ===========================================================================

; ---------------------------------------------------------------------------
; Fixed-point helpers
; ---------------------------------------------------------------------------
; AL=angle 0..255 -> AX=sin(angle), Q2.14. Preserves BX.
city286_get_sin:
        push    bx
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        pop     bx
        ret

city286_get_cos:
        add     al, 64
        jmp     city286_get_sin

; AX=signed value, BX=signed Q2.14 value -> AX=(AX*BX)>>14.
; The low 16 bits of the signed 32-bit arithmetic shift are
; (DX<<2)|(AX>>14), so no 386 SHRD is necessary.
city286_mul_q14:
        imul    bx
        mov     cx, ax
        shr     cx, 14
        shl     dx, 2
        or      cx, dx
        mov     ax, cx
        ret

; AX=signed value, BX=signed Q8 value -> AX=(AX*BX)>>8.
city286_mul_q8:
        imul    bx
        mov     cx, ax
        shr     cx, 8
        shl     dx, 8
        or      cx, dx
        mov     ax, cx
        ret

; AL=angle, BX=signed amplitude -> AX=scaled sine.
city286_get_sin_scaled:
        call    city286_get_sin
        xchg    ax, bx
        ; BX now contains Q14 sine and AX amplitude.
        call    city286_mul_q14
        ret

city286_camera_clear:
        mov     word [city286_cam_x], 0
        mov     word [city286_cam_y], 0
        mov     word [city286_cam_z], 0
        mov     byte [city286_yaw], 0
        mov     byte [city286_pitch], 0
        mov     byte [city286_roll], 0
        ret

city286_update_matrix:
        push    ax
        mov     al, [city286_yaw]
        call    city286_get_sin
        mov     [city286_sin_yaw], ax
        mov     al, [city286_yaw]
        call    city286_get_cos
        mov     [city286_cos_yaw], ax
        mov     al, [city286_pitch]
        call    city286_get_sin
        mov     [city286_sin_pitch], ax
        mov     al, [city286_pitch]
        call    city286_get_cos
        mov     [city286_cos_pitch], ax
        mov     al, [city286_roll]
        call    city286_get_sin
        mov     [city286_sin_roll], ax
        mov     al, [city286_roll]
        call    city286_get_cos
        mov     [city286_cos_roll], ax
        pop     ax
        ret

; AX=signed camera-space component, BX=positive depth.
; Return AX=(component*CITY_FOCAL)/depth, saturated to +/-8191 before
; division if the quotient would leave the useful logical-screen range.
; This also prevents 16-bit DIV/IDIV overflow for near-plane tunnel points.
city286_project_component_safe:
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        xor     si, si
        test    ax, ax
        jns     .abs_ready
        neg     ax
        mov     si, 1
.abs_ready:
        mov     cx, CITY_FOCAL
        mul     cx                      ; DX:AX = abs(component)*focal
        mov     di, dx                  ; numerator high
        mov     cx, ax                  ; numerator low

        mov     ax, bx
        mov     dx, 8191
        mul     dx                      ; DX:AX = depth*8191
        cmp     di, dx
        ja      .saturate
        jb      .divide
        cmp     cx, ax
        ja      .saturate
.divide:
        mov     ax, cx
        mov     dx, di
        div     bx                      ; guaranteed <=8191
        jmp     .sign
.saturate:
        mov     ax, 8191
.sign:
        test    si, si
        jz      .done
        neg     ax
.done:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; AX=world X, BX=world Y, CX=world Z.
; Output: city286_project_x/y/z and city286_project_ok. Registers preserved.
city286_project_point:
        pusha
        mov     byte [city286_project_ok], 0

        sub     ax, [city286_cam_x]
        mov     [city286_dx], ax
        mov     ax, bx
        sub     ax, [city286_cam_y]
        mov     [city286_dy], ax
        mov     ax, cx
        sub     ax, [city286_cam_z]
        mov     [city286_dz], ax

        ; Inverse yaw.  Straight scenes avoid four multiplications entirely.
        cmp     byte [city286_yaw], 0
        jne     .yaw_rotate
        mov     ax, [city286_dx]
        mov     [city286_rx], ax
        mov     ax, [city286_dz]
        mov     [city286_rz], ax
        jmp     .yaw_done
.yaw_rotate:
        mov     ax, [city286_dx]
        mov     bx, [city286_cos_yaw]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_dz]
        mov     bx, [city286_sin_yaw]
        call    city286_mul_q14
        mov     bx, [city286_tmp0]
        sub     bx, ax
        mov     [city286_rx], bx

        mov     ax, [city286_dx]
        mov     bx, [city286_sin_yaw]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_dz]
        mov     bx, [city286_cos_yaw]
        call    city286_mul_q14
        add     ax, [city286_tmp0]
        mov     [city286_rz], ax
.yaw_done:

        ; Inverse pitch.
        cmp     byte [city286_pitch], 0
        jne     .pitch_rotate
        mov     ax, [city286_dy]
        mov     [city286_ry], ax
        mov     ax, [city286_rz]
        mov     [city286_tz], ax
        jmp     .pitch_done
.pitch_rotate:
        mov     ax, [city286_dy]
        mov     bx, [city286_cos_pitch]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_rz]
        mov     bx, [city286_sin_pitch]
        call    city286_mul_q14
        mov     bx, [city286_tmp0]
        sub     bx, ax
        mov     [city286_ry], bx

        mov     ax, [city286_dy]
        mov     bx, [city286_sin_pitch]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_rz]
        mov     bx, [city286_cos_pitch]
        call    city286_mul_q14
        add     ax, [city286_tmp0]
        mov     [city286_tz], ax
.pitch_done:

        ; Roll only affects screen-space X/Y.
        cmp     byte [city286_roll], 0
        jne     .roll_rotate
        mov     ax, [city286_rx]
        mov     [city286_tx], ax
        mov     ax, [city286_ry]
        mov     [city286_ty], ax
        jmp     .roll_done
.roll_rotate:
        mov     ax, [city286_rx]
        mov     bx, [city286_cos_roll]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_ry]
        mov     bx, [city286_sin_roll]
        call    city286_mul_q14
        add     ax, [city286_tmp0]
        mov     [city286_tx], ax

        mov     ax, [city286_ry]
        mov     bx, [city286_cos_roll]
        call    city286_mul_q14
        mov     [city286_tmp0], ax
        mov     ax, [city286_rx]
        mov     bx, [city286_sin_roll]
        call    city286_mul_q14
        mov     bx, [city286_tmp0]
        sub     bx, ax
        mov     [city286_ty], bx
.roll_done:

        mov     ax, [city286_tz]
        cmp     ax, CITY_NEAR_Z
        jge     short .__jcc286_skip_001
        jmp     near .done
.__jcc286_skip_001:
        cmp     ax, CITY_FAR_Z
        jle     short .__jcc286_skip_002
        jmp     near .done
.__jcc286_skip_002:
        mov     [city286_project_z], ax
        mov     si, ax

        mov     ax, [city286_tx]
        mov     bx, si
        call    city286_project_component_safe
        add     ax, CITY_SCREEN_CX
        mov     [city286_project_x], ax

        mov     ax, [city286_ty]
        mov     bx, si
        call    city286_project_component_safe
        neg     ax
        add     ax, CITY_SCREEN_CY
        mov     [city286_project_y], ax
        mov     byte [city286_project_ok], 1
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; 3-D line and box primitives
; ---------------------------------------------------------------------------
; Uses city286_line_* endpoints and current prepared GRCG colour.

; ---------------------------------------------------------------------------
; Final-scene 286 affine overlay retained as a self-contained core helper.
; ---------------------------------------------------------------------------
city286_roto_transform:
        pusha
        mov     [city286_tmp0], ax
        mov     [city286_tmp1], bx
        mov     ax, [city286_tmp0]
        mov     bx, [city286_roto_cos]
        call    city286_mul_q8
        mov     [city286_tmp2], ax
        mov     ax, [city286_tmp1]
        mov     bx, [city286_roto_sin]
        call    city286_mul_q8
        mov     dx, [city286_tmp2]
        sub     dx, ax
        add     dx, [city286_roto_cx]
        mov     [city286_project_x], dx

        mov     ax, [city286_tmp0]
        mov     bx, [city286_roto_sin]
        call    city286_mul_q8
        mov     [city286_tmp2], ax
        mov     ax, [city286_tmp1]
        mov     bx, [city286_roto_cos]
        call    city286_mul_q8
        add     ax, [city286_tmp2]
        add     ax, [city286_roto_cy]
        mov     [city286_project_y], ax
        popa
        ret

; Final rotozoom is implemented by CITY3D286_FAITHFUL.INC.
