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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
cpu 286
org 0x100

; The VA2 CPU does not implement the 386 near-Jcc opcodes (0F 80h-8Fh).
; Keep the conditional half short and use a plain near JMP for distant exits.
%macro long_jc 1
    jnc short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jnz 1
    jz short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jz 1
    jnz short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jne 1
    je short %%done
    jmp near %1
%%done:
%endmacro

%macro long_ja 1
    jbe short %%done
    jmp near %1
%%done:
%endmacro

%define KEYBOARD_BIOS_INT       0x82
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_BMS_SELECTOR       0x01d0
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB1_FBW            0x0224
%define PORT_FB1_DOT            0x0228
%define PORT_FB1_OFX            0x022a
%define PORT_FB1_OFY            0x022c
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH       0x0230
%define PORT_FB1_DSH            0x0232
%define PORT_FB1_DSP            0x0236
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_8BPP   0x0808
%define COMPOSE_G1_OVER_G0      0x0034
%define RGB_COMPOSE_G1_OVER_G0  0x0089
%define TSP_STATUS_VBLANK       0x40

%define G0_SEGMENT              0xa000
%define PAYLOAD_SEGMENT         0x3000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            320
%define G1_BACKING_HEIGHT       400
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_SGP_BASE      0x22fa00
%define G1_PAGE_B_DSA           0x02fa00
%define G1_PAGE_BYTES           0xfa00
%define G1_PAGE_WORD_COUNT      0x7d00
%define G1_BACKING_BYTES        0x1f400
%define SCALE_PUBLICATIONS_PER_CYCLE 58
%define QA_CYCLES               M98R_QA_CYCLES
%define QA_PUBLICATIONS         (SCALE_PUBLICATIONS_PER_CYCLE * QA_CYCLES)
%define TARGET_ANCHOR_X         160
%define TARGET_ANCHOR_Y         100

%ifndef M98Q_BOUNDED_QA
%define M98Q_BOUNDED_QA         0
%endif
%ifndef M98Q_INITIAL_VISIBLE_PAGE
%define M98Q_INITIAL_VISIBLE_PAGE 0
%endif
%ifndef M98Q_CLEAR_MODE
%define M98Q_CLEAR_MODE         1
%endif
%ifndef M98R_BOUNDED_QA
%define M98R_BOUNDED_QA         0
%endif
%ifndef M98R_QA_CYCLES
%define M98R_QA_CYCLES          1
%endif
%ifndef M98R_QA_SCENARIO
%define M98R_QA_SCENARIO        0
%endif

%define CLEAR_MODE_FULL         0
%define CLEAR_MODE_DIRTY        1

%define PAGE_A                  0
%define PAGE_B                  1
%define PAGE_UNINITIALIZED      0
%define PAGE_HIDDEN_CLEAN       1
%define PAGE_HIDDEN_RENDERING   2
%define PAGE_HIDDEN_COMPLETE    3
%define PAGE_VISIBLE            4
%define PAGE_HIDDEN_STALE       5

%define RENDER_IDLE             0
%define RENDER_RENDERING        1
%define RENDER_READY            2

%define KEY_SCAN_ESCAPE         0x00
; INT 82h/AH=09h returns the scan code in AH and internal code in AL.
; Recognize the complete ESC result so a queued command Return is ignored.
%define KEY_INTERNAL_ESCAPE     0x1b
%define KEY_SCAN_SPACE          0x34
%define KEY_SCAN_LEFT           0x3b
%define KEY_SCAN_RIGHT          0x3c

%define CADENCE_MIN             1
%define CADENCE_MAX             8

%define BMS_WINDOW_SEGMENT      0x8000
%define BMS_WINDOW_SGP_BASE     0x080000
%define BMS_BANK_SIZE_LOW       0x0000
%define BMS_BANK_SIZE_HIGH      0x0002
%define BMS_FIRST_SELECTOR      1
%define BMS_SECOND_SELECTOR     2
%define BMS_LAST_SELECTOR       128
%define BMS_INVALID_SELECTOR    129
%define BMS_PROBE_OFFSET        0xffe0
%define NORMAL_OUTSIDE_SEGMENT  0x7ffe
%define NORMAL_OUTSIDE_OFFSET   0x0000
%define NORMAL_UNDER_SEGMENT    0x8001
%define NORMAL_UNDER_OFFSET     0x0000
%define GUARD_BYTES             8

%define ATLAS_METADATA_BYTES    1024
%define ATLAS_DESCRIPTOR_BYTES  32
%define ATLAS_SCALE_COUNT       30
%define ATLAS_SELECTED_DESC     (64 + (ATLAS_SCALE_COUNT - 1) * ATLAS_DESCRIPTOR_BYTES)
%define STAGING_BYTES           4096
%define STAGING_POISON          0xa5

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_BUSY                0x01
%define SGP_COMMAND_LIST_WORDS  64
%define DIRTY_ROW_COMMAND_WORDS 5
%define DIRTY_BATCH_FIXED_WORDS 6
%define DIRTY_ROWS_PER_BATCH    ((SGP_COMMAND_LIST_WORDS - DIRTY_BATCH_FIXED_WORDS) / DIRTY_ROW_COMMAND_WORDS)

%define PROBE_CHECKPOINT_IP     0x4000
%define LOAD_CHECKPOINT_IP      0x4010
%define TRANSFER_CHECKPOINT_IP  0x4020
%define FLIP_CHECKPOINT_IP      0x4030
%define SETTLED_CHECKPOINT_IP   0x4040
%define REPORT_A_CHECKPOINT_IP  0x4050
%define REPORT_B_CHECKPOINT_IP  0x4060
%define REPORT_C_CHECKPOINT_IP  0x4070
%define REPORT_D_CHECKPOINT_IP  0x4080
%define REPORT_E_CHECKPOINT_IP  0x4090
%define REPORT_F_CHECKPOINT_IP  0x40a0
%define REPORT_G_CHECKPOINT_IP  0x40b0
%define REPORT_H_CHECKPOINT_IP  0x40c0
%define REPORT_I_CHECKPOINT_IP  0x40d0
%define REPORT_J_CHECKPOINT_IP  0x40e0
%define REPORT_K_CHECKPOINT_IP  0x40f0
%define PROBE_CHECKPOINT_OFFSET (PROBE_CHECKPOINT_IP - 0x0100)
%define LOAD_CHECKPOINT_OFFSET  (LOAD_CHECKPOINT_IP - 0x0100)
%define TRANSFER_CHECKPOINT_OFFSET (TRANSFER_CHECKPOINT_IP - 0x0100)
%define FLIP_CHECKPOINT_OFFSET  (FLIP_CHECKPOINT_IP - 0x0100)
%define SETTLED_CHECKPOINT_OFFSET (SETTLED_CHECKPOINT_IP - 0x0100)
%define REPORT_A_CHECKPOINT_OFFSET (REPORT_A_CHECKPOINT_IP - 0x0100)
%define REPORT_B_CHECKPOINT_OFFSET (REPORT_B_CHECKPOINT_IP - 0x0100)
%define REPORT_C_CHECKPOINT_OFFSET (REPORT_C_CHECKPOINT_IP - 0x0100)
%define REPORT_D_CHECKPOINT_OFFSET (REPORT_D_CHECKPOINT_IP - 0x0100)
%define REPORT_E_CHECKPOINT_OFFSET (REPORT_E_CHECKPOINT_IP - 0x0100)
%define REPORT_F_CHECKPOINT_OFFSET (REPORT_F_CHECKPOINT_IP - 0x0100)
%define REPORT_G_CHECKPOINT_OFFSET (REPORT_G_CHECKPOINT_IP - 0x0100)
%define REPORT_H_CHECKPOINT_OFFSET (REPORT_H_CHECKPOINT_IP - 0x0100)
%define REPORT_I_CHECKPOINT_OFFSET (REPORT_I_CHECKPOINT_IP - 0x0100)
%define REPORT_J_CHECKPOINT_OFFSET (REPORT_J_CHECKPOINT_IP - 0x0100)
%define REPORT_K_CHECKPOINT_OFFSET (REPORT_K_CHECKPOINT_IP - 0x0100)

%if G1_PAGE_BYTES != SCREEN_WIDTH * SCREEN_HEIGHT
%error "M98q G1 page size does not match the logical viewport"
%endif
%if G1_PAGE_WORD_COUNT * 2 != G1_PAGE_BYTES
%error "M98q G1 word count is inconsistent"
%endif
%if G1_PAGE_B_SGP_BASE - G1_PAGE_A_SGP_BASE != G1_PAGE_BYTES
%error "M98q SGP pages are not adjacent"
%endif
%if G1_PAGE_B_DSA - G1_PAGE_A_DSA != G1_PAGE_BYTES
%error "M98q DSA pages are not adjacent"
%endif
%if G1_BACKING_BYTES != G1_PAGE_BYTES * 2
%error "M98q G1 backing surface is not two pages"
%endif
%if M98Q_INITIAL_VISIBLE_PAGE != PAGE_A && M98Q_INITIAL_VISIBLE_PAGE != PAGE_B
%error "M98q initial visible page must be page A or page B"
%endif
%if M98Q_BOUNDED_QA != 0 && M98Q_BOUNDED_QA != 1
%error "M98q bounded QA flag must be zero or one"
%endif
%if M98R_BOUNDED_QA != 0 && M98R_BOUNDED_QA != 1
%error "M98r bounded QA flag must be zero or one"
%endif
%if M98R_QA_CYCLES < 1 || M98R_QA_CYCLES > 2
%error "M98r bounded QA cycles must be one or two"
%endif
%if M98R_QA_SCENARIO < 0 || M98R_QA_SCENARIO > 3
%error "M98r QA scenario must be static, ladder, pause, or missed-slot"
%endif
%if M98Q_CLEAR_MODE != CLEAR_MODE_FULL && M98Q_CLEAR_MODE != CLEAR_MODE_DIRTY
%error "M98q clear mode must be full or dirty"
%endif
%if DIRTY_ROWS_PER_BATCH < 1
%error "M98q command list cannot hold one dirty row"
%endif
%if DIRTY_BATCH_FIXED_WORDS + DIRTY_ROWS_PER_BATCH * DIRTY_ROW_COMMAND_WORDS > SGP_COMMAND_LIST_WORDS
%error "M98q dirty-row batch exceeds the command-list capacity"
%endif
%if TARGET_ANCHOR_X >= SCREEN_WIDTH || TARGET_ANCHOR_Y >= SCREEN_HEIGHT
%error "M98q target anchor is outside the logical viewport"
%endif

start:
    ; PC-Engine may choose a different COM load segment.  Relocate the guest
    ; to the established private-RAM segment so debug checkpoints are stable.
    mov ax, cs
    mov [cs:psp_segment], ax
    cmp ax, PAYLOAD_SEGMENT
    je relocated_start
    pushf
    pop bp
    cli
    push cs
    pop ds
    mov ax, PAYLOAD_SEGMENT
    mov es, ax
    mov si, 0x0100
    mov di, 0x0100
    mov cx, program_end - $$
    cld
    rep movsb
    push bp
    popf
    push word PAYLOAD_SEGMENT
    push word relocated_start
    retf

relocated_start:
    push cs
    pop ds
    cld
    call parse_cadence_option
    jc cadence_option_failed
    mov ah, 0x0f
    mov al, 1
    int KEYBOARD_BIOS_INT
    mov byte [keyboard_repeat_disabled], 1
    call save_video_state
    mov dx, message_start
    call print_string

    call probe_bms_mapping
    long_jc bms_probe_failed
    mov byte [probe_result], 0
    mov bx, PORT_BMS_SELECTOR
    mov cx, BMS_LAST_SELECTOR
    mov dx, BMS_BANK_SIZE_HIGH
    mov si, 0xa55a
    mov di, BMS_INVALID_SELECTOR
    xor bp, bp
    mov ax, 0x98a1
    jmp probe_checkpoint

probe_resume:
    cmp byte [probe_result], 0
    je .probe_passed
    mov word [exit_message], message_bms_failed
    jmp fatal_exit
.probe_passed:
    call load_atlas_to_bms
    long_jc atlas_load_failed
    mov bx, [atlas_chunk_count]
    mov cx, [atlas_payload_bytes]
    mov dx, [atlas_payload_bytes + 2]
    mov si, STAGING_BYTES
    mov di, [atlas_file_size]
    mov bp, [atlas_file_size + 2]
    mov ax, 0x98b1
    jmp load_checkpoint

load_resume:
    call initialize_video_double_buffer
    jc transfer_failed
    mov bx, G1_PAGE_BYTES
    mov cx, SCREEN_PITCH
    mov dx, SGP_BITBLT_COPY_XPAR
    mov si, [selected_source_low]
    mov di, [selected_source_high]
    mov bp, 0x0201
    mov ax, 0x98c1
    jmp transfer_checkpoint

transfer_resume:
    mov byte [current_scale_id], ATLAS_SCALE_COUNT
    mov byte [scale_direction], 0
    call initialize_cadence_scheduler
render_loop:
    mov al, [current_scale_id]
    call select_scale_descriptor
    jc descriptor_failed
    mov byte [hidden_render_state], RENDER_RENDERING
    call render_hidden_page_to_ready
    jc runtime_failed
    call qa_force_missed_slots_before_ready
    jc runtime_failed
    mov byte [hidden_render_state], RENDER_READY
.wait_slot:
    call wait_scheduler_edge
    jc runtime_failed
    cmp byte [exit_requested], 0
    jne normal_exit
    cmp byte [eligible_publication], 0
    je .wait_slot
    mov byte [eligible_publication], 0
    call publish_ready_hidden_page
    jc runtime_failed
    mov bx, [page_flips]
    xor cx, cx
    mov cl, [last_published_scale_id]
    xor dx, dx
    mov dl, [last_published_direction]
    mov al, [active_divisor]
    mov dh, al
    xor ax, ax
    mov al, [visible_page_index]
    mov si, ax
    mov di, [vblank_edges_total]
    mov bp, [requested_slots]
    mov ax, 0x98d1
    jmp flip_checkpoint

flip_resume:
    call advance_scale_sequence
%if M98R_BOUNDED_QA
    cmp word [cycles_completed], QA_CYCLES
    je settled_start
%endif
    jmp render_loop

settled_start:
    mov byte [settled_capture_count], 0
settled_loop:
    call wait_vblank_edge
    jc runtime_failed
    push cs
    pop ds
    push cs
    pop es
    mov bx, [page_flips]
    mov cx, [cycles_completed]
    mov dx, [direction_reversals]
    mov si, [page_flips]
    xor ax, ax
    mov al, [visible_page_index]
    mov di, ax
    mov bp, [last_published_dsa]
    mov ax, 0x98d2
    jmp settled_checkpoint

settled_resume:
%if M98R_BOUNDED_QA
    call poll_escape
    ; Bounded QA does not depend on keyboard input; an injected key is ignored.
%else
    call poll_escape
    jc normal_exit
%endif
    inc byte [settled_capture_count]
    cmp byte [settled_capture_count], 2
    jb settled_loop

%if M98R_BOUNDED_QA
    call validate_bounded_success
    jc descriptor_failed
    jmp normal_exit
%endif
idle_exit_loop:
    call wait_vblank_edge
    jc runtime_failed
    ; Idle display edges are not measured render/publication edges.
    dec word [vblank_edges_seen]
    call poll_escape
    jnc idle_exit_loop

normal_exit:
    mov word [exit_message], message_done
    mov byte [exit_errorlevel], 0
    jmp common_exit

cadence_option_failed:
    mov dx, message_option_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

bms_probe_failed:
    mov byte [probe_result], 1
    mov bx, [probe_stage]
    xor cx, cx
    xor dx, dx
    xor si, si
    xor di, di
    xor bp, bp
    mov ax, 0x98af
    jmp probe_checkpoint
atlas_load_failed:
    mov word [exit_message], message_atlas_failed
    jmp fatal_exit
transfer_failed:
    mov word [exit_message], message_transfer_failed
    jmp fatal_exit
runtime_failed:
    cmp byte [runtime_failure_kind], 2
    je .vblank
    mov word [exit_message], message_transfer_failed
    jmp fatal_exit
.vblank:
    mov word [exit_message], message_runtime_failed
    jmp fatal_exit
descriptor_failed:
    inc word [descriptor_errors]
    mov word [exit_message], message_descriptor_failed

fatal_exit:
    mov byte [exit_errorlevel], 1
common_exit:
    call close_atlas_if_open
    call restore_bms_and_guards
    call restore_video_state
    cmp byte [keyboard_repeat_disabled], 0
    je .repeat_restored
    mov ah, 0x0f
    xor al, al
    int KEYBOARD_BIOS_INT
    mov byte [keyboard_repeat_disabled], 0
.repeat_restored:
    inc word [cleanup_runs]

    mov bx, [pages_initialized]
    mov cx, [render_batches_started]
    mov dx, [render_batches_completed]
    mov si, [initial_full_page_clears]
    mov di, [steady_full_page_clears]
    mov bp, [transparent_bitblts]
    mov ax, 0x98e1
    jmp report_a_checkpoint

report_a_resume:
    mov bx, [vblank_edges_seen]
    mov cx, [page_a_publications]
    mov dx, [page_b_publications]
    mov si, [sgp_timeouts]
    mov di, [sgp_errors]
    mov bp, [vblank_timeouts]
    mov ax, 0x98e2
    jmp report_b_checkpoint

report_b_resume:
    mov bx, [bms_bank_switches]
    mov cx, [cleanup_runs]
    xor dx, dx
    mov dl, [visible_page_index]
    mov si, [cycles_completed]
    mov di, [direction_reversals]
    mov bp, [descriptor_errors]
    mov ax, 0x98e3
    jmp report_c_checkpoint

report_c_resume:
    mov bx, [source_bytes]
    mov cx, [source_bytes + 2]
    mov dx, [dirty_row_cls_commands]
    mov si, [dirty_row_cls_commands + 2]
    mov di, [bms_bank_selections]
    mov bp, [last_published_dsa]
    mov ax, 0x98e4
    jmp report_d_checkpoint

report_d_resume:
    mov bx, [publication_digest]
    mov cx, [publication_digest + 2]
    mov dx, [scale_publication_total]
    mov si, [scale_shrink_total]
    mov di, [scale_grow_total]
    mov bp, [bounded_validation_pass]
    mov ax, 0x98e5
    jmp report_e_checkpoint

report_e_resume:
    mov bx, [dirty_rect_clears]
    mov cx, [dirty_words_cleared]
    mov dx, [dirty_words_cleared + 2]
    mov si, [dirty_bytes_cleared]
    mov di, [dirty_bytes_cleared + 2]
    mov bp, [guard_failures]
    mov ax, 0x98e6
    jmp report_f_checkpoint

report_f_resume:
    mov bx, [baseline_full_clear_words]
    mov cx, [baseline_full_clear_words + 2]
    mov dx, [baseline_full_clear_bytes]
    mov si, [baseline_full_clear_bytes + 2]
    mov di, [sgp_command_lists]
    mov bp, [sgp_commands]
    mov ax, 0x98e7
    jmp report_g_checkpoint

report_g_resume:
    mov bx, [dirty_full_mismatches]
    mov cx, [page_flips]
    xor dx, dx
    mov dl, [page_old_valid + PAGE_A]
    xor ax, ax
    mov al, [page_old_valid + PAGE_B]
    mov si, ax
    mov di, [page_old_scale_id]
    mov bp, [cleanup_runs]
    mov ax, 0x98e8
    jmp report_h_checkpoint

report_h_resume:
    mov bx, [vblank_edges_total]
    mov cx, [vblank_edges_unpaused]
    mov dx, [vblank_edges_paused]
    mov si, [requested_slots]
    mov di, [published_updates]
    mov bp, [missed_slots]
    mov ax, 0x98e9
    jmp report_i_checkpoint

report_i_resume:
    xor bx, bx
    mov bl, [active_divisor]
    mov bh, [requested_divisor]
    mov cx, [divider_change_requests]
    mov dx, [divider_changes_applied]
    mov si, [divider_boundary_resets]
    mov di, [ready_wait_edges]
    mov bp, [scale_advances]
    mov ax, 0x98ea
    jmp report_j_checkpoint

report_j_resume:
    mov bx, [pause_requests]
    mov cx, [pause_transitions_applied]
    mov dx, [control_endpoint_hits]
    mov si, [partial_publication_attempts]
    xor ax, ax
    mov al, [hidden_render_state]
    mov di, ax
    xor ax, ax
    mov al, [divider_count]
    mov bp, ax
    mov ax, 0x98eb
    jmp report_k_checkpoint

report_k_resume:
    mov dx, [exit_message]
    call print_string
    mov al, [exit_errorlevel]
    mov ah, 0x4c
    int 0x21

; G98l-A: selector 0 is ordinary RAM; selectors 1..128 are independent BMS
; banks in the default 16-MiB configuration; selector 129 reads open bus.
probe_bms_mapping:
    mov word [probe_stage], 1
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    mov [initial_bms_selector], al
    test al, al
    long_jnz .early_failed
    cli
    call save_and_install_normal_guards
    long_jc .cleanup
    mov word [probe_stage], 2
    mov byte [ordinary_guards_active], 1

    mov al, BMS_FIRST_SELECTOR
    mov di, saved_bank_1
    mov si, signature_bank_1
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 3
    or byte [saved_probe_banks], 0x01
    mov al, BMS_SECOND_SELECTOR
    mov di, saved_bank_2
    mov si, signature_bank_2
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 4
    or byte [saved_probe_banks], 0x02
    mov al, BMS_LAST_SELECTOR
    mov di, saved_bank_128
    mov si, signature_bank_128
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 5
    or byte [saved_probe_banks], 0x04

    mov al, BMS_FIRST_SELECTOR
    mov si, signature_bank_1
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 6
    mov al, BMS_SECOND_SELECTOR
    mov si, signature_bank_2
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 7
    mov al, BMS_LAST_SELECTOR
    mov si, signature_bank_128
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 8

    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_INVALID_SELECTOR
    out dx, al
    in al, dx
    cmp al, BMS_INVALID_SELECTOR
    jne .cleanup
    mov word [probe_stage], 9
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
.open_bus:
    cmp byte [es:di], 0xff
    jne .cleanup
    mov byte [es:di], 0x5a
    inc di
    loop .open_bus
    mov al, BMS_FIRST_SELECTOR
    mov si, signature_bank_1
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 10

    call restore_probe_banks
    call select_ordinary_mapping
    call verify_normal_guards
    jc .cleanup
    sti
    clc
    ret
.cleanup:
    call restore_probe_banks
    call select_ordinary_mapping
    sti
.early_failed:
    stc
    ret

save_and_install_normal_guards:
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov si, NORMAL_OUTSIDE_OFFSET
    mov di, saved_normal_outside
    mov cx, GUARD_BYTES
    call save_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov si, NORMAL_UNDER_OFFSET
    mov di, saved_normal_under
    mov cx, GUARD_BYTES
    call save_es_bytes
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, guard_normal_outside
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, guard_normal_under
    mov cx, GUARD_BYTES
    call write_es_bytes
    call verify_normal_guards
    ret

save_es_bytes:
.next:
    mov al, [es:si]
    mov [di], al
    inc si
    inc di
    loop .next
    ret

write_es_bytes:
.next:
    mov al, [si]
    mov [es:di], al
    inc si
    inc di
    loop .next
    ret

compare_es_bytes:
.next:
    mov al, [si]
    cmp al, [es:di]
    jne .failed
    inc si
    inc di
    loop .next
    clc
    ret
.failed:
    stc
    ret

verify_normal_guards:
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, guard_normal_outside
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, guard_normal_under
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    clc
    ret
.failed:
    stc
    ret

save_and_write_probe_bank:
    ; AL=selector, DI=save buffer, SI=signature buffer.
    mov bl, al
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    in al, dx
    cmp al, bl
    jne .failed
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    push si
    mov si, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call save_es_bytes
    pop si
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call write_es_bytes
    clc
    ret
.failed:
    stc
    ret

verify_probe_bank:
    ; AL=selector, SI=expected signature.
    mov bl, al
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    in al, dx
    cmp al, bl
    jne .failed
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    clc
    ret
.failed:
    stc
    ret

restore_probe_banks:
    test byte [saved_probe_banks], 0x01
    jz .bank_2
    mov al, BMS_FIRST_SELECTOR
    mov si, saved_bank_1
    call restore_one_probe_bank
.bank_2:
    test byte [saved_probe_banks], 0x02
    jz .bank_128
    mov al, BMS_SECOND_SELECTOR
    mov si, saved_bank_2
    call restore_one_probe_bank
.bank_128:
    test byte [saved_probe_banks], 0x04
    jz .done
    mov al, BMS_LAST_SELECTOR
    mov si, saved_bank_128
    call restore_one_probe_bank
.done:
    mov byte [saved_probe_banks], 0
    ret

restore_one_probe_bank:
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call write_es_bytes
    ret

select_ordinary_mapping:
    mov dx, PORT_BMS_SELECTOR
    xor al, al
    out dx, al
    ret

; G98l-B: load the 1024-byte header/descriptor region conventionally, then
; stream the payload in at most 4096-byte chunks directly into BMS selector 1.
load_atlas_to_bms:
    call select_ordinary_mapping
    mov dx, atlas_filename
    mov ax, 0x3d00
    int 0x21
    long_jc .failed
    mov [atlas_handle], ax
    mov bx, ax
    mov dx, atlas_metadata
    mov cx, ATLAS_METADATA_BYTES
    mov ah, 0x3f
    int 0x21
    long_jc .failed
    cmp ax, ATLAS_METADATA_BYTES
    long_jne .failed
    call validate_atlas_metadata
    long_jc .failed

    mov word [file_crc_state], 0xffff
    mov word [file_crc_state + 2], 0xffff
    mov word [payload_crc_state], 0xffff
    mov word [payload_crc_state + 2], 0xffff
    ; The file CRC is defined with its own header field zeroed.
    mov word [atlas_metadata + 56], 0
    mov word [atlas_metadata + 58], 0
    push ds
    pop es
    mov si, atlas_metadata
    mov cx, ATLAS_METADATA_BYTES
    mov bp, file_crc_state
    call crc32_update_es

    mov ax, [atlas_payload_bytes]
    mov [atlas_remaining], ax
    mov ax, [atlas_payload_bytes + 2]
    mov [atlas_remaining + 2], ax
    mov word [atlas_loaded], 0
    mov word [atlas_loaded + 2], 0
    mov word [atlas_chunk_count], 0
.read_chunk:
    mov ax, [atlas_remaining]
    or ax, [atlas_remaining + 2]
    long_jz .end_of_payload
    mov cx, STAGING_BYTES
    cmp word [atlas_remaining + 2], 0
    jne .size_ready
    cmp word [atlas_remaining], STAGING_BYTES
    jae .size_ready
    mov cx, [atlas_remaining]
.size_ready:
    mov [current_chunk_bytes], cx
    call select_ordinary_mapping
    mov bx, [atlas_handle]
    mov dx, staging_buffer
    mov ah, 0x3f
    int 0x21
    long_jc .failed
    cmp ax, [current_chunk_bytes]
    long_jne .failed
    push ds
    pop es
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    mov bp, file_crc_state
    call crc32_update_es
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    mov bp, payload_crc_state
    call crc32_update_es

    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_FIRST_SELECTOR
    out dx, al
    mov ax, [atlas_loaded]
    mov dx, [atlas_loaded + 2]
    call bms_segment_from_offset
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    rep movsb
    call select_ordinary_mapping
    mov ax, [current_chunk_bytes]
    add [atlas_loaded], ax
    adc word [atlas_loaded + 2], 0
    sub [atlas_remaining], ax
    sbb word [atlas_remaining + 2], 0
    inc word [atlas_chunk_count]
    jmp .read_chunk

.end_of_payload:
    mov bx, [atlas_handle]
    mov dx, staging_buffer
    mov cx, 1
    mov ah, 0x3f
    int 0x21
    jc .failed
    test ax, ax
    jnz .failed
    call close_atlas_if_open
    call compare_final_crcs
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    call verify_bms_frame_crc
    jc .failed
    call poison_staging_buffer
    call verify_staging_poison
    jc .failed
    call select_ordinary_mapping
    call verify_normal_guards
    ret
.failed:
    call close_atlas_if_open
    call select_ordinary_mapping
    stc
    ret

validate_atlas_metadata:
    cmp word [atlas_metadata + 0], 0x555a
    long_jne .failed
    cmp word [atlas_metadata + 2], 0x444e
    long_jne .failed
    cmp word [atlas_metadata + 4], 0x524f
    long_jne .failed
    cmp word [atlas_metadata + 6], 0x0042
    long_jne .failed
    cmp word [atlas_metadata + 8], 1
    long_jne .failed
    cmp word [atlas_metadata + 10], 64
    long_jne .failed
    cmp word [atlas_metadata + 12], 0
    long_jne .failed
    cmp word [atlas_metadata + 14], 0
    long_jne .failed
    cmp word [atlas_metadata + 16], 1
    long_jne .failed
    cmp word [atlas_metadata + 18], ATLAS_SCALE_COUNT
    long_jne .failed
    cmp word [atlas_metadata + 20], ATLAS_DESCRIPTOR_BYTES
    long_jne .failed
    cmp word [atlas_metadata + 22], 0
    long_jne .failed
    cmp word [atlas_metadata + 24], BMS_BANK_SIZE_LOW
    long_jne .failed
    cmp word [atlas_metadata + 26], BMS_BANK_SIZE_HIGH
    long_jne .failed
    cmp word [atlas_metadata + 28], 1
    long_jne .failed
    cmp word [atlas_metadata + 30], BMS_FIRST_SELECTOR
    long_jne .failed
    cmp word [atlas_metadata + 32], 64
    long_jne .failed
    cmp word [atlas_metadata + 34], 0
    long_jne .failed
    cmp word [atlas_metadata + 36], 960
    long_jne .failed
    cmp word [atlas_metadata + 38], 0
    long_jne .failed
    cmp word [atlas_metadata + 40], ATLAS_METADATA_BYTES
    long_jne .failed
    cmp word [atlas_metadata + 42], 0
    long_jne .failed
    mov ax, [atlas_metadata + 44]
    mov dx, [atlas_metadata + 46]
    or ax, dx
    long_jz .failed
    cmp dx, BMS_BANK_SIZE_HIGH
    long_ja .failed
    jne .payload_size_ok
    test ax, ax
    long_jnz .failed
.payload_size_ok:
    mov [atlas_payload_bytes], ax
    mov [atlas_payload_bytes + 2], dx
    add ax, ATLAS_METADATA_BYTES
    adc dx, 0
    cmp ax, [atlas_metadata + 48]
    long_jne .failed
    cmp dx, [atlas_metadata + 50]
    long_jne .failed
    mov [atlas_file_size], ax
    mov [atlas_file_size + 2], dx
    mov ax, [atlas_metadata + 52]
    mov [expected_payload_crc], ax
    mov ax, [atlas_metadata + 54]
    mov [expected_payload_crc + 2], ax
    mov ax, [atlas_metadata + 56]
    mov [expected_file_crc], ax
    mov ax, [atlas_metadata + 58]
    mov [expected_file_crc + 2], ax
    cmp word [atlas_metadata + 60], 0
    long_jne .failed
    cmp word [atlas_metadata + 62], 0
    long_jne .failed

    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    mov ax, [selected_width]
    mov [atlas_source_width], ax
    mov ax, [selected_height]
    mov [atlas_source_height], ax
    mov ax, [selected_anchor_x]
    mov [atlas_source_anchor_x], ax
    mov ax, [selected_anchor_y]
    mov [atlas_source_anchor_y], ax
    mov word [expected_descriptor_offset], 0
    mov word [expected_descriptor_offset + 2], 0
    mov byte [descriptor_validation_id], 1
.descriptor_loop:
    mov al, [descriptor_validation_id]
    call select_scale_descriptor
    jc .failed
    call validate_canonical_scale
    jc .failed
    mov ax, [selected_bank_offset]
    cmp ax, [expected_descriptor_offset]
    jne .failed
    mov dx, [selected_bank_offset + 2]
    cmp dx, [expected_descriptor_offset + 2]
    jne .failed
    add ax, [selected_payload_bytes]
    adc dx, [selected_payload_bytes + 2]
    add ax, 15
    adc dx, 0
    and ax, 0xfff0
    mov [expected_descriptor_offset], ax
    mov [expected_descriptor_offset + 2], dx
    inc byte [descriptor_validation_id]
    cmp byte [descriptor_validation_id], ATLAS_SCALE_COUNT + 1
    jne .descriptor_loop
    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    clc
    ret
.failed:
    stc
    ret

validate_canonical_scale:
    xor cx, cx
    mov cl, [selected_scale_id]
    cmp cl, ATLAS_SCALE_COUNT
    jne .numerator_ready
    mov cx, 31
.numerator_ready:
    mov ax, [atlas_source_width]
    mul cx
    add ax, 15
    adc dx, 0
    mov bx, 31
    div bx
    test ax, ax
    jnz .width_ready
    inc ax
.width_ready:
    cmp ax, [selected_width]
    jne .failed
    mov ax, [atlas_source_height]
    mul cx
    add ax, 15
    adc dx, 0
    mov bx, 31
    div bx
    test ax, ax
    jnz .height_ready
    inc ax
.height_ready:
    cmp ax, [selected_height]
    jne .failed

    mov ax, [atlas_source_anchor_x]
    shl ax, 1
    inc ax
    mul word [selected_width]
    mov bx, [atlas_source_width]
    shl bx, 1
    div bx
    mov bx, [selected_width]
    dec bx
    cmp ax, bx
    jbe .anchor_x_ready
    mov ax, bx
.anchor_x_ready:
    cmp ax, [selected_anchor_x]
    jne .failed
    mov ax, [atlas_source_anchor_y]
    shl ax, 1
    inc ax
    mul word [selected_height]
    mov bx, [atlas_source_height]
    shl bx, 1
    div bx
    mov bx, [selected_height]
    dec bx
    cmp ax, bx
    jbe .anchor_y_ready
    mov ax, bx
.anchor_y_ready:
    cmp ax, [selected_anchor_y]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

; AL is the implicit public scale ID, 1 through 30.  The descriptor identity
; is its canonical table position; IDs 0 and 31 therefore cannot be selected.
select_scale_descriptor:
    cmp al, 1
    jb .failed
    cmp al, ATLAS_SCALE_COUNT
    ja .failed
    mov [selected_scale_id], al
    xor ah, ah
    dec ax
    mov bx, ATLAS_DESCRIPTOR_BYTES
    mul bx
    add ax, 64
    mov bx, atlas_metadata
    add bx, ax

    mov ax, [bx + 0]
    test ax, ax
    jz .failed
    cmp ax, SCREEN_WIDTH
    ja .failed
    mov [selected_width], ax
    mov ax, [bx + 2]
    test ax, ax
    jz .failed
    cmp ax, SCREEN_HEIGHT
    ja .failed
    mov [selected_height], ax
    mov ax, [bx + 4]
    mov [selected_pitch], ax
    mov dx, [selected_width]
    add dx, 3
    and dx, 0xfffc
    cmp ax, dx
    jne .failed
    mov ax, [bx + 6]
    cmp ax, [selected_width]
    jae .failed
    mov [selected_anchor_x], ax
    mov ax, [bx + 8]
    cmp ax, [selected_height]
    jae .failed
    mov [selected_anchor_y], ax
    cmp word [bx + 10], 0
    jne .failed
    cmp word [bx + 12], 0
    jne .failed
    cmp word [bx + 14], 0
    jne .failed
    mov ax, [bx + 16]
    mov dx, [bx + 18]
    mov [selected_bank_offset], ax
    mov [selected_bank_offset + 2], dx
    test ax, 0x000f
    jnz .failed
    mov si, [bx + 20]
    mov di, [bx + 22]
    sub si, ATLAS_METADATA_BYTES
    sbb di, 0
    cmp si, ax
    jne .failed
    cmp di, dx
    jne .failed
    mov ax, [bx + 24]
    mov dx, [bx + 26]
    test dx, dx
    jnz .failed
    mov [selected_payload_bytes], ax
    mov [selected_payload_bytes + 2], dx
    mov ax, [selected_pitch]
    mul word [selected_height]
    cmp ax, [selected_payload_bytes]
    jne .failed
    test dx, dx
    jnz .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add ax, [selected_payload_bytes]
    adc dx, [selected_payload_bytes + 2]
    cmp dx, BMS_BANK_SIZE_HIGH
    ja .failed
    jne .bank_range_ok
    test ax, ax
    jnz .failed
.bank_range_ok:
    cmp dx, [atlas_payload_bytes + 2]
    ja .failed
    jb .payload_range_ok
    cmp ax, [atlas_payload_bytes]
    ja .failed
.payload_range_ok:
    mov ax, [bx + 28]
    mov [expected_frame_crc], ax
    mov ax, [bx + 30]
    mov [expected_frame_crc + 2], ax
    mov ax, TARGET_ANCHOR_X
    sub ax, [selected_anchor_x]
    jc .failed
    mov [selected_dst_x], ax
    add ax, [selected_width]
    cmp ax, SCREEN_WIDTH
    ja .failed
    mov ax, TARGET_ANCHOR_Y
    sub ax, [selected_anchor_y]
    jc .failed
    mov [selected_dst_y], ax
    add ax, [selected_height]
    cmp ax, SCREEN_HEIGHT
    ja .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add dx, BMS_WINDOW_SGP_BASE >> 16
    mov [selected_source_low], ax
    mov [selected_source_high], dx
    clc
    ret
.failed:
    stc
    ret

compare_final_crcs:
    mov ax, [file_crc_state]
    not ax
    cmp ax, [expected_file_crc]
    jne .failed
    mov ax, [file_crc_state + 2]
    not ax
    cmp ax, [expected_file_crc + 2]
    jne .failed
    mov ax, [payload_crc_state]
    not ax
    cmp ax, [expected_payload_crc]
    jne .failed
    mov ax, [payload_crc_state + 2]
    not ax
    cmp ax, [expected_payload_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

verify_bms_payload_crc:
    mov word [bms_crc_state], 0xffff
    mov word [bms_crc_state + 2], 0xffff
    mov word [crc_range_offset], 0
    mov word [crc_range_offset + 2], 0
    mov ax, [atlas_payload_bytes]
    mov [crc_range_remaining], ax
    mov ax, [atlas_payload_bytes + 2]
    mov [crc_range_remaining + 2], ax
    mov bp, bms_crc_state
    call crc32_bms_range
    mov ax, [bms_crc_state]
    not ax
    cmp ax, [expected_payload_crc]
    jne .failed
    mov ax, [bms_crc_state + 2]
    not ax
    cmp ax, [expected_payload_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

verify_bms_frame_crc:
    mov word [frame_crc_state], 0xffff
    mov word [frame_crc_state + 2], 0xffff
    mov ax, [selected_bank_offset]
    mov [crc_range_offset], ax
    mov ax, [selected_bank_offset + 2]
    mov [crc_range_offset + 2], ax
    mov ax, [selected_payload_bytes]
    mov [crc_range_remaining], ax
    mov ax, [selected_payload_bytes + 2]
    mov [crc_range_remaining + 2], ax
    mov bp, frame_crc_state
    call crc32_bms_range
    mov ax, [frame_crc_state]
    not ax
    cmp ax, [expected_frame_crc]
    jne .failed
    mov ax, [frame_crc_state + 2]
    not ax
    cmp ax, [expected_frame_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

crc32_bms_range:
    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_FIRST_SELECTOR
    out dx, al
.next:
    mov ax, [crc_range_remaining]
    or ax, [crc_range_remaining + 2]
    jz .done
    mov cx, STAGING_BYTES
    cmp word [crc_range_remaining + 2], 0
    jne .size_ready
    cmp word [crc_range_remaining], STAGING_BYTES
    jae .size_ready
    mov cx, [crc_range_remaining]
.size_ready:
    push cx
    mov ax, [crc_range_offset]
    mov dx, [crc_range_offset + 2]
    call bms_segment_from_offset
    mov si, di
    pop cx
    call crc32_update_es
    mov ax, cx
    add [crc_range_offset], ax
    adc word [crc_range_offset + 2], 0
    sub [crc_range_remaining], ax
    sbb word [crc_range_remaining + 2], 0
    jmp .next
.done:
    ret

; AX:DX is a byte offset within one 128-KiB bank.  Return ES:DI inside the
; CPU-visible 80000h-9ffffh aperture.
bms_segment_from_offset:
    mov di, ax
    and di, 0x000f
    mov cx, 4
.shift_low:
    shr ax, 1
    loop .shift_low
    mov cx, 12
.shift_high:
    shl dx, 1
    loop .shift_high
    add ax, dx
    add ax, BMS_WINDOW_SEGMENT
    mov es, ax
    ret

; Update the little-endian CRC32 state at CS:BP over ES:SI/CX.  This is the
; reflected polynomial EDB88320h with initial state FFFFFFFFh.
crc32_update_es:
    push ax
    push bx
    push cx
    push dx
    push si
.byte:
    mov al, [es:si]
    inc si
    xor byte [cs:bp], al
    mov bx, 8
.bit:
    shr word [cs:bp + 2], 1
    rcr word [cs:bp], 1
    jnc .next_bit
    xor word [cs:bp], 0x8320
    xor word [cs:bp + 2], 0xedb8
.next_bit:
    dec bx
    jnz .bit
    loop .byte
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

poison_staging_buffer:
    push ds
    pop es
    mov di, staging_buffer
    mov al, STAGING_POISON
    mov cx, STAGING_BYTES
    rep stosb
    ret

verify_staging_poison:
    push ds
    pop es
    mov di, staging_buffer
    mov al, STAGING_POISON
    mov cx, STAGING_BYTES
    repe scasb
    jne .failed
    clc
    ret
.failed:
    stc
    ret

close_atlas_if_open:
    cmp word [atlas_handle], 0xffff
    je .done
    call select_ordinary_mapping
    mov bx, [atlas_handle]
    mov ah, 0x3e
    int 0x21
    mov word [atlas_handle], 0xffff
.done:
    ret

; M98q initializes both 64,000-byte G1 pages once before either is visible.
initialize_video_double_buffer:
    call verify_page_descriptors
    jc .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add dx, BMS_WINDOW_SGP_BASE >> 16
    mov [selected_source_low], ax
    mov [selected_source_high], dx
    call verify_staging_poison
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    call verify_all_bms_frame_crcs
    jc .failed
    call select_ordinary_mapping
    call verify_normal_guards
    jc .failed

    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_8BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    long_jnz .failed
    mov ax, 0x0b00
    int VIDEO_BIOS_INT
    test ax, ax
    long_jnz .failed
    mov ax, 0x0900
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0a00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, RGB_COMPOSE_G1_OVER_G0
    mov dx, PORT_RGB_COMPOSE
    out dx, ax
    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax
    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax
    call configure_g1_framebuffer
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    call draw_g0_checkerboard
    call build_initialization_commands
    call run_sgp_command_list
    jc .failed
    mov word [initial_full_page_clears], 2
    inc word [sgp_command_lists]
    add word [sgp_commands], 5
    mov byte [page_state + PAGE_A], PAGE_HIDDEN_CLEAN
    mov byte [page_state + PAGE_B], PAGE_HIDDEN_CLEAN
    mov byte [page_old_valid + PAGE_A], 0
    mov byte [page_old_valid + PAGE_B], 0
    mov word [pages_initialized], 2
    call wait_vblank_edge
    jc .failed
    mov al, M98Q_INITIAL_VISIBLE_PAGE
    call publish_page
    jc .failed
    mov byte [visible_page_index], M98Q_INITIAL_VISIBLE_PAGE
%if M98Q_INITIAL_VISIBLE_PAGE = PAGE_A
    mov byte [hidden_page_index], PAGE_B
    mov byte [page_state + PAGE_A], PAGE_VISIBLE
    inc word [page_a_publications]
%else
    mov byte [hidden_page_index], PAGE_A
    mov byte [page_state + PAGE_B], PAGE_VISIBLE
    inc word [page_b_publications]
%endif
    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    call select_ordinary_mapping
    stc
    ret

verify_all_bms_frame_crcs:
    mov byte [descriptor_validation_id], 1
.next:
    mov al, [descriptor_validation_id]
    call select_scale_descriptor
    jc .failed
    call verify_bms_frame_crc
    jc .failed
    inc byte [descriptor_validation_id]
    cmp byte [descriptor_validation_id], ATLAS_SCALE_COUNT + 1
    jne .next
    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    clc
    ret
.failed:
    stc
    ret

verify_page_descriptors:
    cmp word [page_sgp_low + PAGE_A * 2], G1_PAGE_A_SGP_BASE & 0xffff
    jne .failed
    cmp word [page_sgp_high + PAGE_A * 2], G1_PAGE_A_SGP_BASE >> 16
    jne .failed
    cmp word [page_sgp_low + PAGE_B * 2], G1_PAGE_B_SGP_BASE & 0xffff
    jne .failed
    cmp word [page_sgp_high + PAGE_B * 2], G1_PAGE_B_SGP_BASE >> 16
    jne .failed
    cmp word [page_dsa_low + PAGE_A * 2], G1_PAGE_A_DSA & 0xffff
    jne .failed
    cmp word [page_dsa_high + PAGE_A * 2], G1_PAGE_A_DSA >> 16
    jne .failed
    cmp word [page_dsa_low + PAGE_B * 2], G1_PAGE_B_DSA & 0xffff
    jne .failed
    cmp word [page_dsa_high + PAGE_B * 2], G1_PAGE_B_DSA >> 16
    jne .failed
    clc
    ret
.failed:
    stc
    ret

configure_g1_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB1_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB1_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB1_OFX
    out dx, ax
    mov dx, PORT_FB1_OFY
    out dx, ax
    mov dx, PORT_FB1_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB1_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    ret

publish_page:
    ; AL is an explicitly validated page index.
    cmp al, PAGE_B
    ja .failed
    xor ah, ah
    mov si, ax
    mov ah, [page_state + si]
    cmp ah, PAGE_HIDDEN_COMPLETE
    je .state_ready
    cmp ah, PAGE_HIDDEN_CLEAN
    jne .failed
.state_ready:
    shl si, 1
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [page_dsa_low + si]
    mov [last_published_dsa], ax
    out dx, ax
    add dx, 2
    mov ax, [page_dsa_high + si]
    out dx, ax
    clc
    ret
.failed:
    stc
    ret

draw_g0_checkerboard:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov dx, checker_row_a
    mov bp, checker_row_b
    test bl, 0x10
    jz .phase_ready
    xchg dx, bp
.phase_ready:
    mov cx, 20
.tile:
    mov si, dx
    test cl, 1
    jz .tile_source_ready
    mov si, bp
.tile_source_ready:
    push cx
    mov cx, 16
    rep movsb
    pop cx
    dec cx
    jnz .tile
    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

build_initialization_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_B_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_B_SGP_BASE >> 16
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

build_render_commands:
    ; The explicit hidden-page index chooses only the destination page.  The
    ; stored scaled anchor keeps every descriptor on the fixed scene anchor.
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    xor ah, ah
    mov si, ax
    shl si, 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    push si
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    pop si
%if M98Q_CLEAR_MODE = CLEAR_MODE_FULL
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [page_sgp_low + si]
    stosw
    mov ax, [page_sgp_high + si]
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
%endif

    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 2
    stosw
    mov ax, [selected_width]
    stosw
    mov ax, [selected_height]
    stosw
    mov ax, [selected_pitch]
    stosw
    mov ax, [selected_source_low]
    stosw
    mov ax, [selected_source_high]
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [selected_dst_x]
    and ax, 1
    shl ax, 4
    or ax, 2
    stosw
    mov ax, [selected_width]
    stosw
    mov ax, [selected_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [selected_dst_y]
    mul word [screen_pitch_word]
    mov bx, [selected_dst_x]
    add ax, bx
    adc dx, 0
    and ax, 0xfffe
    add ax, [page_sgp_low + si]
    adc dx, [page_sgp_high + si]
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

; Build a bounded list containing one zero-valued CLS per old scanline.
; dirty_row_address is a physical byte address, while CLS length is an exact
; word count.  The live SGP decrements this count after every 16-bit write.
build_dirty_row_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov cx, [dirty_batch_rows]
    test cx, cx
    jz .invalid
    cmp cx, DIRTY_ROWS_PER_BATCH
    ja .invalid
    mov bx, [dirty_row_address]
    mov dx, [dirty_row_address + 2]
.row:
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, bx
    stosw
    mov ax, dx
    stosw
    mov ax, [dirty_words_per_row]
    stosw
    xor ax, ax
    stosw
    add bx, SCREEN_PITCH
    adc dx, 0
    loop .row
    mov [dirty_row_address], bx
    mov [dirty_row_address + 2], dx
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list
.invalid:
    ; The caller validates this state before entering the builder.  Preserve a
    ; bounded END-only list if an internal invariant is nevertheless broken.
    mov word [dirty_builder_failed], 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_END
    stosw
finalize_sgp_command_list:
    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx
    pop es
    pop di
    pop si
    pop dx
    pop ax
    ret

prepare_pending_rectangle:
    mov ax, [selected_dst_x]
    cmp ax, SCREEN_WIDTH
    jae .failed
    mov [pending_x], ax
    mov bx, ax
    add bx, [selected_width]
    jc .failed
    cmp bx, SCREEN_WIDTH
    ja .failed
    cmp bx, ax
    jbe .failed
    mov ax, [selected_dst_y]
    cmp ax, SCREEN_HEIGHT
    jae .failed
    mov [pending_y], ax
    mov bx, ax
    add bx, [selected_height]
    jc .failed
    cmp bx, SCREEN_HEIGHT
    ja .failed
    cmp bx, ax
    jbe .failed
    mov ax, [selected_width]
    mov [pending_width], ax
    mov ax, [selected_height]
    mov [pending_height], ax
    mov al, [selected_scale_id]
    mov [pending_scale_id], al
    clc
    ret
.failed:
    stc
    ret

; Resolve only the hidden physical page's most recently published rectangle.
; Saved rectangles stay logical and half-open; rounding is temporary clear
; state for the one homogeneous G1 object.
prepare_dirty_clear_state:
    mov byte [dirty_clear_needed], 0
    mov word [dirty_builder_failed], 0
    mov al, [hidden_page_index]
    cmp al, PAGE_B
    ja .failed
    xor ah, ah
    mov bx, ax
    cmp byte [page_old_valid + bx], 0
    je .none
    mov byte [dirty_clear_needed], 1
    mov si, bx
    shl si, 1
    mov ax, [page_old_x + si]
    cmp ax, SCREEN_WIDTH
    jae .failed
    mov cx, [page_old_width + si]
    test cx, cx
    jz .failed
    mov dx, ax
    add dx, cx
    jc .failed
    cmp dx, SCREEN_WIDTH
    ja .failed
    cmp dx, ax
    jbe .failed
    and ax, 0xfffe
    mov [dirty_clear_x0], ax
    inc dx
    and dx, 0xfffe
    cmp dx, SCREEN_WIDTH
    ja .failed
    cmp dx, ax
    jbe .failed
    sub dx, ax
    test dx, 1
    jnz .failed
    shr dx, 1
    test dx, dx
    jz .failed
    mov [dirty_words_per_row], dx

    mov ax, [page_old_y + si]
    cmp ax, SCREEN_HEIGHT
    jae .failed
    mov cx, [page_old_height + si]
    test cx, cx
    jz .failed
    mov dx, ax
    add dx, cx
    jc .failed
    cmp dx, SCREEN_HEIGHT
    ja .failed
    cmp dx, ax
    jbe .failed
    mov [dirty_rows_remaining], cx

    mul word [screen_pitch_word]
    add ax, [dirty_clear_x0]
    adc dx, 0
    add ax, [page_sgp_low + si]
    adc dx, [page_sgp_high + si]
    mov [dirty_row_address], ax
    mov [dirty_row_address + 2], dx

    inc word [dirty_rect_clears]
    mov ax, [page_old_height + si]
    add [dirty_row_cls_commands], ax
    adc word [dirty_row_cls_commands + 2], 0
    mul word [dirty_words_per_row]
    add [dirty_words_cleared], ax
    adc [dirty_words_cleared + 2], dx
    shl ax, 1
    rcl dx, 1
    add [dirty_bytes_cleared], ax
    adc [dirty_bytes_cleared + 2], dx
.none:
    clc
    ret
.failed:
    stc
    ret

clear_hidden_dirty_rows:
    call prepare_dirty_clear_state
    jc .failed
    cmp byte [dirty_clear_needed], 0
    je .done
.batch:
    mov ax, [dirty_rows_remaining]
    test ax, ax
    jz .done
    cmp ax, DIRTY_ROWS_PER_BATCH
    jbe .batch_size_ready
    mov ax, DIRTY_ROWS_PER_BATCH
.batch_size_ready:
    mov [dirty_batch_rows], ax
    call build_dirty_row_commands
    cmp word [dirty_builder_failed], 0
    jne .failed
    call run_sgp_command_list
    jc .failed
    inc word [sgp_command_lists]
    mov ax, [dirty_batch_rows]
    add ax, 3
    add [sgp_commands], ax
    mov ax, [dirty_batch_rows]
    sub [dirty_rows_remaining], ax
    jmp .batch
.done:
    clc
    ret
.failed:
    stc
    ret

commit_pending_rectangle:
    mov al, [hidden_page_index]
    cmp al, PAGE_B
    ja .failed
    xor ah, ah
    mov bx, ax
    mov byte [page_old_valid + bx], 1
    mov al, [pending_scale_id]
    mov [page_old_scale_id + bx], al
    shl bx, 1
    mov ax, [pending_x]
    mov [page_old_x + bx], ax
    mov ax, [pending_y]
    mov [page_old_y + bx], ax
    mov ax, [pending_width]
    mov [page_old_width + bx], ax
    mov ax, [pending_height]
    mov [page_old_height + bx], ax
    clc
    ret
.failed:
    stc
    ret

render_hidden_page_to_ready:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov byte [runtime_failure_kind], 1
    mov al, [visible_page_index]
    cmp al, PAGE_B
    ja .state_failed
    cmp al, [hidden_page_index]
    je .state_failed
    mov al, [hidden_page_index]
    cmp al, PAGE_B
    ja .state_failed
    xor ah, ah
    mov si, ax
    mov bl, [page_state + si]
    cmp bl, PAGE_HIDDEN_CLEAN
    je .state_ready
    cmp bl, PAGE_HIDDEN_STALE
    jne .state_failed
.state_ready:
    call prepare_pending_rectangle
    jc .state_failed
    call wait_sgp_idle
    jc .failed
    mov byte [page_state + si], PAGE_HIDDEN_RENDERING
    inc word [render_batches_started]
    add word [baseline_full_clear_words], G1_PAGE_WORD_COUNT
    adc word [baseline_full_clear_words + 2], 0
    add word [baseline_full_clear_bytes], G1_PAGE_BYTES & 0xffff
    adc word [baseline_full_clear_bytes + 2], G1_PAGE_BYTES >> 16
%if M98Q_CLEAR_MODE = CLEAR_MODE_DIRTY
    call clear_hidden_dirty_rows
    jc .failed
%endif
    call select_render_bms
    mov al, [hidden_page_index]
    call build_render_commands
    call run_sgp_command_list
    jc .failed
    inc word [sgp_command_lists]
%if M98Q_CLEAR_MODE = CLEAR_MODE_FULL
    add word [sgp_commands], 7
    inc word [steady_full_page_clears]
%else
    add word [sgp_commands], 5
%endif
    inc word [render_batches_completed]
    inc word [transparent_bitblts]
    mov al, [hidden_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_HIDDEN_COMPLETE
    call verify_staging_poison
    jc .sgp_error
    call verify_bms_frame_crc
    jc .sgp_error
    call select_render_ordinary
    call verify_normal_guards
    jc .sgp_error
    clc
    jmp .done
.sgp_error:
    inc word [sgp_errors]
.state_failed:
    mov byte [runtime_failure_kind], 1
.failed:
    stc
.done:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

publish_ready_hidden_page:
    push ax
    push bx
    push si
    cmp byte [hidden_render_state], RENDER_READY
    jne .failed
    mov al, [hidden_page_index]
    call publish_page
    jc .failed
    call commit_pending_rectangle
    jc .failed
    mov al, [hidden_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_VISIBLE
    cmp al, PAGE_A
    jne .published_b
    inc word [page_a_publications]
    jmp .published
.published_b:
    inc word [page_b_publications]
.published:
    mov al, [visible_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_HIDDEN_STALE
    mov al, [visible_page_index]
    mov bl, [hidden_page_index]
    mov [visible_page_index], bl
    mov [hidden_page_index], al
    inc word [page_flips]
    inc word [published_updates]
    mov al, [selected_scale_id]
    mov [last_published_scale_id], al
    mov al, [scale_direction]
    mov [last_published_direction], al
    call record_scale_publication
    mov byte [hidden_render_state], RENDER_IDLE
    call qa_after_publication
    clc
    jmp .done
.failed:
    stc
.done:
    pop si
    pop bx
    pop ax
    ret

select_render_bms:
    inc word [bms_bank_selections]
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    cmp al, BMS_FIRST_SELECTOR
    je .done
    mov al, BMS_FIRST_SELECTOR
    out dx, al
    inc word [bms_bank_switches]
.done:
    ret

record_scale_publication:
    push ax
    push bx
    push cx
    push dx
    push si
    xor ax, ax
    mov al, [selected_scale_id]
    dec ax
    mov si, ax
    shl si, 1
    inc word [scale_publications + si]
    inc word [scale_publication_total]
    cmp byte [scale_direction], 0
    jne .grow
    inc word [scale_shrink_publications + si]
    inc word [scale_shrink_total]
    jmp .direction_recorded
.grow:
    inc word [scale_grow_publications + si]
    inc word [scale_grow_total]
.direction_recorded:
    shl si, 1
    xor ax, ax
    mov al, [visible_page_index]
    shl ax, 1
    add si, ax
    inc word [scale_page_publications + si]

    mov ax, [selected_payload_bytes]
    add [source_bytes], ax
    adc word [source_bytes + 2], 0

    xor ax, ax
    mov al, [selected_scale_id]
    xor bx, bx
    mov bl, [scale_direction]
    shl bx, 8
    add ax, bx
    add [publication_digest], ax
    adc word [publication_digest + 2], 0
    xor ax, ax
    mov al, [visible_page_index]
    inc ax
    add [publication_digest + 2], ax
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

advance_scale_sequence:
    inc word [scale_advances]
    cmp byte [scale_direction], 0
    jne .grow
    cmp byte [current_scale_id], 1
    jne .shrink_step
    mov byte [scale_direction], 1
    mov byte [current_scale_id], 2
    inc word [direction_reversals]
    ret
.shrink_step:
    dec byte [current_scale_id]
    ret
.grow:
    cmp byte [current_scale_id], 29
    jne .grow_step
    mov byte [scale_direction], 0
    mov byte [current_scale_id], 30
    inc word [direction_reversals]
    inc word [cycles_completed]
    ret
.grow_step:
    inc byte [current_scale_id]
    ret

validate_bounded_success:
    cmp word [render_batches_started], QA_PUBLICATIONS
    jne .failed
    cmp word [render_batches_completed], QA_PUBLICATIONS
    jne .failed
    cmp word [initial_full_page_clears], 2
    jne .failed
%if M98Q_CLEAR_MODE = CLEAR_MODE_DIRTY
    cmp word [steady_full_page_clears], 0
    jne .failed
    cmp word [dirty_rect_clears], QA_PUBLICATIONS - 2
    jne .failed
    mov ax, [dirty_words_cleared + 2]
    cmp ax, [baseline_full_clear_words + 2]
    jb .dirty_volume_ok
    ja .failed
    mov ax, [dirty_words_cleared]
    cmp ax, [baseline_full_clear_words]
    jae .failed
.dirty_volume_ok:
%else
    cmp word [steady_full_page_clears], QA_PUBLICATIONS
    jne .failed
    cmp word [dirty_rect_clears], 0
    jne .failed
    cmp word [dirty_row_cls_commands], 0
    jne .failed
    cmp word [dirty_row_cls_commands + 2], 0
    jne .failed
%endif
    cmp word [transparent_bitblts], QA_PUBLICATIONS
    jne .failed
    cmp word [page_flips], QA_PUBLICATIONS
    jne .failed
    cmp word [published_updates], QA_PUBLICATIONS
    jne .failed
    cmp word [scale_advances], QA_PUBLICATIONS
    jne .failed
    mov ax, [published_updates]
    add ax, [missed_slots]
    cmp ax, [requested_slots]
    jne .failed
    cmp byte [active_divisor], CADENCE_MIN
    jb .failed
    cmp byte [active_divisor], CADENCE_MAX
    ja .failed
    cmp byte [requested_divisor], CADENCE_MIN
    jb .failed
    cmp byte [requested_divisor], CADENCE_MAX
    ja .failed
    cmp word [partial_publication_attempts], 0
    jne .failed
    cmp word [cycles_completed], QA_CYCLES
    jne .failed
    cmp word [direction_reversals], QA_CYCLES * 2
    jne .failed
    cmp word [sgp_timeouts], 0
    jne .failed
    cmp word [sgp_errors], 0
    jne .failed
    cmp word [vblank_timeouts], 0
    jne .failed
    cmp word [descriptor_errors], 0
    jne .failed
    cmp word [dirty_full_mismatches], 0
    jne .failed
    cmp word [guard_failures], 0
    jne .failed
    cmp word [baseline_full_clear_words], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) & 0xffff
    jne .failed
    cmp word [baseline_full_clear_words + 2], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) >> 16
    jne .failed
    cmp word [baseline_full_clear_bytes], (QA_PUBLICATIONS * G1_PAGE_BYTES) & 0xffff
    jne .failed
    cmp word [baseline_full_clear_bytes + 2], (QA_PUBLICATIONS * G1_PAGE_BYTES) >> 16
    jne .failed
    cmp word [scale_publication_total], QA_PUBLICATIONS
    jne .failed
    cmp word [scale_shrink_total], 30 * QA_CYCLES
    jne .failed
    cmp word [scale_grow_total], 28 * QA_CYCLES
    jne .failed
    xor si, si
    mov cx, ATLAS_SCALE_COUNT
.scale_loop:
    mov ax, [scale_publications + si]
    cmp cx, ATLAS_SCALE_COUNT
    je .endpoint
    cmp cx, 1
    je .endpoint
    cmp ax, 2 * QA_CYCLES
    jne .failed
    cmp word [scale_shrink_publications + si], QA_CYCLES
    jne .failed
    cmp word [scale_grow_publications + si], QA_CYCLES
    jne .failed
    jmp .next
.endpoint:
    cmp ax, QA_CYCLES
    jne .failed
    cmp word [scale_shrink_publications + si], QA_CYCLES
    jne .failed
    cmp word [scale_grow_publications + si], 0
    jne .failed
.next:
    add si, 2
    loop .scale_loop
    mov word [bounded_validation_pass], 1
    clc
    ret
.failed:
    stc
    ret

parse_cadence_option:
    push ax
    push bx
    push cx
    push si
    push es
    mov byte [parsed_divisor], CADENCE_MIN
    mov byte [cadence_option_seen], 0
    mov ax, [psp_segment]
    mov es, ax
    xor cx, cx
    mov cl, [es:0x0080]
    mov si, 0x0081
.skip_space:
    test cx, cx
    jz .success
    mov al, [es:si]
    cmp al, ' '
    je .space
    cmp al, 9
    jne .token
.space:
    inc si
    dec cx
    jmp .skip_space
.token:
    cmp byte [cadence_option_seen], 0
    jne .failed
    cmp cx, 3
    jb .failed
    cmp al, '/'
    jne .failed
    mov al, [es:si + 1]
    and al, 0xdf
    cmp al, 'V'
    jne .failed
    mov al, [es:si + 2]
    cmp al, '1'
    jb .failed
    cmp al, '8'
    ja .failed
    sub al, '0'
    mov [parsed_divisor], al
    mov byte [cadence_option_seen], 1
    add si, 3
    sub cx, 3
    test cx, cx
    jz .success
    mov al, [es:si]
    cmp al, ' '
    je .skip_space
    cmp al, 9
    je .skip_space
.failed:
    stc
    jmp .done
.success:
    clc
.done:
    pop es
    pop si
    pop cx
    pop bx
    pop ax
    ret

initialize_cadence_scheduler:
    mov al, [parsed_divisor]
    mov [active_divisor], al
    mov [requested_divisor], al
    mov byte [divider_count], 0
    mov byte [paused], 0
    mov byte [pause_toggle_pending], 0
    mov byte [exit_requested], 0
    mov byte [eligible_publication], 0
    mov byte [hidden_render_state], RENDER_IDLE
    mov dx, PORT_TSP_STATUS
    in al, dx
    and al, TSP_STATUS_VBLANK
    mov [vblank_last_high], al
    mov byte [scheduler_active], 1
    ret

poll_control_requests:
    push ax
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .done
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, KEY_SCAN_ESCAPE
    jne .check_left
    cmp al, KEY_INTERNAL_ESCAPE
    je .escape
.check_left:
    cmp ah, KEY_SCAN_LEFT
    je .faster
    cmp ah, KEY_SCAN_RIGHT
    je .slower
    cmp ah, KEY_SCAN_SPACE
    je .pause
    jmp .done
.escape:
    mov byte [exit_requested], 1
    jmp .done
.faster:
    cmp byte [requested_divisor], CADENCE_MIN
    jbe .clamped
    dec byte [requested_divisor]
    inc word [divider_change_requests]
    jmp .done
.slower:
    cmp byte [requested_divisor], CADENCE_MAX
    jae .clamped
    inc byte [requested_divisor]
    inc word [divider_change_requests]
    jmp .done
.pause:
    xor byte [pause_toggle_pending], 1
    inc word [pause_requests]
    jmp .done
.clamped:
    inc word [control_endpoint_hits]
.done:
    pop ax
    ret

observe_vblank_sample:
    push ax
    push dx
    cmp byte [scheduler_active], 0
    je .no_edge
    mov dx, PORT_TSP_STATUS
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .low
    cmp byte [vblank_last_high], 0
    jne .no_edge
    mov byte [vblank_last_high], TSP_STATUS_VBLANK
    call process_scheduler_edge
    pop dx
    pop ax
    stc
    ret
.low:
    mov byte [vblank_last_high], 0
.no_edge:
    pop dx
    pop ax
    clc
    ret

process_scheduler_edge:
    push ax
    push bx
    inc word [vblank_edges_total]
    mov byte [scheduler_boundary_reset], 0
    call qa_before_scheduler_actions
    cmp byte [exit_requested], 0
    jne .done
    cmp byte [pause_toggle_pending], 0
    je .check_divisor
    xor byte [paused], 1
    mov byte [pause_toggle_pending], 0
    inc word [pause_transitions_applied]
    mov byte [scheduler_boundary_reset], 1
.check_divisor:
    mov al, [requested_divisor]
    cmp al, [active_divisor]
    je .classify
    mov [active_divisor], al
    inc word [divider_changes_applied]
    mov byte [scheduler_boundary_reset], 1
.classify:
    cmp byte [paused], 0
    je .unpaused
    inc word [vblank_edges_paused]
    mov byte [divider_count], 0
    cmp byte [scheduler_boundary_reset], 0
    je .done
    inc word [divider_boundary_resets]
    jmp .done
.unpaused:
    inc word [vblank_edges_unpaused]
    cmp byte [scheduler_boundary_reset], 0
    je .count_divider
    mov byte [divider_count], 0
    inc word [divider_boundary_resets]
    jmp .ready_wait
.count_divider:
    inc byte [divider_count]
    mov al, [divider_count]
    cmp al, [active_divisor]
    jb .ready_wait
    mov byte [divider_count], 0
    inc word [requested_slots]
    cmp byte [hidden_render_state], RENDER_READY
    jne .missed
    mov byte [eligible_publication], 1
    jmp .done
.missed:
    inc word [missed_slots]
    jmp .done
.ready_wait:
    cmp byte [hidden_render_state], RENDER_READY
    jne .done
    inc word [ready_wait_edges]
.done:
    pop bx
    pop ax
    ret

wait_scheduler_edge:
    mov bx, 4
.outer:
    mov cx, 0xffff
.poll:
    call poll_control_requests
    cmp byte [exit_requested], 0
    jne .ready
    call observe_vblank_sample
    jc .ready
    loop .poll
    dec bx
    jnz .outer
    inc word [vblank_timeouts]
    mov byte [runtime_failure_kind], 2
    stc
    ret
.ready:
    clc
    ret

qa_queue_divisor:
    ; AL is a checked divisor used only by compile-time bounded QA scenarios.
    cmp al, CADENCE_MIN
    jb .failed
    cmp al, CADENCE_MAX
    ja .failed
    cmp al, [requested_divisor]
    je .done
    mov [requested_divisor], al
    inc word [divider_change_requests]
.done:
    clc
    ret
.failed:
    stc
    ret

qa_force_missed_slots_before_ready:
%if M98R_BOUNDED_QA && M98R_QA_SCENARIO = 3
    cmp byte [qa_stage], 0
    jne .done
    call wait_scheduler_edge
    jc .failed
    call wait_scheduler_edge
    jc .failed
    mov byte [qa_stage], 1
%endif
.done:
    clc
    ret
.failed:
    stc
    ret

qa_queue_pause:
    xor byte [pause_toggle_pending], 1
    inc word [pause_requests]
    ret

qa_after_sgp_submission:
%if M98R_BOUNDED_QA && M98R_QA_SCENARIO = 1
    cmp byte [hidden_render_state], RENDER_RENDERING
    jne .done
    cmp byte [qa_stage], 0
    jne .done
    cmp word [published_updates], 4
    jne .done
    mov al, 2
    call qa_queue_divisor
    jc .done
    mov byte [qa_stage], 1
%elif M98R_BOUNDED_QA && M98R_QA_SCENARIO = 2
    cmp byte [hidden_render_state], RENDER_RENDERING
    jne .done
    cmp byte [qa_stage], 0
    jne .done
    cmp word [published_updates], 4
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 1
%endif
.done:
    ret

qa_after_publication:
%if M98R_BOUNDED_QA && M98R_QA_SCENARIO = 1
    cmp byte [qa_stage], 1
    jb .done
    cmp byte [qa_stage], 14
    jae .done
    xor bx, bx
    mov bl, [qa_stage]
    dec bx
    mov si, bx
    shl si, 1
    mov ax, [qa_ladder_publications + si]
    cmp ax, [published_updates]
    jne .done
    mov al, [qa_ladder_divisors + bx]
    call qa_queue_divisor
    jc .done
    inc byte [qa_stage]
%elif M98R_BOUNDED_QA && M98R_QA_SCENARIO = 2
    cmp byte [qa_stage], 2
    jne .pause_at_40
    cmp word [published_updates], 20
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 3
    jmp .done
.pause_at_40:
    cmp byte [qa_stage], 4
    jne .done
    cmp word [published_updates], 40
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 5
%endif
.done:
    ret

qa_before_scheduler_actions:
%if M98R_BOUNDED_QA && M98R_QA_SCENARIO = 2
    cmp byte [paused], 0
    je .done
    cmp byte [pause_toggle_pending], 0
    jne .done
    cmp byte [qa_pause_edges_remaining], 0
    je .done
    dec byte [qa_pause_edges_remaining]
    jne .done
    call qa_queue_pause
    inc byte [qa_stage]
%endif
.done:
    ret

select_render_ordinary:
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    test al, al
    jz .done
    xor al, al
    out dx, al
    inc word [bms_bank_switches]
.done:
    ret

run_sgp_command_list:
    call wait_sgp_idle
    jc .failed
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_SGP_COMMAND
    mov ax, [sgp_command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_high]
    out dx, ax
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    call qa_after_sgp_submission
    call wait_sgp_idle
    jnc .complete
    ; Abort only after the bounded completion wait fails.  This leaves the
    ; aperture safe to restore without publishing the incomplete page.
    mov dx, PORT_SGP_CONTROL
    mov al, 0x02
    out dx, al
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    stc
    ret
.complete:
    clc
    ret
.failed:
    stc
    ret

wait_sgp_idle:
    mov dx, PORT_SGP_STATUS
    mov bx, 4
.outer:
    mov cx, 0xffff
.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    cmp byte [scheduler_active], 0
    je .continue
    call poll_control_requests
    call observe_vblank_sample
.continue:
    loop .poll
    dec bx
    jnz .outer
    inc word [sgp_timeouts]
    stc
    ret
.ready:
    clc
    ret

wait_vblank_edge:
    mov dx, PORT_TSP_STATUS
    mov bx, 4
.wait_display:
    mov cx, 0xffff
.display_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_seen
    loop .display_poll
    dec bx
    jnz .wait_display
    inc word [vblank_timeouts]
    stc
    ret
.display_seen:
    mov bx, 4
.wait_vblank:
    mov cx, 0xffff
.vblank_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .vblank_poll
    dec bx
    jnz .wait_vblank
    inc word [vblank_timeouts]
    stc
    ret
.ready:
    inc word [vblank_edges_seen]
    clc
    ret

poll_escape:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, KEY_SCAN_ESCAPE
    jne .none
    cmp al, KEY_INTERNAL_ESCAPE
    je .escape
.none:
    clc
    ret
.escape:
    stc
    ret

physical_address_from_ds_si:
    mov ax, ds
    xor dx, dx
    mov cx, 4
.shift:
    shl ax, 1
    rcl dx, 1
    loop .shift
    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    mov dx, VIDEO_BIOS_DATA_SEG
    mov es, dx
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    ret

restore_video_state:
    push cs
    pop ds
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    cmp byte [video_mode_changed], 0
    je .done
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
.done:
    ret

restore_normal_guards:
    cmp byte [ordinary_guards_active], 0
    je .done
    call select_ordinary_mapping
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, saved_normal_outside
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, saved_normal_under
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov byte [ordinary_guards_active], 0
.done:
    ret

restore_bms_and_guards:
    call restore_probe_banks
    call select_ordinary_mapping
    call restore_normal_guards
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

message_start:
    db "M98R_INIT: selectable VBLANK cadence zoom", 13, 10
    db "Selector 0 is ordinary RAM; selector 1 is the atlas bank.", 13, 10, "$"
message_done:
    db "M98R_EXIT: ordinary mapping, keyboard, and video state restored.", 13, 10, "$"
message_option_failed:
    db "M98R_OPTION: use zero or one exact /V1 through /V8 option.", 13, 10, "$"
message_bms_failed:
    db "M98R_FAIL: predecessor BMS mapping probe failed.", 13, 10, "$"
message_atlas_failed:
    db "M98R_FAIL: atlas validation or streaming failed.", 13, 10, "$"
message_descriptor_failed:
    db "M98R_FAIL: scale descriptor or bounded invariant failed.", 13, 10, "$"
message_transfer_failed:
    db "M98R_FAIL: hidden-page SGP batch failed.", 13, 10, "$"
message_runtime_failed:
    db "M98R_FAIL: bounded VBLANK edge wait timed out.", 13, 10, "$"
atlas_filename:
    db "ZUNDORB.BIN", 0

; Keep every capture PC stable even as the surrounding helpers evolve.
times PROBE_CHECKPOINT_OFFSET - ($ - $$) db 0x90
probe_checkpoint:
    jmp probe_resume
times LOAD_CHECKPOINT_OFFSET - ($ - $$) db 0x90
load_checkpoint:
    jmp load_resume
times TRANSFER_CHECKPOINT_OFFSET - ($ - $$) db 0x90
transfer_checkpoint:
    jmp transfer_resume
times FLIP_CHECKPOINT_OFFSET - ($ - $$) db 0x90
flip_checkpoint:
    jmp flip_resume
times SETTLED_CHECKPOINT_OFFSET - ($ - $$) db 0x90
settled_checkpoint:
    jmp settled_resume
times REPORT_A_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_a_checkpoint:
    jmp report_a_resume
times REPORT_B_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_b_checkpoint:
    jmp report_b_resume
times REPORT_C_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_c_checkpoint:
    jmp report_c_resume
times REPORT_D_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_d_checkpoint:
    jmp report_d_resume
times REPORT_E_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_e_checkpoint:
    jmp report_e_resume
times REPORT_F_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_f_checkpoint:
    jmp report_f_resume
times REPORT_G_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_g_checkpoint:
    jmp report_g_resume
times REPORT_H_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_h_checkpoint:
    jmp report_h_resume
times REPORT_I_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_i_checkpoint:
    jmp report_i_resume
times REPORT_J_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_j_checkpoint:
    jmp report_j_resume
times REPORT_K_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_k_checkpoint:
    jmp report_k_resume

align 2, db 0
sgp_command_list:
    times SGP_COMMAND_LIST_WORDS dw 0
sgp_work_area:
    times 29 dw 0
checker_row_a:
    times 8 db 0x24
    times 8 db 0x49
checker_row_b:
    times 8 db 0x49
    times 8 db 0x24
qa_ladder_publications: dw 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56
qa_ladder_divisors: db 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1
guard_normal_outside: db 0x5a,0xa5,0x3c,0xc3,0x69,0x96,0x0f,0xf0
guard_normal_under:   db 0xa5,0x5a,0xc3,0x3c,0x96,0x69,0xf0,0x0f
signature_bank_1:    db 0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81
signature_bank_2:    db 0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82
signature_bank_128:  db 0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88

align 2, db 0
screen_pitch_word: dw SCREEN_PITCH
page_sgp_low: dw G1_PAGE_A_SGP_BASE & 0xffff, G1_PAGE_B_SGP_BASE & 0xffff
page_sgp_high: dw G1_PAGE_A_SGP_BASE >> 16, G1_PAGE_B_SGP_BASE >> 16
page_dsa_low: dw G1_PAGE_A_DSA & 0xffff, G1_PAGE_B_DSA & 0xffff
page_dsa_high: dw G1_PAGE_A_DSA >> 16, G1_PAGE_B_DSA >> 16
page_state: db PAGE_UNINITIALIZED, PAGE_UNINITIALIZED
page_old_valid: db 0, 0
page_old_scale_id: db 0, 0
visible_page_index: db M98Q_INITIAL_VISIBLE_PAGE
hidden_page_index: db 1 - M98Q_INITIAL_VISIBLE_PAGE
settled_capture_count: db 0
current_scale_id: db ATLAS_SCALE_COUNT
scale_direction: db 0
selected_scale_id: db 0
last_published_scale_id: db 0
last_published_direction: db 0
descriptor_validation_id: db 0
runtime_failure_kind: db 0
exit_errorlevel: db 1
dirty_clear_needed: db 0
parsed_divisor: db CADENCE_MIN
active_divisor: db CADENCE_MIN
requested_divisor: db CADENCE_MIN
divider_count: db 0
paused: db 0
pause_toggle_pending: db 0
exit_requested: db 0
eligible_publication: db 0
hidden_render_state: db RENDER_IDLE
vblank_last_high: db 0
scheduler_active: db 0
scheduler_boundary_reset: db 0
cadence_option_seen: db 0
keyboard_repeat_disabled: db 0
qa_stage: db 0
qa_pause_edges_remaining: db 0
align 2, db 0
page_old_x: dw 0, 0
page_old_y: dw 0, 0
page_old_width: dw 0, 0
page_old_height: dw 0, 0
pending_x: dw 0
pending_y: dw 0
pending_width: dw 0
pending_height: dw 0
pending_scale_id: db 0
align 2, db 0
dirty_clear_x0: dw 0
dirty_words_per_row: dw 0
dirty_rows_remaining: dw 0
dirty_batch_rows: dw 0
dirty_row_address: dw 0, 0
dirty_builder_failed: dw 0
last_published_dsa: dw G1_PAGE_A_DSA & 0xffff
psp_segment: dw 0
exit_message: dw message_transfer_failed
pages_initialized: dw 0
render_batches_started: dw 0
render_batches_completed: dw 0
initial_full_page_clears: dw 0
steady_full_page_clears: dw 0
dirty_rect_clears: dw 0
transparent_bitblts: dw 0
vblank_edges_seen: dw 0
page_flips: dw 0
page_a_publications: dw 0
page_b_publications: dw 0
sgp_timeouts: dw 0
sgp_errors: dw 0
vblank_timeouts: dw 0
bms_bank_switches: dw 0
bms_bank_selections: dw 0
descriptor_errors: dw 0
cleanup_runs: dw 0
vblank_edges_total: dw 0
vblank_edges_unpaused: dw 0
vblank_edges_paused: dw 0
divider_change_requests: dw 0
divider_changes_applied: dw 0
divider_boundary_resets: dw 0
pause_requests: dw 0
pause_transitions_applied: dw 0
control_endpoint_hits: dw 0
requested_slots: dw 0
published_updates: dw 0
missed_slots: dw 0
ready_wait_edges: dw 0
scale_advances: dw 0
partial_publication_attempts: dw 0
dirty_full_mismatches: dw 0
guard_failures: dw 0
sgp_command_lists: dw 0
sgp_commands: dw 0
cycles_completed: dw 0
direction_reversals: dw 0
scale_publication_total: dw 0
scale_shrink_total: dw 0
scale_grow_total: dw 0
bounded_validation_pass: dw 0
source_bytes: dw 0, 0
dirty_row_cls_commands: dw 0, 0
dirty_words_cleared: dw 0, 0
dirty_bytes_cleared: dw 0, 0
baseline_full_clear_words: dw 0, 0
baseline_full_clear_bytes: dw 0, 0
publication_digest: dw 0, 0
scale_publications: times ATLAS_SCALE_COUNT dw 0
scale_shrink_publications: times ATLAS_SCALE_COUNT dw 0
scale_grow_publications: times ATLAS_SCALE_COUNT dw 0
scale_page_publications: times ATLAS_SCALE_COUNT * 2 dw 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
saved_video_mode: dw 0
atlas_handle: dw 0xffff
atlas_payload_bytes: dw 0, 0
atlas_file_size: dw 0, 0
atlas_remaining: dw 0, 0
atlas_loaded: dw 0, 0
atlas_chunk_count: dw 0
current_chunk_bytes: dw 0
expected_payload_crc: dw 0, 0
expected_file_crc: dw 0, 0
expected_frame_crc: dw 0, 0
file_crc_state: dw 0, 0
payload_crc_state: dw 0, 0
bms_crc_state: dw 0, 0
frame_crc_state: dw 0, 0
crc_range_offset: dw 0, 0
crc_range_remaining: dw 0, 0
selected_width: dw 0
selected_height: dw 0
selected_pitch: dw 0
selected_anchor_x: dw 0
selected_anchor_y: dw 0
atlas_source_width: dw 0
atlas_source_height: dw 0
atlas_source_anchor_x: dw 0
atlas_source_anchor_y: dw 0
expected_descriptor_offset: dw 0, 0
selected_bank_offset: dw 0, 0
selected_payload_bytes: dw 0, 0
selected_dst_x: dw 0
selected_dst_y: dw 0
selected_source_low: dw 0
selected_source_high: dw 0
saved_normal_outside: times GUARD_BYTES db 0
saved_normal_under: times GUARD_BYTES db 0
saved_bank_1: times GUARD_BYTES db 0
saved_bank_2: times GUARD_BYTES db 0
saved_bank_128: times GUARD_BYTES db 0
saved_memory_map: db 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
video_mode_changed: db 0
initial_bms_selector: db 0
probe_result: db 0
probe_stage: dw 0
saved_probe_banks: db 0
ordinary_guards_active: db 0

align 16, db 0
atlas_metadata:
    times ATLAS_METADATA_BYTES db 0
align 16, db 0
staging_buffer:
    times STAGING_BYTES db 0
program_end:

%if program_end - $$ >= 65280
%error "M98r guest exceeds the 64-KiB DOS payload limit"
%endif
