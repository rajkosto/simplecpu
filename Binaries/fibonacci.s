//expects address of destination array in R0
//expects number of iterations in R1
//trashes R10-R14
FIBO: 		MOV R11, #0 //a
		MOV R12, #1 //b
		MOV R13, #0 //sum
		MOV R14, #0 //i
		MOV R10, R0 //memory pointer
_REPEAT:	SAL R10, R14, #1 //*2 for word length
		ADD R10, R0, R10 //memptr = &dest[i]
		STR R10, R11 //output a to *memptr
		ADD R13, R11, R12 //sum = a + b
		MOV R11, R12 //a = b
		MOV R12, R13 //b = sum
		ADD R14, R14, #1 //i++
		CMP R1, R14 //if (num > i)
		JGT _REPEAT //keep going
		RET
	
	
	