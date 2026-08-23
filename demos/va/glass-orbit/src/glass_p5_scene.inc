; GLASS ORBIT original work:
; Developed by ChatGPT Plus
; Supervised by SimK, Neko Project 21/W Developer
; Ported By Maho Nakata, 2026.
;
; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; P5 scene connection.  Faces, edges, and grid lines are emitted through the
; existing SGP list builder.  Stars are the one deliberate CPU-pixel path in
; the approved design because SGP has no single-pixel operation.

%define PORT_TSP_STATUS_P5       0x0142
%define TSP_STATUS_VBLANK_P5     0x0040

glass_p5_run:
        mov     word [render_frame_counter], 0
        mov     word [glass_star_scroll_x], 0
        mov     byte [glass_star_subtick], 0
.frame:
        call    glass_compute_cube
        call    glass_p4_sgp_build_list
        jc      .failed
        call    glass_p4_sgp_run_list
        jc      .failed
        call    glass_p4_sgp_apply_endpoint_spans
        call    glass_p4_sgp_build_edge_list
        jc      .failed
        call    glass_p4_sgp_run_list
        jc      .failed
        call    glass_p5_draw_stars
        call    glass_advance_scene_tick
        inc     word [render_frame_counter]
        call    glass_p5_wait_vblank
        jc      .failed
        call    glass_p4_sgp_escape_pressed
        jc      glass_p4_sgp_exit
        jmp     .frame
.failed:
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e5             ; debugger-visible P5 failure marker.
        ret

; Emit the original perspective grid as 15 SGP LINE commands.  Coordinates
; stay in the source logical 640x400 space; the common LINE emitter performs
; the single physical-Y conversion.
glass_p5_draw_grid:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    bp

        mov     al, GLASS_GRID_HORIZ
        call    glass_p5_set_grid_colour
        xor     ax, ax
        mov     bx, 250
        mov     cx, 639
        mov     dx, 250
        call    glass_p5_emit_line

        mov     si, glass_grid_endpoints
        mov     bp, 9
.rays:
        mov     ax, 320
        mov     bx, 399
        mov     cx, [si]
        mov     dx, 250
        call    glass_p5_emit_line
        add     si, 2
        dec     bp
        jnz     .rays

        mov     al, GLASS_GRID_DEPTH
        call    glass_p5_set_grid_colour
        mov     si, glass_grid_y
        mov     bp, 5
.horizontal:
        xor     ax, ax
        mov     bx, [si]
        mov     cx, 639
        mov     dx, bx
        call    glass_p5_emit_line
        add     si, 2
        dec     bp
        jnz     .horizontal

        pop     bp
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

glass_p5_set_grid_colour:
        xor     ah, ah
        mov     [glass_p4_sgp_draw_colour], al
        jmp     glass_p4_sgp_emit_set_colour_index

; AX/BX/CX/DX are logical x0/y0/x1/y1.  Adapt the scene-facing order to the
; existing SGP LINE emitter without introducing another rasterizer.
glass_p5_emit_line:
        mov     [glass_p4_sgp_line_x0], ax
        mov     [glass_p4_sgp_line_y0], bx
        mov     [glass_p4_sgp_line_x1], cx
        mov     [glass_p4_sgp_line_y1], dx
        jmp     glass_p4_sgp_emit_line

; Plot one packed 4bpp pixel through the documented CPU GVRAM aperture.
; Input: AX=physical x, BX=physical y, DL=palette index.
glass_p5_plot_pixel:
        cmp     ax, 0
        jl      .done
        cmp     ax, G0_WIDTH - 1
        ja      .done
        cmp     bx, 0
        jl      .done
        cmp     bx, G0_HEIGHT - 1
        ja      .done
        mov     [glass_p5_pixel_x], ax
        mov     [glass_p5_pixel_colour], dl
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es
        mov     ax, 0xa000
        mov     es, ax
        mov     ax, bx
        mov     cx, G0_PITCH_BYTES
        mul     cx
        mov     di, ax
        mov     ax, [glass_p5_pixel_x]
        mov     bx, ax
        shr     ax, 2
        shl     ax, 1
        add     di, ax
        and     bx, 3
        shl     bx, 1
        mov     cx, [glass_p4_sgp_pixel_masks + bx]
        mov     bx, cx
        mov     ax, [es:di]
        not     cx
        and     ax, cx
        mov     al, [glass_p5_pixel_colour]
        xor     ah, ah
        mov     cx, ax
        shl     ax, 4
        or      ax, cx
        mov     cx, ax
        shl     cx, 8
        or      ax, cx
        and     ax, bx
        or      ax, [es:di]
        mov     [es:di], ax
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
.done:
        ret

align 2, db 0
glass_p5_pixel_x dw 0
glass_p5_pixel_colour db 0

; Draw the retained 256-star table.  The source scene is logical 640x400;
; stars are restricted to the upper sky and therefore use physical y=y/2.
glass_p5_draw_stars:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    bp
        mov     si, glass_star_table
        mov     bp, GLASS_STAR_COUNT
.star:
        mov     ax, [si]
        sub     ax, [glass_star_scroll_x]
        jns     .x_ready
        add     ax, 640
.x_ready:
        mov     [glass_star_plot_x], ax
        mov     bx, [si+2]
        cmp     bx, 240
        jae     .next
        sar     bx, 1
        mov     dl, [si+4]
        cmp     dl, 0
        je      .dim
        cmp     dl, 1
        je      .medium
        mov     dl, GLASS_STAR_BRIGHT
        call    glass_p5_plot_pixel
        mov     ax, [glass_star_plot_x]
        dec     ax
        call    glass_p5_plot_pixel
        mov     ax, [glass_star_plot_x]
        inc     ax
        call    glass_p5_plot_pixel
        jmp     .next
.dim:
        mov     dl, GLASS_STAR_DIM
        call    glass_p5_plot_pixel
        jmp     .next
.medium:
        mov     dl, GLASS_STAR_MED
        call    glass_p5_plot_pixel
.next:
        add     si, 6
        dec     bp
        jnz     .star
        pop     bp
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Wait for one observed display-to-blank transition.  The bound is only a
; fail-closed emulator/test guard and makes no timing claim about hardware.
glass_p5_wait_vblank:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     dx, PORT_TSP_STATUS_P5
        mov     bx, 4
.wait_low:
        mov     cx, 0xffff
.low:
        in      al, dx
        test    al, TSP_STATUS_VBLANK_P5
        jz      .low_seen
        loop    .low
        dec     bx
        jnz     .wait_low
        stc
        jmp     .done
.low_seen:
        mov     bx, 4
.wait_high:
        mov     cx, 0xffff
.high:
        in      al, dx
        test    al, TSP_STATUS_VBLANK_P5
        jnz     .ready
        loop    .high
        dec     bx
        jnz     .wait_high
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
