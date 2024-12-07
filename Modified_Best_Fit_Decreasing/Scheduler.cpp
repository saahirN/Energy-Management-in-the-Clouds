//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

static bool migrating = false;
static unsigned total_machines;
static unsigned numX86Machines = 0; 
static unsigned numArmMachines = 0; 
static unsigned numPowerMachines = 0; 
static unsigned numRiscvMachines = 0; 

vector<MachineId_t> x86Machines;
vector<MachineId_t> armMachines;
vector<MachineId_t> powerMachines;
vector<MachineId_t> riscvMachines;

unsigned activex86 = 0; 
unsigned activeArm = 0;
unsigned activePower = 0;
unsigned activeRiscv = 0; 

std::unordered_map<MachineId_t, vector<VMId_t>> vmMap;
std::unordered_map<TaskId_t, MachineId_t> taskMap;
std::unordered_map<TaskId_t, VMId_t> taskToVM; 
std::unordered_map<MachineId_t, unsigned> remainingMips; 
std::unordered_map<VMId_t, bool> isMigrating; 
std::unordered_map<MachineId_t, unsigned> numTasks; 
std::unordered_map<MachineId_t, unsigned> remainingMemory; 

vector<MachineId_t> turningOff; 


unsigned insert_sorted_ee(vector<MachineId_t>* mList, MachineId_t id);
bool hasEnoughResource(MachineId_t mid, TaskId_t tid); 
unsigned estimatedPower(MachineId_t mid, TaskId_t tid); 
void updateMachines(CPUType_t type);

void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    // 
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 0);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);
    total_machines = Machine_GetTotal(); 

    std::srand(std::time(0));

    /* Find the number of each machine available */
    for(unsigned i = 0; i < total_machines; i++) {
        switch (Machine_GetCPUType(MachineId_t(i))) {
            case X86:
                numX86Machines++; 
                insert_sorted_ee(&x86Machines, MachineId_t(i)); 
                break; 
            case ARM:
                numArmMachines++; 
                insert_sorted_ee(&armMachines, MachineId_t(i)); 
                break; 
            case POWER:
                numPowerMachines++;
                insert_sorted_ee(&powerMachines, MachineId_t(i)); 
                break; 
            default:
                numRiscvMachines++;
                insert_sorted_ee(&riscvMachines, MachineId_t(i)); 
                break; 
        }
        remainingMips[MachineId_t(i)] = Machine_GetInfo(MachineId_t(i)).performance[0] * Machine_GetInfo(MachineId_t(i)).num_cpus; 
        remainingMemory[MachineId_t(i)] = (Machine_GetInfo(MachineId_t(i)).memory_size) * 0.95; 
        numTasks[MachineId_t(i)] = 0; 
    }

    /* Turn on 1/4 of the machines of each type */
    for(int i = 0; i < numX86Machines; i++) {
        /* TO BE UPDATED FOR TURNING ON/OFF */
        if(i <= numX86Machines) {
            Machine_SetState(x86Machines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(x86Machines.at(i), j, P3); 
            }
        } else {
            Machine_SetState(x86Machines.at(i), S5); 
        }
    }
    for(int i = 0; i < numArmMachines; i++) {
        /* TO BE UPDATED FOR TURNING ON/OFF */
        if(i <= numArmMachines) {
            Machine_SetState(armMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(armMachines.at(i), j, P3); 
            }
        } else {
            Machine_SetState(armMachines.at(i), S5); 
        }
    }
    for(int i = 0; i < numPowerMachines; i++) {
        /* TO BE UPDATED FOR TURNING ON/OFF */
        if(i <= numPowerMachines) {
            Machine_SetState(powerMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(powerMachines.at(i), j, P3); 
            }
        } else {
            Machine_SetState(powerMachines.at(i), S5); 
        }
    }
    for(int i = 0; i < numRiscvMachines; i++) {
        /* TO BE UPDATED FOR TURNING ON/OFF */
        if(i < numRiscvMachines) {
            Machine_SetState(riscvMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(riscvMachines.at(i), j, P3); 
            }
        } else {
            Machine_SetState(riscvMachines.at(i), S5); 
        }
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    isMigrating[vm_id] = false; 
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Get the task parameters
    //  IsGPUCapable(task_id);
    //  GetMemory(task_id);
    //  RequiredVMType(task_id);
    //  RequiredSLA(task_id);
    //  RequiredCPUType(task_id);
    // Decide to attach the task to an existing VM, 
    //      vm.AddTask(taskid, Priority_T priority); or
    // Create a new VM, attach the VM to a machine
    //      VM vm(type of the VM)
    //      vm.Attach(machine_id);
    //      vm.AddTask(taskid, Priority_t priority) or
    // Turn on a machine, create a new VM, attach it to the VM, then add the task
    //
    // Turn on a machine, migrate an existing VM from a loaded machine....
    //
    // Other possibilities as desired

    TaskInfo_t info = GetTaskInfo (task_id); 
    CPUType_t cpu = info.required_cpu; 
    VMType_t os = info.required_vm; 

    SimOutput("Handling task " + to_string(task_id), 1); 

    /* Figure out which list of machines to use */
    vector<MachineId_t> list;
    switch (cpu) {
        case X86:
            list = x86Machines;
            break; 
        case ARM:
            list = armMachines; 
            break;
        case POWER:
            list = powerMachines; 
            break;
        default:
            list = riscvMachines; 
            break;
    }

    /* Go through the list and the best machine based on the MBFD algorithm */ 
    uint32_t minPower = INT32_MAX; 

    MachineId_t chosen = -1; 
    unsigned size = list.size(); 
    for(int i = 0; i < size; i++) {
        if(hasEnoughResource(list.at(i), task_id) 
            && std::find(turningOff.begin(), turningOff.end(), list.at(i)) == turningOff.end()
            && Machine_GetInfo(list.at(i)).s_state == S0) {
            unsigned estPower = estimatedPower(list.at(i), task_id); 
            if(estPower < minPower) {
                minPower = estPower; 
                chosen = list.at(i); 
            }
        }
    }

    SimOutput("Chosen machine: " + to_string(chosen), 1);

    while (chosen == -1) {
        int random = std::rand();
        int min = 0, max = list.size() - 1;
        int random_number = min + (random % (max - min + 1));
        chosen = list.at(random_number); 

        if(Machine_GetInfo(chosen).s_state != S0 
            || std::find(turningOff.begin(), turningOff.end(), chosen) != turningOff.end()) {
            chosen = -1; 
        }
    }

    /* Put the task on the machine */
    if(vmMap.find(chosen) == vmMap.end()) {
        vmMap[chosen] = {}; 
    }
    VMId_t vid = VM_Create(info.required_vm, info.required_cpu); 
    isMigrating[vid] = false; 
    vmMap[chosen].push_back(vid); 
    taskToVM[task_id] = vid; 
    taskMap[task_id] = chosen; 
    VM_Attach(vid, chosen); 
    VM_AddTask(vid, task_id, info.priority); 
    if(remainingMips[chosen] >= 1000) {
        remainingMips[chosen] -= 1000; 
    } else {
        remainingMips[chosen] = 0; 
    }
    if(remainingMemory[chosen] >= info.required_memory) {
        remainingMemory[chosen] -= info.required_memory;
    } else {
        remainingMemory[chosen] = 0; 
    }
    if(numTasks[chosen] == 0) {
        switch (info.required_cpu) {
            case X86:
                activex86++; 
                break; 
            case ARM:
                activeArm++;
                break; 
            case POWER:
                activePower++; 
                break; 
            case RISCV:
                activeRiscv++; 
                break; 
            default:
                break; 
        }
        updateMachines(info.required_cpu); 
    }
    numTasks[chosen]++; 

    /* Adjust P-State as needed */
    unsigned mipsNeeded = Machine_GetInfo(chosen).performance[0] - remainingMips[chosen]; 
    CPUPerformance_t pState; 
    for(int i = 3; i >= 0; i--) {

        switch(i) {
            case 3:
                pState = P3;
                break;
            case 2:
                pState = P2; 
                break; 
            case 1:
                pState = P1; 
                break;
            case 0:
                pState = P0; 
                break;
            default:
                break; 
        }

        if(Machine_GetInfo(chosen).performance[i] >= mipsNeeded) {
            for(int j = 0; j < Machine_GetInfo(chosen).num_cpus; j++) {
                Machine_SetCorePerformance(chosen, j, pState); 
            }
            break; 
        }
    }
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    SimOutput("Shutting Down", 0); 
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

    SimOutput("Finishing Task " + to_string(task_id), 1); 
    TaskInfo_t info = GetTaskInfo (task_id); 
    CPUType_t cpu = info.required_cpu; 
    VMType_t os = info.required_vm; 

    MachineId_t mid = taskMap[task_id];
    vector<VMId_t> *machine_vms = &vmMap[mid];
    VMId_t toRemove = taskToVM[task_id]; 

    remainingMemory[mid] += info.required_memory; 
    remainingMips[mid] += 1000; 
    numTasks[mid]--; 

    if(numTasks[mid] == 0) {
        switch (info.required_cpu) {
            case X86:
                activex86--; 
                break; 
            case ARM:
                activeArm--;
                break; 
            case POWER:
                activePower--; 
                break; 
            case RISCV:
                activeRiscv--; 
                break; 
            default:
                break; 
        }
        updateMachines(info.required_cpu); 
    }

    if(!isMigrating[toRemove]) {
        VM_Shutdown(toRemove); 
        (*machine_vms).erase(std::remove((*machine_vms).begin(), (*machine_vms).end(), toRemove), (*machine_vms).end()); 
    }
    

    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
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
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);

    /* Get a list of all tasks */
    vector<TaskId_t> tasks; 
    vector<VMId_t> machine_vms = vmMap[machine_id]; 
    for(int i = 0; i < machine_vms.size(); i++) {
        VMInfo_t vinfo = VM_GetInfo(machine_vms.at(i)); 
        for(int j = 0; j < vinfo.active_tasks.size(); j++) {
            tasks.push_back(vinfo.active_tasks.at(j)); 
        }
    }

    /* Sort the list of tasks by their memory usage*/
    std::sort(machine_vms.begin(), machine_vms.end(), [](VMId_t a, VMId_t b) {
        if(VM_GetInfo(a).active_tasks.size() == 0 || VM_GetInfo(b).active_tasks.size() == 0) {
            return false; 
        } 
        return GetTaskMemory(VM_GetInfo(a).active_tasks.at(0)) > GetTaskMemory(VM_GetInfo(b).active_tasks.at(0)); // Sort in descending order
    });
    

    /* Get the list of machines that can be migrated to */
    vector<MachineId_t> list;
    CPUType_t cpu = Machine_GetInfo(machine_id).cpu; 
    switch (cpu) {
        case X86:
            list = x86Machines; 
            break; 
        case ARM:
            list = armMachines;
            break;
        case POWER:
            list = powerMachines;
            break;
        default:
            list = riscvMachines; 
            break;
    }    

    unsigned listSize = list.size(); 
    for(int i = 0; i < machine_vms.size(); i++) {
        for(int j = 0; j < listSize; j++) {
            if( list.at(j) != machine_id 
                && Machine_GetInfo(list.at(j)).s_state == S0
                && std::find(turningOff.begin(), turningOff.end(), list.at(j)) == turningOff.end()
                && !isMigrating[machine_vms.at(i)] && VM_GetInfo(machine_vms.at(i)).active_tasks.size() != 0
                && hasEnoughResource(list.at(j), VM_GetInfo(machine_vms.at(i)).active_tasks.at(0))) {
                remainingMips[machine_id] += 1000;
                remainingMips[list.at(j)] -= 1000;
                remainingMemory[machine_id] += GetTaskMemory(VM_GetInfo(machine_vms.at(i)).active_tasks.at(0)); 
                remainingMemory[list.at(j)] -= GetTaskMemory(VM_GetInfo(machine_vms.at(i)).active_tasks.at(0)); 
                numTasks[machine_id]--; 
                if(numTasks[machine_id] == 0) {
                    switch(cpu) {
                        case X86:
                            activex86--; 
                            break;
                        case ARM:
                            activeArm--;
                            break;
                        case POWER:
                            activePower--;
                            break; 
                        case RISCV:
                            activeRiscv--;
                            break;
                        default:
                            break; 
                    }
                }
                if(numTasks[list.at(j)] == 0) {
                    switch(cpu) {
                        case X86:
                            activex86++; 
                            break;
                        case ARM:
                            activeArm++;
                            break;
                        case POWER:
                            activePower++;
                            break; 
                        case RISCV:
                            activeRiscv++;
                            break;
                        default:
                            break; 
                    }
                }
                numTasks[list.at(j)]++;
                updateMachines(cpu); 
                isMigrating[machine_vms.at(i)] = true; 
                VM_Migrate(machine_vms.at(i), list.at(j)); 
                j = list.size(); 
            }
        }
    }
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 0);
    isMigrating[vm_id] = false; 
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
    static unsigned counts = 0;
    counts++;
    if(counts == 10) {
        // migrating = true;
        // VM_Migrate(1, 9);
    }
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
    /* Get a list of all tasks */
    MachineId_t machine_id = taskMap[task_id]; 
    vector<TaskId_t> tasks; 
    vector<VMId_t> machine_vms = vmMap[machine_id]; 
    for(int i = 0; i < machine_vms.size(); i++) {
        VMInfo_t vinfo = VM_GetInfo(machine_vms.at(i)); 
        for(int j = 0; j < vinfo.active_tasks.size(); j++) {
            tasks.push_back(vinfo.active_tasks.at(j)); 
        }
    }

    /* Sort the list of tasks by their memory usage*/
    std::sort(machine_vms.begin(), machine_vms.end(), [](VMId_t a, VMId_t b) {
        if(VM_GetInfo(a).active_tasks.size() == 0 || VM_GetInfo(b).active_tasks.size() == 0) {
            return false; 
        } 
        return GetTaskMemory(VM_GetInfo(a).active_tasks.at(0)) > GetTaskMemory(VM_GetInfo(b).active_tasks.at(0)); // Sort in descending order
    });
    

    /* Get the list of machines that can be migrated to */
    vector<MachineId_t> list;
    CPUType_t cpu = Machine_GetInfo(machine_id).cpu; 
    switch (cpu) {
        case X86:
            list = x86Machines; 
            break; 
        case ARM:
            list = armMachines;
            break;
        case POWER:
            list = powerMachines;
            break;
        default:
            list = riscvMachines; 
            break;
    }    

    unsigned listSize = list.size(); 
    for(int i = 0; i < machine_vms.size(); i++) {
        for(int j = 0; j < listSize; j++) {
            if( list.at(j) != machine_id 
                && Machine_GetInfo(list.at(j)).s_state == S0
                && std::find(turningOff.begin(), turningOff.end(), list.at(j)) == turningOff.end()
                && !isMigrating[machine_vms.at(i)] && VM_GetInfo(machine_vms.at(i)).active_tasks.size() != 0
                && hasEnoughResource(list.at(j), VM_GetInfo(machine_vms.at(i)).active_tasks.at(0))) {
                remainingMips[machine_id] += 1000;
                remainingMips[list.at(j)] -= 1000;
                remainingMemory[machine_id] += GetTaskMemory(VM_GetInfo(machine_vms.at(i)).active_tasks.at(0)); 
                remainingMemory[list.at(j)] -= GetTaskMemory(VM_GetInfo(machine_vms.at(i)).active_tasks.at(0)); 
                numTasks[machine_id]--; 
                if(numTasks[machine_id] == 0) {
                    switch(cpu) {
                        case X86:
                            activex86--; 
                            break;
                        case ARM:
                            activeArm--;
                            break;
                        case POWER:
                            activePower--;
                            break; 
                        case RISCV:
                            activeRiscv--;
                            break;
                        default:
                            break; 
                    }
                }
                if(numTasks[list.at(j)] == 0) {
                    switch(cpu) {
                        case X86:
                            activex86++; 
                            break;
                        case ARM:
                            activeArm++;
                            break;
                        case POWER:
                            activePower++;
                            break; 
                        case RISCV:
                            activeRiscv++;
                            break;
                        default:
                            break; 
                    }
                }
                numTasks[list.at(j)]++;
                updateMachines(cpu); 
                VM_Migrate(machine_vms.at(i), list.at(j)); 
                isMigrating[machine_vms.at(i)] = true; 
                j = list.size(); 
            }
        }
    }
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
    SimOutput("Machine " + to_string(machine_id) + " has changed to state " + to_string(Machine_GetInfo(machine_id).s_state), 4); 
    if(Machine_GetInfo(machine_id).s_state != S0) {
        for(int i = 0; i < turningOff.size(); i++) {
            if(turningOff.at(i) == machine_id) {
                turningOff.erase(turningOff.begin() + i); 
                i = turningOff.size(); 
            }
        } 
    }
}

unsigned insert_sorted_ee(vector<MachineId_t>* mList, MachineId_t id) {

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

/* Mayber Update to Use Tresholds */
bool hasEnoughResource(MachineId_t mid, TaskId_t tid) {
    MachineInfo_t minfo = Machine_GetInfo(mid); 
    TaskInfo_t tinfo = GetTaskInfo(tid); 

    unsigned mipsRequired = 1000; 
    if(remainingMips[mid] < mipsRequired) {
        return false; 
    }

    unsigned remaining_memory = remainingMemory[mid]; 
    if(remaining_memory < tinfo.required_memory) {
        return false; 
    }

    return true; 
}

unsigned estimatedPower(MachineId_t mid, TaskId_t tid) {
    
    MachineInfo_t minfo = Machine_GetInfo(mid);
    TaskInfo_t tinfo = GetTaskInfo(tid); 

    int mipsUsed = minfo.p_states.at(0) - remainingMips[mid];
    mipsUsed += 1000;
    unsigned energy; 

    if(minfo.s_states.size() == 0) {
        /* It seems like the s_states aren't populating in some situations */
        energy = 0; 
    } else {
        energy = minfo.s_states.at(0); 
    }
    

    for(int i = 3; i >= 0; i--) {
        if(minfo.performance[i] >= mipsUsed) {           
            energy += minfo.p_states[i]; 
            break; 
        }
    }

    return energy; 
}

/* Turn machines on/off to save energy */
void updateMachines(CPUType_t type) {
    double tresholdS0 = 0.46;
    vector<MachineId_t> list;
    unsigned active = 0; 
    switch(type) {
        case X86:
            list = x86Machines; 
            active = activex86; 
            break;
        case ARM:
            list = armMachines; 
            active = activeArm; 
            break;
        case POWER:
            list = powerMachines; 
            active = activePower; 
            break; 
        case RISCV:
            list = riscvMachines;
            active = activeRiscv; 
            break; 
        default:
            break; 
    } 

    unsigned size = list.size();
    unsigned totalInactive = list.size() - active; 
    unsigned inactiveNum = 0; 
    for(int i = 0; i < size; i++) {
        MachineId_t mid = list.at(i);
        MachineInfo_t minfo = Machine_GetInfo(mid); 
        if(size - active <= 4) {
            if(minfo.s_state != S0) {
                Machine_SetState(mid, S0);
            }
        } else {
            if(numTasks[mid] == 0) {
                if(inactiveNum < totalInactive * tresholdS0) {
                    if(minfo.s_state != S0) {
                        Machine_SetState(mid, S0);
                    }
                } else {
                    if(minfo.s_state != S1) {
                        Machine_SetState(mid, S1); 
                        turningOff.push_back(mid); 
                    }
                }
                inactiveNum++; 
            }
        }
    }
}
