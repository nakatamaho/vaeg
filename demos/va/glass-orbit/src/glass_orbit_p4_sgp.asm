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
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; GLASS ORBIT P4-2 SGP fixed-frame renderer.
;
; The program shares only the preserved cube geometry/data with P4-1.  It
; constructs an SGP list in main RAM: SET WORK, SET COLOR, CLS, per-span CLS,
; per-edge LINE, and END.  It never uses the CPU G0 aperture before the SGP
; has completed; the sole post-completion CPU access is the raw checksum for
; the host-side comparison harness.  Evidence: docs/port/glass_p4_sgp.md.

cpu 286
bits 16
org 0

; Diagnostic build stages isolate the first list section that cannot complete.
; Stage 1 emits SET_WORK, SET_COLOR, full-page CLS, END.  Stage 2 adds face
; spans.  Stage 3 is the P4-2 candidate and adds the twelve cube-edge LINEs.
; The selected stage changes the generated guest payload only; it does not
; alter VAEG's SGP behaviour.
%ifndef GLASS_P4_SGP_STAGE
%define GLASS_P4_SGP_STAGE 3
%endif
%if GLASS_P4_SGP_STAGE < 1 || GLASS_P4_SGP_STAGE > 3
%error "GLASS_P4_SGP_STAGE must be 1, 2, or 3"
%endif

%define VIDEO_BIOS_INT          0x8f
%define KEYBOARD_BIOS_INT       0x82
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define G0_SGP_BASE             0x200000
%define G0_PAGE_WORDS           0x7d00
%define G0_WIDTH                640
%define G0_HEIGHT               200
%define G0_PITCH_BYTES          320
%define SGP_BUSY                0x01
%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_LINE_COPY           0x0005
%define SGP_LINE_HD             0x0400
%define SGP_LINE_VD             0x0800
%define LOADER_RETURN_SS        0xe000
%define LOADER_RETURN_SP        0xe002
%define LOADER_RETURN_FLAGS     0xe004
%define LOADER_RETURN_MAGIC     0xe006
%define LOADER_RETURN_SIGNATURE 0x5034

glass_p4_sgp_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        mov     bx, MODE_G0_640X200_4BPP
        mov     cx, PIXEL_SIZE_G0_4BPP
        xor     dx, dx
        xor     ax, ax                 ; $ScnMode
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        push    cs
        pop     es
        mov     ax, 0x0100             ; $DefBuf: G0 640x400 4bpp FB0.
        mov     cx, 1
        mov     di, glass_p4_sgp_framebuffer_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        push    cs
        pop     es
        mov     ax, 0x0200             ; $DefWin: first 200 source rows.
        mov     cx, 1
        mov     di, glass_p4_sgp_window_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        mov     ax, 0x0900             ; $PalCtl, palette mode 0.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed
        call    glass_p4_sgp_set_palette
        jc      glass_p4_sgp_failed

        mov     ax, 0x0300             ; $Compose: G0 only.
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed
        mov     ax, 0x0b01             ; $ScnDsp: graphics on.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        mov     word [render_frame_counter], 0
        call    glass_compute_cube
        call    glass_p4_sgp_build_list
        jc      glass_p4_sgp_failed
        ; Keep the generated-list size observable at a fixed pre-submit
        ; checkpoint.  It is diagnostic-only and does not participate in SGP
        ; execution.
times 0x0240 - ($ - $$) db 0x90
        mov     bx, [glass_p4_sgp_list_bytes]
glass_p4_sgp_build_ready:
        nop
        call    glass_p4_sgp_run_list
        jc      glass_p4_sgp_failed

        ; Read only after END has made the SGP idle.  This checksum is a
        ; narrow stability witness; P4-2's host comparator owns the complete
        ; CPU-versus-SGP visible-pixel comparison.
        mov     ax, 0xa000
        mov     es, ax
        call    glass_p4_sgp_raw_checksum
        push    cs
        pop     es
        mov     ax, 0x4753             ; "GS" P4-2 SGP success marker.
        jmp     glass_p4_sgp_idle

glass_p4_sgp_failed:
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e2             ; debugger-visible P4-2 failure marker.

times 0x0280 - ($ - $$) db 0
glass_p4_sgp_idle:
        call    glass_p4_sgp_escape_pressed
        jc      glass_p4_sgp_exit
        hlt
        jmp     glass_p4_sgp_idle

glass_p4_sgp_escape_pressed:
        mov     ah, 0x01
        int     KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 0x00
        int     KEYBOARD_BIOS_INT
        cmp     bh, 0
        jne     .none
        cmp     bl, 0x1b
        je      .escape
.none:
        clc
        ret
.escape:
        stc
        ret

glass_p4_sgp_exit:
        cmp     word [cs:LOADER_RETURN_MAGIC], LOADER_RETURN_SIGNATURE
        jne     glass_p4_sgp_idle
        mov     ax, 0x0b00             ; $ScnDsp: graphics off.
        int     VIDEO_BIOS_INT
        mov     ax, 0x0300             ; $Compose: text only.
        mov     cx, 0x0001
        int     VIDEO_BIOS_INT
        cli
        mov     ax, [cs:LOADER_RETURN_SS]
        mov     ss, ax
        mov     sp, [cs:LOADER_RETURN_SP]
        push    word [cs:LOADER_RETURN_FLAGS]
        popf
        retf

glass_p4_sgp_set_palette:
        push    ax
        push    bx
        push    cx
        push    bp
        push    si
        xor     bx, bx
        mov     si, glass_p4_sgp_palette
        mov     bp, 16
.entry:
        mov     ax, 0x0800             ; $SetPal: AL=index, CX=value.
        mov     al, bl
        mov     cx, [si]
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        inc     bl
        add     si, 2
        dec     bp
        jnz     .entry
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     si
        pop     bp
        pop     cx
        pop     bx
        pop     ax
        ret

; Assemble the complete fixed-frame list before sending its physical address
; to the SGP.  Each word is capacity-checked to fail closed before a list
; overrun can alter adjacent payload data.
glass_p4_sgp_build_list:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        push    ds
        pop     es
        mov     word [glass_p4_sgp_list_cursor], glass_p4_sgp_command_list
        mov     byte [glass_p4_sgp_list_overflow], 0
        mov     word [glass_p4_sgp_last_colour], 0xffff

        mov     ax, SGP_COMMAND_SET_WORK
        call    glass_p4_sgp_emit_word
        mov     si, glass_p4_sgp_work_area
        call    glass_p4_sgp_physical_address_from_ds_si
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word

        xor     ax, ax
        call    glass_p4_sgp_emit_set_colour
        mov     ax, G0_SGP_BASE & 0xffff
        mov     dx, G0_SGP_BASE >> 16
        mov     cx, G0_PAGE_WORDS
        call    glass_p4_sgp_emit_cls

%if GLASS_P4_SGP_STAGE >= 2
        call    glass_p4_sgp_draw_faces
%endif
%if GLASS_P4_SGP_STAGE >= 3
        call    glass_p4_sgp_draw_edges
%endif
        mov     ax, SGP_COMMAND_END
        call    glass_p4_sgp_emit_word

        cmp     byte [glass_p4_sgp_list_overflow], 0
        jne     .failed
        mov     ax, [glass_p4_sgp_list_cursor]
        sub     ax, glass_p4_sgp_command_list
        mov     [glass_p4_sgp_list_bytes], ax
        mov     si, glass_p4_sgp_command_list
        call    glass_p4_sgp_physical_address_from_ds_si
        mov     [glass_p4_sgp_command_address_low], ax
        mov     [glass_p4_sgp_command_address_high], dx
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AL contains a palette index.  SET COLOR uses one packed copy in every
; 4-bpp nibble, so palette index 8 becomes 8888h.
glass_p4_sgp_emit_set_colour_index:
        push    bx
        and     ax, 0x000f
        mov     bx, ax
        shl     bx, 4
        or      ax, bx
        mov     bx, ax
        shl     bx, 8
        or      ax, bx
        pop     bx
        jmp     glass_p4_sgp_emit_set_colour

; AX is a complete packed color word.  Suppress adjacent identical commands.
glass_p4_sgp_emit_set_colour:
        cmp     ax, [glass_p4_sgp_last_colour]
        je      .done
        mov     [glass_p4_sgp_last_colour], ax
        push    ax
        mov     ax, SGP_COMMAND_SET_COLOR
        call    glass_p4_sgp_emit_word
        pop     ax
        call    glass_p4_sgp_emit_word
.done:
        ret

; DX:AX is an even physical word address and CX is a nonzero word count.
glass_p4_sgp_emit_cls:
        push    ax
        mov     ax, SGP_COMMAND_CLS
        call    glass_p4_sgp_emit_word
        pop     ax
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word
        mov     ax, cx
        call    glass_p4_sgp_emit_word
        xor     ax, ax
        call    glass_p4_sgp_emit_word
        ret

; Build camera-facing quads as two triangles.  The CPU performs only scan
; conversion and list construction; every visible span is an SGP CLS command.
glass_p4_sgp_draw_faces:
        push    ax
        push    bx
        push    bp
        push    si
        push    di
        mov     si, glass_cube_faces
        mov     bp, 6
.face:
        call    glass_face_is_visible
        jnc     .next
        xor     bx, bx
        mov     bl, [si+4]
        mov     al, [glass_p4_sgp_face_colour_map + bx]
        mov     [glass_p4_sgp_draw_colour], al
        call    glass_p4_sgp_emit_set_colour_index

        mov     al, [si]
        mov     di, glass_p4_sgp_tri_v0
        call    glass_p4_sgp_load_vertex
        mov     al, [si+1]
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_load_vertex
        mov     al, [si+2]
        mov     di, glass_p4_sgp_tri_v2
        call    glass_p4_sgp_load_vertex
        push    si
        call    glass_p4_sgp_fill_triangle
        pop     si

        mov     al, [si]
        mov     di, glass_p4_sgp_tri_v0
        call    glass_p4_sgp_load_vertex
        mov     al, [si+2]
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_load_vertex
        mov     al, [si+3]
        mov     di, glass_p4_sgp_tri_v2
        call    glass_p4_sgp_load_vertex
        push    si
        call    glass_p4_sgp_fill_triangle
        pop     si
.next:
        add     si, 5
        dec     bp
        jnz     .face
        pop     di
        pop     si
        pop     bp
        pop     bx
        pop     ax
        ret

; Emit all retained cube edges after the faces, preserving the source edge
; colors.  The SGP LINE command performs every GVRAM pixel update.
glass_p4_sgp_draw_edges:
        push    ax
        push    bx
        push    bp
        push    si
        mov     si, glass_cube_edges
        mov     bp, 12
.edge:
        mov     al, [si+2]
        mov     [glass_p4_sgp_draw_colour], al
        call    glass_p4_sgp_emit_set_colour_index
        xor     bx, bx
        mov     bl, [si]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_sgp_line_x0], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_sgp_line_y0], ax
        xor     bx, bx
        mov     bl, [si+1]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_sgp_line_x1], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_sgp_line_y1], ax
        call    glass_p4_sgp_emit_line
        add     si, 3
        dec     bp
        jnz     .edge
        pop     si
        pop     bp
        pop     bx
        pop     ax
        ret

; AL is a retained cube-vertex index and DI receives its logical x/y pair.
glass_p4_sgp_load_vertex:
        push    ax
        push    bx
        xor     bx, bx
        mov     bl, al
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [di], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [di+2], ax
        pop     bx
        pop     ax
        ret

; Convert the unchanged logical Y coordinates exactly once, sort the triangle,
; then emit an inward-rounded full-word SGP CLS span for each physical row.
glass_p4_sgp_fill_triangle:
        sar     word [glass_p4_sgp_tri_v0+2], 1
        sar     word [glass_p4_sgp_tri_v1+2], 1
        sar     word [glass_p4_sgp_tri_v2+2], 1
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_swap_if_y_greater
        mov     si, glass_p4_sgp_tri_v1
        mov     di, glass_p4_sgp_tri_v2
        call    glass_p4_sgp_swap_if_y_greater
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_swap_if_y_greater

        mov     ax, [glass_p4_sgp_tri_v0+2]
        cmp     ax, [glass_p4_sgp_tri_v2+2]
        je      .done
        mov     [glass_p4_sgp_scan_y], ax
.scan:
        mov     ax, [glass_p4_sgp_scan_y]
        cmp     ax, [glass_p4_sgp_tri_v2+2]
        jge     .done
        mov     bx, ax
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v2
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_span_x0], ax

        mov     bx, [glass_p4_sgp_scan_y]
        cmp     bx, [glass_p4_sgp_tri_v1+2]
        jl      .upper
        mov     si, glass_p4_sgp_tri_v1
        mov     di, glass_p4_sgp_tri_v2
        jmp     .second_edge
.upper:
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
.second_edge:
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_span_x1], ax
        mov     ax, [glass_p4_sgp_span_x0]
        mov     bx, [glass_p4_sgp_span_x1]
        mov     cx, [glass_p4_sgp_scan_y]
        call    glass_p4_sgp_emit_span
        inc     word [glass_p4_sgp_scan_y]
        jmp     .scan
.done:
        ret

; SI and DI are x/y pairs; BX is physical Y.  Return the signed linear X in
; AX.  The caller never supplies a horizontal edge here.
glass_p4_sgp_interpolate_edge:
        push    bx
        push    cx
        push    dx
        mov     cx, [di+2]
        sub     cx, [si+2]
        jz      .degenerate
        mov     ax, bx
        sub     ax, [si+2]
        mov     dx, [di]
        sub     dx, [si]
        imul    dx
        idiv    cx
        add     ax, [si]
        jmp     .done
.degenerate:
        mov     ax, [si]
.done:
        pop     dx
        pop     cx
        pop     bx
        ret

glass_p4_sgp_swap_if_y_greater:
        push    ax
        mov     ax, [si+2]
        cmp     ax, [di+2]
        jle     .done
        xchg    ax, [di+2]
        mov     [si+2], ax
        mov     ax, [si]
        xchg    ax, [di]
        mov     [si], ax
.done:
        pop     ax
        ret

; AX/BX are inclusive X bounds and CX is a physical Y.  Outward rounding to
; complete packed-4bpp words avoids a diagonal seam when a convex face is
; represented by two independently rounded triangles.
glass_p4_sgp_emit_span:
        cmp     ax, bx
        jle     .ordered
        xchg    ax, bx
.ordered:
        cmp     bx, 0
        jl      .done
        cmp     ax, G0_WIDTH - 1
        jg      .done
        cmp     ax, 0
        jge     .left_clipped
        xor     ax, ax
.left_clipped:
        cmp     bx, G0_WIDTH - 1
        jle     .right_clipped
        mov     bx, G0_WIDTH - 1
.right_clipped:
        and     ax, 0xfffc
        inc     bx
        add     bx, 3
        and     bx, 0xfffc
        cmp     ax, bx
        jae     .done
        mov     [glass_p4_sgp_span_first], ax
        mov     [glass_p4_sgp_span_past], bx
        sub     bx, ax
        shr     bx, 2
        mov     [glass_p4_sgp_span_words], bx
        mov     ax, cx
        mov     bx, G0_PITCH_BYTES
        mul     bx
        mov     bx, [glass_p4_sgp_span_first]
        shr     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, G0_SGP_BASE & 0xffff
        adc     dx, G0_SGP_BASE >> 16
        mov     cx, [glass_p4_sgp_span_words]
        call    glass_p4_sgp_emit_cls
.done:
        ret

; Emit one documented SGP LINE.  The P4 fixed cube is wholly inside the
; physical 640x200 viewport; no implicit clipping is relied upon here.
glass_p4_sgp_emit_line:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ax, SGP_COMMAND_LINE
        call    glass_p4_sgp_emit_word
        mov     bx, SGP_LINE_COPY

        mov     ax, [glass_p4_sgp_line_x1]
        sub     ax, [glass_p4_sgp_line_x0]
        jns     .x_positive
        neg     ax
        or      bx, SGP_LINE_HD
.x_positive:
        inc     ax
        mov     [glass_p4_sgp_line_width], ax

        mov     ax, [glass_p4_sgp_line_y1]
        sar     ax, 1
        mov     [glass_p4_sgp_line_y1_physical], ax
        mov     cx, [glass_p4_sgp_line_y0]
        sar     cx, 1
        mov     [glass_p4_sgp_line_y0_physical], cx
        sub     ax, cx
        jns     .y_positive
        neg     ax
        or      bx, SGP_LINE_VD
.y_positive:
        inc     ax
        mov     [glass_p4_sgp_line_height], ax

        mov     ax, bx
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_x0]
        and     ax, 3
        shl     ax, 4
        or      ax, 1                 ; mode 1 = packed 4bpp.
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_width]
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_height]
        call    glass_p4_sgp_emit_word
        mov     ax, G0_PITCH_BYTES
        call    glass_p4_sgp_emit_word

        mov     ax, [glass_p4_sgp_line_y0_physical]
        mov     bx, G0_PITCH_BYTES
        mul     bx
        mov     bx, [glass_p4_sgp_line_x0]
        and     bx, 0xfffc
        shr     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, G0_SGP_BASE & 0xffff
        adc     dx, G0_SGP_BASE >> 16
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX is one list word.  The main-RAM command cursor is deliberately separate
; from DI: triangle scan conversion uses DI for temporary vertex pointers.
glass_p4_sgp_emit_word:
        push    di
        mov     di, [glass_p4_sgp_list_cursor]
        cmp     di, glass_p4_sgp_command_list_end - 2
        ja      .overflow
        stosw
        mov     [glass_p4_sgp_list_cursor], di
        pop     di
        ret
.overflow:
        mov     byte [glass_p4_sgp_list_overflow], 1
        pop     di
        ret

glass_p4_sgp_run_list:
        call    glass_p4_sgp_wait_idle
        jc      .failed
        mov     dx, PORT_SGP_COMMAND
        mov     ax, [glass_p4_sgp_command_address_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [glass_p4_sgp_command_address_high]
        out     dx, ax
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     dx, PORT_SGP_STATUS
        mov     al, SGP_BUSY
        out     dx, al
        call    glass_p4_sgp_wait_idle
        ret
.failed:
        stc
        ret

glass_p4_sgp_wait_idle:
        mov     dx, PORT_SGP_STATUS
        ; A complete fixed frame contains a full-page clear plus many span
        ; commands.  Permit that finite list to drain before declaring an
        ; SGP timeout.  The outer count is a guest-side safety bound, not a
        ; timing model or a real-hardware performance assertion.
        mov     bx, 0x0100
.outer:
        mov     cx, 0xffff
.poll:
        in      al, dx
        test    al, SGP_BUSY
        jz      .ready
        loop    .poll
        dec     bx
        jnz     .outer
        stc
        ret
.ready:
        clc
        ret

; Convert DS:SI to the physical command-list address required by SGP.
glass_p4_sgp_physical_address_from_ds_si:
        push    cx
        mov     ax, ds
        xor     dx, dx
        mov     cx, 4
.shift:
        shl     ax, 1
        rcl     dx, 1
        loop    .shift
        add     ax, si
        adc     dx, 0
        pop     cx
        ret

glass_p4_sgp_raw_checksum:
        push    ax
        push    cx
        push    di
        xor     bx, bx
        xor     di, di
        mov     cx, G0_PAGE_WORDS
.word:
        rol     bx, 1
        xor     bx, [es:di]
        add     di, 2
        loop    .word
        pop     di
        pop     cx
        pop     ax
        ret

%include "glass_geometry.inc"
%include "glass_data.inc"

align 2, db 0
glass_p4_sgp_framebuffer_descriptor:
        dw 4, 640, 400
glass_p4_sgp_window_descriptor:
        dw 0, 0, 200, 0, 0

glass_p4_sgp_palette:
        dw 0x0000, 0x001f, 0x03e0, 0x03ff
        dw 0xfc00, 0xfc1f, 0xffe0, 0xffff
        dw 0x0015, 0x02a0, 0xac00, 0xac15
        dw 0x02b5, 0xaea0, 0x7def, 0x7fff

glass_p4_sgp_face_colour_map:
        db 0,8,9,12,10,11,13

glass_p4_sgp_draw_colour db 0
glass_p4_sgp_list_overflow db 0
align 2, db 0
glass_p4_sgp_last_colour       dw 0
glass_p4_sgp_tri_v0            dw 0,0
glass_p4_sgp_tri_v1            dw 0,0
glass_p4_sgp_tri_v2            dw 0,0
glass_p4_sgp_scan_y            dw 0
glass_p4_sgp_span_x0           dw 0
glass_p4_sgp_span_x1           dw 0
glass_p4_sgp_span_first        dw 0
glass_p4_sgp_span_past         dw 0
glass_p4_sgp_span_words        dw 0
glass_p4_sgp_line_x0           dw 0
glass_p4_sgp_line_y0           dw 0
glass_p4_sgp_line_x1           dw 0
glass_p4_sgp_line_y1           dw 0
glass_p4_sgp_line_y0_physical  dw 0
glass_p4_sgp_line_y1_physical  dw 0
glass_p4_sgp_line_width        dw 0
glass_p4_sgp_line_height       dw 0
glass_p4_sgp_command_address_low  dw 0
glass_p4_sgp_command_address_high dw 0
glass_p4_sgp_list_bytes           dw 0
glass_p4_sgp_list_cursor          dw 0

align 2, db 0
glass_p4_sgp_work_area:
        times 58 db 0
glass_p4_sgp_command_list:
        ; P2's conservative all-face budget is 32 KiB.  The P4 fixed frame
        ; uses less, but keeps the production candidate's fail-closed bound.
        times 32768 db 0
glass_p4_sgp_command_list_end:

%if ($ - $$) > LOADER_RETURN_SS
%error "GLASS ORBIT P4-2 payload overlaps the loader return reserve"
%endif
