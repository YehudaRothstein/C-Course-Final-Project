.data 6,-9,15
STR: .string "hello” 
.data 22 
M1: .mat [2][2] 1,2,3,4 
K: .data 0 
MAIN: 
mov #4, r1 
mov M1[r0][r0], r2 
prn #-5 
LOOP: add r2, r1 
inc r1 
cmp r1, #10 
bne LOOP 
mov r1, K 
jmp END 
END: 
prn STR 
stop
