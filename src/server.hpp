#ifndef SERVER_HPP
#define SERVER_HPP

#include "includes.hpp"

struct s_command;
class Clinets;

class Server
{
    private:
        int fd_server;
        int port;
        std::string pw;
        struct sockaddr_in ConAddr;
        std::vector<pollfd> allFd;
        std::map<int, Clinets> cl;
        // std::vector<Channels> cha;

    public:
        Server(const char *pt, const char *password);
        // seters
        void setPort(int pr);
        int getPort() const;
        bool isPass(std::string pass);
        // methods
        void createSocket();
        void bindSocket();
        void listenConix();
        void addFd(int fd, struct sockaddr_in Cl);
        bool events();
        void creatServer();
        void chat();
        int findClinet(std::string nickName);
        std::string showReply(int code, int fd, std::vector<std::string> &vars);
        std::string getIp(int fd);


        //validations
        int command_routes(int fd, s_command &c);
        //commands

        int     irc_nick(int fd, s_command &c);
        bool    isValidNickname(int fd, const std::string &nickname);

        int irc_user(int fd, s_command &c);
        int irc_join(int fd, s_command &c);
        bool irc_whois(int fd, s_command &c);
        bool irc_pass(int fd, s_command &c);
        // bool irc_ping(int fd, s_command &c);
        // bool irc_pong(int fd, s_command &c);
        bool irc_privmsg(int fd, s_command &c);
        // bool irc_part(int fd, s_command &c);
        bool irc_quit(int fd, s_command &c);
        ~Server(); 
};

#endif