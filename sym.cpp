#include "lyutils.h"
#include "astree.h"

extern FILE* symfile;

size_t next_block = 1;
symbol_table global_symbol_table;
symbol_table local_symbol_table;
vector<symbol*> local_parameters;
string struct_name;

extern vector<string> stringcon_queue;

void print_attr(symbol& sym)
{
    // type
    if(sym.attributes.test(static_cast<size_t>(attr::VOID))) 
        fprintf (symfile," void");
    if(sym.attributes.test(static_cast<size_t>(attr::INT))) 
        fprintf (symfile," int");
    if(sym.attributes.test(static_cast<size_t>(attr::STRING))) 
        fprintf (symfile," string");
    if(sym.attributes.test(static_cast<size_t>(attr::NULLX))) 
        fprintf (symfile, " null");
    if(sym.attributes.test(static_cast<size_t>(attr::ARRAY))) 
        fprintf (symfile," array");
    if(sym.attributes.test(static_cast<size_t>(attr::STRUCT))) 
        fprintf (symfile," struct %s", struct_name.c_str());

    // attr
    if(sym.attributes.test(static_cast<size_t>(attr::FUNCTION))) 
        fprintf (symfile," function");
    if(sym.attributes.test(static_cast<size_t>(attr::VARIABLE))) 
        fprintf (symfile," variable");
    if(sym.attributes.test(static_cast<size_t>(attr::TYPEID))) 
        fprintf (symfile," typeid");

    // attr
    if(sym.attributes.test(static_cast<size_t>(attr::LVAL))) 
        fprintf (symfile, " lval");
    if(sym.attributes.test(static_cast<size_t>(attr::VADDR))) 
        fprintf (symfile, " vaddr");
    if(sym.attributes.test(static_cast<size_t>(attr::VREG))) 
        fprintf (symfile, " vreg");
    if(sym.attributes.test(static_cast<size_t>(attr::CONST))) 
        fprintf (symfile, " const");

    // loc seq
    if(sym.attributes.test(static_cast<size_t>(attr::PARAM))) 
        fprintf (symfile, " param %lu", sym.sequence);
    if(sym.attributes.test(static_cast<size_t>(attr::LOCAL))) 
        fprintf (symfile, " local %lu", sym.sequence);
}

void print_symbol (astree* node, const string indent = "    ") {
    symbol &sym = node->symbl;

    // indent
    fprintf (symfile, "%s", indent.c_str());

    // str
    fprintf (symfile, "%s (%zu.%zu.%zu)"
    , node->lexinfo->c_str()
    , node->lloc.filenr
    , node->lloc.linenr
    , node->lloc.offset);

    // field
    if(sym.attributes.test(static_cast<size_t>(attr::FIELD)))
    {
        print_attr(sym);
        fprintf (symfile, " field %lu", sym.sequence);
    }
    else
    {
        // block
        fprintf (symfile, " {%lu}", node->lloc.blocknr);
        print_attr(sym);
    }

    fprintf (symfile, "\n");
}

void setAttr(astree* node, attr att, size_t sqs = 0)
{
    if(node != nullptr)
    {
        node->symbl.attributes.set(static_cast<size_t>(att));
        if(sqs != 0)
            node->symbl.sequence = sqs;
    }
}

void setField(astree* node, symbol_table* field_table)
{
    if(node != nullptr)
    {
        node->symbl.fields = field_table;
    }
}

void setParams(astree* node, vector<symbol*>* params)
{
    if(node != nullptr)
    {
        node->symbl.parameters = params;
    }
}

bool isPrimitiveType(attr at){
    return at == attr::VOID 
    || at == attr::INT
    || at == attr::STRING;
}

bool checkTypeValid(astree* node){
    if(global_symbol_table.find(
        node->lexinfo)
        != global_symbol_table.end()){
            return true;
        }
    else{
        // used undeclared type
        return false;
    }
}

attr typeCheck(astree* node);

template<typename Functor>
void typeHandler(astree* node
               , Functor handler){
    // "xxx[] xxx;"
    if(node->tokenCode == TOK_ARRAY
    &&node->children.size() == 2){

        // "int[] xxx;"
        if(isPrimitiveType(
            typeCheck(node->children[0]))){
            handler(node->children[1]);
        }
        // "node[] xxx;"
        else{
            checkTypeValid(node->children[0]);
            setAttr(node->children[1], attr::STRUCT);
            struct_name = *(node
                ->children[0]->lexinfo);
            handler(node->children[1]);
        }
    }
    // "node xxx;"
    else if(node->tokenCode == TOK_TYPEID
        &&!node->children.empty()){
        checkTypeValid(node);
        setAttr(node, attr::STRUCT);
        struct_name = *(node->lexinfo);
        handler(node->children[0]);
    } 
    // "int xxx;"
    else{
        handler(node->children[0]);
    }  
}

void insertToGlobalTable(astree* node){
    global_symbol_table.insert(
        symbol_entry(
        node->lexinfo
        , &(node->symbl)));
}

#define PRIMITIVE_CASE_TEST(__TOK_CASE__, __ATTR__) \
case __TOK_CASE__: \
    if(!node->children.empty()) \
        setAttr(node->children[0], __ATTR__); \
    return __ATTR__;

attr typeCheck(astree* node)
{
    if(node == nullptr)
        return attr::VOID;

switch(node->tokenCode)
{
    PRIMITIVE_CASE_TEST(TOK_INT, attr::INT)
    PRIMITIVE_CASE_TEST(TOK_VOID, attr::VOID)
    PRIMITIVE_CASE_TEST(TOK_NULL, attr::NULLX)
    PRIMITIVE_CASE_TEST(TOK_STRING, attr::STRING)

    case TOK_TYPEID:
        return attr::STRUCT;
    
    case TOK_ARRAY:{
        if(node->children.size() == 2){
            setAttr(node->children[1], attr::ARRAY);
            setAttr(node->children[1]
            , typeCheck(node->children[0]));
        }
        return attr::ARRAY;
    }
    
    case TOK_STRUCT: {
        if(node->children.size() != 2)
            break;
    
        // struct title
        // eg: "node (0.1.7) {0} struct node"

        // attr::STRUCT
        setAttr(node->children[0], attr::STRUCT); 

        // struct name
        struct_name = *(node->children[0]->lexinfo);

        // new global sym
        insertToGlobalTable(node->children[0]);

        // print
        print_symbol (node->children[0], "\n");

        // fields
        // eg: foo (0.2.7) int field 0
        //     link (0.3.8) struct node field 1
        size_t sqs = 0;
        for (auto child : node->children[1]->children){
            if(!child) continue;

            typeHandler(child, [&](astree* node){
                // set field attr
                setAttr(node
                    , attr::FIELD
                    , sqs++);
                // collect fields symbols
                local_symbol_table.insert(
                    symbol_entry(
                    node->lexinfo
                    , &(node->symbl)));
                // print
                print_symbol (node);
            });
        }

        // save fields symbols
        //setField(node->children[0]
        //    , new symbol_table(local_symbol_table));

        // leave struct
        local_symbol_table.clear();
    
        return attr::STRUCT;
    }

    // function
    case TOK_FUNCTION: {
        if(node->children.size() != 3
        || node->children[0]->children.empty())
            break;

        /********* function title *********/

        typeHandler(node->children[0], [&](astree* node){
            // attr function
            setAttr(node, attr::FUNCTION);

            // all the params
            //setParams(node
            //, new vector<symbol*>(local_parameters));

            // insert to global table
            insertToGlobalTable(node);

            // print
            print_symbol (node, "\n");
        });

        /********* function params *********/

        for (auto child : node->children[1]->children)
        {
            if(!child || child->children.empty())
                continue;

            typeHandler(child, [&](astree* node){
                // blk nr
                node->lloc.blocknr = next_block;

                //print
                print_symbol (node);
            });
        }

        /********* function block *********/

        for (auto child : node->children[2]->children)
        {
            if(!child || child->children.empty()
                || child->children[0]->children.empty())
                continue;

            if(child->tokenCode == TOK_VARDECL)
            {
                typeHandler(child->children[0]
                , [&](astree* node){
                    // blk nr
                    node->lloc.blocknr = next_block;

                    //print
                    print_symbol(node);
                });
            }  
        }

        /********* leave function *********/

        local_symbol_table.clear();
        local_parameters.clear();
        next_block++;

        return attr::FUNCTION;
    }

    case TOK_PARAM:{
        size_t sqs = 0;
        for(auto child : node->children)
        {
            if(!child || child->children.empty())
                continue;

            typeHandler(child, [&](astree *node){
                setAttr(node, attr::VARIABLE);
                setAttr(node, attr::LVAL);
                setAttr(node, attr::PARAM, sqs++);

                local_parameters.push_back(&(node->symbl));
            });
        }
        return attr::PARAM;
    }

    case TOK_BLOCK:{
        size_t sqs = 0;
        for(auto child : node->children)
        {
            if(!child || child->children.empty()
                || child->children[0]->children.empty())
                    continue;

            if(child->tokenCode == TOK_VARDECL)
            {
                typeHandler(child->children[0]
                , [&](astree *node){
                    setAttr(node, attr::VARIABLE);
                    setAttr(node, attr::LVAL);
                    setAttr(node, attr::LOCAL, sqs++);
                });
            }
        }
        return attr::VOID;
    }

    case TOK_STRINGCON:{
        stringcon_queue.push_back(*(node->lexinfo));
    }

    default:
        break;
}
    return attr::VOID;
}

void post_order_traversal (astree* node) {
    for (auto child : node->children) {
        post_order_traversal (child);
    }
    typeCheck (node);
}

void type_check (astree* root) {
    post_order_traversal(root);
    fflush (nullptr);
}
