#include <Server.hpp>

Server::Server()
{
    loadConfig();
}

Server::~Server()
{}

void Server::sendPose(const info_vec &info, const frame_vec &frame)
{
    streambuf_ptr message(new boost::asio::streambuf);
    command.formatPose(*message, info, frame);   // first: infovector<int>, second: framevector<int>

    messageQueue.push(message);
}

void Server::sendTrajectory(const info_vec &info, const trajectory_vec &trajectory)
{

    streambuf_ptr message(new boost::asio::streambuf);
    command.formatTrajectory(*message, info, trajectory);   // first: infovector<int>, second: framevector<int>

    messageQueue.push(message);
} // TODO: implement sendTrajectory()

void Server::startListening()
{
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
    sock.reset();
    connected = false;
    handleDisconnect();
}

void Server::closeConnection()
{
    connected = false;
}

bool Server::isConnected()
{
    return connected;
}

bool Server::sendQueueEmpty()
{
    return messageQueue.empty();
}

void Server::onResponse(socket_ptr sock)
{
    while (sock->is_open() && connected)
    {
        try
        {
            streambuf_ptr message;
            responseQueue.wait_and_pop(message);    // blocks

            // we have a message, parse xml and do something
            response.parse(*message);
            if (response.isValid())     // call user defined handle response only if parser has some success
            {
                handleResponse();
            }
            else
            {
                ++invalidParseCount;
            }

            message.reset();
        }
        catch (std::exception &e)
        {
            cout << "OnResponse exception: " << e.what() << endl;
            //closeConnection();
            //return;
        }
    }
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

    invalidParseCount = 0;
    readMessageCount = 0;
    writtenMessageCount = 0;

    // spawn
    boost::thread read_thread(&Server::readMessage,this, sock);
    boost::thread write_thread(&Server::writeMessage,this, sock);
    boost::thread response_thread(&Server::onResponse,this, sock);

    connected = true;


    cout << "Waiting 1 seconds." << endl;
    boost::this_thread::sleep( boost::posix_time::seconds(1) );

}

void Server::readMessage(socket_ptr sock)
{
    // TODO: is lock read necessary (probably not)
    cout << "Server read thread started." << endl;
    try
    {

        while (sock->is_open() && connected)
        {

            //boost::asio::streambuf message(serverConfig.getMaxBufferSize());
            try
            {

                streambuf_ptr message(new boost::asio::streambuf);

                boost::system::error_code error;
                boost::asio::read_until(*sock, *message, serverConfig.getEndString(), error);

                if (error == boost::asio::error::eof)
                {
                    cout << "Client disconnected." << endl;
                    closeConnection(sock);
                    return;
                }

                responseQueue.push(message);

                ++readMessageCount;
            }
            catch (std::exception &e)   // complain but don't quit
            {
                cout << "ReadMessage exception (no matching xml end element?): " << e.what() << endl;
            }
        }
    }
    catch (std::exception &e)   // TODO: this is useless right now, fix
    {
        cout << "Fatal reading exception: " << e.what() << endl;
        closeConnection(sock);
        return;
    }
}  // separate thread

void Server::writeMessage(socket_ptr sock)
{

    cout << "Server write thread started." << endl;

    try
    {
        while (sock->is_open() && connected)
        {

            streambuf_ptr message;
            messageQueue.wait_and_pop(message); // lets try this instead of:

            //streambuf_ptr message = *messageQueue.wait_and_pop();

            boost::asio::write(*sock, *message);

            message.reset();    // TODO: is this how to cleanup? is the streambuf deleted now?

            ++writtenMessageCount;
        }
    }
    catch (std::exception &e)
    {
        cout << "Writing exception: " << e.what() << endl;
        closeConnection(sock);
        return;
    }
};     // separate thread

/*
Returns a pointer to buffer inside streambuf.
*/
const char * Server::streambufToPtr(boost::asio::streambuf &message)
{
    const char* bufPtr=boost::asio::buffer_cast<const char*>(message.data());
    return bufPtr;
}
