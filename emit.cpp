#include "parser.hpp"
#include <cstdio>
#include <cstring>
#include <cstdint>

static int pushed_bytes = 0;
static int loop_depth = 0;
static int if_depth = 0;

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
}

static void ds_pop(FILE *fp, char const *to) {
	fprintf(fp, "mov %s,QWORD PTR [r10]\n", to);
	fprintf(fp, "add r10, 8\n");
}

static int findTokenByText(const std::vector<Token>& tokens, const Token& t) {
	size_t textlen = t.end - t.start;
	
	for(size_t i = 0; i < tokens.size(); ++i) {
		const Token& token = tokens[i];
		size_t tokenlen = token.end - token.start;
		
		if (tokenlen == textlen && std::strncmp(token.start, t.start, textlen) == 0) {
			return static_cast<int>(i);
		}
	}

	return -1;
}

static void emit(WordDefinition *self, FILE *fp) {
	fprintf(fp, "%.*s:\n", (int)(self->name.end - self->name.start), self->name.start);
	fprintf(fp, ".loc 1 %d\n", self->name.line);
	
	fprintf(fp, "push rbp\n");
	fprintf(fp, "mov rbp, rsp\n");
	{ 	// Add locals to the stack frame 1 = rbp + locals * 8 bytes.
		int pushed_bytes = (1 + self->locals.size()) * 8;
		int padding = 32 + ( (16 - (pushed_bytes % 16)) % 16 );
		fprintf(fp, "sub rsp, %d\n", pushed_bytes + padding);
		
		// Set all locals to zero.
		for(size_t i = 0; i < self->locals.size(); ++i) {
			int32_t displacement = ((i + 1) * 8);
			fprintf(fp, "mov QWORD PTR [rbp - %d], 0\n", displacement);
		}
	}
	
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
					case '-': { // Subtract top two values
						ds_pop(fp, "rbx");
						ds_pop(fp, "rax");
						fprintf(fp, "sub rax, rbx\n");
						ds_push(fp, "rax");
						break;
					}
					case '*': {
						ds_pop(fp, "rbx");
						ds_pop(fp, "rax");
						fprintf(fp, "imul rax, rbx\n");
						ds_push(fp, "rax");
						break;
					}
					case '/': {
						ds_pop(fp, "rbx");
						ds_pop(fp, "rax");
						fprintf(fp, "cqo\n");
						fprintf(fp, "idiv rax, rbx\n");
						ds_push(fp, "rax");
						break;
					}
					case '%': {
						ds_pop(fp, "rbx");
						ds_pop(fp, "rax");
						fprintf(fp, "cqo\n");
						fprintf(fp, "idiv rax, rbx\n");
						ds_push(fp, "rdx");
						break;
					}					
					case '.': { // Print the top of the stack value.
						ds_pop(fp, "rdx");
						print(fp, ".STR0", "rdx");
						break;
					}
					case '@': { // Read a memory address
						ds_pop(fp, "rax");
						fprintf(fp, "mov rax, QWORD PTR [rax]\n");
						ds_push(fp, "rax");
						break;
					}
					case '!': { // Write a value to memory address
						ds_pop(fp, "rax");
						ds_pop(fp, "rbx");
						fprintf(fp, "mov QWORD PTR [rax], rbx\n");
						break;
					}
				}
				break;
			}
			case Identifier: {
				if (strncmp(ins.start, "OVER", 4) == 0) {
					ds_pop(fp, "rax");
					ds_pop(fp, "rbx");
					
					ds_push(fp, "rbx");
					ds_push(fp, "rax");
					ds_push(fp, "rbx");
				} else if (strncmp(ins.start, "SWAP", 4) == 0) {
					ds_pop(fp, "r8");
					ds_pop(fp, "r9");
					ds_push(fp, "r8");
					ds_push(fp, "r9");
				} else if (strncmp(ins.start, "DROP", 4) == 0) {
					ds_pop(fp, "rax");
				} else if (strncmp(ins.start, "DUP", 3) == 0) { // Duplicate the top value.
					ds_pop(fp, "rax");
					ds_push(fp, "rax");
					ds_push(fp, "rax");
				} else {
					// Is this a variable?
					int index = findTokenByText(self->locals, ins);
					if (index != -1) {
						fprintf(fp, "lea rax, [rbp - %d]\n", ((index + 1) * 8));
						ds_push(fp, "rax");
					} else {					
						fprintf(fp, "call %.*s\n", (int)(ins.end - ins.start), ins.start);
					}
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
				printf("Unknown token %d '%.*s'\n", ins.type, (int)(ins.end - ins.start), ins.start);
				assert(0);
			}
		}
	}
	
	if (strncmp("main", self->name.start, 4) == 0) {
		ds_pop(fp, "rax");
	}
	
	fprintf(fp, "mov rsp, rbp\n");
	fprintf(fp, "pop rbp\n");
	fprintf(fp, "ret\n");
}

void emit(Parser *self, FILE *fp) {
	fprintf(fp, ".intel_syntax noprefix\n");
	fprintf(fp, ".file 1 \"%s\"\n", self->name);
	
	fprintf(fp, ".section .bss\n");
	fprintf(fp, ".globl data_stack\n");
	fprintf(fp, "data_stack: .space %d\n", 4096);
		
	fprintf(fp, ".section .rodata\n");
	fprintf(fp, ".STR0: .asciz \"%%d \"\n");
	
	fprintf(fp, ".text\n");
	fprintf(fp, ".Ltext_start:\n"); // DEBUG LABEL
	fprintf(fp, ".globl main\n");
	
	for(auto& word: self->words) {
		emit(&word, fp);
	}
	
	fprintf(fp, ".Ltext_end:\n"); // DEBUG LABEL
}
