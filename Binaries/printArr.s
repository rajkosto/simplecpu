//expects address of destination array in R0
//expects number of iterations in R1
//trashes R12-R14

PRINTARR:	MOV R14, #0 //i
		MOV R13, R0 //memory pointer
_REPEAT:	SAL R12, R14, #1 //*2 for word size
		ADD R13, R0, R12 //memptr = &dest[i]
		LDR R12, R13 //val = *memptr
		OUT R12 //print val
		ADD R14, R14, #1 //i++
		CMP R1, R14 // if (num > i)
		JGT _REPEAT
		RET	
	
	