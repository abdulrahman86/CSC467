#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include "ast.h"
#include "common.h"
#include "parser.tab.h"

#define DEBUG_PRINT_TREE 0

node *ast = NULL;

node *ast_allocate(node_kind kind, int yyline, ...) {
  va_list args;

  // make the node
  node *ast = (node *) malloc(sizeof(node));
  memset(ast, 0, sizeof *ast);
  ast->kind = kind;
  ast->line = yyline;
  ast->type_code = -1;
  ast->vec_size = -1;
  ast->is_const = 0;

  ast->reg_name = NULL;
  ast->condi_reg_name = NULL;
  ast->follow_condi_reg_name = NULL;
  
  va_start(args, yyline);
  switch(kind) {


  // type
  case TYPE_NODE:
	ast->type_code = va_arg(args, int);
	ast->vec_size = va_arg(args, int) + 1;
 	break;

  case DECLARATION_NODE:
	ast->is_const = va_arg(args, int);
	ast->declaration.type_node = va_arg(args, node *);
	ast->declaration.var_name = va_arg(args, char *);
	ast->declaration.expr = va_arg(args, node *);
	break;

  // expression
  case CONSTRUCTOR_NODE:
  case SCOPE_NODE:
  case DECLARATIONS_NODE:
  case STATEMENT_NODE:
  case ASSIGNMENT_NODE:
	ast->binary_node.left = va_arg(args, node *);
	ast->binary_node.right = va_arg(args, node *);
	break;
  case ARGUMENTS_NODE:
        ast->argument.arguments = va_arg(args, node *);
        ast->argument.expr = va_arg(args, node *);
        ast->argument.arg_size = 0;
        break;
  case IF_STATEMENT_NODE:
	ast->if_statement.condition = va_arg(args, node *);
	ast->if_statement.inside_if = va_arg(args, node *);
	ast->if_statement.inside_else = va_arg(args, node *);
	break;

  case FUNCTION_NODE:
	ast->func_expr.func_name = va_arg(args, int);
	ast->func_expr.args = va_arg(args, node *);
	break;

  case UNARY_EXPRESION_NODE:
	ast->unary_expr.op = va_arg(args, int);
	ast->unary_expr.right = va_arg(args, node *);
	break;

  case BINARY_EXPRESSION_NODE:
    ast->binary_expr.op = va_arg(args, int);
    ast->binary_expr.left = va_arg(args, node *);
    ast->binary_expr.right = va_arg(args, node *);
    break;

  case NESTED_EXPRESSION_NODE:
  case VAR_EXPRESSION_NODE:
  case NESTED_SCOPE_NODE:
	ast->unary_node.right = va_arg(args, node *);
	break;

  case BOOL_NODE:
	ast->type_code = BOOL_T;
        ast->vec_size = 1;
        ast->is_const = 1;
	ast->literal_expr.int_val = va_arg(args, int);
	break;

  case INT_NODE:
	ast->type_code = INT_T;
        ast->vec_size = 1;
        ast->is_const = 1;
	ast->literal_expr.int_val = va_arg(args, int);
	break;

  case FLOAT_NODE:
	ast->type_code = FLOAT_T;
        ast->vec_size = 1;
        ast->is_const = 1;
	ast->literal_expr.float_val = va_arg(args, double);
  	break;

  case VAR_NODE:
	ast->variable.var_name = va_arg(args, char *);
	ast->variable.is_array = va_arg(args, int);
	ast->variable.index = va_arg(args, int);
        ast->type_code = -1;
        ast->vec_size = -1;
	break;


  // ...

  default: break;
  }

  va_end(args);

  return ast;
}
void ast_post_free(node *ast, int depth){
    if (ast->kind == DECLARATION_NODE && ast->declaration.var_name != NULL){
        free(ast->declaration.var_name);
    }
    if (ast->kind == VAR_NODE && ast->variable.var_name != NULL){
        free(ast->variable.var_name);
    }
    // Codegen
    if (ast->reg_name != NULL) {
        free(ast->reg_name);
    }
    if (ast->condi_reg_name != NULL){
        free(ast->condi_reg_name);
    }
    if (ast->follow_condi_reg_name != NULL){
        free(ast->follow_condi_reg_name);
    }

    free(ast);
}

void ast_free(node *ast) {
    ast_visit(ast, 0, NULL, &ast_post_free);
    fprintf(dumpFile, "ast tear down\n");
}

void ast_print(node * ast) {
	ast_visit(ast, 0, &ast_pre_print, &ast_post_print);
	fprintf(dumpFile, "\n");
}

const char *get_type_name(node *ast) {
  switch(ast->type_code) {
    case FLOAT_T:
      return "float";
    case INT_T:
      return "int";
    case BOOL_T:
      return "bool";
    case BVEC_T:
      switch(ast->vec_size){
        case 2:
          return "bvec2";
        case 3:
          return "bvec3";
        case 4:
          return "bvec4";
      }
    case IVEC_T:
      switch(ast->vec_size){
        case 2:
          return "ivec2";
        case 3:
          return "ivec3";
        case 4:
          return "ivec4";
      }
    case VEC_T:
      switch(ast->vec_size){
        case 2:
          return "vec2";
        case 3:
          return "vec3";
        case 4:
          return "vec4";
      }
    default:
      return "???";
  }
}

const char *get_func_name(int code) {
  switch(code) {
    case 0:
      return "dp3";
    case 1:
      return "rsq";
    case 2:
      return "lit";
    default:
      return "?FUNC?";
  }
}


void print_indent(int depth, int is_open, int is_newline){
	if (is_newline) {
		fprintf(dumpFile, "\n");
		int i;
    	for (i = 0; i < depth; i++) {
      		fprintf(dumpFile, "    ");
    	}
	}

	fprintf(dumpFile, is_open ? "(" : ")");
}

const char* get_op_str(int op){
  switch(op) {
    case '-':
      return "-";
    case '!':
      return "!";
    case AND:
      return "&&";
    case OR:
      return "||";
    case EQ:
      return "==";
    case NEQ:
      return "!=";
    case '<':
      return "<";
    case LEQ:
      return "<=";
    case '>':
      return ">";
    case GEQ:
      return ">=";
    case '+':
      return "+";
    case '*':
      return "*";
    case '/':
      return "/";
    case '^':
      return "^";
    case UMINUS:
      return "-";
    default:
      return "";
  }
}

void ast_pre_print(node *ast, int depth){
	switch (ast->kind) {
        case SCOPE_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "SCOPE");
            break;

        case DECLARATIONS_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "DECLARATIONS");
            break;

        case DECLARATION_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "DECLARATION %s %s",
                    ast->declaration.var_name, get_type_name(ast));
            break;

        case STATEMENT_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "STATEMENTS");
            break;

        case ASSIGNMENT_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "ASSIGN %s", get_type_name(ast));
            break;

        case IF_STATEMENT_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "IF");
            break;

        case UNARY_EXPRESION_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "UNARY %s %s", get_type_name(ast),
                    get_op_str(ast->unary_expr.op));
            break;

        case BINARY_EXPRESSION_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "BINARY %s %s", get_type_name(ast),
                    get_op_str(ast->binary_expr.op));
            break;

        case VAR_NODE:
            if (ast->variable.is_array) {
                print_indent(depth, 1, 1);
                fprintf(dumpFile, "INDEX %s %s %d",
                        get_type_name(ast),
                        ast->variable.var_name,
                        ast->variable.index);
            } else {
                fprintf(dumpFile, " %s", ast->variable.var_name);
            }
            break;
        case BOOL_NODE:
            fprintf(dumpFile, ast->literal_expr.int_val ? " true" : " false");
            break;

        case INT_NODE:
            fprintf(dumpFile, " %d", ast->literal_expr.int_val);
            break;

        case FLOAT_NODE:
            fprintf(dumpFile, " %f", ast->literal_expr.float_val);
            break;

        case VAR_EXPRESSION_NODE:
        case NESTED_EXPRESSION_NODE:
        case NESTED_SCOPE_NODE:
        case TYPE_NODE:
        case ARGUMENTS_NODE:
            //print nothing for those nodes
            break;
        case CONSTRUCTOR_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "CALL %s", get_type_name(ast));
            break;
        case FUNCTION_NODE:
            print_indent(depth, 1, 1);
            fprintf(dumpFile, "CALL %s", get_func_name(ast->func_expr.func_name));
            break;
        default:
            print_indent(depth, 1, 1);
            break;
    }
}



void ast_post_print(node *ast, int depth) {
    switch (ast->kind) {
        case VAR_NODE:
            if (ast->variable.is_array) {
                print_indent(depth, 0, 0);
            }
            break;
        case BOOL_NODE:
        case INT_NODE:
        case FLOAT_NODE:
        case VAR_EXPRESSION_NODE:
        case NESTED_EXPRESSION_NODE:
        case NESTED_SCOPE_NODE:
        case TYPE_NODE:
        case ARGUMENTS_NODE:
            //print nothing for those nodes
            break;
        default: print_indent(depth, 0, 1);
    }

}

void ast_visit(node * ast, int depth, void(*pre_func)(node*,int), void(*post_func)(node*,int)){
    visit_funcs* args = (visit_funcs*)malloc(sizeof(visit_funcs));
    args->pre_func = pre_func;
    args->post_func = post_func;
    args->ex_func = NULL;
    args->str_pass_func = NULL;
    args->passed_string = NULL;
    
    ast_visit(ast, depth, 0, args);
    free(args);
}

void ast_visit(node * ast, int depth, int is_codegen, visit_funcs* pass_args){
	if (!ast) return;
	if (pass_args->pre_func) pass_args->pre_func(ast, depth);
        if (pass_args->str_pass_func) pass_args->str_pass_func(ast, pass_args->passed_string);
	switch(ast->kind){
  		case ARGUMENTS_NODE:
                    ast_visit(ast->argument.arguments, depth, is_codegen, pass_args);
                    ast_visit(ast->argument.expr, depth, is_codegen, pass_args);
                    break;
  		case SCOPE_NODE:
  		case DECLARATIONS_NODE:
  		case STATEMENT_NODE:
		case ASSIGNMENT_NODE:
			ast_visit(ast->binary_node.left, depth+1, is_codegen, pass_args);
			ast_visit(ast->binary_node.right, depth+1, is_codegen, pass_args);
			break;

		case DECLARATION_NODE:
                        ast_visit(ast->declaration.type_node, depth+1, is_codegen, pass_args);
			ast_visit(ast->declaration.expr, depth+1, is_codegen, pass_args);
			break;

		case IF_STATEMENT_NODE:
			ast_visit(ast->if_statement.condition, depth+1, is_codegen, pass_args);
                        if (is_codegen && pass_args->ex_func){
                            pass_args->ex_func(ast, 0);
                        }
			ast_visit(ast->if_statement.inside_if, depth+1, is_codegen, pass_args);
                        if (ast->if_statement.inside_else && is_codegen && pass_args->ex_func){
                            pass_args->ex_func(ast, 1);
                        }
			ast_visit(ast->if_statement.inside_else, depth+1, is_codegen, pass_args);
			break;
                case CONSTRUCTOR_NODE:
                        ast_visit(ast->binary_node.left, depth+1, is_codegen, pass_args);
                        ast_visit(ast->binary_node.right, depth+1, is_codegen, pass_args);
			break;
		case FUNCTION_NODE:
			ast_visit(ast->func_expr.args, depth+1, is_codegen, pass_args);
			break;

		case UNARY_EXPRESION_NODE:
			ast_visit(ast->unary_expr.right, depth+1, is_codegen, pass_args);
			break;

		case BINARY_EXPRESSION_NODE:
			ast_visit(ast->binary_expr.left, depth+1, is_codegen, pass_args);
			ast_visit(ast->binary_expr.right, depth+1, is_codegen, pass_args);
			break;

  		case NESTED_EXPRESSION_NODE:
  		case VAR_EXPRESSION_NODE:
  		case NESTED_SCOPE_NODE:
			ast_visit(ast->unary_node.right, depth, is_codegen, pass_args);
			break;

		default:
			break;

	}

	if (pass_args->post_func) pass_args->post_func(ast, depth);


}
