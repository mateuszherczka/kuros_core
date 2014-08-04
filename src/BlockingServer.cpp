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
        cout << "Can't send because server not connected." << endl;
        startListening();

        return;
    }

    streambuf_ptr message(new boost::asio::streambuf);
    KukaCommand command;
    command.formatTrajectory(*message, info, trajectory);

    messageQueue.push(message);

    // block until trajectoryPending is false again.
    setPending(true);
    //pendingON();
//    boost::unique_lock<boost::mutex> lock(sendBlockMutex);
    blockWhilePending();

//    while (isPending())
//    {
//        boost::this_thread::sleep( boost::posix_time::milliseconds(10));
//    }

    cout << "blockSend exiting." << endl;
}

void BlockingServer::blockWhilePending()
{
    std::unique_lock<std::mutex> lk(sendBlockMutex);
    sendBlockCondition.wait(lk,[this] {return !trajectoryPending;});
}

void BlockingServer::setPending(bool onoff)
{
    //boost::lock_guard<boost::mutex> lock(sendBlockMutex);
    std::lock_guard<std::mutex> lk(sendBlockMutex);
    trajectoryPending=onoff;
}

bool BlockingServer::isPending()
{
    //boost::lock_guard<boost::mutex> lock(sendBlockMutex);
    std::lock_guard<std::mutex> lk(sendBlockMutex);
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
        setPending(false);
        sendBlockCondition.notify_one();
    }

}

void BlockingServer::closeConnection(socket_ptr sock)
{
    if (!isConnected())
    {
        cerr << "BlockingServer trying to close connection but not connected." << endl;
        //return;
    }

    cout << "BlockingServer closing connection." << endl;

    //cout << "BlockingServer closeConnection setting to false." << endl;
    setConnected(false);  // threadsafe setting of connected = false

    //cout << "BlockingServer closeConnection breaking pending ." << endl;
    // break wait for pending trajectory
    // TODO: crashes if done after socket close, boost bug?
//    {
//            boost::lock_guard<boost::mutex> lock(sendBlockMutex);
//            trajectoryPending=false;
//            //robotState = 0;
//    }
    //pendingOFF();
    setPending(false);
    sendBlockCondition.notify_one();

    //cout << "BlockingServer stopping queues." << endl;
    // stop queues, which should terminate write and response threads
    messageQueue.reject();
    responseQueue.reject();

    cout << "BlockingServer closeConnection shutting down socket." << endl;

    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);    // recommended by boost

    //cout << "BlockingServer closeConnection trying to close socket." << endl;
    //sock->close();

    //cout << "BlockingServer closeConnection resetting socket and calling handleDisconnect." << endl;
    //sock.reset();           // deallocate socket

    cout << "BlockingServer closed connection." << endl;
    handleDisconnect();     // user can do something
}
