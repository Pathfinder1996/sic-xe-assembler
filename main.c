#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_OPTAB 100
#define MAX_REGTAB 100
#define MAX_LINEBUFFER 2000
#define MAX_SYMTAB 2000
#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 100
#define MAX_ASMTAB 2000
#define MAX_SYM 2000
#define MAX_OBJTAB 2000
#define MAX_MODIFY 2000

// ====================== //
//    Data Structure      //
// ====================== //
// OPTAB
typedef struct {
    char mnemonic[6];
    int format;  
    char opcode[3];
} OPTABLE;

// Assembler table, pass1 and pass2 list
typedef struct {
    int loc;
    char label[50];
    char opcode[50];
    char operand[50];
    char object_code[50]; // pass2
} ASSEMBLERTABLE;

// SYMTAB
typedef struct {
    char label[7];
    int address;
} SYMTABLE;

// Register table, pass2 use
typedef struct {
    char register_name[3];
    char register_number[2];
} REGTABLE;

// object code table, print record use
typedef struct {
    int code_address;
    char code[100];
} OBJTABLE;

// ====================== //
//     global variable    //
// ====================== //
// OPTAB
OPTABLE OPTAB[MAX_OPTAB];
int optable_size = 0;

// ASM
ASSEMBLERTABLE ASMTAB[MAX_ASMTAB];
int asmtable_size = 0;

// SYMTAB
SYMTABLE SYMTAB[MAX_SYM];
int symtab_count = 0;

REGTABLE REGTAB[MAX_REGTAB];
int regtable_size = 0;

OBJTABLE OBJTAB[MAX_OBJTAB];
int obj_code_size = 0;

// program position counter
int LOCCTR = 0;
// Start label, H record use
char program_name[7] = "";

int program_counter = 0;
int base_address = 0;

int check_program_counter = 0;

int modify[MAX_MODIFY];
int modify_size = 0;

// pass1
void read_mnemonic_file(void);
int pass1(FILE *file_pointer);
void process_asm_input(const char *line_buffer, char *asm_label, char *asm_opc, char *asm_operand);
int split_token(const char *line_buffer, char token_buffer[][MAX_TOKEN_LEN]);
int is_directive_or_mnemonic(const char *token);
int add_symtab(const char *label, int address);
// pass2 
void read_register_file(void);
void pass2();
void process_format2(int index, char *obj_code);
void process_format4(int index, char *obj_code);
void process_format3(int index, char *obj_code);
int find_symbol(const char *find_label);
void write_obj_file();
// Conversion and Relocation Addressing
int hex_to_ten(const char *hex_string);
void ten_to_hex(int value, int width, char *output_string);
void bits_to_hex(int a, int b, int c, int d, char *output_hex);
void reloc_address(const char *address, char *disp);

// ===================== //
//          main         //
// ===================== //
int main() {
    
    FILE *file_pointer = fopen("input.txt", "r");
    if(!file_pointer){
        printf("Cannot open input.txt\n");
        return 0;
    }

    read_mnemonic_file(); // OPTAB
    int pass1_ok = pass1(file_pointer); // assembler pass1 
    if (pass1_ok == -1) {
        fclose(file_pointer);
        return -1;
    }

    read_register_file(); // Register table
    pass2();

    write_obj_file();

    fclose(file_pointer);

    return 0;
}

// ====================== //
//          pass1         //
// ====================== //
// OPTAB
void read_mnemonic_file(void) {

    FILE *file_in = fopen("mnemonic.txt", "r");

    if (!file_in) {
        printf("[Error] cannot open mnemonic.txt\n");
        return;
    }

    while (!feof(file_in)) {
        char op_mnem[6];
        int op_fmat;  
        char op_opc[3];

        // if (fscanf(file_in, "%s %d %s", &op_mnem, &op_fmat, &op_opc) == 3) 
        if (fscanf(file_in, "%5s %d %2s", op_mnem, &op_fmat, op_opc) == 3) {
            strcpy(OPTAB[optable_size].mnemonic, op_mnem);
            OPTAB[optable_size].format = op_fmat;
            strcpy(OPTAB[optable_size].opcode, op_opc);

            optable_size = optable_size + 1;
        }
    }

    fclose(file_in);

    printf("\n=== OPTAB Content ===\n");
    printf("Mnemonic  Format  OPcode\n");
    int i;
    for (i = 0; i < optable_size; i++) {
        printf("%-6s    %-6d  %s\n", OPTAB[i].mnemonic, OPTAB[i].format, OPTAB[i].opcode);
    }
    printf("=====================\n");
}

int pass1(FILE *file_pointer) {
    int is_first_line = 1;
    char line_buffer[MAX_LINEBUFFER];
    int counter = 0;
    int return_value;

    while (fgets(line_buffer, sizeof(line_buffer), file_pointer)) {
        if (line_buffer[0] == '.') {
            continue;
        }

        char asm_label[50];
        char asm_opc[50];
        char asm_operand[50];

        process_asm_input(line_buffer, asm_label, asm_opc, asm_operand);
        // printf(line_buffer);
        counter = counter + 1;

        if (asm_label[0] == '.') {
            continue;
        }

        if (is_first_line) {
            is_first_line = 0;

            if (strcmp(asm_opc, "START") == 0) {
                int start_address = atoi(asm_operand);
                // program position counter = initialize(START line)
                LOCCTR = start_address;

                if (strcmp(asm_label, "*") != 0) {
                    strcpy(program_name, asm_label);
                    add_symtab(asm_label, LOCCTR);
                }

                // pass1 list
                ASMTAB[asmtable_size].loc = LOCCTR;
                strcpy(ASMTAB[asmtable_size].label, asm_label);
                strcpy(ASMTAB[asmtable_size].opcode, asm_opc);
                strcpy(ASMTAB[asmtable_size].operand, asm_operand);
                ASMTAB[asmtable_size].object_code[0] = '\0';
                asmtable_size = asmtable_size + 1;
                continue;
            }
            else {
                LOCCTR = 0;
                if (strcmp(asm_label, "*") != 0) {
                    strcpy(program_name, asm_label);
                    add_symtab(asm_label, LOCCTR);
                }
            }
        }

        ASMTAB[asmtable_size].loc = LOCCTR;
        strcpy(ASMTAB[asmtable_size].label, asm_label);
        strcpy(ASMTAB[asmtable_size].opcode, asm_opc);
        strcpy(ASMTAB[asmtable_size].operand, asm_operand);
        ASMTAB[asmtable_size].object_code[0] = '\0';
        asmtable_size = asmtable_size + 1;

        if (strcmp(asm_label, "*") != 0) {
            return_value = add_symtab(asm_label, LOCCTR);
        }
        if (return_value == -1) {
            printf("\n[Error] at program line: %d, duplicate symbol '%s'\n", counter, asm_label);
            break;
        }

        int LOCCTR_count = 0;
        if (strcmp(asm_opc, "RESW") == 0) {
            LOCCTR_count = atoi(asm_operand) * 3;
        }
        else if (strcmp(asm_opc, "RESB") == 0) {
            LOCCTR_count = atoi(asm_operand);
        }
        else if (strcmp(asm_opc, "BASE") == 0) {
            LOCCTR_count = 0;
        }
        else if (strcmp(asm_opc, "BYTE") == 0) {
            if (asm_operand[0] == 'X') {
                LOCCTR_count = 1;
            }
            else if (asm_operand[0] == 'C') {
                int count = 2;
                while (asm_operand[count] != '\'' && asm_operand[count] != '\0') {
                    count = count + 1;
                }

                LOCCTR_count = count - 2;
            }
        }
        else if (asm_opc[0] == '+') {
            LOCCTR_count = 4;
        }
        else {
            // 去 OPTAB 找 
            int i;
            for (i = 0; i < optable_size; i++) {
                if (strcmp(OPTAB[i].mnemonic, asm_opc) == 0) {
                    LOCCTR_count = OPTAB[i].format;
                    break;
                }
            }
        }

        LOCCTR = LOCCTR + LOCCTR_count;

        if (strcmp(asm_opc, "END") == 0) {
            break;
        }
    }

    if (return_value != -1) {
        printf("\n=== Pass1: ASMTAB ===\n");
        printf("Line  Loc     Source statement  \n");
        int i;
        int Line = 1;
        for (i = 0; i < asmtable_size; i++) {
            printf("%-4d  %04X  %-6s  %-6s  %-10s\n", Line, ASMTAB[i].loc, ASMTAB[i].label, ASMTAB[i].opcode, ASMTAB[i].operand);
            Line = Line + 1;
        }
        printf("=====================\n");

        printf("\n=== Pass1: SYMTAB ===\n");
        for (i = 0; i < symtab_count; i++) {
            printf("%-6s %04X\n", SYMTAB[i].label, SYMTAB[i].address);
        }
        printf("=====================\n");
    }
    else {
        return -1;
    }

    return 0;
}

// token 
void process_asm_input(const char *line_buffer, char *asm_label, char *asm_opc, char *asm_operand) {
    // initialize
    strcpy(asm_label, "*");
    strcpy(asm_opc, "*");
    strcpy(asm_operand, "*");

    while (*line_buffer == ' ' || *line_buffer == '\t') {
        line_buffer = line_buffer + 1;
    }

    if (*line_buffer == '\0' || *line_buffer == '.') {
        return;
    }

    // split token
    char token_buffer[MAX_TOKENS][MAX_TOKEN_LEN];
    int i, j;
    for (i = 0; i < MAX_TOKENS; i++) {
        token_buffer[i][0] = '\0';
    }

    // for (i = 0; i < MAX_TOKENS; i++) {
    //     for (j = 0; j < MAX_TOKEN_LEN; j++) {
    //         printf("%s", token_buffer[i][j]);
    //     }
    //     printf("\n");
    // }

    int token_count = split_token(line_buffer, token_buffer);

    if (token_count == 0) {
        return;
    }

    if (is_directive_or_mnemonic(token_buffer[0])) {
        // opcode = token_buffer[0]
        strcpy(asm_opc, token_buffer[0]);

        if (token_count > 1) {
            char temp[256] = "";
            int i;
            for (i = 1; i < token_count; i++) {
                if (i > 1) {
                    strcat(temp, " ");
                }
                strcat(temp, token_buffer[i]);
            }
            
            strcpy(asm_operand, temp);
        }
    }
    // else label = token_buffer[0], opcode = token_buffer[1], other: operand
    else {
        strcpy(asm_label, token_buffer[0]);

        if (token_count > 1) {
            strcpy(asm_opc, token_buffer[1]);
        }

        if (token_count > 2) {
            char temp[256] = "";
            int i;
            for (i = 2; i < token_count; i++) {
                if (i > 2) {
                    strcat(temp, " ");
                }
                strcat(temp, token_buffer[i]);
            }

            strcpy(asm_operand, temp);
        }
    }
}

// find token
int split_token(const char *line_buffer, char token_buffer[][MAX_TOKEN_LEN]) {
    int count = 0;
    char temp[256];
    strcpy(temp, line_buffer);
    // printf("%s", temp);

    const char *delims = " \t\r\n"; // split
    char *p = strtok(temp, delims); // split temp

    while (p != NULL) {
        strncpy(token_buffer[count], p, MAX_TOKEN_LEN - 1);
        token_buffer[count][MAX_TOKEN_LEN - 1] = '\0';
        count = count + 1;

        if (count >= MAX_TOKEN_LEN - 1) {
            break;
        }

        p = strtok(NULL, delims);
    }

    return count;
}

// directive or mnemonic ?
int is_directive_or_mnemonic(const char *token) {
    // is directive ?
    if (strcmp(token, "START") == 0) return 1;
    if (strcmp(token, "BASE") == 0) return 1;
    if (strcmp(token, "BYTE") == 0) return 1;
    if (strcmp(token, "RESW") == 0) return 1;
    if (strcmp(token, "RESB") == 0) return 1;
    if (strcmp(token, "END") == 0) return 1;

    // is mnemonic ?
    if (strcmp(token, "STL") == 0) return 1;
    if (strcmp(token, "LDB") == 0) return 1;
    if (strcmp(token, "+JSUB") == 0) return 1;
    if (strcmp(token, "LDA") == 0) return 1;
    if (strcmp(token, "COMP") == 0) return 1;
    if (strcmp(token, "JEQ") == 0) return 1;
    if (strcmp(token, "J") == 0) return 1;
    if (strcmp(token, "STA") == 0) return 1;
    if (strcmp(token, "CLEAR") == 0) return 1;
    if (strcmp(token, "+LDT") == 0) return 1;
    if (strcmp(token, "TD") == 0) return 1;
    if (strcmp(token, "RD") == 0) return 1;
    if (strcmp(token, "COMPR") == 0) return 1;
    if (strcmp(token, "STCH") == 0) return 1;
    if (strcmp(token, "TIXR") == 0) return 1;
    if (strcmp(token, "JLT") == 0) return 1;
    if (strcmp(token, "STX") == 0) return 1;
    if (strcmp(token, "RSUB") == 0) return 1;
    if (strcmp(token, "LDT") == 0) return 1;
    if (strcmp(token, "LDCH") == 0) return 1;
    if (strcmp(token, "WD") == 0) return 1;
    if (strcmp(token, "START") == 0) return 1;

    return 0;
}

// SYMTAB add
int add_symtab(const char *label, int address) {
    if (strcmp(label, "*") == 0 || label[0] == '.') {
        return 1;
    }

    // redefined label ?
    int i;
    for (i = 0; i < symtab_count; i++) {
        if (strcmp(SYMTAB[i].label, label) == 0) {
            return -1;
        }
    }

    strcpy(SYMTAB[symtab_count].label, label);
    SYMTAB[symtab_count].address = address;
    symtab_count = symtab_count + 1;

    return 1;
}

// find Symbol
int find_symbol(const char *find_label) {
    int i;
    for (i = 0; i < symtab_count; i++) {
        if (strcmp(SYMTAB[i].label, find_label) == 0) {
            return SYMTAB[i].address;
        }
    }

    return -1;
}

// ====================== //
//          pass2         //
// ====================== //
void pass2() {
    int i;
    for (i = 0; i < asmtable_size; i++) {
        int j = i + 1;

        while (j < asmtable_size && ASMTAB[j].loc == ASMTAB[i].loc) {
            j = j + 1;
            // printf("%d\n", j);
        }

        if (j < asmtable_size) {
            program_counter = ASMTAB[j].loc;
            // printf("%d\n", program_counter);
        } 
        else {
            program_counter = LOCCTR;
            // printf("%d\n", program_counter);
        }

        int format_number = 0;
        char pass2_object_code[100] = "";

        if (ASMTAB[i].opcode[0] == '+') {
            format_number = 4;
        } 
        else {
            for (int k = 0; k < optable_size; k++) {
                if (strcmp(OPTAB[k].mnemonic, ASMTAB[i].opcode) == 0) {
                    format_number = OPTAB[k].format;
                    break;
                }
            }
        }

        if (format_number == 2) {
            process_format2(i, pass2_object_code);
        } 
        else if (format_number == 4) {
            process_format4(i, pass2_object_code);

            if (ASMTAB[i].operand[0] != '#') {
                // modify[modify_size = modify_size + 1] = ASMTAB[i].loc;
                modify[modify_size] = ASMTAB[i].loc;
                modify_size = modify_size + 1;
                // printf("modify: %X\n", modify[modify_size - 1]);
            }
        } 
        else if (format_number == 3) {
            process_format3(i, pass2_object_code);
        } 
        else if (strcmp(ASMTAB[i].opcode, "BASE") == 0) {
            int base = find_symbol(ASMTAB[i].operand);
            if (base >= 0) {
                base_address = base;
            } 
        } 
        else if (strcmp(ASMTAB[i].opcode, "BYTE") == 0) {
            if (ASMTAB[i].operand[0] == 'C') {
                int len = strlen(ASMTAB[i].operand);
                for (int k = 2; k < len - 1; k++) {
                    char temp[3];
                    sprintf(temp, "%02X", ASMTAB[i].operand[k]);
                    strcat(pass2_object_code, temp);
                }
            } 
            else if (ASMTAB[i].operand[0] == 'X') {
                strncpy(pass2_object_code, ASMTAB[i].operand + 2, strlen(ASMTAB[i].operand) - 3);
            }
        } 
        else if (strcmp(ASMTAB[i].opcode, "RESW") == 0 || strcmp(ASMTAB[i].opcode, "RESB") == 0 || strcmp(ASMTAB[i].opcode, "END") == 0) {
            strcpy(pass2_object_code, "");
        }

        strcpy(ASMTAB[i].object_code, pass2_object_code);

        if (strlen(pass2_object_code) > 0) {
            OBJTAB[obj_code_size].code_address = ASMTAB[i].loc;
            strcpy(OBJTAB[obj_code_size].code, pass2_object_code);
            obj_code_size = obj_code_size + 1;
        }
    }

    printf("\n=== Pass2: ASMTAB ===\n");
    printf("Line  Loc     Source statement         Object code\n");
    int Line = 1;
    for (i = 0; i < asmtable_size; i++) {
        printf("%-4d  %04X  %-6s  %-6s  %-10s  %s\n", Line, ASMTAB[i].loc, ASMTAB[i].label, ASMTAB[i].opcode, ASMTAB[i].operand, ASMTAB[i].object_code);
        Line = Line + 1;
    }
    printf("=====================\n");
}

void read_register_file(void) {

    FILE *file_in = fopen("registers.txt", "r");

    if (!file_in) {
        printf("[Error] Cannot open registers.txt\n");
        return;
    }

    while (!feof(file_in)) {
        char reg_name[3];
        char reg_number[2];

        // if (fscanf(file_in, "%s %s", &reg_name, &reg_number) == 2) 
        if (fscanf(file_in, "%2s %1s", reg_name, reg_number) == 2) {
            strcpy(REGTAB[regtable_size].register_name, reg_name);
            strcpy(REGTAB[regtable_size].register_number, reg_number);

            regtable_size = regtable_size + 1;
        }
    }

    fclose(file_in);

    printf("\n=== REGTAB Content ===\n");
    printf("Name   Number\n");
    int i;
    for (i = 0; i < regtable_size; i++) {
        printf("%-6s %s\n", REGTAB[i].register_name, REGTAB[i].register_number);
    }
    printf("=====================\n");
}

// ====================== //
//     format process     //
// ====================== //
void process_format2(int index, char *obj_code) {
    obj_code[0] = '\0';
    char opcode_hex[10] = "";

    int i;
    for (i = 0; i < optable_size; i++) {
        if (strcmp(OPTAB[i].mnemonic, ASMTAB[index].opcode) == 0) {
            strcpy(opcode_hex, OPTAB[i].opcode);
            break;
        }
    }

    if (strlen(opcode_hex) == 0) {
        return;
    }

    char operand[100];
    strcpy(operand, ASMTAB[index].operand);

    // register ?
    char register1[10] = "A";
    char register2[10] = "A";
    char *two_register = strchr(operand, ',');
    if (two_register) {
        *two_register = '\0';
        strcpy(register1, operand);
        strcpy(register2, two_register + 1);
    }
    else {
        strcpy(register1, operand);
        strcpy(register2, "0");
    }

    // register number?
    char register1_number[10] = "0";
    char register2_number[10] = "0";
    for (i = 0; i < regtable_size; i++) {
        if (strcmp(REGTAB[i].register_name, register1) == 0) {
            strcpy(register1_number, REGTAB[i].register_number);
            break;
        }
    }
    for (i = 0; i < regtable_size; i++) {
        if (strcmp(REGTAB[i].register_name, register2) == 0) {
            strcpy(register2_number, REGTAB[i].register_number);
            break;
        }
    }

    sprintf(obj_code, "%s%s%s", opcode_hex, register1_number, register2_number);
}

void process_format4(int index, char *obj_code) {
    obj_code[0] = '\0';
    char f4_operand[100];
    strcpy(f4_operand, ASMTAB[index].operand);

    int n = 0, i = 0, x = 0, b = 0, p = 0, e = 1;
    if (f4_operand[0] == '@') {
        n = 1;
        memmove(f4_operand, f4_operand + 1, strlen(f4_operand));
    }
    else if (f4_operand[0] == '#') {
        i = 1;
        memmove(f4_operand, f4_operand + 1, strlen(f4_operand));
    }
    else {
        n = 1;
        i = 1;
    }

    int length = strlen(f4_operand);
    if (length >= 2 && f4_operand[length - 2] == ',' && f4_operand[length - 1] == 'X') {
        x = 1;
        f4_operand[length - 2] = '\0';
    }

    int f4_case = 0;
    if (n && i) {
        f4_case = 3;
    }
    else if (n && !i) {
        f4_case = 2;
    }
    else if (!n && i) {
        f4_case = 1;
    }

    // clear '+'
    char base_mnem[50];
    strcpy(base_mnem, ASMTAB[index].opcode + 1);

    char base_opcode[10] = "";
    int j;
    for (j = 0; j < optable_size; j++) {
        if (strcmp(OPTAB[j].mnemonic, base_mnem) == 0) {
            strcpy(base_opcode, OPTAB[j].opcode);
            break;
        }
    }

    int base_opcode_value = hex_to_ten(base_opcode);
    int final_opcode_value = base_opcode_value + f4_case;
    char final_opcode_string[10];
    ten_to_hex(final_opcode_value, 2, final_opcode_string);

    int target_address = 0;
    int symbol_value = find_symbol(f4_operand);

    if (symbol_value >= 0) {
        target_address = symbol_value;
    }
    else if (isdigit((unsigned char)f4_operand[0])) {
        target_address = atoi(f4_operand);
    }

    char address_20bits[10];
    ten_to_hex(target_address & 0xFFFFF, 5, address_20bits);

    char mid_value[10];
    bits_to_hex(x, b, p, e, mid_value);

    sprintf(obj_code, "%s%s%s",final_opcode_string, mid_value, address_20bits);
}

void process_format3(int index, char *obj_code) {
    obj_code[0] = '\0';
    char f3_operand[100];
    strcpy(f3_operand, ASMTAB[index].operand);

    int n = 0, i = 0, x = 0, b = 0, p = 0, e = 0;
    int f3_case = 0;

    int length = strlen(f3_operand);
    if (length >= 2 && f3_operand[length - 2] == ',' && f3_operand[length - 1] == 'X') {
        x = 1;
        f3_operand[length - 2] = '\0';
    }
    if (f3_operand[0] == '@') {
        n = 1;
        f3_case = 2;
        memmove(f3_operand, f3_operand + 1, strlen(f3_operand));
    }
    else if (f3_operand[0] == '#') {
        i = 1;
        f3_case = 1;
        memmove(f3_operand, f3_operand + 1, strlen(f3_operand));
    }
    else {
        n = 1;
        i = 1;
        f3_case = 3;
    }

    char opecode_to_hex[10] = "";
    int j;
    for (j = 0; j < optable_size; j++) {
        if (strcmp(OPTAB[j].mnemonic, ASMTAB[index].opcode) == 0) {
            strcpy(opecode_to_hex, OPTAB[j].opcode);
            break;
        }
    }

    int base_opcode_value = hex_to_ten(opecode_to_hex);
    int final_opcode_value = base_opcode_value + f3_case;
    char final_opcode_string[10];
    ten_to_hex(final_opcode_value, 2, final_opcode_string);

    // address
    int direct = 0;
    char address_to_hex[10] = "0000";
    int symbol_value = find_symbol(f3_operand);

    if (symbol_value >= 0) {
        sprintf(address_to_hex, "%03X", symbol_value & 0xFFFF);
    }
    else {
        if (isdigit((unsigned char)f3_operand[0])) {
            int value = atoi(f3_operand);
            sprintf(address_to_hex, "%03X", value & 0xFFFF);
            direct = 1;
        }
        else {
            strcpy(address_to_hex, "0000");
        }
    }

    if (!direct && strcmp(f3_operand, "*") != 0) {
        char displayment[10];
        // printf("PC: %d\n", program_counter);
        reloc_address(address_to_hex, displayment);
        strcpy(address_to_hex, displayment);

        if (check_program_counter) {
            p = 1;
            b = 0;
        }
        else {
            p = 0;
            b = 1;
        }
    }

    int address_value = hex_to_ten(address_to_hex) & 0xFFFF;
    char final_disp[10];
    ten_to_hex(address_value, 3, final_disp);

    char mid_value[10];
    bits_to_hex(x, b, p, e, mid_value);

    sprintf(obj_code, "%s%s%s", final_opcode_string, mid_value, final_disp);
}

// ==================================== //
// Conversion and Relocation Addressing //
// ==================================== //
int hex_to_ten(const char *hex_string) {
    return (int)strtol(hex_string, NULL, 16);
}

void ten_to_hex(int value, int width, char *output_string) {
    char format[20];
    sprintf(format, "%%0%dX", width);
    sprintf(output_string, format, value);
}

void bits_to_hex(int a, int b, int c, int d, char *output_hex) {
    int value = (a << 3) + (b << 2) + (c << 1) + d;
    ten_to_hex(value, 1, output_hex);
}

void reloc_address(const char *address, char *disp) {
    int x = hex_to_ten(address);
    int offset = x - program_counter;
    // printf("offset: %d\n", offset);
    // PC relative
    if (offset >= -2048 && offset <= 2047) {
        check_program_counter = 1;

        if (offset < 0) {
            offset = offset + 0x1000;
        }

        char temp[10];
        ten_to_hex(offset, 3, temp);
        strcpy(disp, temp);

    }
    // Base relative
    else if (base_address != 0 && x - base_address >= 0 && x - base_address <= 4095) {
        check_program_counter = 0;
        int value = x - base_address;
        ten_to_hex(value, 3, disp);
    }
}

// ====================== //
//         record         //
// ====================== //
void write_obj_file() {
    if (obj_code_size == 0) {
        return;
    }

    printf("\n=== OBJTAB ===\n");
    int i;
    for (i = 0; i < obj_code_size; i++) {
        printf("%-6X  %-20s\n", OBJTAB[i].code_address, OBJTAB[i].code);
    }
    printf("=====================\n\n");

    FILE *file_out = fopen("object_program.txt", "w");
    
    int start_address = OBJTAB[0].code_address;
    int program_length = LOCCTR - start_address;
    char temp[20];

    // H record
    // Program Name
    printf("H^%-6s^", program_name);
    fprintf(file_out, "H^%-6s^", program_name);

    // Start Address
    ten_to_hex(start_address, 6, temp);
    printf("%s^", temp);
    fprintf(file_out, "%s^", temp);

    // Program Length
    ten_to_hex(program_length, 6, temp);
    printf("%s\n", temp);
    fprintf(file_out, "%s\n", temp);

    // T record
    i = 0;
    while (i < obj_code_size) {
        int record_start = OBJTAB[i].code_address;
        int collected = 0;
        int bytes_sum = 0;
        char text_record[200] = "";
        char text_length[3];

        while (i < obj_code_size && collected < 0x1D && bytes_sum + strlen(OBJTAB[i].code) / 2 <= 0x1D) {
            if (collected > 0) {
                int gap = OBJTAB[i].code_address - (OBJTAB[i - 1].code_address + strlen(OBJTAB[i - 1].code) / 2);
                if (gap > 0) {
                    break;
                }
            }

            if (collected > 0) {
                strcat(text_record, "^");
            }

            strcat(text_record, OBJTAB[i].code);
            bytes_sum = bytes_sum + strlen(OBJTAB[i].code) / 2;
            collected = collected + 1;
            i = i + 1;
        }

        printf("T^");
        fprintf(file_out, "T^");

        ten_to_hex(record_start, 6, temp);
        printf("%s^", temp);
        fprintf(file_out, "%s^", temp);

        ten_to_hex(bytes_sum, 2, text_length);
        printf("%s^", text_length);
        fprintf(file_out, "%s^", text_length);

        printf("%s\n", text_record);
        fprintf(file_out, "%s\n", text_record);
    }

    for (int j = 0; j < modify_size; j++) {
        printf("M^");
        fprintf(file_out, "M^");

        ten_to_hex(modify[j] + 1, 6, temp);
        printf("%s^", temp);
        fprintf(file_out, "%s^", temp);

        printf("05\n");
        fprintf(file_out, "05\n");
    }

    printf("E^");
    fprintf(file_out, "E^");

    ten_to_hex(start_address, 6, temp);
    printf("%s\n", temp);
    fprintf(file_out, "%s\n", temp);

    fclose(file_out);
}
