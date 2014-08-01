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
    if (!isConnected())
    {
        cerr << "Can't blockSend because BlockingServer not connected to anything." << endl;
        return;
    }

    streambuf_ptr message(new boost::asio::streambuf);
    KukaCommand command;
    command.formatTrajectory(*message, info, trajectory);

    messageQueue.push(message);

    // block until trajectoryPending is false again.
    pendingON();
    boost::unique_lock<boost::mutex> lock(sendBlockMutex);
    while(trajectoryPending)
    {
        sendBlockCondition.wait(lock);
    }

    cout << "blockSend exiting." << endl;
}

void BlockingServer::pendingON()
{
    boost::lock_guard<boost::mutex> lock(sendBlockMutex);
    trajectoryPending=true;
}

void BlockingServer::pendingOFF()
{
    boost::lock_guard<boost::mutex> lock(sendBlockMutex);
    trajectoryPending=false;
}

bool BlockingServer::isPending()
{
    boost::lock_guard<boost::mutex> lock(sendBlockMutex);
    return trajectoryPending;
}

void BlockingServer::callResponseMethods(const KukaResponse &response)
{
    // finally call user response
    handleResponse(response);

    // if finished with a pending trajectory, unblock send thread
    trajectoryDone(response);

    // TODO: which one of these come first?
}


void BlockingServer::trajectoryDone(const KukaResponse &response)
{
    if (isPending() && response.info[KUKA_RSP_STATUS]==KUKA_TRAJ_DONE)
    {
        pendingOFF();
        sendBlockCondition.notify_one();
    }

}

void BlockingServer::closeConnection(socket_ptr sock)
{
    connectionOFF();  // threadsafe setting of connected = false

     //break wait for pending trajectory
    {
            boost::lock_guard<boost::mutex> lock(sendBlockMutex);
            trajectoryPending=false;
            robotState = 0;
    }
    sendBlockCondition.notify_all();


    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_send);    // recommended by boost
    sock->close();

    // stop queues, which should terminate write and response threads
    messageQueue.reject();
    responseQueue.reject();

    sock.reset();           // deallocate socket
    handleDisconnect();     // user can do something

    cout << "BlockingServer closed connection." << endl;
}
