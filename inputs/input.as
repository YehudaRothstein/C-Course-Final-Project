MAIN:   mov M1[r2][r7],LENGTH
        add r2,STR

LOOP:   jmp END
        prn #-5

        sub r1, r4
        inc K
        
        sub NUMBER, r2

STR:    .string “abcdef”
LENGTH: .data 6,-9,15
K:  .data 22
M1: .mat [2][2] 1,2,3,4 
