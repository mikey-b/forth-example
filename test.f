: ten 10 ;
: fourtytwo ten 32 + ;
: iflooptest 77 0 IF 3 0 DO ten + LOOP ELSE fourtytwo + THEN .;

: fib
	VAR c 
	c ! 
	0 DUP . 1 DUP . 
	c @ 0 DO 
		OVER OVER  + DUP . 
	LOOP ;

: main 10 fib 0;
