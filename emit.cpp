#include "lyutils.h"
#include "astree.h"

#include "emit.h"

extern FILE* oilfile;

vector<string> stringcon_queue;
unordered_set<string> global_map;
unordered_map<string, string> paramMap;
unordered_map<string, string> localMap;
unordered_map<string, string> typeMap;

static size_t stringcon_queue_index = 1;
static size_t branch_counter = 1;

void printOilFile(const string& str){
    fprintf(oilfile,"%s",str.c_str());
}

/***************** six types of decl ******************/

void emit_decl(astree* node, string& type, string& ident){
    if(!node) return;

    if(node->tokenCode == TOK_ARRAY){
        if(node->children.size() != 2) return;

        ident = *(node->children[1]->lexinfo);
        if(node->children[0]->tokenCode == TOK_STRING){
            type = "char**";
        }
        else if(node->children[0]->tokenCode == TOK_TYPEID){
            type = "struct " + *(node->children[0]->lexinfo) + "**";
            typeMap[ident] = *(node->children[0]->lexinfo);
        }
        else{
            type = *(node->children[0]->lexinfo) + "*";
        }  
    }
    else{
        if(node->children.size() != 1) return;

        ident = *(node->children[0]->lexinfo);
        if(node->tokenCode == TOK_STRING){
            type = "char*";
        }
        else if(node->tokenCode == TOK_TYPEID){
            type = "struct " + *(node->lexinfo) + "*";
            typeMap[ident] = *(node->lexinfo);
        }
        else{
            type = *(node->lexinfo);
        } 
    }
}

/***************** struct and fields ******************/

void emit_field(astree* node, const string& structName){
    if(!node) return;

    string type, ident;
    emit_decl(node, type, ident);
    printOilFile("        " 
    + type + " " + structName + "_" + ident + ";\n");
}

void emit_struct(astree* node){
    if(!node || node->children.size() != 2) return;

    string structName = *(node->children[0]->lexinfo);

    printOilFile("struct ");
    printOilFile(structName);
    printOilFile(" {\n");

    for (auto child : node->children[1]->children){
        emit_field(child, structName);
    }

    printOilFile("};\n\n");
}

void emit_find_struct(astree* node){
    if(!node) return;

    for(auto child : node->children){
        if(child->tokenCode == TOK_STRUCT){
            emit_struct(child);
        }
    }
}

/******************* string const *********************/

void emit_stringcon(){
    size_t i = 1;
    for(auto s : stringcon_queue){
        printOilFile("char* s" + to_string(i++) + " = " 
            + s + ";\n");
    }
    printOilFile("\n");
}

/*************** expression ****************/

bool emit_expr(astree* node, string& toPrint);

unordered_map<int, string> tokenToString{
    {TOK_EQ, "=="},
    {TOK_NE, "!="},
    {TOK_LT, "<"},
    {TOK_LE, "<="},
    {TOK_GT, ">"},
    {TOK_GE, ">="},
    {'+', "+"},
    {'-', "-"},
    {'*', "*"},
    {'/', "/"},
    {'=', "="},

    {TOK_POS, "+"},
    {TOK_NEG, "-"},
    {TOK_NOT, "!"},
};

bool emit_binop(astree* node, string& toPrint){
    if(!node) return false;

    if(node->tokenCode == TOK_EQ
    ||node->tokenCode == TOK_NE
    ||node->tokenCode == TOK_LT
    ||node->tokenCode == TOK_LE
    ||node->tokenCode == TOK_GT
    ||node->tokenCode == TOK_GE
    ||node->tokenCode == '+'
    ||node->tokenCode == '-'
    ||node->tokenCode == '*'
    ||node->tokenCode == '/'
    ||node->tokenCode == '='
    ){
        if(node->children.size() != 2) return false;

        string toPrint_1, toPrint_2;

        bool needParenthes_1 = 
            emit_expr(node->children[0], toPrint_1);
        bool needParenthes_2 = 
            emit_expr(node->children[1], toPrint_2);

        if(needParenthes_1)
            toPrint += "( " + toPrint_1 + ") ";
        else
            toPrint += toPrint_1;

        toPrint += tokenToString[node->tokenCode] + " ";

        if(needParenthes_2)
            toPrint += "( " + toPrint_2 + ") ";
        else
            toPrint += toPrint_2;

        return true;
    }
    return false;
}

void emit_unop(astree* node, string& toPrint){
    if(!node) return;

    if(node->tokenCode == TOK_POS
    ||node->tokenCode == TOK_NEG
    ||node->tokenCode == TOK_NOT
    ){
        if(node->children.size() != 1) return;

        string toPrint_1;

        bool needParenthes = emit_expr(node->children[0], toPrint_1);

        toPrint += tokenToString[node->tokenCode] + " ";

        if(needParenthes)
            toPrint += "( " + toPrint_1 + ") ";
        else
            toPrint += toPrint_1;
    }
}

bool emit_alloc(astree* node, string& toPrint){
    if(!node) return false;

    if(node->tokenCode == TOK_NEW){
        if(node->children.size() != 1) return false;

        toPrint += "xcalloc (1, sizeof (struct " 
            + *(node->children[0]->lexinfo) + ")) ";
        return true;
    }
    else if(node->tokenCode == TOK_NEWSTR){
        if(node->children.size() != 1) return false;

        toPrint += "xcalloc (" 
            + *(node->children[0]->lexinfo) 
            + ", sizeof (char)) ";
        return true;
    }
    else if(node->tokenCode == TOK_NEWARRAY){
        if(node->children.size() != 2) return false;

        string basetype = *(node->children[0]->lexinfo);

        if(basetype == "string"){
            toPrint += "xcalloc (" 
            + *(node->children[1]->lexinfo) 
            + ", sizeof (char)) ";
        }
        else if(basetype == "int"){
            toPrint += "xcalloc (" 
            + *(node->children[1]->lexinfo) 
            + ", sizeof (int)) ";
        }
        else{
            toPrint += "xcalloc (" 
            + *(node->children[1]->lexinfo) 
            + ", sizeof (struct " + basetype + "*)) ";
        }
        return true;
    }
    else if(node->tokenCode == TOK_NEWARRAY2){
        if(node->children.size() != 2) return false;

        string basetype = *(node->children[0]->lexinfo);

        if(basetype == "string"){
            toPrint += "xcalloc (" 
            + *(node->children[1]->lexinfo) 
            + ", sizeof (char*)) ";
        }
        
        return true;
    }
    return false;
}

void emit_call(astree* node, string& toPrint){
    if(!node) return;

    if(node->tokenCode == TOK_CALL){
        bool first = true;
        bool need_erase = false;
        for(auto child : node->children){
            if(first){
                string funcName = *(child->lexinfo);
                toPrint += "__" + funcName + " (";
                first = false;
            }
            else{
                string expr;
                emit_expr(child, expr);
                toPrint += expr + ", ";
                need_erase = true;
            }
        }
        if(need_erase)
            toPrint = toPrint.substr(0, toPrint.size() - 2);
        toPrint += ") ";  
    }
}

void substituteVariable(string& va){
    if(localMap.find(va) 
        != localMap.end()){
        va = localMap[va];
    }
    else if(paramMap.find(va) 
        != paramMap.end()){
        va = paramMap[va];
    }
    else if(global_map.find(va) 
        != global_map.end()){
        // do nothing?
    }
}

void emit_variable(astree* node, string& toPrint){
    if(!node) return;

    if(node->tokenCode == TOK_IDENT){
        string name = *(node->lexinfo);
        substituteVariable(name);
        toPrint += name + " ";
    }
    else if(node->tokenCode == '['){
        if(node->children.size() != 2) return;

        string name = *(node->children[0]->lexinfo);
        string field = *(node->children[1]->lexinfo);

        substituteVariable(name);
        substituteVariable(field);
        
        toPrint += name + "[" + field + "] ";
    }
    else if(node->tokenCode == '.'){
        if(node->children.size() != 2) return;

        string name = *(node->children[0]->lexinfo);
        string field = *(node->children[1]->lexinfo);

        field = typeMap[name] + "_" + field;

        substituteVariable(name);
 
        toPrint += name + "->" + field + " ";
    }
}

void emit_constant(astree* node, string& toPrint){
    if(!node) return;

    if(node->tokenCode == TOK_INTCON
    ||node->tokenCode == TOK_CHARCON
    ){
        toPrint += *(node->lexinfo) + " ";
    }
    else if(node->tokenCode == TOK_STRINGCON){
        toPrint += "s" + to_string(stringcon_queue_index++) + " ";
    }
    else if(node->tokenCode == TOK_NULL){
        toPrint += "0 ";
    }
}

bool emit_expr(astree* node, string& toPrint){
    if(!node) return false;

    bool needParenthes = false;

    needParenthes |= emit_binop(node, toPrint);
    emit_unop(node, toPrint);
    needParenthes |= emit_alloc(node, toPrint);
    emit_call(node, toPrint);
    emit_variable(node, toPrint);
    emit_constant(node, toPrint);

    return needParenthes;
}

/*************** statements ****************/

void emit_statement(astree* node);

/****************** while ifelse ********************/

void emit_while(astree* node){
    if(!node || node->children.size() != 2) return;

    string loc = "_" + to_string(node->lloc.filenr) 
    + "_" + to_string(node->lloc.linenr) 
    + "_" + to_string(node->lloc.offset);

    string branchName = "b" + to_string(branch_counter++);

    printOilFile("while" + loc + ":;\n");

    string expr;
    emit_expr(node->children[0], expr);
    printOilFile("        char " + branchName + " = " + expr 
        + ";\n        if (!" + branchName 
        + ") goto break" + loc + ";\n");

    emit_statement(node->children[1]);

    printOilFile("        goto while" 
        + loc + ";\nbreak" + loc + ":;\n");
}

void emit_ifelse(astree* node){
    if(!node) return;
    if(node->children.size() <= 0) return;

    string loc = "_" + to_string(node->lloc.filenr) 
    + "_" + to_string(node->lloc.linenr) 
    + "_" + to_string(node->lloc.offset);
    string branchName = "b" + to_string(branch_counter++);
    string expr;

    emit_expr(node->children[0], expr);
    printOilFile("        char " + branchName 
        + " = " + expr + ";\n");

    if(node->children.size() == 2){//no else
        printOilFile("        if (!" 
        + branchName + " ) goto fi" + loc + ";\n");
        emit_statement(node->children[1]);
        printOilFile("fi" + loc + ":;\n");
    }
    else if(node->children.size() == 3){//with else
        printOilFile("        if (!" 
        + branchName + " ) goto else" + loc + ";\n");
        emit_statement(node->children[1]);
        printOilFile("        goto fi" + loc + ":;\n");
        printOilFile("else" + loc + ":;\n");
        emit_statement(node->children[2]);
        printOilFile("fi" + loc + ":;\n");
    }
}

void emit_return(astree* node){
    if(!node) return;

    if(node->children.empty()){
        printOilFile("        return ;\n");
    }
    else{
        string expr;
        emit_expr(node->children[0], expr);
        printOilFile("        return " + expr + ";\n");
    }
}

void emit_statement(astree* node){
    if(!node) return;

    if(node->tokenCode == TOK_BLOCK){
        if(node->children.empty()) return;
        for(auto child : node->children){
            emit_statement(child);
        }
    }
    else if(node->tokenCode == TOK_WHILE){
        emit_while(node);
    }
    else if(node->tokenCode == TOK_IF){
        emit_ifelse(node);
    }
    else if(node->tokenCode == TOK_RETURN){
        emit_return(node);
    }
    else{//expressions
        string expr;
        emit_expr(node, expr);
        printOilFile("        " + expr + ";\n");
    }
}

/*************** func, param and block ****************/

string get_blocknr(astree* node){
    if(!node) return "";

    if(node->tokenCode == TOK_ARRAY
    || node->children.size() == 2)
        return to_string(node->children[1]->lloc.blocknr);
    else if(node->children.size() == 1)
        return to_string(node->children[0]->lloc.blocknr);
    return "";
}

void emit_local_decl(astree* node){
    if(!node || node->children.size() != 2) return;

    string type, ident, expr;
    string newName;
    emit_decl(node->children[0], type, ident);
    emit_expr(node->children[1], expr);

    if(type == "int") newName += 'i';
    if(type.back() == '*') newName += 'p';

    newName += ident;

    printOilFile("        " + type + " " 
    + newName + " = " + expr + ";\n");

    localMap[ident] = newName;
}

void emit_function_block(astree* node){
    if(!node) return;

    printOilFile("{\n");
    for(auto child : node->children){

        if(child->tokenCode == TOK_VARDECL)
            emit_local_decl(child);
        else
            emit_statement(child);
    }
    printOilFile("}\n\n");
}

void emit_function_params(astree* node){
    if(!node) return;

    printOilFile("(");
    if(node->children.empty()){
        printOilFile(" void ");
    }
    else{
        bool first = true;
        for(auto child : node->children){
            if(first)
                first = false;
            else
                printOilFile(",");
            string type, ident;
            emit_decl(child, type, ident);
            string res = "_" + get_blocknr(child) 
                + "_" + ident;
            paramMap[ident] = res;
            printOilFile("\n        " + type + " " + res);
        }
    }
    printOilFile(")\n");
}

void emit_function_name(astree* node){
    if(!node) return;

    string type, ident;
    emit_decl(node, type, ident);
    printOilFile(type + " " + ident);
}

void emit_function(astree* node){
    if(!node || node->children.size() != 3) return;

    paramMap.clear();
    localMap.clear();

    emit_function_name(node->children[0]);
    emit_function_params(node->children[1]);
    emit_function_block(node->children[2]);
}

void emit_find_function(astree* node){
    if(!node) return;

    for(auto child : node->children){
        if(child->tokenCode == TOK_FUNCTION){
            emit_function(child);
        }
    }
}

void emit_global(astree* node){
    if(!node) return;

    for(auto child : node->children){
        if(child->tokenCode == TOK_VARDECL
        ||child->children.size() > 0){
            string type, ident;
            emit_decl(child->children[0], type, ident);
            printOilFile(type + " " + ident + ";\n");
            global_map.insert(ident);
        }
    }
    printOilFile("\n");
}

void emit_il(astree* root){
    if(!root) return;

    printOilFile("#include \"oclib.h\"\n\n");
    emit_find_struct(root);
    emit_stringcon();
    emit_global(root);
    emit_find_function(root);

    global_map.clear();
    paramMap.clear();
    localMap.clear();
    typeMap.clear();
}

