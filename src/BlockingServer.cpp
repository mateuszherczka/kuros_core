#include "BlockingServer.hpp"

BlockingServer::BlockingServer()
{
    //ctor
}

BlockingServer::~BlockingServer()
{
    //dtor
}

void BlockingServer::blockSendTrajectory(const info_vec &info, const trajectory_vec &trajectory)
{

    boost::unique_lock<boost::mutex> lock(sendBlockMutex);

    trajectoryPending = true;   // TODO: should this be inside?

    streambuf_ptr message(new boost::asio::streambuf);
    KukaCommand command;
    command.formatTrajectory(*message, info, trajectory);

    messageQueue.push(message);

    // block until trajectoryPending is false again.
    while(trajectoryPending)
    {
        sendBlockCondition.wait(lock);
    }

}

void BlockingServer::callResponseMethods(const KukaResponse &response)
{
    // if finished with a pending trajectory, unblock send thread
    trajectoryDone(response);

    // finally call user response
    handleResponse(response);
}

void BlockingServer::trajectoryDone(const KukaResponse &response)
{
    if (trajectoryPending==true && response.info[KUKA_RSP_STATUS]==KUKA_TRAJ_DONE)
    {
        boost::lock_guard<boost::mutex> lock(sendBlockMutex);
        trajectoryPending=false;
    }
    sendBlockCondition.notify_one();
}
