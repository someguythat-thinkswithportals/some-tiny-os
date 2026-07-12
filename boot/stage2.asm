[org 0x7E00]
[bits 16]

stage2_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [drive], dl

    mov dx, 0xE9
    mov al, '2'
    out dx, al

    mov si, msg_kernel
    call print_string

    mov dx, 0xE9
    mov al, 'R'
    out dx, al

    mov si, dap_kernel
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jnc kernel_loaded

    mov dx, 0xE9
    mov al, 'F'
    out dx, al
.hang:
    hlt
    jmp .hang

kernel_loaded:
    mov dx, 0xE9
    mov al, 'K'
    out dx, al

    in al, 0x92
    or al, 2
    out 0x92, al

    mov dx, 0xE9
    mov al, 'A'
    out dx, al

    lgdt [gdt_ptr]

    mov dx, 0xE9
    mov al, 'G'
    out dx, al

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    mov dx, 0xE9
    mov al, 'E'
    out dx, al

    jmp 0x08:pm_entry

[bits 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000

    mov dx, 0xE9
    mov al, 'P'
    out dx, al

    cld
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 0x10000
    rep movsb

    mov edi, 0x1000
    xor eax, eax
    mov ecx, 0x1000
    rep stosd

    mov dword [0x1000], 0x2007
    mov dword [0x1004], 0x0000

    mov dword [0x2000], 0x3007
    mov dword [0x2004], 0x0000

    mov dword [0x3000], 0x000083
    mov dword [0x3004], 0x00000000

    mov dword [0x3008], 0x200083
    mov dword [0x300C], 0x00000000

    mov dword [0x3010], 0x400087
    mov dword [0x3014], 0x00000000

    mov dword [0x3018], 0x600083
    mov dword [0x301C], 0x00000000

    mov eax, 0x1000
    mov cr3, eax

    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp 0x18:lm_entry

[bits 64]
lm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x9000

    mov dx, 0xE9
    mov al, 'L'
    out dx, al

    mov rax, 0x100000
    jmp rax

[bits 32]
disk_error:
    mov esi, msg_err
    call print_string_32
    hlt

print_string_32:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_string_32
.done:
    ret

[bits 16]
print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

drive: db 0
msg_kernel: db ".", 0
msg_err: db "Kernel load error", 0

dap_kernel:
    db 0x10
    db 0
    dw 128
    dw 0x0000
    dw 0x1000
    dq 3

gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00209A0000000000
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dq gdt_start
