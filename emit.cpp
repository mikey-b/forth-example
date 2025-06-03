#include "parser.hpp"
#include <cstdio>
#include <cstring>

int pushed_bytes = 0;
int loop_depth = 0;
int if_depth = 0;

static void cs_push(FILE *fp, char const *from) {
	pushed_bytes += 8;	
	fprintf(fp, "push %s\n", from);
}

static void cs_pop(FILE *fp, char const *to) {
	pushed_bytes -= 8;
	fprintf(fp, "pop %s\n", to);
}

static void print(FILE *fp, char const *msg, char const *reg) {
	cs_push(fp, "r8");
	cs_push(fp, "r9");
	cs_push(fp, "r10");	
	int align = 32 + ( (16 - (pushed_bytes % 16)) % 16 );
	fprintf(fp, "sub rsp, %d\n", align);
	fprintf(fp, "lea rcx, %s[rip]\n", msg);
	fprintf(fp, "mov rdx, %s\n", reg);
	fprintf(fp, "call printf\n");
	fprintf(fp, "add rsp, %d\n", align);
	cs_pop(fp, "r10");
	cs_pop(fp, "r9");
	cs_pop(fp, "r8");
}

static void ds_push(FILE *f, int v) {
	fprintf(f, "sub r10, 8\n");
	fprintf(f, "mov QWORD PTR [r10], %d\n", v);
}

static void ds_push(FILE *fp, char const *from) {
	fprintf(fp, "sub r10, 8\n");
	fprintf(fp, "mov QWORD PTR [r10], %s\n", from);
	
	//print(fp, ".STR1", from);
}

static void ds_pop(FILE *fp, char const *to) {
	fprintf(fp, "mov %s,QWORD PTR [r10]\n", to);
	fprintf(fp, "add r10, 8\n");
	
	//print(fp, ".STR2", to);
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
					case '+': { // Add top two values
						ds_pop(fp, "rax");
						ds_pop(fp, "rbx");
						fprintf(fp, "add rax, rbx\n");
						ds_push(fp, "rax");						
						break;
					}
					case '.': { // Print the top of the stack value.
						ds_pop(fp, "rdx");
						print(fp, ".STR0", "rdx");
						break;
					}
				}
				break;
			}
			case Identifier: {
				if (strncmp(ins.start, "dup", 3) == 0) { // Duplicate the top value.
					ds_pop(fp, "rax");
					ds_push(fp, "rax");
					ds_push(fp, "rax");
				} else {
					fprintf(fp, "call %.*s\n", (ins.end - ins.start), ins.start);
				}
				break;
			}
			
			case IfKw: {
				ds_pop(fp, "r8");
				
				fprintf(fp, "test r8, r8\n");
				if_depth += 1;
				fprintf(fp, "jz 8%df\n", if_depth);
				break;
			}
			case ElseKw: {
				fprintf(fp, "jmp 7%df\n", if_depth);
				fprintf(fp, "8%d:\n", if_depth);
				
				break;
			}				
			case ThenKw: { 
				fprintf(fp, "7%d:\n", if_depth);
				fprintf(fp, "8%d:\n", if_depth);				
				if_depth -= 1;
				break;
			}
			
			case DoKw: { // Loops
				loop_depth += 1;
				fprintf(fp, "9%d:\n", loop_depth); // We create a local label to jump back to.
				
				// We move the top two stack values onto the call stack. These are the trip count and exit values.
				ds_pop(fp, "r9");
				cs_push(fp, "r9");
				
				ds_pop(fp, "r8");
				cs_push(fp, "r8");
				break;
			}
			case LoopKw: {
				// Read from the call stack the trip count and exit values.
				
				cs_pop(fp, "r9"); // Trip Count
				cs_pop(fp, "r8"); // Exit Value
				
				fprintf(fp, "sub r9, 1\n");
				
				ds_push(fp, "r9");
				ds_push(fp, "r8");
				
				fprintf(fp, "cmp r9, r8\n");
				
				fprintf(fp, "jnz 9%db\n", loop_depth);
				loop_depth -= 1;
				
				// Throw away the trip count and exit values from the data stack.
				ds_pop(fp, "rax");
				ds_pop(fp, "rax");
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
	fprintf(fp, ".STR1: .asciz \"Pushing: %%d \\n\"\n");
	fprintf(fp, ".STR2: .asciz \"Popping: %%d \\n\"\n");
	
	fprintf(fp, ".text\n");
	fprintf(fp, ".globl main\n");
	
	for(auto& word: self->words) {
		emit(&word, fp);
	}
}
