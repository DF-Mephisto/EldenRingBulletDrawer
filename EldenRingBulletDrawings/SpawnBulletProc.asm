.DATA

.CODE

spawnBulletProc PROC
sub rsp,28h

mov rcx, qword ptr [data+8]
mov rcx, qword ptr [rcx]
mov r8, qword ptr [data+16]
mov r9, r8
add r9, 7E0h
mov rdx, r8
add rdx, 7D0h

;mov qword ptr [data+16], r8

call qword ptr [data]

;mov qword ptr [data+24], rcx
;mov qword ptr [data+32], r8
;mov qword ptr [data+40], r9
;mov qword ptr [data+48], rdx
;call rcx

add rsp,28h

ret
nop
nop
nop
nop
nop
data:
spawnBulletProc ENDP

END