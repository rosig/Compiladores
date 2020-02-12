#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "visitor.h"

#define printm(...) printf(__VA_ARGS__)

int current_ssa = 1;

int checkForLiterals(AST *expr) {
 int out;

 switch (expr->expr.type) {
  case FUNCTION_CALL_EXPRESSION:
   out = 0;
   break;
  case IDENTIFIER_EXPRESSION:
   out = 0;
   break;
  case BINARY_EXPRESSION:
   out = checkForLiterals(expr->expr.binary_expr.left_expr) 
    && checkForLiterals(expr->expr.binary_expr.right_expr);
   break;
  case UNARY_MINUS_EXPRESSION:
   out = checkForLiterals(expr->expr.unary_minus.expr);
   break;
  default:
   out = 1;
   break;
 }

 return out;
}

void visit_file(AST *root)
{
	printm(">>> file\n");
	printm("file has %d declarations\n", root->list.num_items);

	for (ListNode *ptr = root->list.first; ptr != NULL; ptr = ptr->next)
	{
		switch (ptr->ast->decl.type)
		{
		case FUNCTION_DECLARATION:
			visit_function_decl(ptr->ast);
			break;
		case VARIABLE_DECLARATION:
			visit_var_decl(ptr->ast);
			break;
		default:
			fprintf(stderr, "UNKNOWN DECLARATION TYPE %c\n", ptr->ast->decl.type);
			break;
		}
	}
	printm("<<< file\n");
}

void visit_function_decl(AST *ast)
{
	printm(">>> function_decl\n");
	AST *params = ast->decl.function.param_decl_list;

	if (ast->decl.function.type == 2) { // se for 2, tem retorno
		printf("define i32 @%s", ast->decl.function.id->id.string);
	} else { // se for 1, é void
		printf("define void @%s", ast->decl.function.id->id.string);
	}

	if (params != NULL)
	{
		printf("(");
		for (int i = 0; i < params->list.num_items; i++)
		{
			if (i == 0) printf("i32");
			else printf(", i32");
		}
		printf(") {\n");
	} else {
		printf("() {\n");
	}

	if (ast->decl.function.stat_block != NULL)
	{
		visit_stat_block(ast->decl.function.stat_block, params, ast->decl.function.type);
	}

	if (ast->decl.function.type == TYPE_VOID) {
			printf(" ret void\n");
			printf("}");
	}

	printm("<<< function_decl\n");
}

// what is surrounded by { }
ExprResult visit_stat_block(AST *stat_block, AST *params, int return_type)
{
	printm(">>> stat_block\n");
	ExprResult ret = {0, IS_CONSTANT};

	for (ListNode *ptr = stat_block->list.first; ptr != NULL; ptr = ptr->next)
	{
		ret = visit_stat(ptr->ast);
	}
	printm("<<< stat_block\n");
	return ret;
}

ExprResult visit_stat(AST *stat)
{
	printm(">>> statement\n");
	ExprResult ret = {0, IS_CONSTANT};
	switch (stat->stat.type)
	{
	case VARIABLE_DECLARATION:
		visit_var_decl(stat);
		break;
	case ASSIGN_STATEMENT:
		visit_assign_stat(stat);
		break;
	case RETURN_STATEMENT:
		ret = visit_return_stat(stat);
		break;
	case EXPRESSION_STATEMENT:
		visit_expr(stat->stat.expr.expr);
		break;
	default:
		fprintf(stderr, "UNKNOWN STATEMENT TYPE %c\n", stat->stat.type);
		break;
	}
	return ret;
	printm("<<< statement\n");
}

void visit_var_decl(AST *ast)
{
	printm(">>> var_decl\n");
	AST *id = ast->decl.variable.id;
	if (ast->decl.variable.id->id.flags == 1)
	{ // se a declaração de variável global
		if (ast->decl.variable.expr != NULL)
		{
			ExprResult expr = visit_expr(ast->decl.variable.expr);
			ast->decl.variable.id->id.int_value = expr.int_value;
			printf("@%s = global i32 %ld, align 4\n", ast->decl.variable.id->id.string, expr.int_value);
		}
		else
		{
			ast->decl.variable.id->id.int_value = 0;
			printf("@%s = common global i32 0, align 4\n", ast->decl.variable.id->id.string);
		}
	}
	else
	{ // se a declaração de variável dentro de uma função
		if (ast->decl.variable.expr != NULL)
		{
			ExprResult expr = visit_expr(ast->decl.variable.expr);
			ast->decl.variable.id->id.int_value = expr.int_value;
			printf("%s = %ld\n", ast->decl.variable.id->id.string, expr.int_value);
		}
		else
		{
			ast->decl.variable.id->id.int_value = 0;
		}
	}
	printm("<<< var_decl\n");
}

ExprResult visit_return_stat(AST *ast)
{
	printm(">>> return stat\n");
	ExprResult ret = {0, IS_CONSTANT};
	if (ast->stat.ret.expr)
	{
		ret = visit_expr(ast->stat.ret.expr);
		printf(" ret i32 %ld\n", ret.int_value);
		printf("}");
	}
	printm("<<< return stat\n");
	return ret;
}

void visit_assign_stat(AST *assign)
{
	printm(">>> assign stat\n");
	ExprResult expr = visit_expr(assign->stat.assign.expr);
	printm("<<< assign stat\n");
}

ExprResult visit_expr(AST *expr)
{
	printm(">>> expression\n");
	ExprResult ret = {};
	switch (expr->expr.type)
	{
	case BINARY_EXPRESSION:
		switch (expr->expr.binary_expr.operation)
		{
		case '+':
			ret = visit_add(expr);
			break;
		case '-':
			ret = visit_sub(expr);
			break;
		case '*':
			ret = visit_mul(expr);
			break;
		case '/':
			ret = visit_div(expr);
			break;
		case '%':
			ret = visit_mod(expr);
			break;
		default:
			fprintf(stderr, "UNKNOWN OPERATOR %c\n", expr->expr.binary_expr.operation);
			break;
		}
		break;
	case UNARY_MINUS_EXPRESSION:
		ret = visit_unary_minus(expr);
		break;
	case LITERAL_EXPRESSION:
		ret = visit_literal(expr);
		break;
	case IDENTIFIER_EXPRESSION:
		ret = visit_id(expr->expr.id.id);
		break;
	case FUNCTION_CALL_EXPRESSION:
		ret = visit_function_call(expr);
		break;
	default:
		fprintf(stderr, "UNKNOWN EXPRESSION TYPE %c\n", expr->expr.type);
		break;
	}
	printm("<<< expression\n");
	return ret;
}

// não implementar
ExprResult visit_function_call(AST *ast)
{
	printm(">>> function_call\n");
	ExprResult ret = {}, arg_expr[20]; //instead of alloca
	AST *arg_list = ast->expr.function_call.expr_list;

	if (arg_list != NULL)
	{
		int i = 0;
		for (ListNode *ptr = arg_list->list.first; ptr != NULL; ptr = ptr->next)
		{
			arg_expr[i++] = visit_expr(ptr->ast);
		}
	}
	printm("<<< function_call\n");
	return ret;
}

ExprResult visit_id(AST *ast)
{
	printm(">>> identifier\n");
	ExprResult ret = {}; // armazenar aqui
	if (ast->id.type == TYPE_INT)
	{
		ret.int_value = ast->id.int_value;
		ret.type = TYPE_INT;
	}
	else if (ast->id.type == TYPE_FLOAT)
	{
		ret.float_value = ast->id.float_value;
		ret.type = TYPE_FLOAT;
	}
	printm("<<< identifier\n");
	return ret;
}

ExprResult visit_literal(AST *ast)
{
	printm(">>> literal\n");
	ExprResult ret = {};
	ret.int_value = ast->expr.literal.int_value;
	ret.type = INTEGER_CONSTANT;
	printm("<<< literal\n");
	return ret;
}

ExprResult visit_unary_minus(AST *ast)
{
	printm(">>> unary_minus\n");
	ExprResult expr, ret = {};
	expr = visit_expr(ast->expr.unary_minus.expr);
	ret.type = INTEGER_CONSTANT;
	ret.int_value = -expr.int_value;
	printm("<<< unary_minus\n");
	return ret;
}

ExprResult visit_add(AST *ast)
{
	printm(">>> add\n");
	ExprResult left, right, ret = {};
	left = visit_expr(ast->expr.binary_expr.left_expr);
	right = visit_expr(ast->expr.binary_expr.right_expr);
	ret.type = INTEGER_CONSTANT;
	ret.int_value = left.int_value + right.int_value;
	printm("<<< add\n");
	return ret;
}

ExprResult visit_sub(AST *ast)
{
	printm(">>> sub\n");
	ExprResult left, right, ret = {};
	left = visit_expr(ast->expr.binary_expr.left_expr);
	right = visit_expr(ast->expr.binary_expr.right_expr);
	ret.type = INTEGER_CONSTANT;
	ret.int_value = left.int_value - right.int_value;
	printm("<<< sub\n");
	return ret;
}

ExprResult visit_mul(AST *ast)
{
	printm(">>> mul\n");
	ExprResult left, right, ret = {};
	left = visit_expr(ast->expr.binary_expr.left_expr);
	right = visit_expr(ast->expr.binary_expr.right_expr);
	ret.type = INTEGER_CONSTANT;
	ret.int_value = left.int_value * right.int_value;
	printm("<<< mul\n");
	return ret;
}

ExprResult visit_div(AST *ast)
{
	printm(">>> div\n");
	ExprResult left, right, ret = {};
	left = visit_expr(ast->expr.binary_expr.left_expr);
	right = visit_expr(ast->expr.binary_expr.right_expr);
	ret.type = INTEGER_CONSTANT;

	if (right.int_value != 0) {
		ret.type = INTEGER_CONSTANT;
		ret.int_value = left.int_value % right.int_value;
	}

	printm("<<< div\n");
	return ret;
}

ExprResult visit_mod(AST *ast)
{
	printm(">>> mod\n");
	ExprResult left, right, ret = {};
	left = visit_expr(ast->expr.binary_expr.left_expr);
	right = visit_expr(ast->expr.binary_expr.right_expr);

	if (right.int_value != 0) {
		ret.type = INTEGER_CONSTANT;
		ret.int_value = left.int_value % right.int_value;
	}

	printm("<<< mod\n");
	return ret;
}