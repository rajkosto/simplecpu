MOV R0, _FIBOARR
IN R1
CALL FIBO
CALL PRINTARR
HLT
_FIBOARR: DW #0 DUP 1000 //space for values (code goes after it so it must be enough)