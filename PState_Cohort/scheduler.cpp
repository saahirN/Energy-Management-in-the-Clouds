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
vector<MachineId_t> allMachines;

unsigned currAssign = 0;
unsigned currSleep = 3;
bool SLA_warning = false;
CPUPerformance_t currentPerf = P3;

std::unordered_map<MachineId_t, vector<VMId_t>> vmMap;
std::unordered_map<TaskId_t, MachineId_t> taskMap;
std::unordered_map<MachineId_t, signed> mipsCost; 
std::unordered_map<MachineId_t, unsigned> memoryCost; 
std::unordered_map<VMId_t, bool> isMigrating; 

unsigned insertSortedEE(vector<MachineId_t>* mList, MachineId_t id);
bool hasEnoughResource(MachineId_t mid, TaskId_t tid); 
double getCurrentLoad(vector<MachineId_t> machines);

/* We need to make sure we separate machines by VM type and hardware type
   to assign tasks to their requirements*/

void Scheduler::Init() {
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);

    //First organize all of the machines we have available
    for (unsigned i = 0; i < Machine_GetTotal(); i++) {
        allMachines.push_back(MachineId_t(i));
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
        mipsCost[MachineId_t(i)] = 0; 
        memoryCost[MachineId_t(i)] = 0;
        Machine_SetState(MachineId_t(i), S0); 
        for(int j = 0; j < Machine_GetInfo(MachineId_t(i)).num_cpus; j++) {
            Machine_SetCorePerformance(MachineId_t(i), j, currentPerf); 
        }
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t t_info = GetTaskInfo(task_id);
    CPUType_t cpu = t_info.required_cpu; 
    VMType_t os = t_info.required_vm; 

    /* Figure out which list of machines to use */
    vector<MachineId_t> machine_list;
    switch (cpu) {
        case X86:
            machine_list = x86Machines;
            break; 
        case ARM:
            machine_list = armMachines; 
            break;
        case POWER:
            machine_list = powerMachines; 
            break;
        default:
            machine_list = riscvMachines; 
            break;
    }

    //Choose which machine we are going to use; try to assign to the most energy efficient machine
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
    mipsCost[chosen] += 1000; 
    memoryCost[chosen] += t_info.required_memory;
    taskMap.emplace(task_id, chosen);
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary

    //Dynamically adjust current operating performance level based on overall average load of the machines
    // std::cout << getCurrentLoad(allMachines) << std::endl;
    if ((getCurrentLoad(allMachines) > 0.8 || SLA_warning)) {
        if (currentPerf == P0) {
            return;
        }
        currentPerf = P0;
        cout << "Current Perf Changed to P0" << endl; 
        for (MachineId_t machine : allMachines) {
            for(int j = 0; j < Machine_GetInfo(machine).num_cpus; j++) {
                Machine_SetCorePerformance(machine, j, currentPerf); 
            }
        }
    }
    else if (getCurrentLoad(allMachines) > 0.6) {
        if (currentPerf == P1) {
            return;
        }
        currentPerf = P1;
        cout << "Current Perf Changed to P1" << endl;
        for (MachineId_t machine : allMachines) {
            for(int j = 0; j < Machine_GetInfo(machine).num_cpus; j++) {
                Machine_SetCorePerformance(machine, j, currentPerf); 
            }
        }
    }
    else if (getCurrentLoad(allMachines) > 0.4) {
        if (currentPerf == P2) {
            return;
        }
        currentPerf = P2;
        cout << "Current Perf Changed to P2" << endl;
        for (MachineId_t machine : allMachines) {
            for(int j = 0; j < Machine_GetInfo(machine).num_cpus; j++) {
                Machine_SetCorePerformance(machine, j, currentPerf); 
            }
        }
    }
    else {
        if (currentPerf == P3) {
            return;
        }
        currentPerf = P3;
        cout << "Current Perf Changed to P3" << endl;
        for (MachineId_t machine : allMachines) {
            for(int j = 0; j < Machine_GetInfo(machine).num_cpus; j++) {
                Machine_SetCorePerformance(machine, j, currentPerf); 
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
    // cout << "TASK COMPLETED" << endl;
    mipsCost[taskMap[task_id]] -= 1000;
    memoryCost[taskMap[task_id]] -= GetTaskInfo(task_id).required_memory;
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
    if(minfo.performance[currentPerf] * minfo.num_cpus - mipsCost.at(mid) < mipsRequired) {
        return false; 
    }

    if(minfo.memory_size - memoryCost.at(mid) - tinfo.required_memory < 0) {
        return false; 
    }

    return true; 
}

double getCurrentLoad(vector<MachineId_t> machines) {
    signed totalMips = 0;
    signed usedMips = 0;
    signed totalMemory = 0;
    signed usedMemory = 0;
    for (unsigned i = 0; i < machines.size(); i++) {
        MachineInfo_t info = Machine_GetInfo(machines.at(i));
        //totalMips += info.performance[currentPerf] * info.num_cpus;
        totalMemory += info.memory_size;
        //usedMips += mipsCost[machines.at(i)];
        usedMemory += memoryCost[machines.at(i)];
    } 
    // std::cout << "Total mips cost " << usedMips << std::endl;
    // std::cout << "Total mips " << totalMips << std::endl;
    //double mipsLoad = (usedMips * 1.0) / totalMips;
    double memoryLoad = (usedMemory * 1.0) / totalMemory;

    //Return largest overall load, whether it's mips or memory load
    return memoryLoad;
}
