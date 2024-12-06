//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <unordered_map>

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

vector<unsigned> x86Tasks; 
vector<unsigned> armTasks;
vector<unsigned> powerTasks;
vector<unsigned> riscvTasks; 

int x86QuarterSize; 
int armQuarterSize; 
int powerQuarterSize; 
int riscvQuarterSize; 

unsigned x86ActiveQuarter = 1; 
unsigned armActiveQuarter = 1; 
unsigned powerActiveQuarter = 1; 
unsigned riscvActiveQuarter = 1;

std::unordered_map<MachineId_t, vector<VMId_t>> vmMap;
std::unordered_map<TaskId_t, MachineId_t> taskMap;

unsigned checkTreshold = 1000; 
unsigned checks = 0; 

unsigned insert_sorted_ee(vector<MachineId_t>* mList, MachineId_t id);

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

    /* Find the number of each machine available */
    for(unsigned i = 0; i < total_machines; i++) {
        switch (Machine_GetCPUType(MachineId_t(i))) {
            case X86:
                numX86Machines++; 
                x86Tasks.insert(x86Tasks.begin() + insert_sorted_ee(&x86Machines, i), 0);
                break; 
            case ARM:
                numArmMachines++; 
                armTasks.insert(armTasks.begin() + insert_sorted_ee(&armMachines, i), 0);  
                break; 
            case POWER:
                numPowerMachines++;
                powerTasks.insert(powerTasks.begin() + insert_sorted_ee(&powerMachines, i), 0); 
                break; 
            default:
                numRiscvMachines++;
                riscvTasks.insert(riscvTasks.begin() + insert_sorted_ee(&riscvMachines, i), 0);  
                break; 
        }
    }

    if(x86Machines.size() >= 4) {
        x86QuarterSize = x86Machines.size() / 4;
    } else {
        x86QuarterSize = -1; 
    }

    if(armMachines.size() >= 4) {
        armQuarterSize = armMachines.size() / 4;
    } else {
        armQuarterSize = -1; 
    }

    if(powerMachines.size() >= 4) {
        powerQuarterSize = powerMachines.size() / 4;
    } else {
        powerQuarterSize = -1; 
    }

    if(riscvMachines.size() >= 4) {
        riscvQuarterSize = riscvMachines.size() / 4;
    } else {
        riscvQuarterSize = -1; 
    }

    /* Turn on a quarter of all machine types, for the others turn them off*/
    if(x86QuarterSize == -1) {
        for(int i = 0; i < x86Machines.size(); i++) {
            Machine_SetState(x86Machines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
            }
        }
    } else {
        for(int i = 0; i < x86QuarterSize; i++) {
            Machine_SetState(x86Machines.at(i), S0);
            for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
            }
        }

        for(int i = x86QuarterSize; i < 2 * x86QuarterSize; i++) {
            Machine_SetState(x86Machines.at(i), S1); 
        }

        for(int i = 2 * x86QuarterSize; i < 3 * x86QuarterSize; i++) {
            Machine_SetState(x86Machines.at(i), S3); 
        }

        for(int i = 3 * x86QuarterSize; i < x86Machines.size(); i++) {
            Machine_SetState(x86Machines.at(i), S5); 
        }
    }

    if(armQuarterSize == -1) {
        for(int i = 0; i < armMachines.size(); i++) {
            Machine_SetState(armMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(armMachines.at(i), j, P0); 
            }
        }
    } else {
        for(int i = 0; i < armQuarterSize; i++) {
            Machine_SetState(armMachines.at(i), S0);
            for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(armMachines.at(i), j, P0); 
            }
        }

        for(int i = armQuarterSize; i < 2 * armQuarterSize; i++) {
            Machine_SetState(armMachines.at(i), S1); 
        }

        for(int i = 2 * armQuarterSize; i < 3 * armQuarterSize; i++) {
            Machine_SetState(armMachines.at(i), S3); 
        }

        for(int i = 3 * armQuarterSize; i < armMachines.size(); i++) {
            Machine_SetState(armMachines.at(i), S5); 
        }
    }

    if(powerQuarterSize == -1) {
        for(int i = 0; i < powerMachines.size(); i++) {
            Machine_SetState(powerMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
            }
        }
    } else {
        for(int i = 0; i < powerQuarterSize; i++) {
            Machine_SetState(powerMachines.at(i), S0);
            for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
            }
        }

        for(int i = powerQuarterSize; i < 2 * powerQuarterSize; i++) {
            Machine_SetState(powerMachines.at(i), S1); 
        }

        for(int i = 2 * powerQuarterSize; i < 3 * powerQuarterSize; i++) {
            Machine_SetState(powerMachines.at(i), S3); 
        }

        for(int i = 3 * powerQuarterSize; i < powerMachines.size(); i++) {
            Machine_SetState(powerMachines.at(i), S5); 
        }
    }

    if(riscvQuarterSize == -1) {
        for(int i = 0; i < riscvMachines.size(); i++) {
            Machine_SetState(riscvMachines.at(i), S0); 
            for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
            }
        }
    } else {
        for(int i = 0; i < riscvQuarterSize; i++) {
            Machine_SetState(riscvMachines.at(i), S0);
            for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
            }
        }

        for(int i = riscvQuarterSize; i < 2 * riscvQuarterSize; i++) {
            Machine_SetState(riscvMachines.at(i), S1); 
        }

        for(int i = 2 * riscvQuarterSize; i < 3 * riscvQuarterSize; i++) {
            Machine_SetState(riscvMachines.at(i), S3); 
        }

        for(int i = 3 * riscvQuarterSize; i < riscvMachines.size(); i++) {
            Machine_SetState(riscvMachines.at(i), S5); 
        }
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
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
    checks++; 

    /* Figure out which list of machines to use */
    vector<MachineId_t> list;
    vector<unsigned> *taskList; 
    int quarter; 
    int quarterSize; 
    switch (cpu) {
        case X86:
            list = x86Machines;
            taskList = &x86Tasks; 
            quarter = x86ActiveQuarter; 
            quarterSize = x86QuarterSize;  
            break; 
        case ARM:
            list = armMachines; 
            taskList = &armTasks; 
            quarter = armActiveQuarter; 
            quarterSize = armQuarterSize; 
            break;
        case POWER:
            list = powerMachines; 
            taskList = &powerTasks; 
            quarter = powerActiveQuarter; 
            quarterSize = powerQuarterSize; 
            break;
        default:
            list = riscvMachines; 
            taskList = &riscvTasks; 
            quarter = riscvActiveQuarter; 
            quarterSize = riscvQuarterSize; 
            break;
    }

    /* Go through the list and find the machine with the fewest tasks */
    uint32_t fewest = INT32_MAX;
    MachineId_t fewestIndex = 0;
    if(quarterSize != -1) {
        for(int i = 0; i < quarter * quarterSize; i++) {
            if((*taskList).at(i) < fewest && Machine_GetInfo(list.at(i)).s_state == S0) {
                fewest = (*taskList).at(i); 
                fewestIndex = i;
            }
        }
    } else {
        for(int i = 0; i < list.size(); i++) {
            if((*taskList).at(i) < fewest && Machine_GetInfo(list.at(i)).s_state == S0) {
                fewest = (*taskList).at(i); 
                fewestIndex = i;
            }
        }
    }

    (*taskList).at(fewestIndex) = (*taskList).at(fewestIndex) + 1;
    MachineId_t mid = list.at(fewestIndex); 
    taskMap[task_id] = mid;

    /* Add the task to the selected machine */
    if(vmMap.find(fewestIndex) == vmMap.end()) {
        /* Machine has no VMs */
        vmMap[mid] = {}; 
        VMId_t vid = VM_Create(info.required_vm, info.required_cpu); 
        vms.push_back(vid);
        vmMap[mid].push_back(vid); 
        VM_Attach(vid, mid); 
        VM_AddTask(vid, task_id, info.priority); 
        return; 
    } else {
        vector<VMId_t> vmIds = vmMap[mid];
        for(int i = 0; i < vmIds.size(); i++) {
            VMInfo_t vInfo = VM_GetInfo(vmIds.at(i));
            if(vInfo.vm_type == info.required_vm) {
                VM_AddTask(vmIds.at(i), task_id, info.priority); 
                return; 
            }
        }

        /* Required VM is not present */
        VMId_t vid = VM_Create(info.required_vm, info.required_cpu); 
        vms.push_back(vid); 
        VM_Attach(vid, mid); 
        VM_AddTask(vid, task_id, info.priority); 
        return; 
    }
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    checks++; 
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

    // SimOutput("Finishing Task " + to_string(task_id), 1); 

    TaskInfo_t info = GetTaskInfo (task_id); 
    CPUType_t cpu = info.required_cpu; 
    VMType_t os = info.required_vm; 

    // SimOutput("Task Completed", 0); 

    /* Figure out which list of machines to use */
    vector<MachineId_t> list; 
    vector<unsigned> *taskList; 
    switch (cpu) {
        case X86:
            list = x86Machines; 
            taskList = &x86Tasks; 
            break; 
        case ARM:
            list = armMachines;
            taskList = &armTasks;  
            break;
        case POWER:
            list = powerMachines;
            taskList = &powerTasks;  
            break;
        default:
            list = riscvMachines; 
            taskList = &riscvTasks; 
            break;
    }

    /* Reduce Task Count for Machine */
    MachineId_t mid = taskMap[task_id]; 
    for(int i = 0; i < list.size(); i++) {
        if(list.at(i) == mid) {
            (*taskList).at(i) = (*taskList).at(i) - 1; 
            break; 
        }
    }
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 1);
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
    // SimOutput("Trying to activate more machine with checks " + to_string(checks), 0); 
    checks++;
    /* Expand number of machines if check treshold has been passed */
    if(checks >= checkTreshold) {
        // SimOutput("Activiating New Section of Machines!", 0); 
        MachineInfo_t info = Machine_GetInfo(machine_id);
        checks = 0; 
        switch (info.cpu) {
            case X86:
                x86ActiveQuarter = (x86ActiveQuarter < 4) ? x86ActiveQuarter + 1 : 4; 
                if(x86ActiveQuarter == 2) {
                    for(int i = x86QuarterSize; i < x86QuarterSize * 2; i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                } 
                if(x86ActiveQuarter == 3) {
                    for(int i = x86QuarterSize * 2; i < x86QuarterSize * 3; i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                }
                if(x86ActiveQuarter == 4) {
                    for(int i = x86QuarterSize * 3; i < x86Machines.size(); i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                }
                break; 
            case ARM:
                armActiveQuarter = (armActiveQuarter < 4) ? armActiveQuarter + 1 : 4; 
                if(armActiveQuarter == 2) {
                    for(int i = armQuarterSize; i < armQuarterSize * 2; i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(armActiveQuarter == 3) {
                    for(int i = armQuarterSize * 2; i < armQuarterSize * 3; i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                }
                if(armActiveQuarter == 4) {
                    for(int i = armQuarterSize * 3; i < armMachines.size(); i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                }
                break;
            case POWER:
                powerActiveQuarter = (powerActiveQuarter < 4) ? powerActiveQuarter + 1 : 4; 
                checks = 0;
                if(powerActiveQuarter == 2) {
                    for(int i = powerQuarterSize; i < powerQuarterSize * 2; i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(powerActiveQuarter == 3) {
                    for(int i = powerQuarterSize * 2; i < powerQuarterSize * 3; i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                }
                if(powerActiveQuarter == 4) {
                    for(int i = powerQuarterSize * 3; i < powerMachines.size(); i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                }
                break; 
            default:
                riscvActiveQuarter = (riscvActiveQuarter < 4) ? riscvActiveQuarter + 1 : 4; 
                if(riscvActiveQuarter == 2) {
                    for(int i = riscvQuarterSize; i < riscvQuarterSize * 2; i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(riscvActiveQuarter == 3) {
                    for(int i = riscvQuarterSize * 2; i < riscvQuarterSize * 3; i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                }
                if(riscvActiveQuarter == 4) {
                    for(int i = riscvQuarterSize * 3; i < riscvMachines.size(); i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                }
                checks = 0; 
        }
    }
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
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

    MachineId_t machine_id = taskMap[task_id]; 
    // SimOutput("Trying to activate more machine with checks " + to_string(checks), 0); 
    checks++; 
    /* Expand number of machines if check treshold has been passed */
    if(checks >= checkTreshold) {
        // SimOutput("Activiating New Section of Machines!", 0);
        MachineInfo_t info = Machine_GetInfo(machine_id);
        checks = 0; 
                switch (info.cpu) {
            case X86:
                x86ActiveQuarter = (x86ActiveQuarter < 4) ? x86ActiveQuarter + 1 : 4; 
                if(x86ActiveQuarter == 2) {
                    for(int i = x86QuarterSize; i < x86QuarterSize * 2; i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                } 
                if(x86ActiveQuarter == 3) {
                    for(int i = x86QuarterSize * 2; i < x86QuarterSize * 3; i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                }
                if(x86ActiveQuarter == 4) {
                    for(int i = x86QuarterSize * 3; i < x86Machines.size(); i++) {
                        Machine_SetState(x86Machines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(x86Machines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(x86Machines.at(i), j, P0); 
                        }
                    }
                }
                break; 
            case ARM:
                armActiveQuarter = (armActiveQuarter < 4) ? armActiveQuarter + 1 : 4; 
                if(armActiveQuarter == 2) {
                    for(int i = armQuarterSize; i < armQuarterSize * 2; i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(armActiveQuarter == 3) {
                    for(int i = armQuarterSize * 2; i < armQuarterSize * 3; i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                }
                if(armActiveQuarter == 4) {
                    for(int i = armQuarterSize * 3; i < armMachines.size(); i++) {
                        Machine_SetState(armMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(armMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(armMachines.at(i), j, P0); 
                        }
                    }
                }
                break;
            case POWER:
                powerActiveQuarter = (powerActiveQuarter < 4) ? powerActiveQuarter + 1 : 4; 
                checks = 0;
                if(powerActiveQuarter == 2) {
                    for(int i = powerQuarterSize; i < powerQuarterSize * 2; i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(powerActiveQuarter == 3) {
                    for(int i = powerQuarterSize * 2; i < powerQuarterSize * 3; i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                }
                if(powerActiveQuarter == 4) {
                    for(int i = powerQuarterSize * 3; i < powerMachines.size(); i++) {
                        Machine_SetState(powerMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(powerMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(powerMachines.at(i), j, P0); 
                        }
                    }
                }
                break; 
            default:
                riscvActiveQuarter = (riscvActiveQuarter < 4) ? riscvActiveQuarter + 1 : 4; 
                if(riscvActiveQuarter == 2) {
                    for(int i = riscvQuarterSize; i < riscvQuarterSize * 2; i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                } 
                if(riscvActiveQuarter == 3) {
                    for(int i = riscvQuarterSize * 2; i < riscvQuarterSize * 3; i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                }
                if(riscvActiveQuarter == 4) {
                    for(int i = riscvQuarterSize * 3; i < riscvMachines.size(); i++) {
                        Machine_SetState(riscvMachines.at(i), S0);
                        for(int j = 0; j < Machine_GetInfo(riscvMachines.at(i)).num_cpus; j++) {
                            Machine_SetCorePerformance(riscvMachines.at(i), j, P0); 
                        }
                    }
                }
                checks = 0; 
        }
    }
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
    // SimOutput("Machine " + to_string(machine_id) + " has changed to state " + to_string(Machine_GetInfo(machine_id).s_state), 0); 
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
