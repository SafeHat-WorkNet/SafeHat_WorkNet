#pragma once

#include <TaskSchedulerDeclarations.h>
#include "MeshNode.h"

class TaskManager {
public:
    static constexpr unsigned long CHECK_SERVER_INTERVAL = 5000;  // 5 seconds
    static constexpr unsigned long LOG_TOPOLOGY_INTERVAL = 30000; // 30 seconds

    TaskManager(MeshNode& node);
    void update();

private:
    static void checkServerCallback();
    static void logTopologyCallback();
    
    static MeshNode* currentNode;  // Static pointer to current node instance
    Scheduler scheduler;
    
    // Tasks
    Task taskCheckServer;
    Task taskLogTopology;
};