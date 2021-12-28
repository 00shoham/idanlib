#include <stdio.h>

extern char** environ;

int main()
  {
  fputs("Content-Type: text/html\r\n\r\n", stdout );
  fputs("<html>\n", stdout);
  fputs("  <head>\n", stdout);
  fputs("    <title>Environment variables</title>\n", stdout);
  fputs("  </head>\n", stdout);
  fputs("  <body>\n", stdout);

  while( environ!=NULL
         && *environ!=NULL )
    {
    fputs( "    <pre>", stdout );
    fputs( *environ, stdout );
    fputs( "</pre>\n", stdout );
    ++environ;
    }
  
  fputs("  </body>\n", stdout);
  fputs("</html>\n", stdout);

  return 0;
  }
