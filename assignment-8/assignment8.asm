section .text
global _start
_start:
    mov edx, len
    mov ecx, msg
    mov ebx, 1
    mov eax, 0xFFF
    int 0x80

    xor ebx, ebx
    mov eax, 1
    int 0x80
    ret

section .data
msg db 'Hello World.', 0xA
len equ $ - msg