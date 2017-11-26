#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"
#include "common.h"
#include "parser.tab.h"

#define ZERO_VEC "{0.0, 0.0, 0.0, 0.0}"
#define ONE_VEC "{1.0, 1.0, 1.0, 1.0}"

#define BOOL_FALSE ZERO_VEC
#define BOOL_TRUE ONE_VEC

// Global vars
inst* head = NULL;
inst* tail = NULL;
size_t temp_reg_count = 0;

// Helper functions
void to_arb_pre(node* ast, int depth);
void to_arb_post(node* ast, int depth);
void handle_math_expr(node* ast);
void handle_imm_val(node* ast);
void handle_function(node* ast);
void handle_constructor(node* ast);
// Append a new instruction at the end of linked list
void append_inst(inst_code c, char* out, char* in1, char* in2, char* in3);
// return the temp reg name
// @warning: The memory used need to be freed
char* assign_temp_reg();

// Util functions
static const int INT_TYPES[] = {INT_T, INT_C, IVEC_T,
                                -1};  // -1 marks the end of array

int is_in_set(const int* arr, int target) {
    assert(target != -1);
    size_t i = 0;
    while (arr[i] != -1) {
        if (arr[i] == target) {
            return 1;  // true
        }
        i++;
    }
    return 0;  // false
}

inst* to_arb(node* root) {
    ast_visit(root, 0, &to_arb_pre, &to_arb_post);
    return head;
}

void to_arb_pre(node* ast, int depth) {
    // Do nothing
}

void to_arb_post(node* ast, int depth) {
    switch (ast->kind) {
        // All nodes that stores temporary expression values
        case UNARY_EXPRESION_NODE:
        case BINARY_EXPRESSION_NODE:
        case INT_NODE:
        case FLOAT_NODE:
        case BOOL_NODE:
        case FUNCTION_NODE:
        case CONSTRUCTOR_NODE:
            ast->reg_name = assign_temp_reg();
            break;
        default:
            // Do nothing
            break;
    }
    handle_math_expr(ast);
    handle_imm_val(ast);
    handle_function(ast);
    handle_constructor(ast);
}

void handle_math_expr(node* ast) {
    char* out = ast->reg_name;
    switch (ast->kind) {
        case UNARY_EXPRESION_NODE:
            switch (ast->unary_expr.op) {
                case UMINUS:
                    append_inst(SUB, out, ZERO_VEC,
                                ast->unary_expr.right->reg_name, "");
                    break;
                case '!':
                    append_inst(SUB, out, BOOL_TRUE,
                                ast->unary_expr.right->reg_name, "");
                    break;
                default:
                    assert(0);
                    break;
            }
            break;
        case BINARY_EXPRESSION_NODE: {
            char* opr1 = ast->binary_expr.left->reg_name;
            char* opr2 = ast->binary_expr.right->reg_name;
            switch (ast->binary_expr.op) {
                case '*':
                    append_inst(MUL, out, opr1, opr2, "");
                    break;
                case '/':
                    // take reciprocate of opr2 and multiply with
                    // opr1
                    append_inst(RCP, out, opr2, "", "");
                    append_inst(MUL, out, opr1, out, "");
                    // if both operand is int type, convert result to int
                    if (is_in_set(INT_TYPES,
                                  ast->binary_expr.right->type_code) &&
                        is_in_set(INT_TYPES,
                                  ast->binary_expr.left->type_code)) {
                        append_inst(FLR, out, out, "", "");
                    }
                    break;
                case '^':
                    append_inst(POW, out, opr1, opr2, "");
                    break;
                case GEQ:
                    // opr1 >= opr2
                    // !(opr1 < opr2)
                    // !(opr1 - opr2 < 0)
                    append_inst(SUB, out, opr1, opr2, "");
                    append_inst(CMP, out, out, BOOL_FALSE, BOOL_TRUE);
                    break;
                case LEQ:
                    // opr1 <= opr2
                    // !(opr1 > opr2)
                    // !(opr1 - opr2 > 0)
                    // !(opr2 - opr1 < 0)
                    append_inst(SUB, out, opr2, opr1, "");
                    append_inst(CMP, out, out, BOOL_FALSE, BOOL_TRUE);
                    break;
                case '<':
                    // opr1 < opr2
                    // opr1 - opr2 < 0
                    append_inst(SUB, out, opr1, opr2, "");
                    append_inst(CMP, out, out, BOOL_TRUE, BOOL_FALSE);
                    break;
                case '>':
                    // opr1 > opr2
                    // opr2 - opr1 < 0
                    append_inst(SUB, out, opr2, opr1, "");
                    append_inst(CMP, out, out, BOOL_TRUE, BOOL_FALSE);
                    break;
                case '+':
                    append_inst(ADD, out, opr1, opr2, "");
                    break;
                case '-':
                    append_inst(SUB, out, opr1, opr2, "");
                    break;
                case EQ: {
                    // (opr1 >= opr2) && (opr2 <= opr1)
                    char* temp = assign_temp_reg();
                    append_inst(SUB, out, opr1, opr2, "");
                    append_inst(SUB, temp, opr1, opr2, "");
                    append_inst(CMP, out, out, BOOL_FALSE, BOOL_TRUE);
                    append_inst(CMP, temp, temp, BOOL_FALSE, BOOL_TRUE);
                    append_inst(MUL, out, temp, out, "");
                    break;
                }
                case NEQ: {
                    // (opr1 < opr2) || (opr1 > opr2)
                    char* temp = assign_temp_reg();
                    append_inst(SUB, out, opr1, opr2, "");
                    append_inst(SUB, temp, opr1, opr2, "");
                    append_inst(CMP, out, out, BOOL_TRUE, BOOL_FALSE);
                    append_inst(CMP, temp, temp, BOOL_TRUE, BOOL_FALSE);
                    append_inst(ADD, out, out, temp, "");
                    break;
                }
                case AND:
                    // opr1 * opr2
                    append_inst(MUL, out, opr1, opr2, "");
                case OR:
                    // opr2 + opr1 - (opr1 * opr2)
                    append_inst(MUL, out, opr1, opr2, "");
                    append_inst(ADD, out, opr1, out, "");
                    append_inst(ADD, out, opr2, out, "");
                default:
                    assert(0);
                    break;
            }
            break;
        }
        default:
            return;
    }
}

void handle_imm_val(node* ast) {
    char literal_expr[MAX_INS_LEN];
    switch (ast->kind) {
        case INT_NODE: {
            float val = (float)ast->literal_expr.int_val;
            snprintf(literal_expr, MAX_INS_LEN, "{%.1f, %.1f, %.1f, %.1f}", val,
                     val, val, val);
            append_inst(MOV, ast->reg_name, literal_expr, "", "");
            break;
        }
        case FLOAT_NODE: {
            float val = ast->literal_expr.float_val;
            snprintf(literal_expr, MAX_INS_LEN, "{%.1f, %.1f, %.1f, %.1f}", val,
                     val, val, val);
            append_inst(MOV, ast->reg_name, literal_expr, "", "");
            break;
        }
        case BOOL_NODE:
            if (ast->literal_expr.int_val == 1) {
                append_inst(MOV, ast->reg_name, BOOL_TRUE, "", "");
            } else if (ast->literal_expr.int_val == 0) {
                append_inst(MOV, ast->reg_name, BOOL_FALSE, "", "");
            } else {
                assert(0);
            }
            break;
        default:
            return;
    }
}

void handle_function(node* ast) {
    if (ast->kind != FUNCTION_NODE) return;
    node* argument_node = ast->func_expr.args;
    char* args[2];
    char* out = ast->reg_name;

    int arg_size = argument_node->argument.arg_size;
    int i = 0;
    while (i < arg_size) {
        assert(argument_node != NULL);
        node* cur_expr = argument_node->argument.expr;
        if (cur_expr != NULL) {
            args[i] = cur_expr->reg_name;
            i++;
        }
        argument_node = argument_node->argument.arguments;
    }

    switch (ast->func_expr.func_name) {
        case 0: {  // dp3
            assert(argument_node->argument.arg_size == 2);
            append_inst(DP3, out, args[0], args[1], "");
            break;
        }
        case 1:  // lit
            assert(argument_node->argument.arg_size == 1);
            append_inst(LIT, out, args[0], "", "");
            break;
        case 2:  // rsq
            assert(argument_node->argument.arg_size == 1);
            append_inst(RSQ, out, args[0], "", "");
            break;
        default:
            assert(0);
            break;
    }
}

static const char* REG_INDEX[] = {"x", "y", "z", "w"};
void handle_constructor(node* ast) {
    if (ast->kind != CONSTRUCTOR_NODE) return;
    char* out = ast->reg_name;
    node* argument_node = ast->binary_node.right;

    int arg_size = argument_node->argument.arg_size;
    assert(arg_size <= 4 && arg_size > 0);
    char var[MAX_VAR_LEN];
    int i = 0;
    while (i < arg_size) {
        assert(argument_node != NULL);
        node* cur_expr = argument_node->argument.expr;
        if (cur_expr != NULL) {
            snprintf(var, MAX_VAR_LEN, "%s.%s", out, REG_INDEX[i]);
            append_inst(MOV, var, cur_expr->reg_name, "", "");
            i++;
        }
        argument_node = argument_node->argument.arguments;
    }
}

void print_insts(inst* instruction) {
    fprintf(outputFile, "!!ARBfp1.0\n");
    inst* cur_ins = instruction;
    char ins_str[MAX_INS_LEN];

    while (cur_ins != NULL) {
        if (cur_ins->code == TEMP) {
            snprintf(ins_str, MAX_INS_LEN, "TEMP %s;", cur_ins->out);
        } else {
            snprintf(ins_str, MAX_INS_LEN, "%s %s", INST_STRING[cur_ins->code],
                     cur_ins->out);
            for (size_t i = 0; i < 3; i++) {
                if (strcmp(cur_ins->in[i], "") == 0) break;
                strcat(ins_str, ", ");
                strcat(ins_str, cur_ins->in[i]);
            }
            strcat(ins_str, ";");
        }
        cur_ins = cur_ins->_next;
        fprintf(outputFile, "%s\n", ins_str);
    }
    fprintf(outputFile, "END\n");
}

void append_inst(inst_code c, char* out, char* in1, char* in2, char* in3) {
    inst* ins = (inst*)malloc(sizeof(inst));
    ins->code = c;
    assert(out != NULL);
    assert(in1 != NULL);
    assert(in2 != NULL);
    assert(in3 != NULL);

    strncpy(ins->out, out, MAX_VAR_LEN);
    strncpy(ins->in[0], in1, MAX_VAR_LEN);
    strncpy(ins->in[1], in2, MAX_VAR_LEN);
    strncpy(ins->in[2], in3, MAX_VAR_LEN);

    ins->_next = NULL;

    if (tail == NULL) {
        assert(head == NULL);
        head = ins;
    } else {
        tail->_next = ins;
    }
    tail = ins;
}

void clear_all_inst(inst* head) {
    inst* cur = head;
    while (cur != NULL) {
        inst* to_des = cur->_next;
        free(cur);
        cur = to_des;
    }
}

char* assign_temp_reg() {
    assert(temp_reg_count + 1 < SIZE_MAX);
    char* temp_reg = (char*)calloc(MAX_VAR_LEN, sizeof(char));
    snprintf(temp_reg, MAX_VAR_LEN, "tempVar%lu", temp_reg_count++);
    append_inst(TEMP, temp_reg, "", "", "");
    return temp_reg;
}
