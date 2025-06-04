#pragma once
#include "lexer.hpp"
#include <vector>
#include <cstdio>
#include <cassert>

struct WordDefinition {
	Token name;
	std::vector<Token> body;
	std::vector<Token> locals;
	
	WordDefinition(Lexer &l) {
		assert(l.front.type == Identifier);
		name = l.front;

		l.advance();
		while ((l.front.type != SemiColon) && (l.front.type != EndOfFile)) {
			if (l.front.type == VarKw) {
				l.advance();
				assert(l.front.type == Identifier);
				locals.push_back(l.front);
			} else {
				body.push_back(l.front);
			}
			l.advance();
		}
		assert(l.front.type == SemiColon);
		l.advance();
	}
};

struct Parser {
	char *source;
	char const*name;
	
	std::vector<WordDefinition> words;
	
	Parser(char *_source): source(_source) {
		Lexer l{source};
		while(l.front.type != EndOfFile) {
			if (l.front.type == Colon) {
				l.advance();
				words.emplace_back(l);
			} else {
				printf("Unknown Top Level Declaration\n");
				assert(0);
			}
		}
		assert(l.front.type == EndOfFile);
	}
};
