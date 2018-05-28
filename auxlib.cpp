// $Id: auxlib.cpp,v 1.3 2017-10-11 14:28:14-07 - - $

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "auxlib.h"

string exec::execname;
int exec::exit_status = EXIT_SUCCESS;

const char* debugflags = "";
bool alldebugflags = false;

static void eprint_signal (const char* kind, int signal) {
   eprintf (", %s %d", kind, signal);
   const char* sigstr = strsignal (signal);
   if (sigstr != nullptr) fprintf (stderr, " %s", sigstr);
}

void eprint_status (const char* command, int status) {
   if (status == 0) return;
   eprintf ("%s: status 0x%04X", command, status);
   if (WIFEXITED (status)) {
      eprintf (", exit %d", WEXITSTATUS (status));
   }
   if (WIFSIGNALED (status)) {
      eprint_signal ("Terminated", WTERMSIG (status));
      #ifdef WCOREDUMP
      if (WCOREDUMP (status)) eprintf (", core dumped");
      #endif
   }
   if (WIFSTOPPED (status)) {
      eprint_signal ("Stopped", WSTOPSIG (status));
   }
   if (WIFCONTINUED (status)) {
      eprintf (", Continued");
   }
   eprintf ("\n");
}

void veprintf (const char* format, va_list args) {
   assert (exec::execname.size() != 0);
   assert (format != nullptr);
   fflush (nullptr);
   if (strstr (format, "%:") == format) {
      fprintf (stderr, "%s: ", exec::execname.c_str());
      format += 2;
   }
   vfprintf (stderr, format, args);
   fflush (nullptr);
}

void eprintf (const char* format, ...) {
   va_list args;
   va_start (args, format);
   veprintf (format, args);
   va_end (args);
}

void errprintf (const char* format, ...) {
   va_list args;
   va_start (args, format);
   veprintf (format, args);
   va_end (args);
   exec::exit_status = EXIT_FAILURE;
}

void syserrprintf (const char* object) {
   errprintf ("%:%s: %s\n", object, strerror (errno));
}

void __stubprintf (const char* file, int line, const char* func,
                   const char* format, ...) {
   va_list args;
   fflush (nullptr);
   printf ("%s: %s[%d] %s: "
        , exec::execname.c_str(), file, line, func);
   va_start (args, format);
   vprintf (format, args);
   va_end (args);
   fflush (nullptr);
}


void set_debugflags (const char* flags) {
   debugflags = flags;
   assert (debugflags != nullptr);
   if (strchr (debugflags, '@') != nullptr) alldebugflags = true;
   DEBUGF ('x', "Debugflags = \"%s\", all = %d\n",
           debugflags, alldebugflags);
}

bool is_debugflag (char flag) {
   return alldebugflags or strchr (debugflags, flag) != nullptr;
}

void __debugprintf (char flag, const char* file, int line,
                    const char* func, const char* format, ...) {
   va_list args;
   if (not is_debugflag (flag)) return;
   fflush (nullptr);
   va_start (args, format);
   fprintf (stderr, "DEBUGF(%c): %s[%d] %s():\n",
             flag, file, line, func);
   vfprintf (stderr, format, args);
   va_end (args);
   fflush (nullptr);
}
