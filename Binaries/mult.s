MULT: MOV R0, #0 //expects op1 in R1, op2 in R2, result is in R0, trashes R14
MOV R14, R2
_REPEAT: ADD R0, R0, R1
SUB R14, R14, #1
CMP R14, #0
JZ _DONE
JMP _REPEAT
_DONE: MOV R14, #0
RET
