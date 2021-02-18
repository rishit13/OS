#include<stdio.h>
#include<iostream>
#include<cstring>
#include<string.h>
#include<vector>
#include<map>
#include<ctype.h>
#include<stdlib.h>
#include<set>
#include<list>
#include<fstream>
#include<unordered_map>
#include<utility>
#include<iomanip>
using namespace std;

ifstream file;
map<string,int> symbol_table;
set<string>mul_symbols;
class Module
{
    public:
    int base_address = 0;
    int instruction_count = 0;
    int id = 1;
};
class Memory_Map

{   private:
    int count  = 0;
    map<int, int> err_map;
    map<int,int> print_map;
    map<pair<int,int>,string> err_sym;
    public:
    void  do_R(int val, int base,int module_size);
    void  do_I(int val);
    void  do_E(int val, map<int,string> use_list, vector<int>& visited);
    void  do_A(int val);
    void  print_mem_map();
    void get_map(char add, int val,map<int,string>use_list,int base,int module_size,vector<int>& visited);
    bool check_opcode(int opcode);
    bool check_oprand(int oprand, int module_size);
    bool check_sym(string s);
    bool check_mem_size(int oprand);
    bool check_uselist(int n, int oprand);
    bool check_in_def(string sym);
    bool check_I(int val);
};
class Tokenizer
{
    string test,filename;
    char input[1024];
    char *word;
    char *sep = " \t\r\n";
    char *temp;
    bool flag = true;
    
    public:
    int offset, linenumber = 0;
    int prev_length;
    char* getTok();
    int readInt();
    char* readSymbol();
    char readIEAR();
    void parseerror(int eno);
    void check_def(int val);
    void check_use(int val);
};

int Tokenizer::readInt()
{   if(file.eof() && word == NULL)
    {   int n = offset + prev_length+1;
        printf("Parse Error line %d offset %d: NUM_EXPECTED\n", linenumber, n);
        exit(0);
    }
    
    char *s = getTok();
    int value  = stoi(s);
    if(!isdigit(s[0])) 
    {
        parseerror(0);
        exit(0);
    }
        
    else if(isdigit(s[0]))
        {
            return value;
        }
}
char* Tokenizer::readSymbol()
{   if(file.eof() && word == NULL)
    {   int n = offset + prev_length+1;
        printf("Parse Error line %d offset %d: SYM_EXPECTED\n", linenumber, n);
        exit(0);
    }
    
    char *s = getTok();
    int count = 0;
    if(!isalpha(s[0]))
    {
        parseerror(1);
        exit(0);
    }
    for(int i =1;s[i] !='\0'; i++)
    {   
        if(!isalnum(s[i]))
        {   
            parseerror(1);
            exit(0);
        }
    }
    for(int i = 0; s[i] != '\0';i++)
    {
        count +=1;
    }
    if(count > 16)
    {
        parseerror(3);
        exit(0);
    }

    return s;
}
char Tokenizer::readIEAR()
{   char c;
    int count  =0;
    if(file.eof() && word == NULL)
    {   int n = offset + prev_length+1;
        printf("Parse Error line %d offset %d: ADDR_EXPECTED\n", linenumber, n);
        exit(0);
    }
    char *s = getTok();
    for(int i =0;s[i] !='\0';i++)
    {
            count  +=1;
    }
    if (count > 1)
        {
            parseerror(2);
            exit(0);
        }
    if(!isalpha(s[0]))
    {
        parseerror(2);
        exit(0);
    }
    else
    {   for(int i =0; s[i] != '\0'; i++)
        {
        if (s[i] == 'R' || s[i] == 'I' || s[i] == 'A' || s[i] == 'E')
            {
                return s[i];
            }
        }
    }
}
char* Tokenizer::getTok()
{   if(flag == true)
    {
    while (getline(file, test, '\n') && test.empty())
        {
            linenumber++;
        }
        linenumber++;
        strcpy(input, test.c_str());
        flag = false;
        word = strtok(input,sep);
    }
    temp = word;
    prev_length = strlen(temp);
    offset = word - input;
    word = strtok(NULL,sep);
    if(word == NULL)
        flag = true;
    else if(word != NULL)
        flag = false;
    return temp;
}
void Tokenizer::parseerror(int eno)
{
    vector<char*> ErrorList;
    ErrorList.push_back("NUM_EXPECTED");
    ErrorList.push_back("SYM_EXPECTED");
    ErrorList.push_back("ADDR_EXPECTED");
    ErrorList.push_back("SYM_TOO_LONG");
    ErrorList.push_back("TOO_MANY_DEF_IN_MODULE");
    ErrorList.push_back("TOO_MANY_USE_IN_MODULE");
    ErrorList.push_back("TOO_MANY_INSTR");
    printf("Parse Error line %d offset %d: %s\n", linenumber, offset+1, ErrorList[eno]);
}
void Tokenizer::check_def(int val)
{
    if(val > 16)
    {
        parseerror(4);
        exit(0);
    }
}
void Tokenizer::check_use(int val)
{
    if (val > 16)
    {
        parseerror(5);
        exit(0);
    }
}
bool Memory_Map::check_opcode(int opcode)
{
    if(opcode >= 10)
    {
        err_map[count] = 11;
        return false;
    }
    return true;
}
bool Memory_Map::check_oprand(int oprand, int module_size)
{
    if(oprand > module_size)
    {
        err_map[count] = 9;
        return false;
    }
    return true;
}
void Memory_Map:: do_R(int val, int base,int module_size)
{
    int opcode = val/1000;
    if (!check_opcode(opcode))
    {
         val = 9999;
         print_map[count++] = val;
    }
    else
    {
        int oprand = val%1000;
    if(!check_oprand(oprand,module_size))
    {
        oprand = 0;
        val = oprand*1000 + oprand;
    }
    int add = val + base;
    print_map[count++] = add;
    }   
}
bool Memory_Map::check_I(int val)
{
    if(val >=10000)
    {
        err_map[count] = 10;
        return false;
    }
    return true;
}
void Memory_Map:: do_I(int val)
{
    if (!check_I(val))
        val = 9999;
    print_map[count++] = val;

}
bool Memory_Map::check_uselist(int n, int oprand)
{
    if(oprand > n)
    {
        err_map[count] = 6;
        return false;
    }
    return true;
}

bool Memory_Map::check_in_def(string sym)
{
    if(symbol_table.find(sym) == symbol_table.end())
    {
        err_map[count] = 3;
        err_sym[make_pair(count,3)] = sym;
        return false;
    }
    return true;
}
void Memory_Map:: do_E(int val, map<int,string> use_list,vector<int>& visited)
{
    int opcode =val/1000;
    if (!check_opcode(opcode))
    {
         val = 9999;
         visited.push_back(val%1000);
         print_map[count++] = val;
    }
    else
    {   int oprand  = val%1000;
        int n = use_list.size();
        if(!check_uselist(n,oprand))
        {
            print_map[count++] = val;
        }
        else if(!check_in_def(use_list[oprand]))
        {
            val = 0;
            print_map[count++] = val;
            visited.push_back(oprand);
        }
        else
        {
            int off_set_add = symbol_table[use_list[oprand]];
            int add_val = opcode*1000 + off_set_add;
            print_map[count++] = add_val;
            visited.push_back(oprand);
        }
    }
}
bool Memory_Map::check_mem_size(int oprand)
{
    if(oprand >= 512)
    {
        err_map[count] = 8;
        return false;
    }
    return true;
}
void Memory_Map:: do_A(int val)
{
        int opcode = val/1000;
        if (!check_opcode(opcode))
        {
            val = 9999;
            print_map[count++] = val;
        }
        else
        {
            int oprand = val%1000;
            if(!check_mem_size(oprand))
                {
                    print_map[count++] = opcode*1000;
                }
            else
            {
                print_map[count++] = val;
            }
        }
}
void Memory_Map:: print_mem_map()
{   cout<<"\n";
    cout<<"\n";
    cout<<"MEMORY MAP"<<"\n";
    map<int,string> ps;
    ps[11] = "Error: Illegal opcode; treated as 9999\n";
    ps[10] = "Error: Illegal immediate value; treated as 9999\n";
    ps[6] = "Error: External address exceeds length of uselist; treated as immediate\n";
    ps[9] = "Error: Relative address exceeds module size; zero used\n";
    ps[8] = "Error: Absolute address exceeds machine size; zero used\n";
    ps[3] = "Error: %s is not defined; zero used\n";
    
    for(auto x : print_map)
    {
        cout<<setfill('0')<<setw(3)<<x.first<<" "<<":"<<x.second<<" ";
        if(err_map.find(x.first) != err_map.end() && x.first != 3)
        {
            cout<<err_map[x.first];
        }
        else if (err_map.find(x.first) != err_map.end() && x.second == 3)
        {
            string str = err_sym[make_pair(x.first,3)];
            printf("Error: %s is not defined; zero used\n",str.c_str());
        }
        cout<<"\n";
    }

}
void Memory_Map:: get_map(char add, int val,map<int,string>use_list,int base,int module_size,vector<int>& visited)
{   
    if(add == 'R')
        do_R(val,base,module_size);
    if(add == 'I')
        do_I(val);
    if(add == 'E')
        do_E(val,use_list,visited);
    if(add == 'A')
        do_A(val);
}
void storesym(string sym , int value)
{
    if(symbol_table.find(sym) == symbol_table.end())
    {
        symbol_table[sym] = value;
    }
}
void print_sym_table()
{   cout<< "Symbol Table"<<"\n";
    for(auto const& it : symbol_table)
    {
        cout<<it.first <<"="<<it.second<<" ";
        if(mul_symbols.find(it.first) != mul_symbols.end())
            printf("Error: This variable is multiple times defined; first value used \n");
        cout<<"\n";
    }
}
bool check_sym(string s)
{
    if(symbol_table.find(s) != symbol_table.end())
        {
            mul_symbols.insert(s);
            return true;
        }
        
    return false;
}

void check_def_use_pre(int m_id, vector<pair<string,int> >def_values,int module_size)
{
    int length = def_values.size();
    for(int i =0;i<length;i++)
    {
        if (def_values[i].second > module_size)
        {   string sym  = def_values[i].first;
            int old_value = symbol_table[def_values[i].first];
            int new_value = symbol_table[def_values[i].first] - def_values[i].second;
            symbol_table[def_values[i].first] = new_value;
            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", m_id,sym.c_str(),old_value,module_size);
        }
    }
}

void check_isfallin_use(int id, vector<int> visited, map<int, string> use_index,map<string,int>& global)
{
    int n = visited.size();
    for(int i =0;i<n;i++)
    {   
        if(use_index.find(visited[i]) == use_index.end())
        {   
            string s = use_index[visited[i]];
            global[s]++;
            printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n",id,s.c_str());
        }
    }
}

void check_not_used_def( map<string,int> global_visited, map<string,int> def_module)
{  
    for( auto x: symbol_table)
    {
        if(global_visited.find(x.first) != global_visited.end())
        {   string s = x.first;
            printf("Warning: Module %d: %s was defined but never used\n",def_module[x.first],s.c_str() );
        }
    }
}

void pass_1()
{   Tokenizer tokenizer;
    Module module;
    vector<pair<string,int> > def_list_values;
    int module_size;
    while(!file.eof())
    {   
        int defcount = tokenizer.readInt();
        tokenizer.check_def(defcount);
        for(int i=0;i<defcount;i++)
        {
            char* sym = tokenizer.readSymbol();
            string s = sym;
            int value_offset = tokenizer.readInt();
            int address = module.base_address + value_offset;
            if(!check_sym(s))
                {   
                    def_list_values.push_back(make_pair(s,value_offset));
                    storesym(s, address);
                }  
        }
        int usecount = tokenizer.readInt();
        tokenizer.check_use(usecount);
        for(int i=0;i<usecount;i++)
        {
            char* sym = tokenizer.readSymbol();
            //cout<< "uselist "<<sym<<"\n";
        }
        int icount = tokenizer.readInt();
        module.base_address += icount;
        module.instruction_count += icount;
        if(module.instruction_count  >= 512)
        {
            tokenizer.parseerror(6);
            exit(0);
        }  
        module_size = icount;
        for(int i=0;i<icount;i++)
        {
            char add = tokenizer.readIEAR();
            int val = tokenizer.readInt();
            //cout<< "inlist "<<add<<":"<<val<<"\n";
        }
        check_def_use_pre(module.id, def_list_values, module_size);
        if(!file.eof())
            module.id++;
    }
    print_sym_table();
    file.close();
}

void pass_2()
{   Tokenizer tokenizer1;
    Module module;
    Memory_Map mmap;
    map<string,int>global_visited;
    map<string,int> def_mod;
    while(!file.eof())
    {   vector<int> visited;
        map<int,string> use_index;
        int defcount = tokenizer1.readInt();
        tokenizer1.check_def(defcount);
        for(int i=0;i<defcount;i++)
        {
            char* sym = tokenizer1.readSymbol();
            string s = sym;
            int value_offset = tokenizer1.readInt();
            int address = module.base_address + value_offset; 
            def_mod[s] = module.id;
        }
        int usecount = tokenizer1.readInt();
        tokenizer1.check_use(usecount);
        for(int i=0;i<usecount;i++)
        {
            char* sym = tokenizer1.readSymbol();
            string s = sym;
            use_index[i] = sym; //add to the list
            //cout<< "uselist "<<sym<<"\n";
        }
        int icount = tokenizer1.readInt();
        module.instruction_count += icount;
        if(module.instruction_count  >= 512)
        {
            tokenizer1.parseerror(6);
            exit(0);
        }  
        for(int i=0;i<icount;i++)
        {
            char add = tokenizer1.readIEAR();
            int val = tokenizer1.readInt();
            mmap.get_map(add, val,use_index,module.base_address,icount,visited);
            //cout<< "inlist "<<add<<":"<<val<<"\n";
        }
        module.base_address += icount;
        check_isfallin_use(module.id,visited,use_index,global_visited);
        if(!file.eof())
            module.id++;
    }

    mmap.print_mem_map();
    check_not_used_def(global_visited,def_mod);
    file.close();
}
int main()
{   
    file.open("input12.txt");
    pass_1();
    file.open("input12.txt");
    pass_2();
    return 0;
}