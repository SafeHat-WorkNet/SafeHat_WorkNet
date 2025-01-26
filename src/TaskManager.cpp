#include "TaskManager.h"

TaskManager::TaskManager(MeshNode& node) : 
    meshNode(node),
    taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, [this]() {
        String msg = "Hello from " + meshNode.getNodeName();
        MeshNode::getMesh().sendBroadcast(msg);
    }),
    taskCheckServer(TASK_SECOND * 5, TASK_FOREVER, [this]() {
        meshNode.checkServer();
    }),
    taskBridgeForward(TASK_SECOND * 10, TASK_FOREVER, [this]() {
        meshNode.sendBridgeHeartbeat();
    }),
    taskLogTopology(TASK_SECOND * 30, TASK_FOREVER, [this]() {
        meshNode.logTopology();
    })
{}

void TaskManager::init() {
    scheduler.addTask(taskSendMessage);
    scheduler.addTask(taskCheckServer);
    scheduler.addTask(taskBridgeForward);
    scheduler.addTask(taskLogTopology);

    taskSendMessage.enable();
    taskCheckServer.enable();
    taskBridgeForward.enable();
    taskLogTopology.enable();
}

void TaskManager::execute() {
    scheduler.execute();
} 