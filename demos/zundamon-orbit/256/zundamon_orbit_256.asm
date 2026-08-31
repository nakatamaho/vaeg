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
%define G1_BACKING_WORD_COUNT   0xfa00

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

%define PROBE_CHECKPOINT_IP     0x3000
%define LOAD_CHECKPOINT_IP      0x3010
%define TRANSFER_CHECKPOINT_IP  0x3020
%define IDLE_CHECKPOINT_IP      0x3030
%define PROBE_CHECKPOINT_OFFSET (PROBE_CHECKPOINT_IP - 0x0100)
%define LOAD_CHECKPOINT_OFFSET  (LOAD_CHECKPOINT_IP - 0x0100)
%define TRANSFER_CHECKPOINT_OFFSET (TRANSFER_CHECKPOINT_IP - 0x0100)
%define IDLE_CHECKPOINT_OFFSET  (IDLE_CHECKPOINT_IP - 0x0100)

start:
    ; PC-Engine may choose a different COM load segment.  Relocate the guest
    ; to the established private-RAM segment so debug checkpoints are stable.
    mov ax, cs
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
    mov dx, message_bms_failed
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
    call initialize_video_and_transfer
    jc transfer_failed
    mov bx, [selected_dst_x]
    mov cx, [selected_dst_y]
    mov dx, SGP_BITBLT_COPY_XPAR
    mov si, [selected_source_low]
    mov di, [selected_source_high]
    mov bp, 0x0101
    mov ax, 0x98c1
    jmp transfer_checkpoint

transfer_resume:
idle_loop:
    call wait_vblank_start
    jc runtime_failed
    call poll_escape
    jc normal_exit
    ; No redraw occurs in this loop.  The two captures must be identical.
    push cs
    pop ds
    push cs
    pop es
    mov bx, [selected_width]
    mov cx, [selected_height]
    mov dx, [selected_pitch]
    mov si, 0x0101
    mov di, [selected_dst_y]
    mov bp, [selected_dst_x]
    mov ax, 0x984c
    jmp idle_checkpoint

idle_resume:
    jmp idle_loop

normal_exit:
    call restore_bms_and_guards
    call restore_video_state
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
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
    mov dx, message_atlas_failed
    jmp fatal_exit
transfer_failed:
    mov dx, message_transfer_failed
    jmp fatal_exit
runtime_failed:
    mov dx, message_runtime_failed

fatal_exit:
    push dx
    call close_atlas_if_open
    call restore_bms_and_guards
    call restore_video_state
    pop dx
    call print_string
    mov ax, 0x4c01
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

    mov bx, atlas_metadata + ATLAS_SELECTED_DESC
    mov ax, [bx + 0]
    test ax, ax
    long_jz .failed
    cmp ax, SCREEN_WIDTH
    long_ja .failed
    mov [selected_width], ax
    mov ax, [bx + 2]
    test ax, ax
    long_jz .failed
    cmp ax, SCREEN_HEIGHT
    long_ja .failed
    mov [selected_height], ax
    mov ax, [bx + 4]
    mov [selected_pitch], ax
    mov dx, [selected_width]
    add dx, 3
    and dx, 0xfffc
    cmp ax, dx
    long_jne .failed
    cmp word [bx + 10], 0
    long_jne .failed
    cmp word [bx + 12], 0
    long_jne .failed
    cmp word [bx + 14], 0
    long_jne .failed
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
    long_jne .failed
    cmp di, dx
    long_jne .failed
    mov ax, [bx + 24]
    mov dx, [bx + 26]
    test dx, dx
    long_jnz .failed
    mov [selected_payload_bytes], ax
    mov [selected_payload_bytes + 2], dx
    mov ax, [selected_pitch]
    mul word [selected_height]
    cmp ax, [selected_payload_bytes]
    long_jne .failed
    test dx, dx
    long_jnz .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add ax, [selected_payload_bytes]
    adc dx, [selected_payload_bytes + 2]
    cmp dx, [atlas_payload_bytes + 2]
    long_ja .failed
    jb .range_ok
    cmp ax, [atlas_payload_bytes]
    long_ja .failed
.range_ok:
    mov ax, [bx + 28]
    mov [expected_frame_crc], ax
    mov ax, [bx + 30]
    mov [expected_frame_crc + 2], ax
    mov ax, SCREEN_WIDTH
    sub ax, [selected_width]
    shr ax, 1
    mov [selected_dst_x], ax
    mov ax, SCREEN_HEIGHT
    sub ax, [selected_height]
    shr ax, 1
    mov [selected_dst_y], ax
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

; G98l-C: establish the M98k 320x200 8-bpp G0/G1 path, then issue one
; transparent BITBLT whose source remains inside the selected BMS window.
initialize_video_and_transfer:
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
    call build_bms_commands

    call verify_staging_poison
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_FIRST_SELECTOR
    out dx, al
    call run_sgp_command_list
    jc .failed
    call verify_staging_poison
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    call select_ordinary_mapping
    call verify_normal_guards
    jc .failed
    call restore_normal_guards
    call display_page_a
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

display_page_a:
    mov dx, PORT_FB1_DSA_LOW
    mov ax, G1_PAGE_A_DSA & 0xffff
    out dx, ax
    add dx, 2
    mov ax, G1_PAGE_A_DSA >> 16
    out dx, ax
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

build_bms_commands:
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
    ; The sole SGP submission clears G1 and performs exactly one BITBLT.
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    stosw
    mov ax, G1_BACKING_WORD_COUNT
    stosw
    xor ax, ax
    stosw

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
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add dx, BMS_WINDOW_SGP_BASE >> 16
    mov [selected_source_low], ax
    mov [selected_source_high], dx
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 2
    stosw
    mov ax, [selected_width]
    stosw
    mov ax, [selected_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [selected_dst_y]
    mul word [screen_pitch_word]
    add ax, [selected_dst_x]
    adc dx, 0
    add ax, G1_PAGE_A_SGP_BASE & 0xffff
    adc dx, G1_PAGE_A_SGP_BASE >> 16
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    mov ax, SGP_COMMAND_END
    stosw
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
    call wait_sgp_idle
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
    loop .poll
    dec bx
    jnz .outer
    stc
    ret
.ready:
    clc
    ret

wait_vblank_start:
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
    stc
    ret
.ready:
    clc
    ret

poll_escape:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0
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
    db "M98L_INIT: BMS stream and direct SGP-to-G1 proof", 13, 10
    db "Selector 0 is ordinary RAM; selector 1 is the atlas bank.", 13, 10, "$"
message_done:
    db "M98L_EXIT: ordinary mapping and video state restored.", 13, 10, "$"
message_bms_failed:
    db "M98L_A_FAIL: BMS mapping probe failed.", 13, 10, "$"
message_atlas_failed:
    db "M98L_B_FAIL: atlas validation or streaming failed.", 13, 10, "$"
message_transfer_failed:
    db "M98L_C_FAIL: direct BMS SGP transfer failed.", 13, 10, "$"
message_runtime_failed:
    db "M98L_C_FAIL: bounded VBLANK wait timed out.", 13, 10, "$"
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
times IDLE_CHECKPOINT_OFFSET - ($ - $$) db 0x90
idle_checkpoint:
    jmp idle_resume

align 2, db 0
sgp_command_list:
    times 64 dw 0
sgp_work_area:
    times 29 dw 0
checker_row_a:
    times 8 db 0x24
    times 8 db 0x49
checker_row_b:
    times 8 db 0x49
    times 8 db 0x24
guard_normal_outside: db 0x5a,0xa5,0x3c,0xc3,0x69,0x96,0x0f,0xf0
guard_normal_under:   db 0xa5,0x5a,0xc3,0x3c,0x96,0x69,0xf0,0x0f
signature_bank_1:    db 0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81
signature_bank_2:    db 0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82
signature_bank_128:  db 0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88

align 2, db 0
screen_pitch_word: dw SCREEN_PITCH
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
%error "M98l guest exceeds the 64-KiB DOS payload limit"
%endif
