#ifndef BLOCKINGSERVER_HPP
#define BLOCKINGSERVER_HPP

#include <Server.hpp>


#include <boost/thread.hpp>

class BlockingServer : public Server
{

public:
    BlockingServer();
    virtual ~BlockingServer();

    void blockSendTrajectory(const info_vec &info, const trajectory_vec &trajectory);

protected:

private:

    bool trajectoryPending = false;
    boost::condition_variable sendBlockCondition;
    boost::mutex sendBlockMutex;

    int robotState = 0;

    void callResponseMethods(const KukaResponse &response) override;    // hiding parent function

    void trajectoryDone(const KukaResponse &response);

};

#endif // BLOCKINGSERVER_HPP
