#include<stdio.h>
#include<iostream>
#include<cstring>
#include<string.h>
#include<vector>
#include<map>
#include<ctype.h>
#include<stdlib.h>
#include<regex>
#include<set>
#include<list>
#include<fstream>
#include<unordered_map>
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
    char* getTok();
    int readInt();
    char* readSymbol();
    char readIEAR();
    void parseerror(int eno);
    void check_def(int val);
    void check_use(int val);
};

int Tokenizer::readInt()
{   if(file.eof() && temp == NULL)
    {
        printf("Parse Error line %d offset %d: NUM_EXPECTED\n", linenumber, offset + sizeof(temp)+1);
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
{   if(file.eof() && temp == NULL)
    {
        printf("Parse Error line %d offset %d: SYM_EXPECTED\n", linenumber, offset + sizeof(temp)+1);
        exit(0);
    }
    
    char *s = getTok();
    int count = 0;
    if(!isalpha(s[0]))
    {   cout<<s;
        parseerror(1);
        exit(0);
    }
    for(int i =1;s[i] !='\0'; i++)
    {   
        if(!isalnum(s[i]))
        {   cout<<"a";
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
    if(file.eof() && temp == NULL)
    {
        printf("Parse Error line %d offset %d: ADDR_EXPECTED\n", linenumber, offset + sizeof(temp)+1);
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
    if (getline(file, test, '\n'))
        {
            linenumber++;
            strcpy(input, test.c_str());
            flag = false;
            word = strtok(input,sep);
        }
    }
     
     temp = word;
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
void storesym(string sym , int value)
{
    if(symbol_table.find(sym) == symbol_table.end())
    {
        symbol_table[sym] = value;
    }
}

void print_sym_table()
{
    for(auto const& it : symbol_table)
    {
        cout<<it.first <<"="<<it.second<<"";
        if(mul_symbols.find(it.first) != mul_symbols.end())
            printf("Error: This variable is multiple times defined; first value used \n");
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
void pass_1()
{   Tokenizer tokenizer;
    Module module;
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
                storesym(s, address);
            //cout<< "deflist "<<sym<<":"<<address<<"\n";
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
        for(int i=0;i<icount;i++)
        {
            char add = tokenizer.readIEAR();
            int val = tokenizer.readInt();
            //cout<< "inlist "<<add<<":"<<val<<"\n";
        }
        if(!file.eof())
            module.id++;
    }
    cout<<module.base_address<<" "<<":"<<module.instruction_count<<" "<<":"<<module.id<<"\n";
    print_sym_table();
    file.close();
}

void pass_2()
{   Tokenizer tokenizer;
    Module module;
    while(!file.eof())
    {   unordered_map<int,string> use_index;
        int defcount = tokenizer.readInt();
        tokenizer.check_def(defcount);
        for(int i=0;i<defcount;i++)
        {
            char* sym = tokenizer.readSymbol();
            string s = sym;
            int value_offset = tokenizer.readInt();
            int address = module.base_address + value_offset;
            
        }
        int usecount = tokenizer.readInt();
        tokenizer.check_use(usecount);
        for(int i=0;i<usecount;i++)
        {
            char* sym = tokenizer.readSymbol();
            string s = sym;
            use_index[i] = sym; //add to the list
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
        for(int i=0;i<icount;i++)
        {
            char add = tokenizer.readIEAR();
            int val = tokenizer.readInt();
            //cout<< "inlist "<<add<<":"<<val<<"\n";
        }
        if(!file.eof())
            module.id++;
    }
    cout<<module.base_address<<" "<<":"<<module.instruction_count<<" "<<":"<<module.id<<"\n";
    print_sym_table();
    file.close();
}


int main()
{   
    file.open("test-3.txt");
    pass_1();
    return 0;
}