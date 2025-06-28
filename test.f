: ten 10 ;
: fourtytwo ten 32 + ;
: iflooptest 77 0 IF 3 0 DO ten + LOOP ELSE fourtytwo + THEN .;
: testdiv 33 4 / . ;
: testmod 33 4 % . ;
: testmul 33 4 * . ;

: fib
	VAR c 
	2 -
	c !
	0 DUP . 1 DUP . 
	c @ 0 DO 
		OVER OVER  + DUP . 
	LOOP ;

: main 
	iflooptest 
	
	testdiv
	testmod
	testmul
	
	10 fib 
	
	0;
