#pragma once

#include <painlessMesh.h>
#include "MeshNode.h"

class TaskManager {
public:
    TaskManager(MeshNode& node);
    void init();
    void execute();

private:
    MeshNode& meshNode;
    Scheduler scheduler;
    
    Task taskSendMessage;
    Task taskCheckServer;
    Task taskBridgeForward;
    Task taskLogTopology;
    
    void setupTasks();
};