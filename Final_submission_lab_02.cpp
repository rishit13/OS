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

using namespace std;
int maxprio = 4; 
typedef enum {STATE_CREATED,STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_PREMPT,STATE_NULL} states;
char *state_types[] = {"STATE_CREATED","STATE_READY","STATE_RUNNING", "STATE_BLOCKED", "STATE_PREMPT"};
bool call_schedular;
bool initial_events = true;
int map_arr;
int map_arr_1;
ifstream random_file;
vector<int> random_array;
vector<pair<int, int> > IO_utils;
int offset = 0;
bool verbose = false;
bool preempted = false;
int current_time;
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
int t_cpu = 0; // totalcput_time
int b_cpu_lim = 0; // cpu_burst_limit
int b_io_lim = 0; // ioburst_limit
int stat_prio; // static_priority
int state_timestamp;
int remaining_time = 0; // total_time  - current_time
int turnaround_time = 0; // completion_time - arrival_time
int waiting_time = 0; 
int in_block_time = 0;
int in_cpu_time = 0;
int cpu_burst = 0;
int io_burst = 0;
int preempt = 0;
int dynamic_prio;
int burst_difference;
bool flag =false;
bool signal_preprio = false;
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
deque<process*> runqueue;
queue<process*> *activeQ = new queue<process*>[maxprio];
queue<process*> *expireQ = new queue<process*>[maxprio];
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
        virtual void add_process(){}
       
        virtual void get_next_process(){}
        virtual bool test_preempt()
        {
            return false;
        }
};

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
        pr->stat_prio = random_value(maxprio);
        pr->state_timestamp = pr->timestamp;
        pr->remaining_time = pr->t_cpu;
        pr->dynamic_prio = pr->stat_prio-1;
        pr->arrival_time = stoi(v1);
        p_arr.push_back(*pr);
        delete pr;
    }
    for(int i = 0;i<p_arr.size(); i++)
    {   p_arr[i].p_id = i;
        process *pr = &p_arr[i];
        p_qu.push(pr);  
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
    if(verbose){
    printf("AddEvent(%d:%d",time,eve->prc->p_id);
    cout<<state_types[eve->transition_to]<<")"<<":"<<" ";
    for(int i =0;i<e_qu.size();i++)
    {
        cout<<e_qu[i].eve_timestamp<<":"<<e_qu[i].prc->p_id<<":"<<state_types[e_qu[i].transition_to]<<" ";
    }
    }
    insert_in_event(*eve);
    if (verbose){
    cout<<"==> ";
    for(int i =0;i<e_qu.size();i++)
    {
        cout<<e_qu[i].eve_timestamp<<":"<<e_qu[i].prc->p_id<<":"<<state_types[e_qu[i].transition_to]<<" ";
    }
    cout<<"\n";
    cout<<"\n";
    }
    }
    
}

class FCFS : public Schedular
{
    public:
    void  add_process()
    {
        process *pr = e_qu.front().prc;
        runqueue.push_back(pr);
    }
void get_next_process()
{       
    if(runqueue.empty())
            curr_process_running = NULL;
        else
        {
            process *pr = runqueue.front();
            curr_process_running = pr;
            runqueue.pop_front();
        }   
}
};

class LCFS : public Schedular
{
void add_process()
{
        
        process *pr = e_qu.front().prc;
        runqueue.push_front(pr);

}
void get_next_process()
{   
 if(runqueue.empty())
            curr_process_running = NULL;
        else
        {
            process *pr = runqueue.front();
            curr_process_running = pr;
            runqueue.pop_front();
        }


}
};
class Srt : public Schedular
{   public:
    void insert_srt_event(process *pr)
{
    if(runqueue.empty())
    {
        runqueue.push_front(pr);
        return;
    }
    else{
        for( int i = runqueue.size()-1; i >=0 ;i++)
       {
           if (runqueue[i]->remaining_time == pr->remaining_time)
           {    
               runqueue.insert(runqueue.begin() + i,pr);
               return;
           }
           else if(runqueue[i]->remaining_time < pr->remaining_time)
           {    
                if( i == runqueue.size() -1)
                {
                    runqueue.push_back(pr);
                    return;
                }
                else
                {  runqueue.insert(runqueue.begin()+i,pr);
                    return;
                }   
           } 
       }
    }
    runqueue.push_front(pr);
}
void add_process()
{
        
        process *pr = e_qu.front().prc;
        bool yes = test_preempt();
        insert_srt_event(pr);

}
void get_next_process()
{   
 if(runqueue.empty())
            curr_process_running = NULL;
        else
        {
            process *pr = runqueue.front();
            curr_process_running = pr;
            runqueue.pop_front();
    }
}
};

class RR : public Schedular
{   public:
    void add_process()
{
        process *pr = e_qu.front().prc;
        runqueue.push_back(pr);
}
void get_next_process()
{       if(runqueue.empty())
            curr_process_running = NULL;
        else
        {
            process *pr = runqueue.front();
            curr_process_running = pr;
            runqueue.pop_front();
    }
}
};

class prio : public Schedular
{
    void  add_process()
{
        process *pr = e_qu.front().prc;
        if(preempted)
        {
             if(pr->dynamic_prio -1  == -1)
             {
                int n = pr->stat_prio - 1;
                expireQ[n].push(pr);
                map_arr_1 += 1;
             }
             else
             {
                 activeQ[pr->dynamic_prio - 1].push(pr);
                 map_arr +=1;
             }
        }
       
        else if(!preempted)
        {
            activeQ[pr->dynamic_prio].push(pr);
            map_arr += 1;
        }
}
void get_next_process()
{    int i = maxprio-1;   
    if(map_arr == 0 && map_arr_1 == 0)
    {
        curr_process_running = NULL;
         queue<process*> *temp_arr;
             temp_arr = activeQ;
            activeQ = expireQ;
            expireQ = temp_arr;
            map_arr = map_arr_1;
            map_arr_1 = 0;
        cout<<" context switched here as well";
    } 
    else
    {
        if(map_arr ==0 && map_arr_1>0)
        {   queue<process*> *temp_arr;
             temp_arr = activeQ;
            activeQ = expireQ;
            expireQ = temp_arr;
            map_arr = map_arr_1;
            map_arr_1 = 0;
            cout<<"Context switched ";
        }
        
         while(activeQ[i].empty())
            {
                i--;
            }
    process *pr = activeQ[i].front();
    curr_process_running = pr;
    activeQ[i].pop();
    map_arr -=1;
    }
}
};

class eprio : public Schedular
{
    bool test_preempt()
{
     process *pr = e_qu.front().prc;
    process *temp;
 if(curr_process_running != NULL && pr->dynamic_prio > curr_process_running->dynamic_prio)
    {   
    cout<<curr_process_running->timestamp <<" "<<e_qu[1].eve_timestamp<<" "<<pr->timestamp<<"\n";
        if((curr_process_running->p_id == e_qu[1].prc->p_id && pr->timestamp != e_qu[1].eve_timestamp )|| (curr_process_running->p_id != e_qu[1].prc->p_id ))
        {
        
            cout<<" Comes here "<<"\n";
                for(int i = 0;i<e_qu.size(); i++)
                {
                    if(e_qu[i].prc->p_id == curr_process_running->p_id)
                    {   cout<<"Comes here as well too"<<"\n";
                        e_qu.erase(e_qu.begin()+i+1, e_qu.begin()+i+2);
                        break;
                    }
                }       
                curr_process_running->signal_preprio = true;
                curr_process_running->burst_difference =  current_time - curr_process_running->state_timestamp;
                cout<<curr_process_running->signal_preprio<<" "<<curr_process_running->burst_difference<<"\n";
                curr_process_running->signal_preprio = true;
                temp_eve.temp_process = curr_process_running;
                //curr_process_running->timestamp = pr->timestamp;
                temp_eve.temp_timestamp = pr->timestamp;
                temp_eve.temp_transition = STATE_PREMPT;
                cout<<" premt time stamps"<<current_time;
                create_events();
                
                //pr->timestamp = m;
                temp_eve.temp_process = pr;
                temp_eve.temp_timestamp = pr->timestamp;
                temp_eve.temp_transition = STATE_RUNNING;
                create_events();
                curr_process_running = pr;
                
                return true;
        }
    }
    return false;
}

void  add_process()
{
        process *pr = e_qu.front().prc;
        bool yes = test_preempt();
        if(!yes){
        if(preempted)
        {
             if(pr->dynamic_prio -1  == -1)
             {
                int n = pr->stat_prio - 1;
                expireQ[n].push(pr);
                map_arr_1 += 1;
             }
             else
             {
                 activeQ[pr->dynamic_prio - 1].push(pr);
                 map_arr +=1;
             }
        }
       
        else if(!preempted)
        {
            activeQ[pr->dynamic_prio].push(pr);
            map_arr += 1;
        }
        }

}
void  get_next_process()
{    int i = maxprio-1;   
    if(map_arr == 0 && map_arr_1 == 0)
    {
        curr_process_running = NULL;
         queue<process*> *temp_arr;
             temp_arr = activeQ;
            activeQ = expireQ;
            expireQ = temp_arr;
            map_arr = map_arr_1;
            map_arr_1 = 0;
    } 
    else
    {
        if(map_arr ==0 && map_arr_1>0)
        {   queue<process*> *temp_arr;
             temp_arr = activeQ;
            activeQ = expireQ;
            expireQ = temp_arr;
            map_arr = map_arr_1;
            map_arr_1 = 0;
        }
        
         while(activeQ[i].empty())
            {
                i--;
            }
    process *pr = activeQ[i].front();
    curr_process_running = pr;
    activeQ[i].pop();
    map_arr -=1;
    }
}
};

event *getevent()
{   if(e_qu.empty())
        return NULL;
    event curr_event = e_qu.front();
    event *currevent_ptr = &curr_event;
    return currevent_ptr;
}

Schedular *schedular;
void simulation()
{
    event *evt;
    while((evt =  getevent())&& evt != NULL)
    {   
        process *prc = evt->prc;
        current_time = evt->eve_timestamp;
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
                if (prc->dynamic_prio == -1)
                    prc->dynamic_prio = prc->stat_prio - 1;
                schedular->add_process();
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<"\n";
                break;
            }
            case STATE_RUNNING:
            {   int burst = prc->b_cpu_lim;
                if(prc->curr_state == STATE_READY || prc->curr_state ==STATE_PREMPT)
                        prc->in_cpu_time+=in_prev_state_time;
                bool flag = false;
                if(prc->signal_preprio)
                {
                    prc->cpu_burst = (prc->cpu_burst - prc->burst_difference) + prc->preempt;

                }
                else{
                        if(prc->preempt ==0)
                        {
                            prc->cpu_burst = random_value(burst);
                            prc->dynamic_prio = prc->stat_prio -1;
                        }  
                        else
                            prc->cpu_burst = prc->preempt;
                }

                if (prc->cpu_burst > prc->remaining_time)
                    prc->cpu_burst = prc->remaining_time;
                if(prc ->cpu_burst > schedular->quantum)
                {   prc->preempt = prc->cpu_burst - schedular->quantum;
                    prc->cpu_burst  = schedular->quantum;
                    flag = true;;
                }
                else if(prc->cpu_burst <= schedular->quantum && !prc->signal_preprio)
                    prc->preempt = 0;
                prc->signal_preprio = false;
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
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<" CPU "<<prc->cpu_burst<<"rem = "<<prc->remaining_time<<"prio="<<prc->dynamic_prio<<"\n";
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
                prc->in_block_time += prc->io_burst;
                prc->timestamp = current_time + prc->io_burst;
                prc->state_timestamp = current_time;
                IO_utils.push_back(make_pair(prc->state_timestamp,prc->timestamp));
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_BLOCKED;
                prc->curr_state = STATE_BLOCKED;
                prc->dynamic_prio = prc->stat_prio - 1;
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
            {   
                prc->timestamp = current_time;
                prc->state_timestamp = current_time;
                prc->old_event = prc->curr_state;
                prc->new_event = STATE_READY;
                prc->curr_state = STATE_PREMPT;
                preempted = true;
                if(!prc->signal_preprio)
                {
                    curr_process_running = NULL;
                    prc->remaining_time -= schedular->quantum;
                }
                else{
                    prc->remaining_time -= prc->burst_difference;
                }
                schedular->add_process();  
                temp_eve.temp_transition = STATE_READY;
                
                if(verbose)
                    cout<<current_time<<" "<<prc->p_id<<" "<<in_prev_state_time<<" "<<state_types[prc->old_event]<<" -> "<<state_types[prc->new_event]<<" "<<prc->remaining_time<<"  prip="<<prc->dynamic_prio<<"\n";
                call_schedular = true;
                if (prc->dynamic_prio == -1)
                    prc->dynamic_prio = prc->stat_prio - 1;
                else 
                {
                     prc->dynamic_prio = prc->dynamic_prio - 1;
                     if(prc->dynamic_prio == -1)
                        prc->dynamic_prio = prc->stat_prio - 1;
                }
                 
                preempted =false;
                 break;
            }
    
        }
        e_qu.pop_front();
        evt = NULL;
        if(call_schedular)
        {
            if(e_qu.front().eve_timestamp == current_time)
            {
                continue;
            }
            call_schedular = false;
            if(curr_process_running == NULL)
            {   
                schedular->get_next_process();
                if(curr_process_running == NULL)
                {   
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
void print_summary()
{   vector<pair<int,int> > results;
    int utilization =0;
    for(int i = 0;i<p_arr.size();i++)
    {
        int td = p_arr[i].timestamp - p_arr[i].arrival_time;
        cout<<p_arr[i].p_id<<" "<<" Finishing time "<<p_arr[i].timestamp<<" Turnaround time "<<td <<" In CW  "<<p_arr[i].in_cpu_time<<" In IW "<<p_arr[i].in_block_time<<"\n";
    }
    pair<int,int>  temp;
    temp = IO_utils[0];
    for(int i =1;i<IO_utils.size();i++)
    {   
        if( IO_utils[i].first<= temp.second)
        {
            temp. second = max(temp.second, IO_utils[i].second);
        }
        else{
            results.push_back(temp);
            temp = IO_utils[i];
        }
    }
    results.push_back(temp);
    for(int i =0;i < results.size();i++)
    {   
        int difference = results[i].second - results[i].first;
        utilization += difference;
    }
    int t = ((double) utilization/current_time) *100;
    cout<<" IO utils time "<< t;
}

int main ()
{   
    //file.open(argv[1]);
    file.open("input1_test");
    char algo;
    get_random_nums();
    get_process(); // create process objects
    create_events(); // create event queue
    algo = 'E';
    if(algo == 'F')
    {
        FCFS sched;
        schedular = &sched;
        simulation();
    }
    else if(algo == 'L')
    {
        LCFS sched;
        schedular = &sched;
        simulation();
    }
    else if(algo == 'S')
    {
        Srt sched;
        schedular = &sched;
        simulation();
    }
    else if(algo == 'R')
    {
        RR sched;
        schedular = &sched;
        simulation();
    }
    else if(algo == 'P')
    {
        prio sched;
        schedular = &sched;
        simulation();
    }
    if(algo == 'E')
    {
        eprio sched;
        schedular = &sched;
        simulation();
    }
    print_summary();
    file.close();
    return 0;
}