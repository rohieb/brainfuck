//Brainfuck Compiler.

#include <stdio.h>
#include <string.h>

#define FILENAME 1024

#ifndef STACKSIZE
#define STACKSIZE 262144	//shaves 256KB off stack
#endif

static int labeln;
static int emit_loop_start;
static int emit_loop_end;

void process_command (int c, FILE * finput, FILE * foutput);

void
process_x86_command (int c, FILE * finput, FILE * foutput)
{
  while (1)
    {
      if (c == '>' || c == '<')
	{
	  int levels = 0;
	  do
	    {
	      if (c == '<')
		--levels;
	      else if (c == '>')
		++levels;
	      c = fgetc (finput);
	    }
	  while (c == '>' || c == '<');
	  if (c == ']')
	    ungetc (c, finput);
	  if (levels)
	    {
	      emit_loop_start = 0;
	      emit_loop_end = 0;
	      fprintf (foutput, "leal %d(%%ebx),%%ebx\n", levels);
	    }
	}
      else if (c == '+' || c == '-')
	{
	  char count = 0;
	  do
	    {
	      if (c == '-')
		--count;
	      else if (c == '+')
		++count;
	      c = fgetc (finput);
	    }
	  while (c == '+' || c == '-');
	  if (c == ']')
	    ungetc (c, finput);
	  if (count)
	    {
	      emit_loop_start = 0;
	      emit_loop_end = 0;
	      fprintf (foutput, "addb $%d,(%%ebx)\n", count);
	    }
	}
      else if (c == '.')
	{
	  emit_loop_start = 0;
	  emit_loop_end = 0;
	  fputs ("movsbl (%ebx),%eax\nmovl %eax,(%esp)\n", foutput);
	  do
	    {
#ifdef _WIN32
	      fputs ("call _putchar\n", foutput);
#else
	      fputs ("call putchar\n", foutput);
#endif
	      c = fgetc (finput);
	    }
	  while (c == '.');
	  if (c == ']')
	    ungetc (c, finput);
	}
      else if (c == ',')
	{
	  emit_loop_start = 0;
	  emit_loop_end = 0;
	  do
	    {
#ifdef _WIN32
	      fputs ("call _getchar\n", foutput);
#else
	      fputs ("call getchar\n", foutput);
#endif
	      c = fgetc (finput);
	    }
	  while (c == ',');
	  if (c == ']')
	    ungetc (c, finput);
	  fputs ("movb %al,(%ebx)\n", foutput);
	}
      else
	break;
    }
  if (c == '[')
    {
      int uniq_label = labeln++;
      if (!emit_loop_start)
	{
	  fputs ("cmpb $0,(%ebx)\n", foutput);
	  fprintf (foutput, "je bflp_%d_end\n", uniq_label);
	  emit_loop_start = 1;
	}
      fprintf (foutput, "bflp_%d_start:\n", uniq_label);
      //Recurse.
      c = fgetc (finput);
      while (c != ']' && c != EOF)
	{
	  process_command (c, finput, foutput);
	  c = fgetc (finput);
	}
      if (!emit_loop_end)
	{
	  fputs ("cmpb $0,(%ebx)\n", foutput);
	  fprintf (foutput, "jne bflp_%d_start\n", uniq_label);
	  emit_loop_end = 1;
	}
      fprintf (foutput, "bflp_%d_end:\n", uniq_label);
    }
}

void
process_command (int c, FILE * finput, FILE * foutput)
{
  process_x86_command (c, finput, foutput);
}

void
compile_stream (FILE * finput, FILE * foutput)
{
  int c = fgetc (finput);
  while (c != EOF)
    {
      process_command (c, finput, foutput);
      c = fgetc (finput);
    }
}

void
generate_x86_prolog (FILE * foutput)
{
  fputs ("pushl %ebx\n", foutput);
  fputs ("leal -4(%esp),%esp\n", foutput);	//Pre-allocate putchar space.
  fputs ("movl $bfs,%ebx\n", foutput);
}

void
generate_prolog (FILE * foutput)
{
#ifdef _WIN32
  fputs (".extern _putchar\n", foutput);
  fputs (".extern _getchar\n", foutput);
  fputs (".globl _main\n", foutput);
#else
  fputs (".extern putchar\n", foutput);
  fputs (".extern getchar\n", foutput);
  fputs (".globl main\n", foutput);
#endif
  fputs (".data\n", foutput);
  fputs ("bfs:\n", foutput);
  fprintf (foutput, ".zero %d\n", STACKSIZE);
  fputs (".text\n", foutput);
#ifdef _WIN32
  fputs ("_main:\n", foutput);
#else
  fputs ("main:\n", foutput);
#endif
  generate_x86_prolog (foutput);
}

void
generate_x86_epilog (FILE * foutput)
{
  fputs ("leal 4(%esp),%esp\n", foutput);	//De-allocate putchar space.
  fputs ("popl %ebx\n", foutput);	//Real EBX
  fputs ("xorl %eax,%eax\n", foutput);
  fputs ("ret\n", foutput);
}

void
generate_epilog (FILE * foutput)
{
  generate_x86_epilog (foutput);
  fputs (".ident  \"brainfuck\"\n", foutput);
}

void
compile (char *filename)
{
  char targfilename[FILENAME];
  FILE *finput = NULL, *foutput = NULL;


  if (strcmp (filename, "-"))
    {
      finput = fopen (filename, "r");
      snprintf (targfilename, FILENAME, "%s.s", filename);
    }
  else
    {
      finput = stdin;
      foutput = stdout;
    }


  if (!finput)
    {
      fprintf (stderr, "Unable to open program: %s\n", filename);
      return;
    }

  if (!foutput)
    foutput = fopen (targfilename, "w");
  if (!foutput)
    {
      fprintf (stderr, "Unable to open target program: %s\n", filename);
      return;
    }

  labeln = 0;
  emit_loop_start = 0;
  emit_loop_end = 0;
  generate_prolog (foutput);
  compile_stream (finput, foutput);
  generate_epilog (foutput);

  fclose (foutput);
  fclose (finput);
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s file.b\n", *argv);
      return -1;
    }
  while (argc > 1)
    {
      compile (argv[(argc--) - 1]);
    }
  return 0;
}
