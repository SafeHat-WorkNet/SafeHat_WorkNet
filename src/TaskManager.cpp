#include "TaskManager.h"

TaskManager::TaskManager(MeshNode& node) : meshNode(node),
    taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, std::bind(&MeshNode::logTopology, &meshNode)) {
}

void TaskManager::init() {
    setupTasks();
}

void TaskManager::execute() {
    MeshNode::getScheduler().execute();
}

void TaskManager::setupTasks() {
    MeshNode::getScheduler().addTask(taskLogTopology);
    taskLogTopology.enable();
} 