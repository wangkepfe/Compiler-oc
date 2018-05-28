// $Id: astree.h,v 1.10 2016-10-06 16:42:35-07 - - $

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

#include "auxlib.h"
#include "sym.h"

struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
   size_t blocknr;
};

struct astree {

   // Fields.
   int tokenCode;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
   symbol symbl;

   // Functions.
   astree (int _symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr, astree* child3 = nullptr);
   astree* sym(int symbol);
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   static void print (FILE* outfile, astree* tree, int depth = 0);
};

void destroy (astree* tree1, astree* tree2 = nullptr, astree* tree3 = nullptr);

void errllocprintf (const location&, const char* format, const char*);

void type_check (astree* root);

#endif

