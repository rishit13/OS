#include<stdio.h>
#include<vector>
#include<iostream>
#include<string>
#include<cstring>
#include<fstream>
#include<sstream>
#include<deque>
#include<queue>
#include<map>
#include<stdlib.h>
#include<iomanip>

using namespace std;

ifstream file;
const int max_pages = 64;
const int max_frames = 4;
int instruction_count;
bool write_protected_flag = false;
bool o_print = true;
bool Aselect = true;
bool PT = true;
bool S = true;
bool F = true;
int exits = 0;
int ctx_switches = 0;
unsigned long long cost = 0;
int cost_context_switch = 130;
int cost_instruction_count = 1;
int process_exit_instruction = 1250;
int read_write_cost = 1;
int cost_maps = 300;
int unmaps = 400;
int ins = 3100;
int outs = 2700;
int fins = 2800;
int fouts = 2400;
int zeros = 140;
int segv = 340;
int segport = 420;
vector<pair<char,int> > instructions;
struct  frame_table_entry
{   int frame_id; 
    int proc_id = -1;
    int vpage;
    bool filled = false;
};

frame_table_entry f_table[max_frames]; // declaring the frame table
deque<int> free_pool;
class vmas
{
    public:
    int start_page;
    int end_page;
    int write;
    int filemapped;
    
};

struct PTE
{
     unsigned int present:1;
     unsigned int referenced:1;
     unsigned int modified:1;
     unsigned int write_protect:1;
     unsigned int pageout:1;
     unsigned int physical_number:7;
     unsigned int file_mapped:1;
     unsigned int in_vma:1;
     unsigned int new_entry:1;
};
PTE page;
class states
{
    public:
    unsigned long unmaps = 0;
    unsigned long maps = 0;
    unsigned long ins = 0;
    unsigned long outs = 0;
    unsigned long fins = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segv = 0;
    unsigned long segport = 0;
};

class process
{
    public:
    int proc_id;
    vector<vmas> list_vams;
    PTE page_table[max_pages];
    states st;
};
vector<process> processes;

void get_processes()
{ bool full_loop = false;
 string str;
    string line_string;
    int process_count;

    while(getline(file,line_string))
    {   cout<<line_string[0];
        if( line_string[0] == '#')
        {
            //cout<<line_string;
            continue;
        }
            stringstream line(line_string);
            string lm;
            line >> str;

            process_count = stoi(str);
            for(int i = 0;i<process_count;i++)
            {   
                process *pr = new process;
                bool process_flag = false;
                while(getline(file,line_string) && !file.eof() )
                {   
                    if (line_string[0] == '#')
                        continue;
                    stringstream line(line_string);
                    line >> str;
                    int VMAs = stoi(str);
                    for(int i =0 ; i< VMAs ;i++)
                    {
                        getline(file,line_string);
                        stringstream line(line_string);
                        string v1,v2,v3,v4;
                        line >> v1 >> v2 >> v3 >> v4;
                        vmas *vm = new vmas;
                        vm->start_page = stoi(v1);
                        vm->end_page =  stoi(v2);
                        vm->write =  stoi(v3);
                        vm->filemapped =  stoi(v4);
                        pr->list_vams.push_back(*vm);
                        delete vm;
                        process_flag = true;
                    }
                    
                    if(process_flag)
                        break;
                }
                processes.push_back(*pr);
                    delete pr;
            }

            while(getline(file,line_string) && !file.eof())
            {   
                bool instruction_flag = false;
                if(line_string[0] == '#')
                    continue;
                else{
                    string operation;
                            string vpage;
                            stringstream line(line_string);
                            line >> operation >> vpage;
                            instructions.push_back(make_pair(operation[0],stoi(vpage)));
                    while(getline(file,line_string) && !file.eof())
                    {
                        if (line_string[0] == '#')
                            break;
                        else{
                            instruction_flag = true;
                            stringstream line(line_string);
                            line >> operation >> vpage;
                            instructions.push_back(make_pair(operation[0],stoi(vpage)));
                        }
                    }
                }
                if(instruction_flag)
                {   full_loop = true;
                     break;
                }  
            }
        
        if(full_loop)
            break;
    }
}

process * curr_running_process = NULL;

class Pager
{
    public:
    int hand = -1;
    virtual frame_table_entry *select_victim_frame() = 0;
};

class FIFO : public Pager
{   
    public:
    frame_table_entry *select_victim_frame()
    {
        hand = (hand +1)%max_frames;
        frame_table_entry *frame = &f_table[hand];
        if(Aselect)
            cout<<"ASELECT "<<hand<<"\n";
        return frame;
    }
};

class CLOCK : public Pager
{
    public:
    frame_table_entry *select_victim_frame()
    {   int traversed = 0;
        bool page_present = false;
        frame_table_entry *frame;
        int start = (hand + 1)%max_frames;
        while(!page_present)
        {   traversed += 1;
            hand = (hand +1)%max_frames;
            frame = &f_table[hand];
            if(processes[frame->proc_id].page_table[frame->vpage].referenced == 0)
                page_present = true;
            else if(processes[frame->proc_id].page_table[frame->vpage].referenced == 1)
            {   
                processes[frame->proc_id].page_table[frame->vpage].referenced = 0;
            }
                
        }
        if(Aselect)
            cout<<"ASELECT "<<start<<" "<<traversed<<"\n";
        return frame;
    }
};

class NRU : public Pager
{
    public:
    frame_table_entry *select_victim_frame()
    {
        int class_1 = -1;
        int class_2 = -1;
        int class_3 = -1;
        int class_4 = -1;
        int traversed = 0;
        frame_table_entry *frame;
        int count_frames = max_frames;
        int value = -1;
        int start = (hand+1)%max_frames;
        int print_class = -1;
        int get_instr = instruction_count %50;
        if(get_instr == 0)
        {
            for( int i =0;i <max_frames;i++)
            {
                frame = &f_table[i];
                processes[frame->proc_id].page_table[frame->vpage].referenced = 0;
            }
        }
        while(count_frames >0)
        {   traversed+=1;
            hand = (hand+1)%max_frames;
            frame = &f_table[hand];
            bool reference = processes[frame->proc_id].page_table[frame->vpage].referenced;
            bool modified = processes[frame->proc_id].page_table[frame->vpage].modified;
            cout<<frame->vpage<<" "<<reference<<" "<<modified<<"\n";
            if(!reference && !modified && class_1 == -1)
            {
                    
                    print_class = 0;
                    class_1 = hand;
                    break;
            }
            if(!reference && modified && class_2 ==-1)
            {
                
                
                class_2 = hand;
            }
            if(reference && !modified  && class_3 == -1)
            {
               
                
                class_3 = hand;
            }
            if(reference && modified && class_4 == -1)
            {
                
               
                class_4 = hand;
            }
            count_frames -= 1;
        }
        cout<<class_1<<" "<<class_2<<" "<<class_3<<" "<<class_4<<"\n";
        if(class_1 !=  -1)
            {
                    
                    print_class = 0;
                    hand = class_1;
                    class_1 = -1;

            }
        else if(class_2 != -1)
        {
            
                    
                    hand = class_2;
                    print_class = 1;
                    class_2 = -1;
        }
        else if(class_3 != -1)
        {
            
                    
                    hand = class_3;
                    print_class = 2;
                    class_3 = -1;
        }
        else if(class_4 != -1)
        {
            
                    
                    hand = class_4;
                    print_class = 3;
                    class_4 = -1;
        }

        if(Aselect)
            cout<<"ASELECT: "<<start<<" "<<0<<" "<<"| "<<print_class<<" "<<hand<<" "<<traversed<<"\n";
        frame = &f_table[hand];
        return frame;
    }
};


Pager *pgr;
frame_table_entry *allocate_frame_from_free_list()
{
    if(free_pool.empty())
        {   
            return NULL;
        }
        
    else{
        int frame_id  = free_pool.front();
        free_pool.pop_front();
        frame_table_entry *frame = &f_table[frame_id];
        return frame;

    }
}

frame_table_entry* get_frame()
{
    frame_table_entry* frame = allocate_frame_from_free_list();
    if (frame == NULL)
    {
        frame = pgr->select_victim_frame();
        PTE  *removed_page = &processes[frame->proc_id].page_table[frame->vpage];
        removed_page->physical_number = 0;
        removed_page->referenced = 0;
        removed_page->present = 0;
            processes[frame->proc_id].st.unmaps +=1;
            cost += unmaps;
            if(o_print)
                cout<<" UNMAP "<<frame->proc_id<<":"<<frame->vpage<<"\n";
            
            if(removed_page->modified == 1)
        {
            removed_page->pageout  = 1;
        
            if(removed_page->file_mapped == 1)
            {   removed_page->modified = 0;
                processes[frame->proc_id].st.fouts +=1;
                cost += fouts;
                if(o_print)
                    cout<<" FOUT"<<"\n";
            }
            else 
            {   removed_page->modified = 0;
                processes[frame->proc_id].st.outs +=1;
                cost += outs;
                if(o_print)
                    cout<<" OUT"<<"\n";
            }
        }
    }  
    return frame;
}

void pagefault_handler(PTE *pte, int vpage, process &present_process, char let)
{
    if(pte->new_entry == 0)
        {
            for (int i = 0;i< present_process.list_vams.size();i++)
            {
                if( vpage >= present_process.list_vams[i].start_page && vpage <= present_process.list_vams[i].end_page)
                {
                    pte->in_vma = 1;
                    pte->file_mapped = present_process.list_vams[i].filemapped;
                    pte->write_protect = present_process.list_vams[i].write;
                    break;
                }
            }
        }
    
    if(pte->in_vma == 1)
    {   
        frame_table_entry* frame  = get_frame();
        present_process.page_table[vpage].physical_number = frame->frame_id;
        frame->vpage = vpage;
        frame->filled = true;
        frame->proc_id = curr_running_process->proc_id; 
        if (pte->file_mapped == 1)
        {
            present_process.st.fins +=1;
            cost+= fins;
            if(o_print)
                cout<<" FIN"<<"\n";
        } 
        else if (pte->pageout == 1)
        {
            present_process.st.ins +=1;
            cost += ins;
            if(o_print)
                cout<<" IN"<<"\n";
        }
        else if( pte->pageout == 0)
        {
            present_process.st.zeros +=1;
            cost += zeros;
            if(o_print)
                cout<<" ZERO"<<"\n"; 
        }
        cout<<" MAP "<<frame->frame_id<<"\n";
        cost += cost_maps;
        present_process.st.maps +=1;
        pte->referenced = 1;
    }

}

pair<char,int> get_next_instruction()
{
    if(instruction_count > instructions.size()-1)
    {
        pair<char,int> value = make_pair('\0',NULL);
        return value;
    } 
    else
    {   
        int me = instruction_count;
        instruction_count++;
        cout<<me<<": ==> ";
        return instructions[me];
    }
       
}

void free_frames(int procid)
{
    for(int i =0; i<max_pages;i++)
    {   processes[procid].page_table[i].pageout  = 0;
        if(processes[procid].page_table[i].present == 1)
        {   processes[procid].page_table[i].present = 0;
            free_pool.push_back(processes[procid].page_table[i].physical_number);
            f_table[processes[procid].page_table[i].physical_number].vpage = -1;
            f_table[processes[procid].page_table[i].physical_number].proc_id = -1;
            processes[procid].page_table[i].pageout  = 0;
            processes[procid].st.unmaps +=1;
            cost += unmaps;
            if(o_print)
                cout<<" UNMAP "<<procid<<": "<<i;
            if(processes[procid].page_table[i].modified == 1)
            {
                processes[procid].page_table[i].pageout  = 0;
        
            if(processes[procid].page_table[i].file_mapped == 1)
            {   processes[procid].page_table[i].modified = 0;
                cost += fouts;
                processes[procid].st.fouts +=1;
                if(o_print)
                    cout<<" FOUT"<<"\n";
            }
            else 
            {   processes[procid].page_table[i].modified = 0;
                processes[procid].st.outs +=1;
                cost += outs;
                if(o_print)
                    cout<<"OUT"<<"\n";
            }
        
            }
        }
    }
}

void simulation()
{   pair<char,int> inst;
    while(true)
    {   inst = get_next_instruction();
        char operation = inst.first;
        int vpage = inst.second;

        if (operation == '\0')
            break;
        else{
            cout<<operation<<" "<<vpage<<"\n";
        if( operation == 'c')
        {   cost += cost_context_switch;
            ctx_switches +=1;
            curr_running_process = &processes[vpage];
            // remove the process and see where t add the current process   
            continue;
        }
        if (operation == 'e')
        {   exits +=1;
            cost += process_exit_instruction;
            free_frames(vpage);
            curr_running_process = NULL;
            continue;
        }
        if (operation == 'r' || operation == 'w')
        {   cost += read_write_cost;
            PTE *pte = &curr_running_process->page_table[vpage];
            if(pte->present == 0)
                pagefault_handler(pte,vpage,*curr_running_process, operation);
            if(operation == 'w')
            {   
                if(pte->in_vma == 0)
                    {   
                        if(o_print)
                    {
                        cout<<" SEGV"<<"\n";
                    }
                     curr_running_process->st.segv +=1;      
                     cost += segv; 
                    }
                else{
                        pte->present = 1;
                        pte->referenced = 1;
                        if(pte->write_protect == 1)
                        {   
                            
                            if(o_print){
                                
                                cout<<" SEGPORT"<<"\n";
                                
                            }
                            curr_running_process->st.segport +=1;    
                            cost += segport;
                        }
                            
                        else
                        {
                        pte->modified = 1;                
                        }
                    }
            }
            if(operation == 'r')
            {   
                if(pte->in_vma == 0)
                    {   
                        if(o_print)
                    {
                        
                        cout<<"SEGV"<<"\n";
                    }
                        curr_running_process->st.segv +=1;    
                        cost += segv;
                    }
                    
                else{
                pte->present = 1;
                pte->referenced = 1;
                }

            }
          }
    }
}
}

void print_summary()
{
    if(PT)
    {
        for(int i = 0;i<processes.size();i++)
        {   printf("PROC[%d]: ",i);
            for(int j = 0; j<max_pages;j++)
            {   
                if(processes[i].page_table[j].present == 1)
                {   cout<<j<<":";
                    if(processes[i].page_table[j].referenced == 1)
                        cout<<"R";
                    else
                        cout<<"-";
                    if(processes[i].page_table[j].modified == 1)
                        cout<<"M";
                    else
                        cout<<"-";
                    if(processes[i].page_table[j].pageout == 1)
                        cout<<"S";
                    else
                        cout<<"-";
                    cout<<" ";
                }
                else if(processes[i].page_table[j].present == 0 && processes[i].page_table[j].referenced == 1) 
                    cout <<"* ";
                else if(processes[i].page_table[j].present == 0 && processes[i].page_table[j].pageout == 1) 
                    cout <<"# ";
                else if(processes[i].page_table[j].present == 0 && processes[i].page_table[j].pageout == 0)
                    cout<<"* ";
            }
            cout<<"\n";
        }
    }
    if(F)
    {   cout << "FT: ";
            for(int i = 0;i <max_frames; i++)
            {
                if(f_table[i].proc_id == -1)
                {
                    cout<<"*";
                }
                else
                {
                    cout<<f_table[i].proc_id<<":"<<f_table[i].vpage<<" ";
                }
            }
            cout<<"\n";
    }
    if(S){
    for ( int i =0; i<processes.size();i++)
    {   
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                     processes[i].proc_id,
                     processes[i].st.unmaps,  processes[i].st.maps, processes[i].st.ins, processes[i].st.outs,
                     processes[i].st.fins, processes[i].st.fouts, processes[i].st.zeros,
                     processes[i].st.segv, processes[i].st.segport);
    }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
              instruction_count, ctx_switches, exits, cost, sizeof(page));
    }
}

int main()
{
    file.open("in1");
    get_processes();

    for(int i = 0;i<processes.size();i++)
    {
        processes[i].proc_id = i;
    }
    for(int i = 0;i<max_frames; i++)
    {
        f_table[i].frame_id = i;
    }
    for(int i = 0;i<max_frames;i++)
    {
        free_pool.push_back(i);
    }

    for(int i =0; i<processes.size();i++)
    {   
        for(int j = 0;j<processes[i].list_vams.size();j++)
        {   
            cout<<processes[i].list_vams[j].filemapped<<" " <<processes[i].list_vams[j].start_page<<"\n";
        }
        
        //cout<<<processes[3].list_vams.size();
    }
    //cout<<processes[3].list_vams.size();
    /*for(int i = 0;i<instructions.size(); i++)
    {
        cout<<instructions[i].first << " "<<instructions[i].second<<"\n";
    }*/
    NRU dd;
    pgr = &dd;
    simulation();
    print_summary();
    return 0;
}
