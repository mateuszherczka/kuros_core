#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/locks.hpp>
//#include <boost/thread/lock_guard.hpp>

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

        /*
        handleResponse() is called from a separate response thread.
        There is a queue between the socket and this call. Still, handleResponse()
        blocks the thread until return. If you expect a 12ms stream of responses,
        it's probably a good idea to deal with the response swiftly.
        */
        virtual void handleResponse(const KukaResponse &response) = 0; // This is mandatory to implement in derived class
        virtual void handleDisconnect() = 0; // This is mandatory to implement in derived class

        // ---------------------------------------------------------------------------
        // Methods
        // ---------------------------------------------------------------------------

        // void sendPose(const info_vec &info, const frame_vec &frame); // deprecated

        void sendTrajectory(const info_vec &info, const trajectory_vec &trajectory);

        virtual void startListening();  // can be overridden

        void loadConfig();

        bool isConnected();

        bool busy();

        //bool sendQueueEmpty();

    protected:

        ThreadSafeQueue<streambuf_ptr> messageQueue;
        ThreadSafeQueue<streambuf_ptr> responseQueue;

        void setReadThreadAlive(bool onoff);
        void setWriteThreadAlive(bool onoff);
        void setResponseThreadAlive(bool onoff);

        virtual void closeConnection(socket_ptr sock);

        /*
        Helpers for threadsafe setting of the connected flag.
        */
        void setConnectionOFF();
        void setConnectionON();
        void setConnected(bool onoff);

        void resetData();

    private:

        // ---------------------------------------------------------------------------
        // Data
        // ---------------------------------------------------------------------------

        bool connected = false;         // breaks read,write and onresponse loops when set to false
        /*
        This server allows only one connenction at a time.
        We want to make sure all threads are dead before allowing next connection.
        */
        bool readThreadAlive = false;
        bool writeThreadAlive = false;
        bool responseThreadAlive = false;

        int invalidParseCount = 0;      // handy counters
        int readMessageCount = 0;
        int writtenMessageCount = 0;

        // XML parsers
        ServerConfig serverConfig;

        mutable std::mutex connectedMutex;
        mutable std::mutex cleanupMutex;

        //boost::mutex connectedMutex;
        //boost::mutex cleanupMutex;
        //boost::mutex startupMutex;

        // ---------------------------------------------------------------------------
        // Private Methods
        // ---------------------------------------------------------------------------

        /*
        Specify port. Blocks until incoming connection.
        When connected, spawns read/write threads, then exits.
        */
        void startListening(unsigned short port);

        /*
        Specify socket pointer.
        Blocks until connection ends.
        */
        void onResponse(socket_ptr sock);

        /*
        To be overridden by a derived class.
        Provides a way for server to take internal action
        on responses, in addition to calling handleResponse().
        */
        virtual void callResponseMethods(const KukaResponse &response);

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

