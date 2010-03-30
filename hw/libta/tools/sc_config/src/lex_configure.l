%{

#include "global.h"
#include "yacc_configure.hh"

#include <stdio.h>

#ifdef DEBUG
#define PRINTF printf
#else
#define PRINTF(...)
#endif

unsigned int line_counter = 1;
void comment();
char line_buffer[1024];
char last_yytext[1024];

#define BUILD_LINE_BUFFER \
sprintf(line_buffer,"%s%s",line_buffer,last_yytext); \
sprintf(last_yytext,"%s",yytext);

%}

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%%



"#"                     { comment(); }

"string"                { PRINTF("string\n");   BUILD_LINE_BUFFER; return(STRING); }
"char"                  { PRINTF("char\n");     BUILD_LINE_BUFFER; return(CHAR); }
"double"                { PRINTF("double\n");   BUILD_LINE_BUFFER; return(DOUBLE); }
"float"                 { PRINTF("float\n");    BUILD_LINE_BUFFER; return(FLOAT); }
"int"                   { PRINTF("int\n");      BUILD_LINE_BUFFER; return(INT); }
"long"                  { PRINTF("long\n");     BUILD_LINE_BUFFER; return(LONG); }
"short"                 { PRINTF("short\n");    BUILD_LINE_BUFFER; return(SHORT); }
"signed"                { PRINTF("signed\n");   BUILD_LINE_BUFFER; return(SIGNED); }
"unsigned"							{ PRINTF("unsigned\n"); BUILD_LINE_BUFFER; return(UNSIGNED); }
";"                     { PRINTF(";\n");        BUILD_LINE_BUFFER; return(';'); }
"*"                     { PRINTF("*\n");        BUILD_LINE_BUFFER; return('*'); }
"="                     { PRINTF("=\n");        BUILD_LINE_BUFFER; return('='); }

{D}+                    { yylval.integer = atoi(yytext);      BUILD_LINE_BUFFER; return(INTEGER); }
{D}+"."{D}+             { yylval.real = atof(yytext);         BUILD_LINE_BUFFER; return(REAL); }
\".*\"                  { sprintf(yylval.string,"%s",yytext); BUILD_LINE_BUFFER; return(COTED_STRING); }

"["                     { PRINTF("Start path\n"); BUILD_LINE_BUFFER; return(PATH_START); }
"]"                     { PRINTF("End path\n");   BUILD_LINE_BUFFER; return(PATH_END); }
"."                     { PRINTF(".\n");          BUILD_LINE_BUFFER; return(PATH_SEPARATOR); }

{L}({L}|{D})*           { PRINTF("LITERAL : %s\n",yytext); sprintf(yylval.string,"%s",yytext); BUILD_LINE_BUFFER; return(LITERAL); }

[ \t\v\f]		{ BUILD_LINE_BUFFER; }
[\n]                    { line_counter++; PRINTF("End of line %d\n",line_counter); BUILD_LINE_BUFFER; }

<<EOF>>                 { PRINTF("End of file (%d lines)\n",line_counter);  return 0 ;}

%%

void comment()
{
	char c;
	PRINTF("Skip comment\n");
	while ((c = input()) != '\n' && c != 0);
	unput(c);
	line_buffer[0] = 0;
	last_yytext[0] = 0;
}

