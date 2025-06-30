start:
    mov r1, r2

mcro whathell
    mov r0, #"Hello"
    b   print
mcroend

mcro test
    add r1, r2
mcroend
