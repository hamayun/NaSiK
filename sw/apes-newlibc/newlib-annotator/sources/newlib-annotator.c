#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <libgen.h>

//#define DEBUG
#ifdef DEBUG
#define PRINTF  printf
#else
#define PRINTF(...)
#endif

char * format_arg(char * arg);
char * get_arch(char * str);
char * get_prefix(char * str);
int execute(char * argv[]);
int compile_with_gnu_gcc(int argc, char *argv[]);

typedef struct
{
  const char * opt;
  uint32_t     skip;
  uint32_t     skipped;
} skip_t;

static const char * arch_names[] = 
{ 
  (const char*) "mips", 
  (const char*) "mipsel",
  (const char*) "arm", 
  (const char*) "i386" 
};

static skip_t mips_skip_desc[] =
{
  { (const char*)"-march", 1, 0 },
  { (const char*)"-G0", 1, 0 },
  { (const char*)"-EL", 1,0 },
  { (const char*)"-annotate", 1,0 },
  { (const char*)NULL, 0 }
};

static skip_t arm_skip_desc[] =
{
  { (const char*)"-mfloat-abi", 1,0 },
  { (const char*)NULL, 0 }
};

static skip_t i386_skip_desc[] =
{
  { (const char*)"-march", 1, 0 },
  { (const char*)"-g", 1, 0 },
  { (const char*)"-annotate", 1,0 },
  { (const char*)"-mfloat-abi", 1,0 },
  { (const char*)"-EL", 1,0 },
  { (const char*)"-G0", 1, 0 },
  { (const char*)NULL, 0 }
};

static skip_t * skip_desc[] =
{
  mips_skip_desc,   /* mips */
  mips_skip_desc,   /* mipsel */
  arm_skip_desc,    /* arm */
  i386_skip_desc    /* i386 */
};

static const char * mips_llc_options[] =
{
  (const char*)"-relocation-model=static",
  (const char*)NULL
};

static const char * arm_llc_options[] =
{
  (const char*)NULL
};

static const char * i386_llc_options[] =
{
  (const char*)NULL
};

static const char** arch_llc_options[] =
{
  mips_llc_options,   /* mips */
  mips_llc_options,   /* mipsel */
  arm_llc_options,     /* arm  */
  i386_llc_options     /* arm  */
};

static const char * mips_as_options[] =
{
  (const char*)NULL
};

static const char * arm_as_options[] =
{
  (const char*)"-mfloat-abi=soft",
  (const char*)NULL
};

static const char * i386_as_options[] =
{
  (const char*)NULL
};

static const char** arch_as_options[] =
{
  mips_as_options,   /* mips */
  mips_as_options,   /* mipsel */
  arm_as_options,     /* arm  */
  i386_as_options     /* arm  */
};

int main(int argc, char * argv[])
{
  uint32_t    i,j;
  char        *_argv[1024];
  char        *_arg;
  char        bc_filename[L_tmpnam + /*.bc*/ 3 + /* '\0' */ 1];
  char        s_filename[L_tmpnam + /*.s*/ 2 + /* '\0' */ 1];
  char        *tmpname;
  char        *output_file = NULL;
  char        buffer[256];
  char        out_specified = 0;
  char        *gnu_gcc_path = NULL;
  char        *gnu_gcc = NULL;
  skip_t      *arch_skip_desc = NULL;
  uint32_t    skip_desc_idx;
  uint32_t    skip;
  char        *arch;
  uint32_t    not_a_o_file = 0;
  const char**  llc_options;
  const char**  as_options;
  uint32_t     annotation_mode = 0;

  if( (gnu_gcc_path = getenv("GNU_TOOLCHAINS")) == NULL )
  {
    printf("GNU_TOOLCHAINS must be set\n");
    exit(EXIT_FAILURE);
  }
  sprintf(buffer,"%s/%s/bin/%s",gnu_gcc_path,get_prefix(argv[0]),argv[0]);
  gnu_gcc = strdup(buffer);

  if( getenv("FORCE_GNU_GCC") != NULL )
  {
    PRINTF("FORCE_GNU_GCC set ...\n");
    argv[0] = gnu_gcc;
    return(execute(&(argv[0])));
  }

  arch = strdup(get_arch(argv[0]));
  for(i=0;i<(sizeof(skip_desc)/4);i++)
  {
    arch_skip_desc = skip_desc[i];
    llc_options = arch_llc_options[i];
    as_options = arch_as_options[i];
    if(strcmp(arch_names[i],arch) == 0) break;
  }
  PRINTF("Arch = %s\n",arch_names[i]);

  tmpname = tmpnam(NULL);
  sprintf(bc_filename,"%s.bc",tmpname);

  /* Run LLVM for cross compilation*/
  j = 0;
  _argv[j++] = strdup("llvm-gcc");
  _argv[j++] = strdup("-emit-llvm");
  _argv[j++] = strdup("-U__linux__");

  for(i = 1; i < argc ; i++)
  {
    _arg = argv[i];
    PRINTF("argv[%d] = %s\n",i,argv[i]);
    if(strcmp("-o",argv[i-1]) == 0)
    {
      if( _arg[strlen(_arg)-1] != 'o' )
      {
        PRINTF("%s is not an object file\n",_arg);
        not_a_o_file = 1;
        break;
      }
      _arg = strdup(bc_filename);
      output_file = argv[i];
      out_specified = 1;
    }

    /* skip options */
    skip = 0;
    for( skip_desc_idx = 0; arch_skip_desc[skip_desc_idx].opt != NULL ; skip_desc_idx++)
    {
      PRINTF("AnnotatorDBG: check options %s/%s\n",arch_skip_desc[skip_desc_idx].opt,_arg);
      if( strncmp(  arch_skip_desc[skip_desc_idx].opt,
            _arg,
            strlen(arch_skip_desc[skip_desc_idx].opt) ) == 0)
      {
        skip = arch_skip_desc[skip_desc_idx].skip;
        arch_skip_desc[skip_desc_idx].skipped++;
        PRINTF("AnnotatorDBG: skip option %s\n",arch_skip_desc[skip_desc_idx].opt);
        break;
      }
    }

    if(skip == 0)
      _argv[j++] = _arg;
    else
      i += skip - 1;

  }
  _argv[j] = NULL;

  if( out_specified == 0 || not_a_o_file == 1)
  {
    PRINTF("No output specified or not a O file ... use gnu %s\n",gnu_gcc);
    return(compile_with_gnu_gcc(argc,argv));
  }

  execute(_argv);

  if(getenv("ANNOTATED_ARCH") != NULL) annotation_mode = 1;

  /* Run LLC */
  sprintf(s_filename,"%s.s",tmpname);
  j = 0;
  _argv[j++] = "llc";

  for(i=0; llc_options[i] != NULL ; i++) 
	_argv[j++] = (char *)llc_options[i];

//  _argv[j++] = "-print-annotation-warnings";
//  _argv[j++] = "-print-dual-cfg";
//  _argv[j++] = "-print-annotated-cfg";
//  _argv[j++] = "-annotate-branch-penalty";
 
  if( annotation_mode > 0)
  {
    PRINTF("AnnotatorDBG: Annotation mode\n");
    sprintf(buffer,"-annotate=%s",getenv("ANNOTATED_ARCH"));
  }
  else
  {
    PRINTF("AnnotatorDBG: Cross compilation mode\n");
    sprintf(buffer,"-march=%s",get_arch(argv[0]));
  }
  
  _argv[j++] = strdup(buffer);
  _argv[j++] = strdup(bc_filename);
  _argv[j++] = "-o";
  _argv[j++] = strdup(s_filename);
  _argv[j] = NULL;

  execute(_argv); 

  /* Run as */
  sprintf(s_filename,"%s.s",tmpname);

  j = 0;
  if(annotation_mode > 0) 
	{ sprintf(buffer,"as"); }
  else 
	{ sprintf(buffer,"%s-as",get_prefix(argv[0])); }

  _argv[j++] = strdup(buffer);
  _argv[j++] = strdup(s_filename);

  for(i=0 ; as_options[i] != NULL ; i++) 
	_argv[j++] = as_options[i];

  _argv[j++] = "-o";
  _argv[j++] = strdup(output_file);
  _argv[j] = NULL;

  execute(_argv);

  FILE * out;
  if( (out_specified == 1) && ((out = fopen(output_file,"r")) == NULL) )
  {
    printf("AnnotatorDBG: !!!! LLVM-GCC FAILED  --> RETRY WITH GCC !!!!\n");
    return(compile_with_gnu_gcc(argc,argv));
  }
  else
  {
    fclose(out);
  }

#if 0
  sprintf(buffer,"rm -f %s",s_filename); system(buffer);
  sprintf(buffer,"rm -f %s",bc_filename); system(buffer);
#endif

  PRINTF("End...\n");
  return(EXIT_SUCCESS);
}

char * format_arg(char * arg)
{
  char          buffer[1024];
  uint32_t      i,j;

  i = j = 0;
  while(arg[i] != 0)
  {
    switch(arg[i])
    {
      //      case '"' :
      //      case ' ':
      //        buffer[j++] = '\\';
    }
    buffer[j++] = arg[i++];
  }
  buffer[j] = 0;

  return(strdup(buffer));
}

int execute(char * argv[])
{
  uint32_t    i;
  pid_t       child_pid;
  int         child_ExitStatus;

  i = 0;
  while(argv[i] != NULL)
  {
    PRINTF("argv[%d] = %s\n",i,argv[i]);
    i++;
  }

  child_pid = fork();
  if( child_pid == 0)
  {
    if(execvp(argv[0],argv) == -1)
      perror("Failed to exec");
    exit(-1);
  }
  else if( child_pid < 0 )
  {
    PRINTF("Failed to fork\n");
    exit(1);
  }
  else
  {
    waitpid( child_pid, &child_ExitStatus, 0);
    if( !WIFEXITED(child_ExitStatus) )
    {
      printf("waitpid() exited with an error: Status= %d\n",WEXITSTATUS(child_ExitStatus));
      return(-1);
    }
    else if( WIFSIGNALED(child_ExitStatus) )
    {
      printf("waitpid() exited due to a signal: %d\n",WTERMSIG(child_ExitStatus));
      return(-1);
    }
    if(WEXITSTATUS(child_ExitStatus) == 255)
      exit(-1);
  }
  return(0);
}

/*
 * get the program-prefix of the toolchain
 * str muste be formated as follow:
 * <1>-<2>- ... -<n>
 * return <1>-<2>- ... -<n-1>
 */
char * get_prefix(char * str)
{
  char  buffer[128];
  char  *token;
  char  *prev_token;

  buffer[0] = 0;
  prev_token = strtok(strdup(basename(str)),"-");
  token = strtok(NULL,"-");
  while(token != NULL)
  {
    sprintf(buffer,"%s%s",buffer,prev_token);
    prev_token = token;
    token = strtok(NULL,"-");
    if(token != NULL)
      sprintf(buffer,"%s-",buffer);
  }
  return(strdup(buffer));
}

/*
 * get processor type.
 * str muste be formated as follow:
 * arch-*
 * return arch
 */
char * get_arch(char * str)
{
  char *token;

  token = strtok(strdup(basename(str)),"-");
  if(token != NULL)
    return(strdup(token));

  return(NULL);
}

int compile_with_gnu_gcc(int argc, char *argv[])
{
  uint32_t    i,j;
  char        *_argv[1024];
  char        *_arg;
  char        buffer[256];
  char        *gnu_gcc_path = NULL;
  char        *gnu_gcc = NULL;
  skip_t      *arch_skip_desc = NULL;
  uint32_t    skip_desc_idx;
  uint32_t    skip;
  char        *arch;

  PRINTF("%s\n",__func__);

  if( (gnu_gcc_path = getenv("GNU_TOOLCHAINS")) == NULL )
  {
    printf("GNU_TOOLCHAINS must be set\n");
    exit(EXIT_FAILURE);
  }
  sprintf(buffer,"%s/bin/%s",gnu_gcc_path,argv[0]);
  gnu_gcc = strdup(buffer);

  arch = strdup(get_arch(argv[0]));
  for(i=0;i<(sizeof(skip_desc)/4);i++)
  {
    arch_skip_desc = skip_desc[i];
    if(strcmp(arch_names[i],arch) == 0) break;
  }
  PRINTF("Arch = %s\n",arch_names[i]);

  j = 0;
  _argv[j++] = gnu_gcc;
  for(i = 1; i < argc ; i++)
  {
    _arg = argv[i];
    _argv[j++] = _arg;
#if 0
    PRINTF("argv[%d] = %s\n",i,_arg);

    /* skip options */
    skip = 0;
    for( skip_desc_idx = 0; arch_skip_desc[skip_desc_idx].opt != NULL ; skip_desc_idx++)
    {
  //    PRINTF("check options %s/%s\n",arch_skip_desc[skip_desc_idx].opt,_arg);
      if( strncmp(  arch_skip_desc[skip_desc_idx].opt,
            _arg,
            strlen(arch_skip_desc[skip_desc_idx].opt) ) == 0)
      {
        skip = arch_skip_desc[skip_desc_idx].skip;
        arch_skip_desc[skip_desc_idx].skipped++;
        PRINTF("skip option %s\n",arch_skip_desc[skip_desc_idx].opt);
        break;
      }
    }
    //if(strncmp("-mcpu",_arg,5) == 0) skip = 1;

    if(skip == 0)
      _argv[j++] = _arg;
    else
      i += skip - 1;
#endif

  }

  _argv[j] = NULL;
  return(execute(&(_argv[0])));
}

