#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "chatserver.h"
#include "response.h"


int main(int argc, char **argv) {
    extern int processquestion(char *question, int fd);
    int port;
    struct hostent *hp;

    if (argc == 2) {
        port = 3000;
    } else if (argc == 3) {
        port = atoi(argv[2]);
    } else {
        fprintf(stderr, "usage: %s host [port]\n", argv[0]);
        return 1;
    }

    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "%s: no such host\n", argv[1]);
        return(1);
    }
    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) {
        fprintf(stderr, "%s: not an internet protocol host name\n", argv[1]);
        return(1);
    }

    int fd;
    struct sockaddr_in r;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return(1);
    }

    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    memcpy(&r.sin_addr, hp->h_addr_list[0], hp->h_length);
    r.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&r, sizeof r) < 0) {
        perror("connect");
        return(1);
    }

    char buf[MAXMESSAGE];
    int len;
    if ((len = read(fd, buf, sizeof buf - 1)) < 0) {
        perror("read");
        return(1);
    }
    buf[len] = '\0';
    if (strncmp(buf, chatserver_banner "\r\n", 14) != 0) {
        printf("wrong server\n");
        return(1);
    }

    char handle[MAXHANDLE];
    strncpy(handle, botname, sizeof(handle));
    strncat(handle, "bot\r\n", sizeof(handle) - strlen(handle) - 1);
    if (write(fd, handle, strlen(handle)) != strlen(handle)) {
        perror("write");
        return(1);
    }

    int maxfd;
    fd_set fds;

    while (1) {
        
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
        maxfd = 0; 
        if (fd > maxfd) {
            maxfd = fd;
        }

        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("select");
            return(1);
        }

        if (FD_ISSET(0, &fds)){
            char message1[MAXMESSAGE];
            int len1;
            if ((len1 = read(0, message1, sizeof message1 - 1)) < 0) {
                perror("read");
                return(1);
            }
            if (len1 == 0) {
                break;
            }
            message1[len1] = '\0';
            if (write(fd, message1, len1) != len1) {
                perror("write");
                return(1);
            }
        }

        if (FD_ISSET(fd, &fds)) {
            char message2[MAXMESSAGE];
            int len2;
            if ((len2 = read(fd, message2, sizeof message2 - 1)) < 0) {
                perror("read");
                return(1);
            }
            if (len2 == 0) {
                return(0);
            }
            message2[len2] = '\0';
            printf("%s", message2);
            char *p;
            if ((p = strchr(message2, '?')) != NULL) {
                int n = (int) (p - message2);
                char question[MAXMESSAGE];
                strncpy(question, message2, n);
                question[n] = '\0';
                if ((p = strchr(question, ':')) != NULL) {
                    strncpy(question, p+1, sizeof(question));
                }
                processquestion(question, fd);
            }
        }

    }

}


int processquestion(char *question, int fd) {
    // remove spaces
    int i = 0;
    int j = 0;
    char prompt[MAXMESSAGE];
    while (i < strlen(question)) {
        if (isspace(question[i]) == 0) {
            prompt[j] = question[i];
            j++;
        }
        i++;
    }
    prompt[j] = '\0';

    // convert string to lower case
    for (i = 0; i < strlen(prompt); i++){
        prompt[i] = tolower(prompt[i]);
    }
    
    int p = 0;

    // remove what, how, who, why, where
    if (strncmp(prompt, "what", 4) == 0){
        p += 4;
    } else if (strncmp(prompt, "how", 3) == 0){
        p += 3;
    } else if (strncmp(prompt, "who", 3) == 0){
        p += 3;
    } else if (strncmp(prompt, "why", 3) == 0){
        p += 3;
    } else if (strncmp(prompt, "where", 5) == 0){
        p += 5;
    }

    //remove is, 's, isn't, are, 're, aren't, do, does, doesn't, don't
    if (strncmp(prompt+p, "isn\'t", 5) == 0){
        p += 5;
    } else if (strncmp(prompt+p, "is", 2) == 0){
        p += 2;
    } else if (strncmp(prompt+p, "\'s", 2) == 0){
        p += 2;
    } else if (strncmp(prompt+p, "aren\'t", 6) == 0){
        p += 6;
    } else if (strncmp(prompt+p, "are", 3) == 0){
        p += 3;
    } else if (strncmp(prompt+p, "\'re", 3) == 0){
        p += 3;
    } else if (strncmp(prompt+p, "doesn\'t", 7) == 0){
        p += 7;
    } else if (strncmp(prompt+p, "does", 4) == 0){
        p += 4;
    } else if (strncmp(prompt+p, "don\'t", 5) == 0){
        p += 5;
    } else if (strncmp(prompt+p, "do", 2) == 0){
        p += 2;
    }

    // remove "the"
    if (strncmp(prompt+p, "the", 3) == 0){
        p += 3;
    }

    if (response(prompt+p)) {
        char ans[MAXMESSAGE];
        strncpy(ans, response(prompt+p), sizeof(ans));
        ans[strlen(ans)] = '\0';
        strncat(ans, "\r\n", sizeof(ans) - strlen(ans) - 1);
        if (write(fd, ans, strlen(ans)) != strlen(ans)) {
            perror("write");
            return(1);
        }
    }
    return(0);
}