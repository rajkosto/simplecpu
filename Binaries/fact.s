//expects number to factor in R1
//returns result in R0
//trashes R2, R14
FACT:		MOVFSP R14 //sp
		SUB R14, R14, #2 //sp-2
		STR R14, R3 //back up the local
		MOVTSP R14 //apply stack change
		MOV R3, R1 //put parameter in local
		CMP R3, #1 //(number(now local) <= 1)
		JGT _NOTSMALL //if we are bigger than 1
_SMALL:		MOV R0, #1 //return 1 for small numbers
		MOVFSP R14 //sp
		LDR R3, R14 //get the original local back
		ADD R14, R14, #2 //move the stack back
		MOVTSP R14 //apply stack move
		RET //exit
_NOTSMALL:	SUB R1, R3, #1 //number--
		CALL FACT //r14 now trashed
		MOV R2, R3 //number is 2nd operand
		MOV R1, R0 //factorial is 1st operand
		CALL MULT //r0 now has mult result
		MOVFSP R14 //sp
		LDR R3, R14 //get the original local back
		ADD R14, R14, #2 //move the stack back
		MOVTSP R14 //apply stack move
		RET
		