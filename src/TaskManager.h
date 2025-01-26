#pragma once

#include "MeshNode.h"

class TaskManager {
public:
    TaskManager(MeshNode& node);
    void init();
    void execute();

private:
    MeshNode& meshNode;
    Task taskCheckServer;
    Task taskBridgeHeartbeat;
    Task taskLogTopology;

    void setupTasks();
};