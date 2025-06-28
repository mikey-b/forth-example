#include "parser.hpp"
#include <cstdio>
#include <cstring>

void emit(Parser*, FILE*);

char *readfile(char const *filename) {
	FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return nullptr;
    }
	
	fseek(f, 0, SEEK_END);
	auto fsize = ftell(f);
	rewind(f);
	
	char *result = static_cast<char*>(malloc(fsize + 1));
    if (!result) {
        fprintf(stderr, "malloc failed\n");
        fclose(f);
        return nullptr;
    }
	
    if (fread(result, 1, fsize, f) != static_cast<size_t>(fsize)) {
        fprintf(stderr, "fread failed\n");
        free(result);
        fclose(f);
        return nullptr;
    }
	result[fsize] = 0;
	
	fclose(f);
	return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_binary>\n", argv[0]);
        return 1;
    }	
	
    const char* sourceFile = argv[1];
    const char* outputBinary = argv[2];	
	
	auto source = readfile(sourceFile);
    if (!source) {
        fprintf(stderr, "Error: Could not read source file '%s'\n", sourceFile);
        return 1;
    }	
	
	Parser p{source};
	p.name = sourceFile;
	
	char outcmd[255];
	snprintf(outcmd, sizeof(outcmd), "gcc -g -o \"%s\" -x assembler -", outputBinary);
	
	FILE *target;
	if (strncmp(outputBinary, "-", 1) == 0) {
		target = stdout;
	} else {
		target = popen(outcmd, "w");
	}
	
    if (!target) {
        fprintf(stderr, "Error: Failed to invoke GCC\n");
        free(source);
        return 1;
    }
	
	emit(&p, target);
	//emit_debug(&p, target);
	
	pclose(target);
	free(source);
	return 0;
}
