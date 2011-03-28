%{

#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include "systemc.h"
#include <vector>
#include <map>

using std::vector;
using std::map;

#ifdef DEBUG
#define PRINTF printf
#else
#define PRINTF(...) 
#endif

extern "C"
{
    int yyparse(void);
    int yylex(void); 
    void yyerror(const char *str); 
    void yyrestart(FILE*);
    int yywrap()
    {
	    return 1;
    }

}

typedef enum {
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING
} parameter_type;

typedef enum {
    SIGN_NONE,
    SIGN_SIGNED,
    SIGN_UNSIGNED
} parameter_sign;

typedef struct 
{
  parameter_type type;
  parameter_sign sign;
  union {
   char   t_char;
   short  t_short;
   int    t_int;
   long   t_long;
   float  t_float;
   double t_double;
   char   t_string[256];
  } value;
} parameter_declaration_t;


static parameter_declaration_t    Parameter_Declaration;
static char                       Module_Path[256];
static sc_simcontext              *Simulation_Context;
static sc_attr_base               *Current_Attribute;
static bool                       globalAttr;
static vector< sc_attr_base *>    globalAttrVector;
static map< sc_object *, sc_object * >       modulesMap;

int create_attribute(char *);
int add_attribute(sc_simcontext *context, char *obj_name, sc_attr_base *attribute);
int add_global_attributes();

extern unsigned int line_counter;
extern char line_buffer[1024];
extern char last_yytext[1024];
char * CurrentFileName;
%}

%union  
{ 
  char   string[256];
  int    integer;
  double real;
}

%token CHAR STRING SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE 
%token <integer> INTEGER
%token <real> REAL 
%token <string> COTED_STRING
%token <string> LITERAL 
%token PATH_START PATH_END PATH_SEPARATOR

%start Input
%%

Input:
     /* Empty */                                { line_buffer[0] = 0; last_yytext[0] = 0; }
   | ModuleConf Input                           { line_buffer[0] = 0; last_yytext[0] = 0; }
   ;

ModuleConf:
     ModulePath ModuleParams                     { }
   ;

ModulePath:
     PATH_START Path PATH_END                    { line_buffer[0] = 0; last_yytext[0] = 0; }
   ;

Path:
     /* Empty */																 { globalAttr = true;}
   | LITERAL                                     { sprintf(Module_Path,"%s",$1); globalAttr = false; }
   | Path PATH_SEPARATOR LITERAL                 { sprintf(Module_Path,"%s.%s",Module_Path,$3); }
   ;

ModuleParams:
     /* Empty */                                 { line_buffer[0] = 0; last_yytext[0] = 0;}
	 | ModuleParam ModuleParams			               { line_buffer[0] = 0; last_yytext[0] = 0;}
   ;

ModuleParam:
     ParamType LITERAL '=' ParamValue ';'        { if( create_attribute($2) == -1) return(-1); line_buffer[0] = 0; last_yytext[0] = 0;}

ParamType:
     CHAR                                        { Parameter_Declaration.type = TYPE_CHAR;
                                                   Parameter_Declaration.sign = SIGN_NONE; } 
   | CHAR '*'                                    { Parameter_Declaration.type = TYPE_STRING; }
   | STRING                                      { Parameter_Declaration.type = TYPE_STRING; }
   | SIGNED CHAR                                 { Parameter_Declaration.type = TYPE_CHAR; 
                                                   Parameter_Declaration.sign = SIGN_SIGNED; } 
   | UNSIGNED CHAR                               { Parameter_Declaration.type = TYPE_CHAR;
                                                   Parameter_Declaration.sign = SIGN_UNSIGNED; } 
   | ParamSign SHORT                             { Parameter_Declaration.type = TYPE_SHORT; }
   | ParamSign INT                               { Parameter_Declaration.type = TYPE_INT; }
   | ParamSign LONG                              { Parameter_Declaration.type = TYPE_LONG; }
   | FLOAT                                       { Parameter_Declaration.type = TYPE_FLOAT; }
   | DOUBLE                                      { Parameter_Declaration.type = TYPE_DOUBLE; }
   ;

ParamSign:
     /* Empty */                                 { Parameter_Declaration.sign = SIGN_NONE; } 
   | SIGNED                                      { Parameter_Declaration.sign = SIGN_SIGNED; } 
   | UNSIGNED                                    { Parameter_Declaration.sign = SIGN_UNSIGNED; } 
   ;

ParamValue:
     INTEGER                                     { 
                                                   switch(Parameter_Declaration.type)
                                                   {
                                                     case TYPE_CHAR : Parameter_Declaration.value.t_char = $1; break;
                                                     case TYPE_SHORT : Parameter_Declaration.value.t_short = $1; break;
                                                     case TYPE_INT : Parameter_Declaration.value.t_int = $1; break;
                                                     case TYPE_LONG : Parameter_Declaration.value.t_long = $1; break;
                                                     default: return(-1);
                                                   }
                                                 }
   | REAL                                        {
                                                   switch(Parameter_Declaration.type)
                                                   {
                                                     case TYPE_FLOAT : Parameter_Declaration.value.t_float = $1; break;
                                                     case TYPE_DOUBLE : Parameter_Declaration.value.t_double = $1; break;
                                                     default: return(-1);
                                                   }
                                                 }
   | COTED_STRING                                {
                                                   sprintf(Parameter_Declaration.value.t_string,"%s",&$1[1]);
                                                   Parameter_Declaration.value.t_string[strlen($1)-2] = 0; 
                                                 }
   ;

%%

void yyerror(const char *str)
{
	unsigned int index;

        fprintf(stderr,"%s at line %d\n",str,line_counter);

	index = 0;
	while(line_buffer[index] == '\n') index++;
	fprintf(stderr,"%s\n",&(line_buffer[index]));
}


int add_attribute(sc_simcontext *context, char *obj_name, sc_attr_base *attribute)
{
	sc_object *tmp_obj;
	if(globalAttr == false)
	{
		tmp_obj = sc_find_object(obj_name);
		if(tmp_obj == NULL)
		{
			printf("*******************************************************\n");
			printf("SystemC configuration  warning !!!\n");
			printf("\tIn %s line %d : %s object not found for attribute %s (skipped)\n",CurrentFileName,line_counter,obj_name,attribute->name().c_str());
			printf("*******************************************************\n");
		}
    else
    {
      PRINTF("Add attribute \"%s\"(0x%08x) to sc_object \"%s\"\n",attribute->name().c_str(),(unsigned int)attribute,obj_name);
      tmp_obj->add_attribute(*attribute);
      modulesMap[tmp_obj] = tmp_obj;
    }
  }
  else
  {
    globalAttrVector.push_back(attribute);
  }
  return(0);
}

int add_global_attributes()
{
  map< sc_object *, sc_object* >::iterator modulesMapIt;
  unsigned int index_attr;
  sc_attr_base *attr;
  sc_object    *obj;

  PRINTF("Add global attributes...\n");

  for(modulesMapIt = modulesMap.begin(); modulesMapIt != modulesMap.end(); modulesMapIt++)
  {
    for(index_attr = 0; index_attr < globalAttrVector.size(); index_attr++)
    {
      attr = globalAttrVector[index_attr];
      obj = (*modulesMapIt).first;
      PRINTF("Add attribute \"%s\"(0x%08x) to sc_object \"%s\"\n",attr->name().c_str(),(unsigned int)attr,obj->name());
      obj->add_attribute(*attr);
    }
  }
  return(0);
}

#define ATTRIBUTE_DECL_SIGNED(param,ptype,pname) \
  if(param.sign == SIGN_SIGNED) \
{ \
  sc_attribute< signed ptype > *tmp_attr = new sc_attribute< signed ptype >(pname); \
  tmp_attr->value = (signed ptype) param.value.t_##ptype ; \
  Current_Attribute = tmp_attr; \
}
//cout << tmp_attr->name() << " = " << tmp_attr->value << endl; 

#define ATTRIBUTE_DECL_UNSIGNED(param,ptype,pname) \
  if(param.sign == SIGN_UNSIGNED) \
{ \
  sc_attribute< unsigned ptype > *tmp_attr = new sc_attribute< unsigned ptype >(pname); \
  tmp_attr->value = (unsigned ptype) param.value.t_##ptype ; \
  Current_Attribute = tmp_attr; \
}
//cout << tmp_attr->name() << " = " << tmp_attr->value << endl; 

#define ATTRIBUTE_DECL(param,ptype,pname) \
  if(param.sign == SIGN_NONE) \
{ \
  sc_attribute< ptype > *tmp_attr = new sc_attribute< ptype >(pname); \
  tmp_attr->value = param.value.t_##ptype ; \
  Current_Attribute = tmp_attr; \
}
//cout << tmp_attr->name() << " = " << tmp_attr->value << endl; 

int create_attribute(char * name)
{
  switch(Parameter_Declaration.type)
  {
    case TYPE_CHAR :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,char,name);
        ATTRIBUTE_DECL_SIGNED(Parameter_Declaration,char,name);
        ATTRIBUTE_DECL_UNSIGNED(Parameter_Declaration,char,name);
        break;
      }
    case TYPE_SHORT :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,short,name);
        ATTRIBUTE_DECL_SIGNED(Parameter_Declaration,short,name);
        ATTRIBUTE_DECL_UNSIGNED(Parameter_Declaration,short,name);
        break;
      }
    case TYPE_INT :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,int,name);
        ATTRIBUTE_DECL_SIGNED(Parameter_Declaration,int,name);
        ATTRIBUTE_DECL_UNSIGNED(Parameter_Declaration,int,name);
        break;
      }
    case TYPE_LONG :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,long,name);
        ATTRIBUTE_DECL_SIGNED(Parameter_Declaration,long,name);
        ATTRIBUTE_DECL_UNSIGNED(Parameter_Declaration,long,name);
        break;
      }
    case TYPE_FLOAT :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,float,name);
        break;
      }
    case TYPE_DOUBLE :
      {
        ATTRIBUTE_DECL(Parameter_Declaration,double,name);
        break;
      }
    case TYPE_STRING :
      {
        sc_attribute< char * > *tmp_attr = new sc_attribute< char * >(name); 
        tmp_attr->value = strdup(Parameter_Declaration.value.t_string);
        Current_Attribute = tmp_attr; 
        //cout << tmp_attr->name() << " = " << tmp_attr->value << endl; 
        break;
      }
  }

  return(add_attribute(Simulation_Context, Module_Path, Current_Attribute));

}


int sc_configure(sc_simcontext* simcontext, char * filename) 
{
  FILE*  configFile = NULL;
  int result;

  memset((void*)line_buffer,0,sizeof(line_buffer));
  memset((void*)last_yytext,0,sizeof(last_yytext));

  PRINTF("SystemC configuration ...\n");

  if( (Simulation_Context = simcontext) == NULL)
  {
    PRINTF("SystemC context arg is NULL\n");
    return(-1);
  }

  configFile = fopen(filename,"r");
  if(configFile == NULL)
  {
    printf("%s: ERROR\n",__func__);
    printf("    opening %s\n",filename);
    return(-1);
  }

  CurrentFileName = filename;
  line_counter = 1;

  yyrestart(configFile);
  result = yyparse();
  fclose(configFile);

  if( result != -1) result = add_global_attributes();

  return(result);
}

