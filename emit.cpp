#include "parser.hpp"
#include <cstdio>
#include <cstring>

static void ds_push(FILE *f, int v) {
	fprintf(f, "sub r10, 8\n");
	fprintf(f, "mov QWORD PTR [r10], %d\n", v);
}

static void ds_push(FILE *f, char const *from) {
	fprintf(f, "sub r10, 8\n");
	fprintf(f, "mov QWORD PTR [r10], %s\n", from);
}

static void ds_pop(FILE *f, char const *to) {
	fprintf(f, "mov %s,QWORD PTR [r10]\n", to);
	fprintf(f, "add r10, 8\n");
}

static void emit(int self, FILE *fp) {
	fprintf(fp, "sub r10, 8\n");
	fprintf(fp, "mov QWORD PTR [r10], %d\n", self);
}

static void emit(WordDefinition *self, FILE *fp) {
	fprintf(fp, "%.*s:\n", (self->name.end - self->name.start), self->name.start);
	fprintf(fp, ".loc 1 %d\n", self->name.line);
	fprintf(fp, "push rbp\n");
	fprintf(fp, "mov rbp, rsp\n");
	fprintf(fp, "sub rsp, 32\n");
	
	if (strncmp("main", self->name.start, 4) == 0) {
		fprintf(fp, "lea r10, [rip + data_stack + %d]	# Set Data Stack Pointer\n", 4096);
	}
	
	for(auto& ins: self->body) {
		fprintf(fp, ".loc 1 %d %d\n", ins.line, ins.col);
		switch(ins.type) {
			case Integer: {
				size_t len = ins.end - ins.start;
				assert(len < 20);
				char tmp[20];
				strncpy(tmp, ins.start, len);
				tmp[len] = '\0';
				auto value = atoi(tmp);
				ds_push(fp, value);
				break;
			}
			case Operator: {
				switch(*ins.start) {
					case '+': {
						ds_pop(fp, "rax");
						ds_pop(fp, "rbx");
						fprintf(fp, "add rax, rbx\n");
						ds_push(fp, "rax");						
						break;
					}
					case '.': {
						// Print the top of the stack value.
						fprintf(fp, "push r10\n");  // Save the datastack pointer before a call
						fprintf(fp, "lea rcx, .STR0[rip]\n");
						ds_pop(fp, "rdx"); 
						fprintf(fp, "call printf\n");
						fprintf(fp, "pop r10\n");  // Restore the datastack pointer
						break;
					}
				}
				break;
			}
			case Identifier: {
				fprintf(fp, "call %.*s\n", (ins.end - ins.start), ins.start);
				break;
			}
			default: {
				printf("Unknown token %d '%.*s'\n", ins.type, (ins.end - ins.start), ins.start);
				assert(0);
			}
		}
	}
	
	if (strncmp("main", self->name.start, 4) == 0) {
		ds_pop(fp, "rax");
	}
	
	fprintf(fp, "add rsp, 32\n");
	fprintf(fp, "pop rbp\n");
	fprintf(fp, "ret\n");
}

void emit(Parser *self, FILE *fp) {
	fprintf(fp, ".intel_syntax noprefix\n");
	fprintf(fp, ".file 1 \"%s\"\n", self->name);
	
	fprintf(fp, ".section .bss\n");
	fprintf(fp, ".lcomm data_stack, %d\n", 4096);
	
	fprintf(fp, ".section .rodata\n");
	fprintf(fp, ".STR0: .asciz \"%%d \"\n");	
	
	fprintf(fp, ".text\n");
	fprintf(fp, ".globl main\n");
	
	for(auto& word: self->words) {
		emit(&word, fp);
	}
}
