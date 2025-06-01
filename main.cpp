#include "parser.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

void emit(Parser*, FILE*);

char *readfile(char const *filename) {
	FILE *f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	auto fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char *result = static_cast<char*>(malloc(fsize + 1));
	fread(result, fsize, 1, f);
	fclose(f);
	
	result[fsize] = 0;
	return result;
}

int main(int argc, char *argv[]) {
	auto sourceFile = "test.f";
	auto source = readfile(sourceFile);
	
	Parser p{source};
	p.name = "test.f";
	
	char outfile[] = "test.exe";
	char outcmd[80];
	snprintf(outcmd, sizeof(outcmd), "gcc -g -o %s -x assembler -", outfile);
	
	FILE *target = popen(outcmd, "w");	
	//target = stdout;
	
	emit(&p, target);
	
	fclose(target);
	free(source);
	return 0;
}
