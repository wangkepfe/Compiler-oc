// main.cpp
// cmpe104a asg1
// Apr 15
// Ke Wang

#include <string>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lyutils.h"
#include "astree.h"
#include "auxlib.h"
#include "string_set.h"

using namespace std;

const string CPP = "/usr/bin/cpp -nostdinc";
string cpp_command;

string Dstring{};
string program{};

void scanTokens(){
    FILE* tokenFile = fopen((program + ".tok").c_str(), "w");

    for (;;) {
      int token = yylex();
      DEBUGF('m', "token = %d", token);
      if (yy_flex_debug) fflush (NULL);
      if (token == YYEOF) break;

      string_set::intern(yytext);
      yylval->dump_node(tokenFile);
    }

    fclose(tokenFile);

    // dump string sets
    FILE* stringSetFile = fopen((program + ".str").c_str(), "w");
    string_set::dump(stringSetFile);
    fclose(stringSetFile);
}

void cpp_popen (const char* filename) {
   cpp_command = CPP + " " + Dstring + " " + filename;
   yyin = popen (cpp_command.c_str(), "r");
   if (yyin == nullptr) {
      syserrprintf (cpp_command.c_str());
      exit (exec::exit_status);
   }else {
      if (yy_flex_debug) {
         fprintf (stderr, "-- popen (%s), fileno(yyin) = %d\n",
                  cpp_command.c_str(), fileno (yyin));
      }

      lexer::newfilename (cpp_command);
      scanTokens();
   }
}

void cpp_pclose() {
   int pclose_rc = pclose (yyin);
   eprint_status (cpp_command.c_str(), pclose_rc);
   if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
}

void scan_opts (int argc, char** argv) {
   opterr = 0;

   yy_flex_debug = 0;
   yydebug = 0;

   for(;;)
   {
      int opt = getopt (argc, argv, "@:D:ly");
      if (opt == EOF) break;
      switch (opt)
      {
         case '@': set_debugflags (optarg);   break;
         case 'D': Dstring = "-D" + string(optarg);  break;
         case 'l': yy_flex_debug = 1;         break;
         case 'y': yydebug = 1;               break;
         default:  errprintf ("bad option (%c)\n", optopt); break;
      }
   }
   if (optind > argc) {
      errprintf ("Usage: %s [-ly] [filename]\n", exec::execname.c_str());
      exit (exec::exit_status);
   }

   const char* filename = optind == argc ? "-" : argv[optind];

   program = string(filename);
   program = program.substr(0, program.find_last_of('.'));

   cpp_popen (filename);
}

int main(int argc, char** argv)
{
    exec::execname = basename (argv[0]);
    scan_opts (argc, argv);
    cpp_pclose();
    yylex_destroy();
    return exec::exit_status;
}
