#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <ThreadSafeQueue.hpp>

#include <types.hpp>

#include <ServerConfig.hpp>
#include <KukaResponse.hpp>
#include <KukaCommand.hpp>

using boost::asio::ip::tcp;
using boost::thread;

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::asio::streambuf> streambuf_ptr;

// ---------------------------------------------------------------------------
// Class
// ---------------------------------------------------------------------------

class Server
{
    public:

        // ---------------------------------------------------------------------------
        // Constructor / Destructor
        // ---------------------------------------------------------------------------

        Server();

        virtual ~Server();

        // ---------------------------------------------------------------------------
        // Pure Virtual
        // ---------------------------------------------------------------------------

        virtual void handleResponse() = 0; // This is mandatory to implement in derived class
        virtual void handleDisconnect() = 0; // This is mandatory to implement in derived class

        // ---------------------------------------------------------------------------
        // Methods
        // ---------------------------------------------------------------------------

        void sendPose(const info_vec &info, const frame_vec &frame);

        void sendTrajectory(const info_vec &info, const trajectory_vec &trajectory);

        void startListening();

        void loadConfig();

        void closeConnection();
        void closeConnection(socket_ptr sock);

        bool isConnected();

        bool sendQueueEmpty();

    protected:

        KukaResponse response;  // accesible to derived class which implements handleResponse

    private:

        // ---------------------------------------------------------------------------
        // Data
        // ---------------------------------------------------------------------------

        bool connected = false;         // breaks read,write and onresponse loops when set to false

        int invalidParseCount = 0;      // handy counters
        int readMessageCount = 0;
        int writtenMessageCount = 0;

        // XML parsers
        ServerConfig serverConfig;
        KukaCommand command;

        ThreadSafeQueue<streambuf_ptr> messageQueue;
        ThreadSafeQueue<streambuf_ptr> responseQueue;

        // ---------------------------------------------------------------------------
        // Methods
        // ---------------------------------------------------------------------------

        /*
        Blocks while connected.
        */
        void onResponse(socket_ptr sock);

        /*
        Specify port. Blocks until incoming connection.
        When connected, spawns read/write threads, then exits.
        */
        void startListening(unsigned short port);

        /*
        Specify socket pointer.
        Blocks until connection ends.
        */
        void readMessage(socket_ptr sock);

        /*
        Specify socket pointer.
        Blocks until connection ends.
        */
        void writeMessage(socket_ptr sock);

        /*
        Returns a pointer to buffer inside streambuf.
        */
        const char * streambufToPtr(boost::asio::streambuf &message);


};

#endif // SERVER_H

