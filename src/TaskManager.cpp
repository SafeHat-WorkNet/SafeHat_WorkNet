#include "TaskManager.h"

// Initialize static member
MeshNode* TaskManager::currentNode = nullptr;

// Static callbacks
void TaskManager::checkServerCallback() {
    if (currentNode) currentNode->checkServer();
}

void TaskManager::logTopologyCallback() {
    if (currentNode) currentNode->logTopology();
}

TaskManager::TaskManager(MeshNode& node) : 
    taskCheckServer(CHECK_SERVER_INTERVAL, TASK_FOREVER, &checkServerCallback),
    taskLogTopology(LOG_TOPOLOGY_INTERVAL, TASK_FOREVER, &logTopologyCallback)
{
    currentNode = &node;
    
    // Add tasks to scheduler
    scheduler.addTask(taskCheckServer);
    scheduler.addTask(taskLogTopology);
    
    // Enable tasks
    taskCheckServer.enable();
    taskLogTopology.enable();
}

void TaskManager::update() {
    scheduler.execute();
} 