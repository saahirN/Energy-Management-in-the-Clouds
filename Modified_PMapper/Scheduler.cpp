//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <unordered_map>
#include <cmath>
#include <limits.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <iterator>
#include <random>

vector<MachineId_t> x86Machines;
vector<MachineId_t> armMachines;
vector<MachineId_t> powerMachines;
vector<MachineId_t> riscvMachines;

vector<MachineId_t> x86OffMachines;
vector<MachineId_t> x86OnMachines;
vector<MachineId_t> armOffMachines;
vector<MachineId_t> armOnMachines;
vector<MachineId_t> powerOffMachines;
vector<MachineId_t> powerOnMachines;
vector<MachineId_t> riscvOffMachines;
vector<MachineId_t> riscvOnMachines;

unsigned currAssign = 0;
unsigned currSleep = 3;
bool SLA_warning = false;

std::unordered_map<MachineId_t, vector<VMId_t>> vmMap;
std::unordered_map<TaskId_t, MachineId_t> taskMap;
std::unordered_map<MachineId_t, signed> remainingMips; 
std::unordered_map<MachineId_t, unsigned> remainingMemory; 
std::unordered_map<VMId_t, bool> isMigrating; 
std::unordered_map<MachineId_t, unsigned> idleCounter;

unsigned insertSortedEE(vector<MachineId_t>* mList, MachineId_t id);
bool hasEnoughResource(MachineId_t mid, TaskId_t tid); 
signed getCurrUtilization(MachineId_t mid);
double getCurrentLoad(vector<MachineId_t> machines);

/* We need to make sure we separate machines by VM type and hardware type
   to assign tasks to their requirements*/

void Scheduler::Init() {
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);

    //First organize all of the machines we have available
    for (unsigned i = 0; i < Machine_GetTotal(); i++) {
        switch (Machine_GetCPUType(MachineId_t(i))) {
            case X86:
                insertSortedEE(&x86Machines, MachineId_t(i)); 
                break; 
            case ARM:
                insertSortedEE(&armMachines, MachineId_t(i)); 
                break; 
            case POWER:
                insertSortedEE(&powerMachines, MachineId_t(i)); 
                break; 
            default:
                insertSortedEE(&riscvMachines, MachineId_t(i)); 
                break; 
        }
        //Compute effective performance of each machine and turn all machines on initially
        remainingMips[MachineId_t(i)] = Machine_GetInfo(MachineId_t(i)).performance[0] * Machine_GetInfo(MachineId_t(i)).num_cpus; 
        remainingMemory[MachineId_t(i)] = Machine_GetInfo(MachineId_t(i)).memory_size;
        Machine_SetState(MachineId_t(i), S0); 
        idleCounter.emplace(MachineId_t(i), 0);
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
    isMigrating[vm_id] = false;
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t t_info = GetTaskInfo(task_id);
    CPUType_t cpu = t_info.required_cpu; 
    VMType_t os = t_info.required_vm; 

    /* Figure out which list of machines to use */
    vector<MachineId_t> machine_list;
    switch (cpu) {
        case X86:
            machine_list = x86OnMachines;
            break; 
        case ARM:
            machine_list = armOnMachines; 
            break;
        case POWER:
            machine_list = powerOnMachines; 
            break;
        default:
            machine_list = riscvOnMachines; 
            break;
    }

    //Choose which machine we are going to use; try to assign to the most energy efficient machine (this should
    //also generally congregate tasks onto the same machines)
    MachineId_t chosen = -1; 
    for (int i = 0; i < machine_list.size(); i++) {
        
        MachineInfo_t curr_info = Machine_GetInfo(machine_list.at(i));
        if((curr_info.s_state == S0) && hasEnoughResource(machine_list.at(i), task_id)) {
            chosen = machine_list.at(i);
            break;
        }
    }

    //Check that we actually found a machine that can service the task
    if (chosen == -1) {
        //If we didn't, just use a random one to disperse load
        unsigned random = rand() % machine_list.size();
        chosen = machine_list.at(random);
    }

    /* Put the task on the machine */
    if(vmMap.find(chosen) == vmMap.end()) {
        vmMap[chosen] = {}; 
    }
    VMId_t v_id = VM_Create(os, cpu); 
    isMigrating[v_id] = false;
    vmMap[chosen].push_back(v_id); 
    VM_Attach(v_id, chosen); 
    VM_AddTask(v_id, task_id, t_info.priority); 
    remainingMips[chosen] -= 1000; 
    remainingMemory[chosen] -= t_info.required_memory;
    taskMap.emplace(task_id, chosen);
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary

    vector<vector<MachineId_t> *> types = {&x86OnMachines, &x86OffMachines, &armOnMachines, &armOffMachines, &powerOnMachines, &powerOffMachines, &riscvOnMachines, &riscvOffMachines};
    vector<vector<MachineId_t> *> totalTypes = {&x86Machines, &armMachines, &powerMachines, &riscvMachines};
    for (int a = 0; a < types.size(); a+=2) {
        vector<MachineId_t> *currOn = types.at(a);
        vector<MachineId_t> *currOff = types.at(a+1);

        //First compute the idle time for all onMachines
        for (int i = 0; i < (*currOn).size(); i++) {
            MachineId_t curr = (*currOn).at(i);
            if (getCurrUtilization(curr) == 0) {
                idleCounter.at(curr)++;
            }
            else {
                idleCounter.at(curr) = 0;
            }
        }

        if (getCurrentLoad((*currOn)) > 0.5 || SLA_warning == true) {
            unsigned numTurnOn = (*currOff).size() / 5;
            for (int i = 0; i < (*currOff).size(); i++) {
                if (numTurnOn == 0) {
                    break;
                }
                MachineId_t curr = (*currOff).at(i);
                (*currOff).erase((*currOff).begin() + i);
                Machine_SetState(curr, S0);
                numTurnOn--;
                i--;
            }
            SLA_warning = false;
        }
        else {
            for (int i = 0; i < (*currOn).size(); i++) {
                MachineId_t curr = (*currOn).at(i);
                if (idleCounter.at(curr) == 500 && (*currOn).size() > totalTypes.at(a/2)->size() / 4) {
                    currSleep = (currSleep != 6) ? currSleep + 1 : 3;
                    (*currOn).erase((*currOn).begin() + i);
                    Machine_SetState(curr, (MachineState_t) currSleep);
                    i--;    
                }
            }
        }
    }

}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);

    remainingMips[taskMap[task_id]] += 1000;
    remainingMemory[taskMap[task_id]] += GetTaskInfo(task_id).required_memory;

    //First find the machine with the least utilization that's still turned on and the max utilization as well
    signed minUtilization = INT_MAX;
    MachineId_t min = -1;
    for (unsigned i = 0; i < Machine_GetTotal(); i++) {
        signed currUtilization = getCurrUtilization(MachineId_t(i));
        //Note: If currUtilization = 0, that means there's nothing to migrate on this machine to begin with
        if (currUtilization < minUtilization && currUtilization != 0 && Machine_GetInfo(MachineId_t(i)).s_state == S0) {
            min = MachineId_t(i);
            minUtilization = currUtilization;
        }
    }

    if (min == -1) {
        //There's nothing to migrate, all machines are at a currUtilization of 0
        taskMap.erase(task_id);
        return;
    }

    //Now find the workload on this machine with the most instructions remaining
    vector<VMId_t> vms = vmMap[min];
    unsigned maxLoad = 0;
    TaskId_t task_max = -1;
    VMId_t vm_max = -1;
    for (VMId_t vm : vms) {
        for (TaskId_t task : VM_GetInfo(vm).active_tasks){
            TaskInfo_t t_info = GetTaskInfo(task);
            if (t_info.remaining_instructions > maxLoad) {
                task_max = task;
                vm_max = vm;
                maxLoad = t_info.remaining_instructions;
            }
        }
    }

    //If we cannot find any tasks to migrate then return OR if the task is close to completed
    if (vm_max == -1 || GetTaskInfo(task_max).remaining_instructions < GetTaskInfo(task_max).total_instructions * 0.25) {
        taskMap.erase(task_id);
        return;
    }

    //Now migrate that task from this machine to a machine with high utilization
    CPUType_t cpu = GetTaskInfo(task_max).required_cpu;
    vector<MachineId_t> machine_list;
    switch (cpu) {
        case X86:
            machine_list = x86OnMachines;
            break; 
        case ARM:
            machine_list = armOnMachines; 
            break;
        case POWER:
            machine_list = powerOnMachines; 
            break;
        default:
            machine_list = riscvOnMachines; 
            break;
    }
    signed maxUtilization = INT_MIN;
    MachineId_t max = -1;
    for (unsigned i = 0; i < machine_list.size(); i++) {
        MachineId_t curr = machine_list.at(i);
        signed currUtilization = getCurrUtilization(curr);
        if (currUtilization > maxUtilization && hasEnoughResource(curr, task_max)) {
            max = curr;
            maxUtilization = currUtilization;
        }
    }

    //Double check the minimum load machine isn't the same as the max load
    if (minUtilization == maxUtilization) {
        taskMap.erase(task_id);
        return;
    }

    if (max != -1 && !isMigrating[vm_max]) {
        VM_Migrate(vm_max, max);
        isMigrating[vm_max] = true;
        remainingMips[max] -= 1000;
        remainingMips[min] += 1000;
        remainingMemory[max] -= GetTaskInfo(task_max).required_memory;
        remainingMemory[min] += GetTaskInfo(task_max).required_memory;
    }
}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 4);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
    
    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    SLA_warning = true;
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
    if (Machine_GetInfo(machine_id).s_state == S0) {
        CPUType_t cpu = Machine_GetCPUType(machine_id);
        switch (cpu) {
            case X86:
                insertSortedEE(&x86OnMachines, machine_id);
                break; 
            case ARM:
                insertSortedEE(&armOnMachines, machine_id); 
                break;
            case POWER:
                insertSortedEE(&powerOnMachines, machine_id); 
                break;
            default:
                insertSortedEE(&riscvOnMachines, machine_id);
                break;
        }
    }
    else {
        //We just turned off a machine to some lower power state
        CPUType_t cpu = Machine_GetCPUType(machine_id);
        switch (cpu) {
            case X86:
                insertSortedEE(&x86OffMachines, machine_id);
                break; 
            case ARM:
                insertSortedEE(&armOffMachines, machine_id); 
                break;
            case POWER:
                insertSortedEE(&powerOffMachines, machine_id); 
                break;
            default:
                insertSortedEE(&riscvOffMachines, machine_id);
                break;
        }
    }
}





unsigned insertSortedEE(vector<MachineId_t>* mList, MachineId_t id) {

    if((*mList).size() == 0) {
        (*mList).push_back(id); 
        return 0; 
    }

    MachineInfo_t newInfo = Machine_GetInfo (id); 
    double new_ee = newInfo.performance.at(0) / newInfo.p_states.at(0); 

    for (unsigned i = 0; i < (*mList).size(); i++) {
        MachineInfo_t info = Machine_GetInfo((*mList).at(i));
        double ee = info.performance.at(0) / info.p_states.at(0); 
        if (new_ee > ee) {
            (*mList).insert((*mList).begin() + i, id); 
            return i; 
        }
    }

    (*mList).push_back(id); 
    return (*mList).size() - 1; 
}

bool hasEnoughResource(MachineId_t mid, TaskId_t tid) {
    MachineInfo_t minfo = Machine_GetInfo(mid); 
    TaskInfo_t tinfo = GetTaskInfo(tid); 

    unsigned mipsRequired = 1000; 
    if(remainingMips[mid] < mipsRequired) {
        return false; 
    }

    if(remainingMemory[mid] < tinfo.required_memory) {
        return false; 
    }

    return true; 
}

signed getCurrUtilization(MachineId_t mid) {
    MachineInfo_t minfo = Machine_GetInfo(mid); 
    unsigned maxPerformance = minfo.performance[0] * minfo.num_cpus;
    signed currUtilization = maxPerformance - remainingMips[mid];
    return currUtilization;
}

double getCurrentLoad(vector<MachineId_t> machines) {
    signed totalMips = 0;
    signed usedMips = 0;
    signed totalMemory = 0;
    signed usedMemory = 0;
    for (unsigned i = 0; i < machines.size(); i++) {
        MachineInfo_t info = Machine_GetInfo(machines.at(i));
        totalMips += info.performance[0] * info.num_cpus;
        usedMips += info.performance[0] * info.num_cpus - remainingMips[machines.at(i)];
        totalMemory += info.memory_size;
        usedMemory += info.memory_size - remainingMemory[machines.at(i)];
    } 
    double mipsLoad = (usedMips * 1.0) / totalMips;
    double memoryLoad = (usedMemory * 1.0) / totalMemory;

    //Return largest overall load, whether it's mips or memory load
    return (mipsLoad > memoryLoad) ? mipsLoad : memoryLoad;
}
