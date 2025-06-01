#pragma once

enum tokenType {
	EndOfFile,
	Integer,
	Operator,
	Identifier,
	SemiColon,
	Colon,
	
	DoKw,
	LoopKw,
};

struct Token {
	tokenType type;
	char *start;
	char *end;
	int line, col;
};

struct Lexer {
	char *pos;
	Token front;
	int line, col;
	
	Lexer(char *source): pos(source) {
		line = 1;
		col = 0;
		advance();
	}
		
	void advance();
};
