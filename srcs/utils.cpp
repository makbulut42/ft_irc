#include "../inc/ircserv.hpp"

int sockAttr( struct sockaddr_in *servSock, short int port)
{
    int fdSock, x = 1;

    servSock->sin_family = AF_INET;
    servSock->sin_port = htons(port);
    servSock->sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(servSock->sin_zero), 0, 8);
    fdSock = socket(AF_INET, SOCK_STREAM, 0);
    if( fdSock == -1 )
    {
        cerr << "[-]Socket is not created!" << endl;
        return -1;
    }
    if( setsockopt( fdSock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x) ) == -1 )
    {
        cerr << "[-]Socket could be not set options!" << endl;
        close(fdSock);
        return -1;
    }
    if ( bind( fdSock, (struct sockaddr*)servSock, sizeof(struct sockaddr_in) ) == -1)
    {
        cerr << "[-]Socket is not binding: " << endl;
        perror("Reason is");
        close(fdSock);
        return -1;
    }

    cout << "[+]Bind to the port number: " << port << endl;
    return (fdSock);
}

void clientDisconnecter( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, sockaddr_in *servSock, int cli )
{
    close(plFd[cli].fd);
    plFd.erase( plFd.begin() + cli);
    usr.erase( usr.begin() + cli - 1 );
    cout << RED << "[-]Connection Refused-> " << inet_ntoa( servSock->sin_addr ) << ":" << ntohs(  servSock->sin_port) << RESET << endl;
}

int clientAdder( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, sockaddr_in *servSock, int servSock_fd )
{
    struct pollfd pl;
    int clientSock_fd;
    Client newClient;
    socklen_t sockLen = sizeof(sockaddr);

    while (true)
    {
        clientSock_fd = accept( servSock_fd, (sockaddr*)servSock, &sockLen );
        if( clientSock_fd < 0 )
        {
            if( errno != EWOULDBLOCK )
            {
                cerr << "[-]Client cannot accepted!" << endl;
                return -1;
            }
            break;
        }
        fcntl( clientSock_fd, F_SETFL, O_NONBLOCK );
        pl.fd = clientSock_fd;
        pl.events = POLLIN;
        pl.revents = 0;
        newClient.setChannel("");
        newClient.setName("");
        newClient.setData("");
        newClient.setNick("");
        newClient.setReply("");
        newClient.setPasswdStat(0);
        newClient.setNameStat(0);
        newClient.setNickStat(0);
        newClient.setIsOpr(0);
        plFd.push_back( pl );
        usr.push_back( newClient );
        string msg = "[*]Please Enter The Server Password:\n";
        send( clientSock_fd, msg.data(), msg.length(), 0 ); // or msg.size()
        cout << GRN << "[+]Connection accepted from-> " << inet_ntoa( servSock->sin_addr ) << ":" << ntohs(servSock->sin_port) << RESET << endl;
    }
    return 0;
}

static int readClient_data( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, int cli )
{
    char bfr[1024];
    int stt;

    while (true)
    {
        memset( bfr, 0, sizeof(bfr) );
        stt = recv( plFd[cli].fd, bfr, sizeof(bfr), 0 );
        if( stt < 0 )
            break;
        if( stt == 0 )
            return -1;
        bfr[stt] = 0;
        usr[cli - 1].setData(usr[cli - 1].getData() + bfr);
    }
    return 0;
};

static void chrDeleter( char ch, string& s )
{
    s.erase( std::remove( s.begin(), s.end(), ch ), s.end() );
}

//static void chrTrimer( string& s, char ch )
//{
//    s = s.substr( s.find(ch) + 1, s.length() - s.find(ch) );
//}

static int passChecker( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, string data, int cli )
{
    string outPass(data);
    string errMsg("[-]Wrong Password, Try Again Please:\n");
    string noticeMsg("[.]Enter <NICK> your_nick Than <USER> your_username\n");

    if( outPass != g_PASS )
    {
        send( plFd[cli].fd, errMsg.data(), errMsg.size(), 0 );
        return 0;
    }
    usr[cli - 1].setPasswdStat(1);
    send( plFd[cli].fd, noticeMsg.data(), noticeMsg.size(), 0 );
    cerr << outPass << endl;
    return 0;
}

// look_cmd kismidir eren hocam, senin yazacagin kisim yani. :)
static int cmdChecker( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, string data, int cli )
{
    Commands    cmd;
    int         retValue;

    retValue = cmd.EnterCommand(data, cli, usr, plFd);

    return  retValue;
}

int clientAuth( std::vector<struct pollfd> &plFd, std::vector<Client> &usr, int cli )
{
    string data;

    while (true)
    {
        if( readClient_data( plFd, usr, cli ) == -1 )
            return -1;
        if( usr[cli - 1].getData().find('\n') == string::npos )
            return 0;
        else
        {
            while (usr[cli - 1].getData().find('\r') != string::npos )
                chrDeleter( '\r', usr[cli - 1].getData() );
        }
        data = usr[cli - 1].getData().substr( 0, usr[cli - 1].getData().find('\n') );
        chrTrimer( usr[ cli - 1 ].getData(), '\n' );
        cout << MAGEN << ">> " << "[" << plFd[cli].fd << "]" << " << Client: " << CYN << data << RESET << endl;
        if( usr[cli - 1].getPasswdStat() == 0 )
        {
            if ( passChecker( plFd, usr, data, cli ) == -1 )
                return -1;
            else
                continue;
        }
        // ?
        // ?
        if( cmdChecker( plFd, usr, data, cli ) == -1 )
            return -1;
        break;
    }
    return 0;
}