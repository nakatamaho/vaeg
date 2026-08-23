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
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
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

; GLASS ORBIT P4-1 CPU verification renderer.
;
; This is a fixed-frame, verification-only direct packed-GVRAM renderer.  It
; deliberately has no SGP command path, no frame loop, no PC-98 graphics
; primitive, and no DOS call.  It exercises the scene's retained cube geometry
; through independently written CPU clear, line, and solid-triangle routines.
; The resulting raw GVRAM checksum is a stability observation only; it is not
; a real-PC-88VA hardware golden.  Evidence: docs/port/glass_p4_cpu.md.

cpu 286
bits 16
org 0

%define VIDEO_BIOS_INT          0x8f
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define G0_PAGE_WORDS           0x7d00
%define G0_WIDTH                640
%define G0_HEIGHT               200
%define G0_PITCH_BYTES          320

glass_p4_cpu_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        ; $ScnMode enters the documented V3 G0 640x200 packed-4bpp boundary.
        mov     bx, MODE_G0_640X200_4BPP
        mov     cx, PIXEL_SIZE_G0_4BPP
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_cpu_failed

        ; Palette setup is owned by the VA Graphics BIOS.  Drawing below is
        ; direct CPU access to the documented G0 aperture, never a BIOS draw.
        mov     ax, 0x0900             ; $PalCtl, palette mode 0.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_cpu_failed
        call    glass_p4_cpu_set_palette
        jc      glass_p4_cpu_failed

        mov     ax, 0x0300             ; $Compose: G0 only.
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_cpu_failed
        mov     ax, 0x0b01             ; $ScnDsp: graphics on.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_cpu_failed

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     ax, 0xa000
        mov     es, ax

        ; Fixed state: do not advance the preserved scene tick or animation.
        mov     word [render_frame_counter], 0
        call    glass_p4_cpu_clear
        call    glass_compute_cube
        call    glass_p4_cpu_draw_faces
        call    glass_p4_cpu_draw_edges
        call    glass_p4_cpu_raw_checksum

        push    cs
        pop     es
        mov     ax, 0x4750             ; "GP" P4-1 success marker.
        jmp     glass_p4_cpu_idle

glass_p4_cpu_failed:
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e4             ; debugger-visible P4-1 failure marker.

; The debugger observes a completed fixed frame at this stable address.
times 0x0200 - ($ - $$) db 0
glass_p4_cpu_idle:
        hlt
        jmp     glass_p4_cpu_idle

; Configure the source-colour palette plus the P2 face-colour mapping.
; Palette entries 8..13 are only a visual CPU-reference profile; they are not
; a claim about the later production palette tuning.
glass_p4_cpu_set_palette:
        push    ax
        push    bx
        push    cx
        push    si
        xor     bx, bx
        mov     si, glass_p4_cpu_palette
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
        pop     cx
        pop     bx
        pop     ax
        ret

; Clear exactly the established 640x200 packed-4bpp page.  ES is A000h.
glass_p4_cpu_clear:
        push    ax
        push    cx
        push    di
        xor     ax, ax
        xor     di, di
        mov     cx, G0_PAGE_WORDS
        rep     stosw
        pop     di
        pop     cx
        pop     ax
        ret

; Draw visible cube quads as two solid triangles each.  The retained geometry
; routine owns winding and back-face classification; this renderer only maps
; source face colours to the approved VA face palette indices.
glass_p4_cpu_draw_faces:
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
        mov     al, [glass_p4_face_colour_map + bx]
        mov     [glass_p4_draw_colour], al

        mov     al, [si]
        mov     di, glass_p4_tri_v0
        call    glass_p4_cpu_load_vertex
        mov     al, [si+1]
        mov     di, glass_p4_tri_v1
        call    glass_p4_cpu_load_vertex
        mov     al, [si+2]
        mov     di, glass_p4_tri_v2
        call    glass_p4_cpu_load_vertex
        push    si
        call    glass_p4_cpu_fill_triangle
        pop     si

        mov     al, [si]
        mov     di, glass_p4_tri_v0
        call    glass_p4_cpu_load_vertex
        mov     al, [si+2]
        mov     di, glass_p4_tri_v1
        call    glass_p4_cpu_load_vertex
        mov     al, [si+3]
        mov     di, glass_p4_tri_v2
        call    glass_p4_cpu_load_vertex
        push    si
        call    glass_p4_cpu_fill_triangle
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

; Draw all retained cube edges after faces, preserving their original colours.
glass_p4_cpu_draw_edges:
        push    ax
        push    bx
        push    bp
        push    si
        mov     si, glass_cube_edges
        mov     bp, 12
.edge:
        mov     al, [si+2]
        mov     [glass_p4_draw_colour], al
        xor     bx, bx
        mov     bl, [si]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_line_x0], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_line_y0], ax
        xor     bx, bx
        mov     bl, [si+1]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_line_x1], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_line_y1], ax
        call    glass_p4_cpu_line
        add     si, 3
        dec     bp
        jnz     .edge
        pop     si
        pop     bp
        pop     bx
        pop     ax
        ret

; AL is a retained cube-vertex index and DI receives its logical x/y pair.
glass_p4_cpu_load_vertex:
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

; Fill one triangle with CPU pixels.  Triangle coordinates start in the
; unchanged logical 640x400 space; this is the sole Y conversion point for
; the span primitive.  The scan line excludes the final bottom row, matching
; the top-left style convention later used by the SGP reference path.
glass_p4_cpu_fill_triangle:
        sar     word [glass_p4_tri_v0+2], 1
        sar     word [glass_p4_tri_v1+2], 1
        sar     word [glass_p4_tri_v2+2], 1
        mov     si, glass_p4_tri_v0
        mov     di, glass_p4_tri_v1
        call    glass_p4_cpu_swap_if_y_greater
        mov     si, glass_p4_tri_v1
        mov     di, glass_p4_tri_v2
        call    glass_p4_cpu_swap_if_y_greater
        mov     si, glass_p4_tri_v0
        mov     di, glass_p4_tri_v1
        call    glass_p4_cpu_swap_if_y_greater

        mov     ax, [glass_p4_tri_v0+2]
        cmp     ax, [glass_p4_tri_v2+2]
        je      .done
        mov     [glass_p4_scan_y], ax
.scan:
        mov     ax, [glass_p4_scan_y]
        cmp     ax, [glass_p4_tri_v2+2]
        jge     .done
        mov     bx, ax
        mov     si, glass_p4_tri_v0
        mov     di, glass_p4_tri_v2
        call    glass_p4_cpu_interpolate_edge
        mov     [glass_p4_span_x0], ax

        mov     bx, [glass_p4_scan_y]
        cmp     bx, [glass_p4_tri_v1+2]
        jl      .upper
        mov     si, glass_p4_tri_v1
        mov     di, glass_p4_tri_v2
        jmp     .second_edge
.upper:
        mov     si, glass_p4_tri_v0
        mov     di, glass_p4_tri_v1
.second_edge:
        call    glass_p4_cpu_interpolate_edge
        mov     [glass_p4_span_x1], ax
        mov     ax, [glass_p4_span_x0]
        mov     bx, [glass_p4_span_x1]
        mov     cx, [glass_p4_scan_y]
        mov     dl, [glass_p4_draw_colour]
        call    glass_p4_cpu_span
        inc     word [glass_p4_scan_y]
        jmp     .scan
.done:
        ret

; SI and DI are x/y pairs; BX is a physical Y.  Return the signed linearly
; interpolated X in AX.  The caller never supplies a horizontal edge here.
glass_p4_cpu_interpolate_edge:
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

; Exchange the x/y words at SI and DI when SI has the larger Y value.
glass_p4_cpu_swap_if_y_greater:
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

; AX/BX are inclusive X bounds, CX is physical Y, and DL is a palette index.
; It clips to the packed G0 page before plotting pixels independently.
glass_p4_cpu_span:
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
        mov     [glass_p4_span_x], ax
        mov     [glass_p4_span_end], bx
        mov     [glass_p4_span_y], cx
        mov     [glass_p4_span_colour], dl
.pixel:
        mov     ax, [glass_p4_span_x]
        mov     bx, [glass_p4_span_y]
        mov     dl, [glass_p4_span_colour]
        call    glass_p4_cpu_physical_pixel
        inc     word [glass_p4_span_x]
        mov     ax, [glass_p4_span_x]
        cmp     ax, [glass_p4_span_end]
        jle     .pixel
.done:
        ret

; Bresenham line with logical endpoints.  It converts Y to physical rows once,
; then uses the same packed pixel writer as the triangle-span reference path.
glass_p4_cpu_line:
        sar     word [glass_p4_line_y0], 1
        sar     word [glass_p4_line_y1], 1
        mov     ax, [glass_p4_line_x1]
        sub     ax, [glass_p4_line_x0]
        jns     .dx_ready
        neg     ax
        mov     word [glass_p4_line_sx], -1
        jmp     .dx_store
.dx_ready:
        mov     word [glass_p4_line_sx], 1
.dx_store:
        mov     [glass_p4_line_dx], ax
        mov     ax, [glass_p4_line_y1]
        sub     ax, [glass_p4_line_y0]
        jns     .dy_ready
        neg     ax
        mov     word [glass_p4_line_sy], -1
        jmp     .dy_store
.dy_ready:
        mov     word [glass_p4_line_sy], 1
.dy_store:
        neg     ax
        mov     [glass_p4_line_dy], ax
        mov     ax, [glass_p4_line_dx]
        add     ax, [glass_p4_line_dy]
        mov     [glass_p4_line_error], ax
.plot:
        mov     ax, [glass_p4_line_x0]
        mov     bx, [glass_p4_line_y0]
        mov     dl, [glass_p4_draw_colour]
        call    glass_p4_cpu_physical_pixel
        mov     ax, [glass_p4_line_x0]
        cmp     ax, [glass_p4_line_x1]
        jne     .continue
        mov     ax, [glass_p4_line_y0]
        cmp     ax, [glass_p4_line_y1]
        je      .done
.continue:
        mov     ax, [glass_p4_line_error]
        shl     ax, 1
        mov     [glass_p4_line_e2], ax
        cmp     ax, [glass_p4_line_dy]
        jl      .skip_x
        mov     ax, [glass_p4_line_dy]
        add     [glass_p4_line_error], ax
        mov     ax, [glass_p4_line_sx]
        add     [glass_p4_line_x0], ax
.skip_x:
        mov     ax, [glass_p4_line_e2]
        cmp     ax, [glass_p4_line_dx]
        jg      .skip_y
        mov     ax, [glass_p4_line_dx]
        add     [glass_p4_line_error], ax
        mov     ax, [glass_p4_line_sy]
        add     [glass_p4_line_y0], ax
.skip_y:
        jmp     .plot
.done:
        ret

; AX is an X coordinate, BX is a physical Y coordinate, and DL is a palette
; index.  ES must be the proved G0 CPU aperture at A000h.  Pixel packing is
; high-nibble first, as established by GA-2.
glass_p4_cpu_physical_pixel:
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        cmp     ax, 0
        jl      .done
        cmp     ax, G0_WIDTH
        jge     .done
        cmp     bx, 0
        jl      .done
        cmp     bx, G0_HEIGHT
        jge     .done
        mov     cx, ax
        mov     di, bx
        shl     di, 6
        mov     ax, bx
        shl     ax, 8
        add     di, ax
        mov     ax, cx
        shr     ax, 1
        add     di, ax
        mov     ah, [es:di]
        test    cl, 1
        jnz     .odd
        mov     al, dl
        shl     al, 4
        and     ah, 0x0f
        or      al, ah
        jmp     .write
.odd:
        mov     al, dl
        and     al, 0x0f
        and     ah, 0xf0
        or      al, ah
.write:
        mov     [es:di], al
.done:
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Return a deterministic rolling checksum of the completed raw packed page in
; BX.  This is a local stability witness, not a substitute for raw capture or
; for a real-PC-88VA golden result.
glass_p4_cpu_raw_checksum:
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
glass_p4_cpu_palette:
        dw 0x0000, 0x001f, 0x03e0, 0x03ff
        dw 0xfc00, 0xfc1f, 0xffe0, 0xffff
        dw 0x0015, 0x02a0, 0xac00, 0xac15
        dw 0x02b5, 0xaea0, 0x7def, 0x7fff

; Source face values 1,2,4,5,3,6 map to indices 8..13 as P2 specifies.
glass_p4_face_colour_map:
        db 0,8,9,12,10,11,13

glass_p4_draw_colour db 0
align 2, db 0
glass_p4_tri_v0      dw 0,0
glass_p4_tri_v1      dw 0,0
glass_p4_tri_v2      dw 0,0
glass_p4_scan_y      dw 0
glass_p4_span_x0     dw 0
glass_p4_span_x1     dw 0
glass_p4_span_x      dw 0
glass_p4_span_end    dw 0
glass_p4_span_y      dw 0
glass_p4_span_colour db 0
align 2, db 0
glass_p4_line_x0     dw 0
glass_p4_line_y0     dw 0
glass_p4_line_x1     dw 0
glass_p4_line_y1     dw 0
glass_p4_line_dx     dw 0
glass_p4_line_dy     dw 0
glass_p4_line_sx     dw 0
glass_p4_line_sy     dw 0
glass_p4_line_error  dw 0
glass_p4_line_e2     dw 0

%if ($ - $$) > 0xef00
%error "GLASS ORBIT P4-1 payload overlaps the fixed stack reserve"
%endif
