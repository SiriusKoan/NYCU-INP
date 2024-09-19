#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAXLEN 400
#define CHANNELS 1000

char WELCOME_MSG[] = R""""(
:mircd 001 %s :Welcome to the minimized IRC daemon!
:mircd 251 %s :There are %d users and 0 invisible on 1 server
:mircd 375 %s :- mircd Message of the day -
:mircd 372 %s :-  Hello, World!
:mircd 372 %s :-               @                    _ 
:mircd 372 %s :-   ____  ___   _   _ _   ____.     | |
:mircd 372 %s :-  /  _ `'_  \ | | | '_/ /  __|  ___| |
:mircd 372 %s :-  | | | | | | | | | |   | |    /  _  |
:mircd 372 %s :-  | | | | | | | | | |   | |__  | |_| |
:mircd 372 %s :-  |_| |_| |_| |_| |_|   \____| \___,_|
:mircd 372 %s :-  minimized internet relay chat daemon
:mircd 372 %s :-
:mircd 376 %s :End of message of the day
)"""";
char RPL_LISTSTART[] = ":micrd 321 %s Channel :Users Name\n";
char RPL_LIST[] = ":micrd 322 %s %s %d :%s\n";
char RPL_LISTEND[] = ":micrd 323 %s :End of /LIST\n";
char RPL_USERSSTART[] = ":micrd 392 %s :UserID\t\t\tTerminal\tHost\n";
char RPL_USERS[] = ":micrd 393 %s :%s\t\t\t-\t%s\n";
char RPL_ENDOFUSERS[] = ":micrd 394 %s :End of users\n";
char RPL_NOTOPIC[] = ":micrd 331 %s %s :No topic is set\n";
char RPL_TOPIC[] = ":micrd 332 %s %s :%s\n";
char RPL_NAMREPLY[] = ":micrd 353 %s %s :%s\n";
char RPL_ENDOFNAMES[] = ":micrd 366 %s %s :End of Names List\n";

char JOIN_MSG[] = ":%s JOIN %s\n";
char PART_MSG[] = ":%s PART :%s\n";
char PRIVMSG[] = ":%s PRIVMSG %s :%s\n";

char ERR_NOTREGISTERED[] = ":micrd 451 :You have not registered\n";
char ERR_UNKNOWNCOMMAND[] = ":mircd 421 %s %s :Unknown command\n";
char ERR_NONICKNAMEGIVEN[] = ":mircd 431 %s :No nickname given\n";
char ERR_NICKCOLLISION[] = ":mircd 436 %s %s :Nickname collision KILL\n";
char ERR_NEEDMOREPARAMS[] = ":mircd 461 %s %s :Not enough parameters\n";
char ERR_NOTONCHANNEL[] = ":mircd 442 %s %s :You are not on that channel\n";
char ERR_NORECIPIENT[] = ":micrd 411 %s :No recipient given (%s)\n";
char ERR_NOTEXTTOSEND[] = ":micrd 412 %s :No text to send\n";
char ERR_NOSUCHNICK[] = ":micrd 401 %s %s :No such nick/channel\n";
char ERR_NOSUCHCHANNEL[] = ":micrd 403 %s %s :No such channel\n";

typedef struct User {
    int id;
    char nickname[MAXLEN];
    char realname[MAXLEN];
    char host[MAXLEN];
} User;

typedef struct Channel {
    char name[MAXLEN];
    char topic[MAXLEN];
    int num;
    User* people[MAXLEN];
} Channel;

Channel* channels[CHANNELS] = {NULL};
User* users[FD_SETSIZE] = {NULL};

int find_channel(char ch_name[]) {
    for (int j = 0; j < CHANNELS; j++) if (channels[j] && strcmp(channels[j]->name, ch_name) == 0) return j;
    return -1;
}

int create_channel(char ch_name[]) {
    Channel* channel = (Channel*)malloc(sizeof(Channel));
    channel->num = 0;
    strcpy(channel->name, ch_name);
    strcpy(channel->topic, "");
    for (int j = 0; j < CHANNELS; j++) if (!channels[j]) {
        channels[j] = channel;
        return j;
    }
    return -1;
}

bool on_channel(int i, int ch_i) {
    for (int j = 0; j < channels[ch_i]->num; j++) {
        if (channels[ch_i]->people[j] == users[i]) return true;
    }
    return false;
}

void leave_channel(int i, int ch_i) {
    for (int j = 0; j < channels[ch_i]->num; j++) {
        if (channels[ch_i]->people[j] == users[i]) {
            channels[ch_i]->people[j] = NULL;
            for (int k = j + 1; k < channels[ch_i]->num; k++) channels[ch_i]->people[k - 1] = channels[ch_i]->people[k];
            break;
        }
    }
    channels[ch_i]->num--;
}
