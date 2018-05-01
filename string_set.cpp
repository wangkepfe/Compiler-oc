// $Id: string_set.cpp,v 1.3 2017-10-05 16:39:39-07 - - $

#include <string>
#include <unordered_set>
using namespace std;

#include "auxlib.h"
#include "string_set.h"

unordered_set<string> string_set::set;

string_set::string_set() {
   set.max_load_factor (0.5);
}

const string* string_set::intern (const char* string) {
   auto handle = set.insert (string);
   DEBUGF ('s', "inserted \"%s\" %s\n", handle.first->c_str(),
           handle.second ? "newly inserted" : "already there");
   return &*handle.first;
}

void string_set::dump (FILE* file) {
   static unordered_set<string>::hasher hash_fn
               = string_set::set.hash_function();
   size_t max_bucket_size = 0;
   for (size_t bucket = 0; bucket < set.bucket_count(); ++bucket) {
      bool need_index = true;
      size_t curr_size = set.bucket_size (bucket);
      if (max_bucket_size < curr_size) max_bucket_size = curr_size;
      for (auto itor = set.cbegin (bucket);
           itor != set.cend (bucket); ++itor) {
         if (need_index) fprintf (file, "string_set[%4zu]: ", bucket);
                    else fprintf (file, "          %4s   ", "");
         need_index = false;
         const string* str = &*itor;
         fprintf (file, "%22zu %p->\"%s\"\n", hash_fn(*str),
         str, str->c_str());
      }
   }
   fprintf (file, "load_factor = %.3f\n", set.load_factor());
   fprintf (file, "bucket_count = %zu\n", set.bucket_count());
   fprintf (file, "max_bucket_size = %zu\n", max_bucket_size);
}

