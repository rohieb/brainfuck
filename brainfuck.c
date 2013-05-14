//Brainfuck Interpreter.

#include <stdio.h>
#include <string.h>

#ifndef STACKSIZE
#define STACKSIZE 262144	//shaves 256KB off stack
#endif

void
process_command (int c, char **ptr, FILE * finput)
{
  if (c == '>')
    ++(*ptr);
  if (c == '<')
    --(*ptr);
  if (c == '+')
    ++(**ptr);
  if (c == '-')
    --(**ptr);
  if (c == '.')
    putchar (**ptr);
  if (c == ',')
    **ptr = (char) getchar ();
  if (c == '[')
    {
      long pos = ftell (finput);
      int nestlevel = 1;
      //Interpret Loop.
      while (**ptr)
	{
	  fseek (finput, pos, SEEK_SET);
	  c = fgetc (finput);
	  while (c != ']' && c != EOF)
	    {
	      process_command (c, ptr, finput);
	      c = fgetc (finput);
	    }
	}
      //Fix for Zero-Loop.
      fseek (finput, pos, SEEK_SET);
      while (nestlevel)
	{
	  c = fgetc (finput);
	  if (c == '[')
	    ++nestlevel;
	  if (c == ']')
	    --nestlevel;
	  if (c == EOF)
	    nestlevel = 0;
	}
    }
}

int
interpret_stream (FILE * finput)
{
  int result = -1;
  char array[STACKSIZE], *ptr = array;

  memset (ptr, 0, sizeof (char) * STACKSIZE);

  while (!feof (finput))
    process_command (fgetc (finput), &ptr, finput);

  return result;
}

int
interpret (char *filename)
{
  int result = -1;
  FILE *finput = NULL, *foutput = NULL;


  if (strcmp (filename, "-"))
    {
      finput = fopen (filename, "r");
    }
  else
    {
      fprintf (stderr, "Stdin not supported\n");
      return result;
    }

  if (!finput)
    {
      fprintf (stderr, "Unable to open program: %s\n", filename);
      return result;
    }

  result = interpret_stream (finput);

  fclose (finput);

  return result;
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
      interpret (argv[(argc--) - 1]);
    }
  return 0;
}
