//expects address of destination array in R0
//expects number of iterations in R1
//trashes R12-R14
MOV R0, TESTDATA
MOV R1, #30
CALL PRINTARR
HLT
TESTDATA: DW #-1, #0, #1 DUP 10
	
	