#include "server.hpp"
#include "commands.hpp"

Channels* Server::getChannel(std::string name)
{
    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == name)
            return &(*it);
    }
    return NULL;
}

bool Server::is_user_in_channel(std::string name, int fd)
{
    Channels *ch = getChannel(name);
    if (ch == NULL)
        return false;
    std::map<int, Clients> tmp = ch->getUsers();
    for (std::map<int, Clients>::iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
        if (it->first == fd)
            return true;
    }
    return false;
}

bool Server::is_user_in_server(std::string name)
{
    for (std::map<int, Clients>::iterator it = this->cl.begin(); it != this->cl.end(); ++it)
        if (it->second.getNick() == name)
            return true;
    return false;
}

void Server::save_user(std::vector<Channels>::iterator it, int fd, s_command c)
{
    (void)c;
    it->addUser(this->cl.find(fd)->second);
    std::vector<std::string> tmp;
    std::string msg = ":"+this->cl.find(fd)->second.getNick()+"!~"+this->cl.find(fd)->second.getUser()+"@"+this->getIp(fd) + ".ip " + "JOIN :" + it->getName() + "\n";
    sendToChannel(it->getName(), msg);
    tmp.push_back(it->getName());
    tmp.push_back(it->getTopic());
    msg = showReply(332, fd, tmp);
    send(fd, msg.c_str(), msg.length(), 0);
    Commands names_cmd = Commands("names " + it->getName());
    irc_names(fd, names_cmd.getList()[0]);
}

bool Server::find_channel(std::string &name, int fd, s_command c)
{
    std::string key = c.first_pram != "" ? c.first_pram : c.second_pram;

    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == name)
        {
            if (it->getUsers().find(fd)->first == fd)
                return true;
            if (it->getInviteSet())
            {
                if (it->getInvited().find(fd)->first == fd)
                {
                    if (it->getKeySet())
                    {
                        if (it->getKey() == key)
                        {
                            if (it->getLimitSet())
                            {
                                if (it->getUsers().size() < (unsigned long)it->getLimit())
                                    save_user(it, fd, c);
                                else
                                {
                                    std::vector<std::string> tmp;
                                    tmp.push_back(it->getName());
                                    send(fd, showReply(471, fd, tmp).c_str(), showReply(471, fd, tmp).size(), 0);
                                    return true;
                                }
                            }
                            else
                                save_user(it, fd, c);
                        }
                        else
                        {
                            std::vector<std::string> tmp;
                            tmp.push_back(it->getName());
                            send(fd, showReply(475, fd, tmp).c_str(), showReply(475, fd, tmp).size(), 0);
                            return true;
                        }
                    }
                    else
                        save_user(it, fd, c);
                }
                else
                {
                    std::vector<std::string> tmp;
                    tmp.push_back(it->getName());
                    send(fd, showReply(473, fd, tmp).c_str(), showReply(473, fd, tmp).size(), 0);
                    return true;
                }
            }
            else
            {
                if (it->getKeySet())
                {
                    if (it->getKey() == key)
                        save_user(it, fd, c);
                    else
                    {
                        std::vector<std::string> tmp;
                        tmp.push_back(it->getName());
                        send(fd, showReply(475, fd, tmp).c_str(), showReply(475, fd, tmp).size(), 0);
                        return true;
                    }
                }
                else
                    save_user(it, fd, c);
            }
            return (true);
        }
    }
    return false;
}

void Server::sendToChannel(std::string channel, std::string message)
{
    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == channel)
        {
            std::map<int, Clients> users = it->getUsers();
            for (std::map<int, Clients>::iterator it2 = users.begin(); it2 != users.end(); ++it2)
                send(it2->second.getFd(), message.c_str(), message.length(), 0);
            break;
        }
    }
}

int Server::irc_join(int fd, s_command &c)
{
    if (c.target.size() == 0)
    {
        //error
        std::vector<std::string> params;
        params.push_back(c.command);
        std::string error = showReply(461, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
        return 0;
    }
    std::vector<std::string> &targs = c.target; 
    for (std::vector<std::string>::iterator it = targs.begin(); it != targs.end(); ++it)
    {
        if (it->c_str()[0] != '#')
        {
            std::vector<std::string> params;
            std::cout << "it: " << *it << std::endl;
            params.push_back(*it);
            send(fd, showReply(403, fd, params).c_str(), showReply(403, fd, params).size(), 0);
        }
        else if (!find_channel(*it, fd, c))
        {
            std::string pass = "";
            pass = c.first_pram;
            if (pass.empty())
                pass = c.second_pram;
            Channels newChannel(*it ,pass);
            newChannel.addUser(this->cl.find(fd)->second);
            newChannel.addOp(this->cl.find(fd)->second);
            std::vector<std::string> tmp;
            std::string msg = ":"+this->cl.find(fd)->second.getNick()+"!~"+this->cl.find(fd)->second.getUser()+"@"+this->getIp(fd) + ".ip " + "JOIN :" + *it + "\n";
            send(fd, msg.c_str(), msg.length(), 0);
            ch.push_back(newChannel);
            tmp.push_back(*it);
            tmp.push_back(newChannel.getTopic());
            msg = showReply(332, fd, tmp);
            send(fd, msg.c_str(), msg.length(), 0);
            Commands names_cmd = Commands("names " + *it);
            irc_names(fd, names_cmd.getList()[0]);
        }
    }
    return 1;
}

bool Server::is_channel(std::string name)
{
    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == name)
            return true;
    }
    return false;
}

bool Server::irc_part(int fd, s_command &c)
{
    // TODO: params size check
    if (c.target.size() == 0)
    {
        std::vector<std::string> params;
        params.push_back(c.command);
        std::string error = showReply(461, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    for (std::vector<std::string>::iterator it = c.target.begin() ; it != c.target.end(); ++it)
    {
        if (!is_channel(*it))
        {
            std::vector<std::string> params;
            params.push_back(*it);
            send(fd, showReply(403, fd, params).c_str(), showReply(403, fd, params).size(), 0);
            continue;
        }
        else
        {
            Channels *channel = getChannel(*it);
            if(channel->getUsers().find(fd)->first != fd)
            {
                std::vector<std::string> params;
                params.push_back(*it);
                send(fd, showReply(442, fd, params).c_str(), showReply(442, fd, params).size(), 0);
                continue;
            }
            else
            {
                channel->removeUser(channel->getUsers().find(fd)->second);
                if (channel->getOps().find(fd) != channel->getOps().end())
                    channel->removeOp(channel->getOps().find(fd)->second);
                std::string msg = ":"+this->cl.find(fd)->second.getNick()+"!~"+this->cl.find(fd)->second.getUser()+"@"+this->getIp(fd) + ".ip " + "PART " + *it + "\n";
                send(fd, msg.c_str(), msg.length(), 0);
                sendToChannel(*it, msg);
                if (channel->getUsers().size() == 0)
                {
                    for (std::vector<Channels>::iterator it2 = this->ch.begin(); it2 != this->ch.end(); ++it2)
                    {
                        if (it2->getName() == *it)
                        {
                            this->ch.erase(it2);
                            break;
                        }
                    }
                }
            }
        }
    }
    return (true);
}

bool Server::irc_topic(int fd, s_command &c)
{
    // TODO: params size check
    if (c.target.size() == 0)
    {
        //error 461
        std::vector<std::string> params;
        params.push_back(c.command);
        std::string error = showReply(461, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    if (c.target.size() > 1)
    {
        //error 403
        std::string channel = splitString2(c.original, " ")[1];
        std::vector<std::string> params;
        params.push_back(channel);
        send(fd, showReply(403, fd, params).c_str(), showReply(403, fd, params).size(), 0);
        return false;
    }
    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == c.target[0])
        {
            if (it->getUsers().find(fd)->first == fd)
            {
                if (!c.second_pram.empty() || !c.first_pram.empty())
                {
                    if (!c.first_pram.empty())
                        it->setTopic(c.first_pram);
                    else
                        it->setTopic(c.second_pram);
                    std::vector<std::string> params;
                    params.push_back(c.target[0]);
                    params.push_back(it->getTopic());
                    std::string topic = showReply(332, fd, params);
                    sendToChannel(it->getName(), topic);
                    return (true);
                }
                else
                {
                    std::vector<std::string> params;

                    if (!it->getTopic().empty())
                    {
                        params.push_back(it->getName());
                        params.push_back(it->getTopic());
                        send(fd, showReply(332, fd, params).c_str(), showReply(332, fd, params).size(), 0);
                    }
                    else
                    {
                        params.push_back(it->getName());
                        send(fd, showReply(331, fd, params).c_str(), showReply(331, fd, params).size(), 0);
                    }
                    return (true);
                }
            }
            //error 442
            std::vector<std::string> params;
            params.push_back(c.target[0]);
            std::string error = showReply(442, fd, params);
            send(fd, error.c_str(), error.size(), 0);
            return false;
        }
    }
    return (true);
}

bool Server::irc_invite(int fd, s_command &c)
{
    std::vector<std::string> params;
    std::string msg = "";
    std::string channel_str = c.first_pram == "" ? c.second_pram : c.first_pram;
    if (c.target.size() == 0)
    {
        //error 461
        std::vector<std::string> params;
        params.push_back(c.command);
        std::string error = showReply(461, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    if (c.target.size() > 1)
    {
        std::string channel = splitString2(c.original, " ")[2];
        std::vector<std::string> params;
        params.push_back(channel);
        send(fd, showReply(403, fd, params).c_str(), showReply(403, fd, params).size(), 0);
        return false;
    }
    if(is_channel(channel_str))
    {
        Channels *channel = getChannel(channel_str);
        if (is_user_in_server(c.target[0]))
        {
            if (is_user_in_channel(channel_str, this->findClinet(c.target[0])))
            {
                params.clear();
                params.push_back(c.target[0]);
                params.push_back(channel_str);
                msg = showReply(443, fd, params);
            }
            else
            {
                if (channel->getOps().find(fd)->first == fd)
                {
                    std::cout << "channel_str111: " << channel_str << std::endl;
                    params.clear();
                    params.push_back(channel_str);
                    params.push_back(c.target[0]);
                    msg = showReply(341, fd, params);
                    sendToChannel(channel_str, msg);
                    msg = showReply(341, fd, params);
                    send(this->findClinet(c.target[0]), msg.c_str(), msg.size(), 0);
                    return (true);
                }
                else
                {
                    params.clear();
                    params.push_back(channel_str);
                    msg = showReply(482, fd, params);
                    send(fd, msg.c_str(), msg.size(), 0);
                }
            }
        }
        else
        {
            params.clear();
            params.push_back(c.target[0]);
            msg = showReply(401, fd, params);
            send(fd, msg.c_str(), msg.size(), 0);
        }
    }
    else
    {
        std::cout << "channel_str222: " << channel_str << std::endl;
        params.clear();
        params.push_back( splitString2(c.original, " ")[2]);
        std::string error = showReply(403, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    return (true);
}

bool Server::irc_mode(int fd, s_command &c)
{
    (void)fd;
    (void)c;

    return (true);
}

bool Server::irc_kick(int fd, s_command &c)
{
    std::vector<std::string> params;
    std::vector<std::string> users = splitString(c.first_pram == "" ? c.second_pram : c.first_pram, ",");
    std::string msg;
    Channels *channel;
    for (std::vector<std::string>::iterator it = c.target.begin(); it != c.target.end(); ++it)
    {
        if (!is_channel(*it))
        {
            //error 403
            std::vector<std::string> params;
            params.push_back(*it);
            std::string error = showReply(403, fd, params);
            send(fd, error.c_str(), error.size(), 0);
            return false;
        }
        channel = getChannel(*it);
        for (std::vector<std::string>::iterator it2 = users.begin(); it2 != users.end(); ++it2)
        {
            std::cout << "ch ; " << *it << " user ; " << *it2 << is_user_in_channel(*it, fd) << std::endl;
            if (is_user_in_channel(*it, fd))
            {
                if (channel->getOps().find(fd)->first == fd)
                {
                    if (is_user_in_channel(*it, this->findClinet(*it2)))
                    {
                        params.clear();
                        params.push_back(*it);
                        params.push_back(*it2);
                        msg = showReply(341, fd, params);
                        sendToChannel(*it, msg);
                        channel->removeUser(this->cl.find(this->findClinet(*it2))->second);
                    }
                    else
                    {
                        params.clear();
                        params.push_back(*it);
                        params.push_back(*it2);
                        msg = showReply(441, fd, params);
                        send(fd, msg.c_str(), msg.size(), 0);
                    }
                }
                else
                {
                    params.clear();
                    params.push_back(*it);
                    msg = showReply(482, fd, params);
                    send(fd, msg.c_str(), msg.size(), 0);
                }
            }
            else
            {
                params.clear();
                params.push_back(*it);
                msg = showReply(442, fd, params);
                send(fd, msg.c_str(), msg.size(), 0);
            }
        }
    }
    return (true);
}

bool Server::irc_names(int fd, const s_command &c)
{
    std::vector<std::string> params;
    if (c.target.size() == 0)
    {
        params.push_back("*");
        std::string error = showReply(366, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    params.push_back(c.target[0]);
    if (c.target.size() > 1)
    {
        //error 403
        std::string channel = splitString2(c.original, " ")[1];
        std::vector<std::string> params;
        params.push_back(channel);
        std::string error = showReply(403, fd, params);
        send(fd, error.c_str(), error.size(), 0);
        return false;
    }
    for (std::vector<Channels>::iterator it = this->ch.begin(); it != this->ch.end(); ++it)
    {
        if (it->getName() == c.target[0])
        {
            if (it->getUsers().find(fd)->first == fd)
            {
                std::map<int, Clients> users = it->getUsers();
                for (std::map<int, Clients>::iterator it2 = users.begin(); it2 != users.end(); ++it2)
                    params.push_back(it2->second.getNick());
            }
            else
            {
                std::vector<std::string> params;
                params.push_back(c.target[0]);
                std::string error = showReply(442, fd, params);
                send(fd, error.c_str(), error.size(), 0);
                return false;
            }
        }
    }
    std::cout << "params size :" << params.size() << std::endl;
    if (params.size() == 1)
    {
        std::string channel = splitString2(c.original, " ")[1];
        std::vector<std::string> params;
        params.push_back(channel);
        send(fd, showReply(403, fd, params).c_str(), showReply(403, fd, params).size(), 0);
        return false;
    }
    else
    {
        for (std::vector<std::string>::iterator it = params.begin() + 1; it != params.end(); ++it)
        {
            for (std::vector<Channels>::iterator it2 = this->ch.begin(); it2 != this->ch.end(); ++it2)
            {
                if (it2->getName() == c.target[0])
                {
                    std::map<int, Clients> users = it2->getOps();
                    for (std::map<int, Clients>::iterator it3 = users.begin(); it3 != users.end(); ++it3)
                        if (it3->second.getNick() == *it)
                            *it = "@" + *it;
                }
            }
        }
        std::string names = "";
        for (std::vector<std::string>::iterator it = params.begin()+1; it != params.end(); ++it)
            names += *it + " ";
        params.clear();
        params.push_back(c.target[0]);
        params.push_back(names);
        send(fd, showReply(353, fd, params).c_str(), showReply(353, fd, params).size(), 0);
        send(fd, showReply(366, fd, params).c_str(), showReply(366, fd, params).size(), 0);
    }
    return (true);
}

bool Server::irc_ban(int fd, s_command &c)
{
    (void)fd;
    (void)c;

    return (true);
}
