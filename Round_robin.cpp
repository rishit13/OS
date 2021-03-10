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

using namespace std;
int maxprio = 4; 
typedef enum {STATE_CREATED,STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_PREMPT,STATE_NULL} states;
char *state_types[] = {"STATE_CREATED","STATE_READY","STATE_RUNNING", "STATE_BLOCKED", "STATE_PREMPT"};
bool call_schedular;
bool initial_events = true;
ifstream random_file;
vector<int> random_array;
int offset = 0;
bool verbose = true;

void get_random_nums()
{   string str;
    random_file.open("rfile");
    getline(random_file, str);
    int size_file = stoi(str);
    while(getline(random_file,str)){
       int ran_num = stoi(str);
       random_array.push_back(ran_num);
    }
    random_file.close();
    //cout<<random_array[size_file - 10];
}

int random_value(int burst)
{
    return  1 + (random_array[offset++] % burst);
}

class process
{
public:
int p_id; // process_id
int arrival_time; // int arrival_time
int timestamp; //current_timestamp
int t_cpu; // totalcput_time
int b_cpu_lim; // cpu_burst_limit
int b_io_lim; // ioburst_limit
int stat_prio; // static_priority
int state_timestamp;
int remaining_time; // total_time  - current_time
int turnaround_time; // completion_time - arrival_time
int waiting_time; 
int completion_time;
int cpu_burst;
int io_burst;
int preempt = 0;
bool flag =false;
states old_event = STATE_NULL;
states new_event = STATE_NULL;
states curr_state = STATE_CREATED;
};

process *curr_process_running;

class event
{   public:
    int eve_timestamp;
    process *prc;
    states transition_to;
};

queue<process*> p_qu; //queue of process
vector<process> p_arr; // array of process objects
deque<event> e_qu; //queue of events
queue<process*> runqueue; //queue of events to be run
ifstream file;

class temp
{
    public:
    int  temp_timestamp;
    process *temp_process;
    states temp_transition;

};
temp temp_eve;

class Schedular
{       
        public:
        int quantum = 6;
        void add_process();
        void get_next_process();
        void test_preempt(process *p, int curtime);

};

void Schedular :: add_process()
{
        process *pr = e_qu.front().prc;
        runqueue.push(pr);
}
void Schedular :: get_next_process()
{       if(runqueue.empty())
            curr_process_running = NULL;
        else
        {
            process *pr = runqueue.front();
            curr_process_running = pr;
            runqueue.pop();
    }
        }
        

void get_process()
{   
    string str;
    string line_string;
    //char *sep = " \t\r\n";
    while(getline(file,line_string) && !file.eof()) // get the processes line by line
    {   stringstream line(line_string);
        string  v1,v2,v3,v4;
        line >> v1 >> v2 >> v3 >> v4;
        process *pr = new process;
        pr->timestamp = stoi(v1);
        pr->t_cpu = stoi(v2);
        pr->b_cpu_lim = stoi(v3);
        pr->b_io_lim = stoi(v4);
        pr->stat_prio = random_value(maxprio) -1;
        pr->state_timestamp = pr->timestamp;
        pr->remaining_time = pr->t_cpu;
        p_arr.push_back(*pr);
        delete pr;
    }
    for(int i = 0;i<p_arr.size(); i++)
    {   p_arr[i].p_id = i;
        p_qu.push(&p_arr[i]);  
    }
}

void insert_in_event(event eve)
{   //deque<event> :: reverse_iterator it = e_qu.rbegin();
    if(e_qu.empty() == true)
    {
        e_qu.push_back(eve);
        return;
    }  
    else
    {
       for( auto it = e_qu.rbegin(); it!= e_qu.rend(); ++it)
       {
           if (it->eve_timestamp == eve.eve_timestamp)
           {    
               e_qu.insert(it.base(),eve);
               return;
           }
           else if(it->eve_timestamp < eve.eve_timestamp )
           {    
                if( it == e_qu.rbegin())
                {
                    e_qu.push_back(eve);
                    return;
                }
                else
                {  e_qu.insert(it.base(),eve);
                    return;
                }   
           } 
       }
    }
    e_qu.push_front(eve);
}
void create_events()
{   if(initial_events)
    {
    while(!p_qu.empty())
    {   event *eve = new event;
        process *ptr = p_qu.front();
        p_qu.pop();
        eve->eve_timestamp = ptr->timestamp;
        eve->prc = ptr;
        eve->transition_to = STATE_READY;
        insert_in_event(*eve);
        delete eve;
    }
    }
    else
    {   
        event *eve = new event;
        eve->eve_timestamp = temp_eve.temp_timestamp;
        int time = temp_eve.temp_timestamp;
        eve->prc = temp_eve.temp_process;
        eve->transition_to = temp_eve.temp_transition;
        /*for(int i =0; i< e_qu.size(); i++)
    {
        cout<<e_qu[i].prc->p_id<<" " <<e_qu[i].eve_timestamp<<" "<<e_qu[i].prc->b_cpu_lim<<"\n";
    }*/
    printf("AddEvent(%d:%d",time,eve->prc->p_id);
    cout<<state_types[eve->transition_to]<<")"<<":"<<" ";
    for(int i =0;i<e_qu.size();i++)
    {
        cout<<e_qu[i].prc->timestamp<<":"<<e_qu[i].prc->p_id<<":"<<state_types[e_qu[i].transition_to]<<" ";
    }
    insert_in_event(*eve);
    cout<<"==> ";
    for(int i =0;i<e_qu.size();i++)
    {
        cout<<e_qu[i].prc->timestamp<<":"<<e_qu[i].prc->p_id<<":"<<state_types[e_qu[i].transition_to]<<" ";
    }
    cout<<"\n";
    cout<<"\n";
    }
    
}
event *getevent()
{   if(e_qu.empty())
        return NULL;
    event curr_event = e_qu.front();
    event *currevent_ptr = &curr_event;
    return currevent_ptr;
}

Schedular schedular;
void simulation()
{
    event *evt;
    while((evt =  getevent())&& evt != NULL)
    {   
        process *prc = evt->prc;
        int current_time = evt->eve_timestamp;
        int in_prev_state_time = current_time - prc->state_timestamp;
        switch(evt->transition_to)
        {
            case STATE_READY:
            {   
                call_schedular = true;
                temp_eve.temp_timestamp = current_time;
                prc->state_timestamp = current_time;
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_READY;
                prc->curr_state = STATE_READY;
                schedular.add_process();
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<"\n";
                break;
            }
            case STATE_RUNNING:
            {   int burst = prc->b_cpu_lim;
                bool flag = false;
                if(prc->preempt ==0)
                    prc->cpu_burst = random_value(burst);
                else
                    prc->cpu_burst = prc->preempt;
                if (prc->cpu_burst > prc->remaining_time)
                    prc->cpu_burst = prc->remaining_time;
                if(prc ->cpu_burst > schedular.quantum)
                {   prc->preempt = prc->cpu_burst - schedular.quantum;
                    prc->cpu_burst  = schedular.quantum;
                    flag = true;;

                }
                else if(prc->cpu_burst <= schedular.quantum)
                    prc->preempt = 0;
                prc->timestamp = current_time + prc->cpu_burst;
                prc->state_timestamp = current_time;
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_RUNNING;
                temp_eve.temp_process = prc;
                temp_eve.temp_timestamp = prc->timestamp;
                if (flag)
                    temp_eve.temp_transition = STATE_PREMPT;
                else
                    temp_eve.temp_transition = STATE_BLOCKED;
                prc->curr_state = STATE_RUNNING;
                
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<" CPU "<<prc->cpu_burst<<"rem = "<<prc->remaining_time<<"prio="<<"\n";
                 create_events();
                break;
            }

            case STATE_BLOCKED:
            {
                prc->remaining_time -= prc->cpu_burst;
                if (prc->remaining_time == 0)
                {
                    if(verbose)
                    {
                        cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<"\n";
                        cout<<"Done";
                    }
                }
                else{
                int burst = prc->b_io_lim;
                prc->io_burst = random_value(burst);
                prc->timestamp = current_time + prc->io_burst;
                prc->state_timestamp = current_time;
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_BLOCKED;
                prc->curr_state = STATE_BLOCKED;
                temp_eve.temp_process = prc;
                temp_eve.temp_timestamp = prc->timestamp;
                temp_eve.temp_transition = STATE_READY;
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<" "<<"IO "<<prc->io_burst<<"rem = "<<prc->remaining_time<<"\n";
                 create_events();
                }
                curr_process_running = NULL;
                call_schedular = true;
                break;
            }
            case STATE_PREMPT:
            {   prc->remaining_time -= schedular.quantum;
                prc->timestamp = current_time;
                prc->state_timestamp = current_time;
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_READY;
                prc->curr_state = STATE_PREMPT;
                schedular.add_process();
                temp_eve.temp_transition = STATE_READY;
                curr_process_running = NULL;
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<" "<<prc->remaining_time<<"\n";
                call_schedular = true;
                 break;
            }
    
        }
        e_qu.pop_front();
        evt = NULL;
        prc->completion_time = current_time - prc->arrival_time;
        prc->turnaround_time = prc->completion_time - prc->arrival_time;
        if(call_schedular)
        {
            if(e_qu.front().eve_timestamp == current_time)
            {
                continue;
            }
            call_schedular = false;
            if(curr_process_running == NULL)
            {   
                schedular.get_next_process();
                if(curr_process_running == NULL)
                {   cout<<"Comes here"<<"\n";
                    continue;
                }
                    
                temp_eve.temp_timestamp = current_time;
                temp_eve.temp_process = curr_process_running;
                temp_eve.temp_transition = STATE_RUNNING;
                initial_events = false;
                create_events();
                
            }
        }
    }
}

int main ()
{   
    //file.open(argv[1]);
    file.open("input1_test");
    get_random_nums();
    get_process(); // create process objects
    create_events(); // create event queue
    
    simulation();
    /*for(int i =0; i< e_qu.size(); i++)
    {
        cout<<e_qu[i].prc->p_id<<" " <<e_qu[i].eve_timestamp<<" "<<e_qu[i].prc->b_cpu_lim<<"\n";
    }*/
    //file.close();
    return 0;
}