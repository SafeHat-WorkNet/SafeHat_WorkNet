#include "TaskManager.h"

TaskManager::TaskManager(MeshNode& node) : meshNode(node),
    taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, std::bind(&MeshNode::checkServer, &meshNode)),
    taskBridgeHeartbeat(TASK_SECOND * 1, TASK_FOREVER, std::bind(&MeshNode::sendBridgeHeartbeat, &meshNode)),
    taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, std::bind(&MeshNode::logTopology, &meshNode)) {
}

void TaskManager::init() {
    setupTasks();
}

void TaskManager::execute() {
    MeshNode::getScheduler().execute();
}

void TaskManager::setupTasks() {
    MeshNode::getScheduler().addTask(taskCheckServer);
    MeshNode::getScheduler().addTask(taskBridgeHeartbeat);
    MeshNode::getScheduler().addTask(taskLogTopology);

    taskCheckServer.enable();
    taskBridgeHeartbeat.enable();
    taskLogTopology.enable();
} 