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
        cerr << "Can't send because Server not connected to anything." << endl;
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
    if (isConnected())
    {
        cerr << "Server is connected to a client. Disconnect before starting to listen for next one." << endl;
    }
    else
    {
        startListening(serverConfig.getPort());
    }
}

void Server::loadConfig()
{

    serverConfig.load();
    cout << "Port MaxBufferSize EndString:" << endl;
    serverConfig.printValues();
}

void Server::closeConnection(socket_ptr sock)
{
    connectionOFF();  // threadsafe setting of connected = false
    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_send);    // recommended by boost
    sock->close();

    // stop queues, which should terminate write and response threads
    messageQueue.reject();
    responseQueue.reject();

    sock.reset();           // deallocate socket
    handleDisconnect();     // user can do something

    cout << "Server closed connection." << endl;
}

void Server::resetData()
{
    boost::lock_guard<boost::mutex> guard(cleanupMutex);
    invalidParseCount   = 0;
    readMessageCount    = 0;
    writtenMessageCount = 0;
}

void Server::connectionOFF()
{
    boost::lock_guard<boost::mutex> guard(connectedMutex);
    connected = false;
}

void Server::connectionON()
{
    boost::lock_guard<boost::mutex> guard(connectedMutex);
    connected = true;
}

bool Server::isConnected()
{
    boost::lock_guard<boost::mutex> guard(connectedMutex);
    return connected;
}

//bool Server::sendQueueEmpty()
//{
//    boost::lock_guard<boost::mutex> guard(queueMutex);
//    return messageQueue.empty();
//}

void Server::onResponse(socket_ptr sock)
{
    cout << "Server response thread started." << endl;

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

    cout << "Server response thread exiting." << endl;
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

    invalidParseCount = 0;
    readMessageCount = 0;
    writtenMessageCount = 0;

    connectionON();

    // spawn
    boost::thread read_thread(&Server::readMessage,this, sock);
    boost::thread write_thread(&Server::writeMessage,this, sock);
    boost::thread response_thread(&Server::onResponse,this, sock);

    // block for a second
    boost::this_thread::sleep( boost::posix_time::seconds(1) );

}

void Server::readMessage(socket_ptr sock)
{
    cout << "Server read thread started." << endl;

    while (isConnected())
    {
        try
        {
            streambuf_ptr message(new boost::asio::streambuf);

            boost::system::error_code error;
            boost::asio::read_until(*sock, *message, serverConfig.getEndString(), error);   // waits for incoming data or eof

            if (error == boost::asio::error::eof)
            {
                if (isConnected())
                {
                    closeConnection(sock);
                }

                cout << "Client disconnected, server read thread exiting." << endl;
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

    cout << "Server read thread exiting." << endl;
}  // separate thread

void Server::writeMessage(socket_ptr sock)
{

    cout << "Server write thread started." << endl;

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
                boost::asio::write(*sock, *message);
                ++writtenMessageCount;
                //cout << "writeMessage wrote a message." << endl;
            }
        }
    }
    catch (std::exception &e)
    {
        cout << "Writing exception: " << e.what() << endl;
    }

    cout << "Server write thread exiting." << endl;

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
