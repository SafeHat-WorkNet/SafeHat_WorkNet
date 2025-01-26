#pragma once

#include "MeshNode.h"

class TaskManager {
public:
    TaskManager(MeshNode& node);
    void init();
    void execute();

private:
    MeshNode& meshNode;

    // Task objects
    Task taskSendMessage;
    Task taskCheckServer;
    Task taskBridgeHeartbeat;
    Task taskLogTopology;
    Task taskBroadcastNodeData; // New task to broadcast node data

    void setupTasks();
};
