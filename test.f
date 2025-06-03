: ten 10 ;
: fourtytwo ten 32 + ;
: iflooptest 77 0 IF 3 0 DO ten + LOOP ELSE fourtytwo + THEN .;

: fib31 0 DUP . 1 DUP . 30 0 DO OVER OVER  + DUP . LOOP ;

: main fib31 0;