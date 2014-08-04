#include "Server.hpp"

Server::Server()
{
    loadConfig();
}

Server::~Server()
{

}

/*  deprecated
void Server::sendPose(const info_vec &info, const frame_vec &frame)
{
    streambuf_ptr message(new boost::asio::streambuf);
    command.formatPose(*message, info, frame);   // first: infovector<int>, second: framevector<int>

    messageQueue.push(message);
}
*/

void Server::sendTrajectory(const info_vec &info, const trajectory_vec &trajectory)
{
    if (!isConnected())
    {
        cout << "Can't send because server not connected." << endl;
        //startListening();
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
            // block until it's possible to connect again
            cout << "Server connected. Waiting..." << endl;
            boost::this_thread::sleep( boost::posix_time::seconds(1) );
        }

    startListening(serverConfig.getPort());

}

void Server::loadConfig()
{

    serverConfig.load();
    cout << "Port MaxBufferSize EndString:" << endl;
    serverConfig.printValues();
}

void Server::closeConnection(socket_ptr sock)
{
    if (!isConnected())
    {
        cerr << "Server trying to close connection but not connected." << endl;
        //return;
    }

    cout << "Server closing connection." << endl;
    //cout << "CloseConnection setting to false." << endl;
    setConnected(false);  // threadsafe setting of connected = false
    //cout << "CloseConnection shutting down socket." << endl;
    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_send);    // recommended by boost
    //sock->close();
    //cout << "CloseConnection stopping queues." << endl;
    // stop queues, which should terminate write and response threads
    messageQueue.reject();
    responseQueue.reject();
    //cout << "CloseConnection resetting socket and calling handleDIsconnect()." << endl;
    sock.reset();           // deallocate socket
    handleDisconnect();     // user can do something
    cout << "Server closed connection." << endl;
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
        cout << "Read thread started." << endl;
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
        cout << "Write thread started." << endl;
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
        cout << "Response thread started." << endl;
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

bool Server::busy()
{
    //boost::lock_guard<boost::mutex> guard(connectedMutex);
    std::lock_guard<std::mutex> lk(connectedMutex);
    return (connected || readThreadAlive || writeThreadAlive || responseThreadAlive);
}

//bool Server::sendQueueEmpty()
//{
//    boost::lock_guard<boost::mutex> guard(queueMutex);
//    return messageQueue.empty();
//}

void Server::onResponse(socket_ptr sock)
{
    setResponseThreadAlive(true);

    while (isConnected())
    {
        try
        {
            streambuf_ptr message;
            responseQueue.wait_and_pop(message);    // blocks until something is in the queue or disconnencted

            if (isConnected())  // might have disconnected in the meanwhile
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
        }
        catch (std::exception &e)
        {
            cerr << "OnResponse exception: " << e.what() << endl;
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

    /*
    If previously connected, threads and socket shall be dead,
    queues and data reset.
    */
    messageQueue.reset();
    responseQueue.reset();
    resetData();    // counters etc

    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

    cout << "Server listening on port " << port << "." << endl;

    socket_ptr sock(new tcp::socket(io_service));
    acceptor.accept(*sock);

    cout << "A client connected." << endl;

//    boost::asio::socket_base::linger option;
//    sock->get_option(option);
//    bool is_set = option.enabled();
//    unsigned short timeout = option.timeout();
//    cout << "Socket options - is_set: " << is_set << " timeout: " << timeout << endl;

    invalidParseCount = 0;
    readMessageCount = 0;
    writtenMessageCount = 0;

    setConnected(true);
    //setConnectionON();

    // spawn
    boost::thread read_thread(&Server::readMessage,this, sock);
    boost::thread write_thread(&Server::writeMessage,this, sock);
    boost::thread response_thread(&Server::onResponse,this, sock);

    // block for a second to let system get things rolling
    boost::this_thread::sleep( boost::posix_time::seconds(1) );

}

void Server::readMessage(socket_ptr sock)
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
                closeConnection(sock);
                setReadThreadAlive(false);
                return;
            }

            responseQueue.push(message);

            ++readMessageCount;
        }
        catch (std::exception &e)   // complain but don't quit
        {
            cout << "ReadMessage exception: " << e.what() << endl;
        }
    }

    setReadThreadAlive(false);
}  // separate thread

void Server::writeMessage(socket_ptr sock)
{
    setWriteThreadAlive(true);

    try
    {
        while (isConnected())
        {

            //cout << "writeMessage waiting." << endl;
            streambuf_ptr message;
            messageQueue.wait_and_pop(message); // wait until something in queue or disconnect
            //cout << "writeMessage awaking." << endl;
            if (isConnected())   // might have disconnected
            {
                boost::system::error_code error;
                boost::asio::write(*sock, *message, error);
                ++writtenMessageCount;
                //cout << "writeMessage wrote a message." << endl;
                if (error == boost::asio::error::eof)
                {
                    cout << "Write thread discovered that client disconnected." << endl;
                    setWriteThreadAlive(false);
                    return;
                }
            }
        }
    }
    catch (std::exception &e)
    {
        cout << "Writing exception: " << e.what() << endl;
    }

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
