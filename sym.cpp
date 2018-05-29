#include "lyutils.h"
#include "astree.h"

extern FILE* symfile;

size_t next_block = 1;
symbol_table global_symbol_table;
symbol_table local_symbol_table;
vector<symbol*> local_parameters;
string struct_name;

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
        if(sqs)
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

attr typeCheck(astree* node)
{
    if(node == nullptr)
        return attr::VOID;

    switch(node->tokenCode)
    {
        case TOK_VOID: 
            if(!node->children.empty())
                setAttr(node->children[0], attr::VOID); 
            return attr::VOID;

        case TOK_INT: 
            if(!node->children.empty())
                setAttr(node->children[0], attr::INT); 
            return attr::INT;

        case TOK_NULL: 
            if(!node->children.empty())
                setAttr(node->children[0], attr::NULLX); 
            return attr::NULLX;
        case TOK_STRING: 
            if(!node->children.empty())
                setAttr(node->children[0], attr::STRING); 
            return attr::STRING;
        case TOK_ARRAY: 
        {
            if(node->children.size() == 2)
            {
                setAttr(node->children[1], attr::ARRAY);
                setAttr(node->children[1], typeCheck(node->children[0]));
            }
            return attr::ARRAY;
        } 
        
        case TOK_STRUCT: 
        {
        if(node->children.size() == 2)
        {
            // print struct title
            setAttr(node->children[0], attr::STRUCT); 
            //setField(node->children[0]
            //    , new symbol_table(local_symbol_table));
            
            global_symbol_table.insert(symbol_entry(
                node->children[0]->lexinfo
                , &(node->children[0]->symbl)));

            print_symbol (node->children[0], "\n");

            // print field
            for (auto child : node->children[1]->children)
            {
                if(!child)
                    continue;

                if(child->tokenCode == TOK_ARRAY
                    ||child->children.size() == 2)
                {
                    if(global_symbol_table.find(child
                        ->children[0]->lexinfo)
                        != global_symbol_table.end())
                    {
                        setAttr(child->children[1], attr::STRUCT);
                        struct_name = *(child
                            ->children[0]->lexinfo);
                        print_symbol (child->children[1]);
                    }
                    else
                    {
                        // used undeclared type
                    }
                }
                else if(child->tokenCode == TOK_TYPEID
                    ||!child->children.empty())
                {
                    if(global_symbol_table.find(child->lexinfo)
                        != global_symbol_table.end())
                    {
                        setAttr(child->children[0], attr::STRUCT);
                        struct_name = *(child->lexinfo);
                        print_symbol (child->children[0]);
                    }
                    else
                    {
                        // used undeclared type
                    }
                }
                else
                {
                    if(!child->children.empty())
                        print_symbol (child->children[0]);
                }
            }

            // leave struct
            local_symbol_table.clear();
        }
            return attr::STRUCT;
        }

        case TOK_FIELD:
        {
            size_t sqs = 0;
            for(auto child : node->children)
            {
                if(!child)
                    continue;
                
                if(!child->children.empty())
                {
                    if(child->tokenCode == TOK_ARRAY
                    ||child->children.size() == 2)
                    {
                        setAttr(child->children[1]
                        , attr::FIELD, sqs++);

                        local_symbol_table.insert(symbol_entry(
                        child->children[1]->lexinfo
                        , &(child->children[1]->symbl)));
                    }
                    else
                    {
                        setAttr(child->children[0]
                        , attr::FIELD, sqs++);

                        local_symbol_table.insert(symbol_entry(
                        child->children[0]->lexinfo
                        , &(child->children[0]->symbl)));
                    }  
                }
            }
            return attr::FIELD;
        }
        
        // function
        case TOK_FUNCTION: 
        {
            if(node->children.size() != 3
            ||node->children[0]->children.empty())
                break;

            /********* function title *********/

            // function attr
            if(node->children[0]->tokenCode == TOK_ARRAY
                ||node->children[0]->children.size() == 2)
            {
                if(global_symbol_table.find(node
                        ->children[0]
                        ->children[0]->lexinfo)
                        != global_symbol_table.end())
                {
                    setAttr(node->children[0]
                        ->children[1], attr::STRUCT);
                    struct_name = *(node
                        ->children[0]
                        ->children[0]->lexinfo);
                    setAttr(node->children[0]
                    ->children[1], attr::FUNCTION);

                    global_symbol_table.insert(symbol_entry(
                    node->children[0]
                    ->children[1]->lexinfo
                    , &(node->children[0]
                    ->children[1]->symbl)));

                    print_symbol (node->children[0]
                        ->children[1], "\n");
                }
                else
                {
                    // used undeclared type
                }
            }
            else if(node->children[0]->tokenCode == TOK_TYPEID)
            {
                if(global_symbol_table.find(node
                        ->children[0]->lexinfo)
                        != global_symbol_table.end())
                {
                    setAttr(node->children[0]
                        ->children[0], attr::STRUCT);
                    struct_name = *(node
                        ->children[0]->lexinfo);
                    setAttr(node->children[0]->children[0], attr::FUNCTION);

                    global_symbol_table.insert(symbol_entry(
                    node->children[0]->children[0]->lexinfo
                    , &(node->children[0]->children[0]->symbl)));

                    print_symbol (node->children[0]
                        ->children[0], "\n");
                }
                else
                {
                    // used undeclared type
                }
            }
            else
            {
                setAttr(node->children[0]->children[0], attr::FUNCTION);
                //setParams(node->children[0]->children[0]
                //, new vector<symbol*>(local_parameters));

                global_symbol_table.insert(symbol_entry(
                node->children[0]->children[0]->lexinfo
                , &(node->children[0]->children[0]->symbl)));

                // print
                print_symbol(node->children[0]->children[0], "\n");
            }

            /********* function params *********/

            for (auto child : node->children[1]->children)
            {
                if(!child || child->children.empty())
                    continue;

                child->children[0]->lloc.blocknr = next_block;

                if(child->tokenCode == TOK_ARRAY
                    ||child->children.size() == 2)
                {
                    if(global_symbol_table.find(child
                        ->children[0]->lexinfo)
                        != global_symbol_table.end())
                    {
                        setAttr(child->children[1], attr::STRUCT);
                        struct_name = *(child
                            ->children[0]->lexinfo);
                        print_symbol (child->children[1]);
                    }
                    else
                    {
                        // used undeclared type
                    }
                }
                else if(child->tokenCode == TOK_TYPEID)
                {
                    if(global_symbol_table.find(child->lexinfo)
                        != global_symbol_table.end())
                    {
                        setAttr(child->children[0], attr::STRUCT);
                        struct_name = *(child->lexinfo);
                        print_symbol (child->children[0]);
                    }
                    else
                    {
                        // used undeclared type
                    }
                }
                else
                {
                    print_symbol (child->children[0]);
                }
            }

            /********* function block *********/

            for (auto child : node->children[2]->children)
            {
                if(!child || child->children.empty()
                    || child->children[0]->children.empty())
                    continue;

                if(child->tokenCode == TOK_VARDECL)
                {
                    if(child->children[0]->tokenCode == TOK_ARRAY
                    ||child->children[0]->children.size() == 2)
                    {
                        child->children[0]->
                        children[1]->lloc.blocknr = next_block;

                        if(global_symbol_table
                        .find(child->children[0]
                        ->children[0]->lexinfo)
                            != global_symbol_table.end())
                        {
                            setAttr(child->
                                children[0]->children[1]
                                    , attr::STRUCT);
                            struct_name = 
                                *(child->children[0]
                                ->children[0]->lexinfo);
                            print_symbol 
                                (child->children[0]->children[1]);
                        }
                        else
                        {
                            // used undeclared type
                        }
                    }
                    else if(child->children[0]->tokenCode == TOK_TYPEID)
                    {
                        child->children[0]->
                        children[0]->lloc.blocknr = next_block;
                        if(global_symbol_table
                        .find(child->children[0]->lexinfo)
                            != global_symbol_table.end())
                        {
                            setAttr(child->
                                children[0]->children[0]
                                    , attr::STRUCT);
                            struct_name = 
                                *(child->children[0]->lexinfo);
                            print_symbol 
                                (child->children[0]->children[0]);
                        }
                        else
                        {
                            // used undeclared type
                        }
                    }
                    else
                    {
                        print_symbol (child->children[0]->children[0]);
                    }
                }  
            }

            /********* leave function *********/

            local_symbol_table.clear();
            local_parameters.clear();
            next_block++;

            return attr::FUNCTION;
        }

        case TOK_PARAM:
        {
            size_t sqs = 0;
            for(auto child : node->children)
            {
                if(!child || child->children.empty())
                    continue;

                if(child->tokenCode == TOK_ARRAY
                    ||child->children.size() == 2)
                {
                    setAttr(child->children[1], attr::VARIABLE);
                    setAttr(child->children[1], attr::LVAL);
                    setAttr(child->children[1], attr::PARAM, sqs++);

                    local_parameters.push_back(
                    &(child->children[1]->symbl));
                }
                else
                {
                    setAttr(child->children[0], attr::VARIABLE);
                    setAttr(child->children[0], attr::LVAL);
                    setAttr(child->children[0], attr::PARAM, sqs++);

                    local_parameters.push_back(
                    &(child->children[0]->symbl));
                }
            }
            return attr::PARAM;
        } 

        case TOK_BLOCK:
        {
            size_t sqs = 0;
            for(auto child : node->children)
            {
                if(!child || child->children.empty()
                    || child->children[0]->children.empty())
                        continue;

                if(child->tokenCode == TOK_VARDECL)
                {
                    if(child->children[0]->tokenCode == TOK_ARRAY
                    ||child->children.size() == 2)
                    {
                        setAttr(child->
                        children[0]->children[1]
                        , attr::LOCAL, sqs++);
                    }
                    else{
                        setAttr(child->
                        children[0]->children[0]
                        , attr::LOCAL, sqs++);
                    }
                }
            }
            return attr::VOID;
        }

        case TOK_VARDECL:
        {
            if(node->children.empty()
            || node->children[0]->children.empty())
                break;

            if(node->children[0]->tokenCode == TOK_ARRAY
                    ||node->children[0]
                    ->children.size() == 2)
            {
                setAttr(node->children[0]->children[1], attr::VARIABLE);
                setAttr(node->children[0]->children[1], attr::LVAL);

                local_symbol_table.insert(symbol_entry(
                node->children[0]->children[1]->lexinfo
                ,&(node->children[0]->children[1]->symbl)));
            }
            else
            {
                setAttr(node->children[0]->children[0], attr::VARIABLE);
                setAttr(node->children[0]->children[0], attr::LVAL);

                local_symbol_table.insert(symbol_entry(
                node->children[0]->children[0]->lexinfo
                ,&(node->children[0]->children[0]->symbl)));
            }

            return attr::VARIABLE;
        }

        case TOK_TYPEID:
        {
            return attr::STRUCT;
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
