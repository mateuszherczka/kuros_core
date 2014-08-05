#include "Server.hpp"

Server::Server()
{
    loadConfig();
}

Server::~Server()
{

}

void Server::sendTrajectory(const info_vec &info, const trajectory_vec &trajectory)
{
    if (!accepting)
    {
        cout << "Can't send because server not connected." << endl;
        return;
    }

    streambuf_ptr message(new boost::asio::streambuf);
    KukaCommand command;
    command.formatTrajectory(*message, info, trajectory);   // first: infovector<int>, second: framevector<int>

    //cout << "sendTrajectory about to push a message." << endl;
    messageQueue.push(message);

    //cout << "sendTrajectory pushed message." << endl;
}

void Server::startListening()
{

    while (busy())
    {
        cout << "Waiting for server to reset." << endl;
        boost::this_thread::sleep( boost::posix_time::milliseconds(1000));
    }
    boost::this_thread::sleep( boost::posix_time::milliseconds(500));

    startListening(serverConfig.getPort());

}

void Server::loadConfig()
{

    serverConfig.load();
    cout << "Port MaxBufferSize EndString:" << endl;
    serverConfig.printValues();
}

void Server::closeConnection()
{
//    if (!isConnected())
//    {
//        cerr << "Server trying to close connection but not connected." << endl;
//        //return;
//    }

    cout << "Server closing connection." << endl;

    //cout << "Server no longer accepting." << endl;
    accepting = false;      // atomic for external use

    //cout << "CloseConnection setting to false." << endl;
    setConnected(false);  // threadsafe setting of connected = false

    //cout << "CloseConnection stopping queues." << endl;
    messageQueue.reject();  // stop queues, which should terminate write and response threads
    responseQueue.reject();

    handleDisconnect();     // user can do something
    //cout << "CloseConnection shutting down socket." << endl;
    //sock->shutdown(boost::asio::ip::tcp::socket::shutdown_send);    // recommended by boost
    //sock->close();
    //sock.reset();           // deallocate socket
    //cout << "Server closed connection." << endl;
    // just let socket go out of scope
}

void Server::resetData()
{
    //boost::lock_guard<boost::mutex> guard(cleanupMutex);
    std::lock_guard<std::mutex> lk(cleanupMutex);
    invalidParseCount   = 0;
    readMessageCount    = 0;
    writtenMessageCount = 0;
}

void Server::setConnectionOFF()
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    std::lock_guard<std::mutex> lk(connectedMutex);
    connected = false;
}

void Server::setConnectionON()
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    std::lock_guard<std::mutex> lk(connectedMutex);
    connected = true;
}

void Server::setConnected(bool onoff)
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    std::lock_guard<std::mutex> lk(connectedMutex);
    connected = onoff;
}

void Server::setReadThreadAlive(bool onoff)
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    //std::lock_guard<std::mutex> lk(connectedMutex);
    readThreadAlive = onoff;
    if (onoff)
    {
        //cout << "Read thread started." << endl;
    }
    else
    {
        cout << "Read thread stopped." << endl;
    }
}

void Server::setWriteThreadAlive(bool onoff)
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    //std::lock_guard<std::mutex> lk(connectedMutex);
    writeThreadAlive = onoff;
    if (onoff)
    {
        //cout << "Write thread started." << endl;
    }
    else
    {
        cout << "Write thread stopped." << endl;
    }
}

void Server::setResponseThreadAlive(bool onoff)
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    //std::lock_guard<std::mutex> lk(connectedMutex);
    responseThreadAlive = onoff;
    if (onoff)
    {
        //cout << "Response thread started." << endl;
    }
    else
    {
        cout << "Response thread stopped." << endl;
    }
}

bool Server::isConnected()
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    std::lock_guard<std::mutex> lk(connectedMutex);
    return connected;
}

bool Server::isAccepting()
{
    return accepting;   // atomic
}

bool Server::busy()
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    //std::lock_guard<std::mutex> lk(connectedMutex);
    return (accepting || readThreadAlive || writeThreadAlive || responseThreadAlive);   // all atomic
}


//void Server::responseThread(socket_ptr sock)
void Server::responseThread()
{
    setResponseThreadAlive(true);

    while (isConnected())
    {
        try
        {
            streambuf_ptr message;
            responseQueue.wait_and_pop(message);    // Blocks until something is in the queue. If disconnencted, doesnt touch message pointer.

            //if (isConnected())
            if (message)    // TODO: experimental
            {
                // we have a message, parse xml and do something
                KukaResponse response;
                response.parse(*message);
                if (response.isValid())     // call user defined handle response only if parser has some success
                {
                    callResponseMethods(response);     // this can be hidden by a derived class
                }
                else
                {
                    ++invalidParseCount;
                    cerr << "Warning! Invalid response, no response handler invoked! Total " << invalidParseCount << " invalid responses received." << endl;
                }
            }
            else
            {
                break;
//                setResponseThreadAlive(false);
//                return;
            }
        }
        catch (std::exception &e)
        {
            cerr << "responseThread exception: " << e.what() << endl;
        }
    }

    setResponseThreadAlive(false);
}   // separate thread

void Server::callResponseMethods(const KukaResponse &response)
{
    handleResponse(response);
}

void Server::startListening(unsigned short port)
{
    // TODO: use boost error handling here



    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

    cout << "Server listening on port " << port << "." << endl;

    socket_ptr sock(new tcp::socket(io_service));
    acceptor.accept(*sock);

    cout << "A client connected." << endl;

    /*
    If previously connected, threads and socket shall be dead,
    queues and data reset.
    */
    messageQueue.reset();
    responseQueue.reset();
    resetData();    // counters etc

    setConnected(true);

    // spawn
    boost::thread read_thread(&Server::readThread,this, sock);
    boost::thread write_thread(&Server::writeThread,this, sock);
    boost::thread response_thread(&Server::responseThread,this);

    read_thread.detach();
    write_thread.detach();
    response_thread.detach();

    // block for a second to let system get things rolling
    boost::this_thread::sleep( boost::posix_time::seconds(1) );

    accepting = true;   // atomic for external use

}

void Server::readThread(socket_ptr sock)
{
    setReadThreadAlive(true);

    while (isConnected())
    {
        try
        {
            streambuf_ptr message(new boost::asio::streambuf);

            boost::system::error_code error;
            boost::asio::read_until(*sock, *message, serverConfig.getEndString(), error);   // waits for incoming data or eof

            if (error == boost::asio::error::eof)
            {
                cout << "Client disconnected." << endl;
                closeConnection();
                break;
            }

            responseQueue.push(message);
            ++readMessageCount;
        }
        catch (std::exception &e)   // complain but don't quit
        {
            cerr << "Read thread exception: " << e.what() << endl;
        }
    }

    /*
    boost::system::error_code shutdownError;
    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, shutdownError);    // recommended by boost
    if (shutdownError)
    {
        cout << "Read thread socket shutdown complains:" << shutdownError << endl;
    }

    boost::system::error_code sockCloseError;
    sock->close(sockCloseError);
    if (sockCloseError)
    {
        cout << "Read thread socket close complains:" << sockCloseError << endl;
    }
    */
    setReadThreadAlive(false);
}  // separate thread

void Server::writeThread(socket_ptr sock)
{
    setWriteThreadAlive(true);

    try
    {
        while (isConnected())
        {

            //cout << "writeThread waiting." << endl;
            streambuf_ptr message;
            messageQueue.wait_and_pop(message); // Wait until something in queue. If disconnected exits without touching message pointer.
            //cout << "writeThread awaking." << endl;
            //if (isConnected())   // might have disconnected
            if (message)    // TODO: experimental
            {
                boost::asio::write(*sock, *message);
                ++writtenMessageCount;
            }
            else
            {
                break;
            }
        }
    }
    catch (std::exception &e)
    {
        cerr << "Writing exception: " << e.what() << endl;
    }

    /*
    boost::system::error_code shutdownError;
    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_send, shutdownError);    // recommended by boost
    if (shutdownError)
    {
        cerr << "Write thread socket shutdown complains:" << shutdownError << endl;
    }

    boost::system::error_code sockCloseError;
    sock->close(sockCloseError);
    if (sockCloseError)
    {
        cout << "Write thread socket close complains:" << sockCloseError << endl;
    }
    */

    setWriteThreadAlive(false);
};     // separate thread

/*
Returns a pointer to buffer inside streambuf.
*/
const char * Server::streambufToPtr(boost::asio::streambuf &message)
{
    const char* bufPtr=boost::asio::buffer_cast<const char*>(message.data());
    return bufPtr;
}


// while (sock->is_open() && connected)
