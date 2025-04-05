#include "lexer.hpp"
#include <cassert>
#include <cstdio>

static bool isalpha(char c) {
	return ((unsigned)c|32)-'a' < 26;
}

static bool isdigit(char c) {
	return (unsigned)c-'0' < 10;
}

static bool isalnum(char c) {
	return isalpha(c) || isdigit(c);
}

void Lexer::advance() {
	for(;;) {
		switch(*pos) {
			case '\n': line+=1; col=0;
			case ' ': pos+=1;
				continue;
		}
		break;
	}
	
	switch(*pos) {
		case '\0': {
			front = (Token){EndOfFile};
			return;
		}
		case ':': {
			front = (Token){.type = Colon, .start = pos, .end = pos + 1, .line = line, .col = col};
			pos += 1;
			return;
		}
		case ';': {
			front = (Token){.type = SemiColon, .start = pos, .end = pos + 1, .line = line, .col = col};
			pos += 1;
			return;
		}
		case '0'...'9': {
			auto start = pos;
			while(isdigit(*pos)) pos+=1;
			front = (Token){.type = Integer, .start = start, .end = pos, .line = line, .col = col};
			return;
		}
		case 'a'...'z': case 'A'...'Z': {
			auto start = pos;
			while(isalnum(*pos)) pos+=1;
			front = (Token){.type = Identifier, .start = start, .end = pos, .line = line, .col = col};
			return;
		}
		case '+': {
			front = (Token){.type = Operator, .start = pos, .end = pos + 1, .line = line, .col = col};
			pos+=1;
			return;
		}
		case '.': {
			front = (Token){.type = Operator, .start = pos, .end = pos + 1, .line = line, .col = col};
			pos+=1;
			return;
		}
		default: {
			printf("Unknown Token! '%s'\n", pos);
			assert(0);
		}
	}
}
