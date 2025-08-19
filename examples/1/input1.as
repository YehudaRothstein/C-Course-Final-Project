; file input1.as

.extern Skibidi

mcro text_macro
mov r3, LIST
add r2, r3
mcroend

STR: .string "lol"
LIST: .data 6,7
S1: inc Skibidi

text_macro
text_macro

stop
