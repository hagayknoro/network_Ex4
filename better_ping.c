#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> // gettimeofday()
#include <sys/types.h>
#include <unistd.h>
#include <sys/fcntl.h>

// IPv4 header len without options
#define IP4_HDRLEN 20

// ICMP header len for echo req
#define ICMP_HDRLEN 8

#define SOURCE_IP "127.0.0.1"

#define SERVER_PORT 3000
#define SERVER_IP_ADDRESS "127.0.0.1"

// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);

// run 2 programs using fork + exec
// command: make clean && make all && ./partb
int main(int count, char *argv[])
{
    char *args[2];
    // compiled watchdog.c by makefile
    args[0] = "./watchdog";
    args[1] = NULL;
    int status;
    int pid = fork();
    if (pid == 0)
    {
        printf("in child \n");
        execvp(args[0], args);
    }
    printf("child exit status is: %d\n", status);

    if ( count != 2 )
    {
        printf("usage: %s <addr> \n", argv[0]);
        exit(0);
    }
    char *DesIp = argv[1];

    sleep(3);


    //Listen to Tcp Connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Unable to create socket\n");
        return -1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, (const char *) SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
        printf("connect() failed with error code : %d", errno);
    }
    printf("connected to server\n");

    struct sockaddr_in clientAddress; //
    socklen_t clientAddressLen = sizeof(clientAddress);


    // Create raw socket for IP-RAW (make IP-header by yourself)
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        fprintf(stderr, "socket() failed with error: %d\n", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }

    //Set the IP address of the host to ping
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;
    dest_in.sin_addr.s_addr = inet_addr(DesIp);

    //Create time struct
    struct timeval start, end;
    int sequence = 0;

    int checkSend = -1;
    while (checkSend < 0 )
    {
        int pingSend = 300;
        checkSend = send(sockfd, &pingSend , sizeof(pingSend),0);
        if(checkSend > 0)
        {
            printf("Sent start to watchdog\n");
        }
        else{
            printf("Error in send\n");
        }
    }



    while(1)
    {
        //Starting the time count
        gettimeofday(&start, 0);

        char data[IP_MAXPACKET] = "This is the ping.\n";
        int datalen = strlen(data) + 1;
        //Create the buffer
        char packet[IP_MAXPACKET];
        memset(packet, 0, IP_MAXPACKET);

        //Create the ICMP header
        struct icmp icmphdr;
        icmphdr.icmp_type = ICMP_ECHO;
        icmphdr.icmp_code = 0;
        icmphdr.icmp_id = 18;
        icmphdr.icmp_seq = sequence++;
        icmphdr.icmp_cksum = 0;

        // Next, ICMP header
        memcpy((packet), &icmphdr, ICMP_HDRLEN);

        // After ICMP header, add the ICMP data.
        memcpy(packet + ICMP_HDRLEN, data, datalen);

        // Calculate the ICMP header checksum
        icmphdr.icmp_cksum = calculate_checksum((unsigned short *)(packet), ICMP_HDRLEN + datalen);
        memcpy((packet), &icmphdr, ICMP_HDRLEN);

        // Send the packet using sendto() for sending datagrams.
        int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *)&dest_in, sizeof(dest_in));
        if (bytes_sent == -1)
        {
            fprintf(stderr, "sendto() failed with error: %d\n", errno);
            return -1;
        }

        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        socklen_t len = sizeof(dest_in);
        ssize_t bytes_received = -1;

        //Set the timeout for recevfrom
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) !=0)
        {
            perror("Error in set TTL option");
        }

        if(fcntl(sock, F_SETFL, O_NONBLOCK) != 0)
        {
            perror("Non blocking error\n");
        }

        //Receiving loop
        while ((bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dest_in, &len)))
        {
            // Extract the IP header and ICMP header from the received packet
            char *ip_header = packet;
            int ip_header_len = (ip_header[0] & 0x0f) * 4;
            struct icmphdr *icmp_reply = (struct icmphdr*)(packet + ip_header_len);
            if (bytes_received > 0)
            {
                gettimeofday(&end, 0);
                float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
                unsigned long microseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec);
                // Check the IP header
                struct iphdr *iphdr = (struct iphdr *)packet;
                struct icmphdr *icmphdr = (struct icmphdr *)(packet + (iphdr->ihl * 4));


                printf("%s: seq=%d time=%fms\n", inet_ntoa(dest_in.sin_addr), icmp_reply->un.echo.sequence, milliseconds);

                int pingSend = 256;
                int checkSend = send(sockfd, &pingSend , sizeof(pingSend),0);
                if(checkSend > 0)
                {
                    printf("watchdog, make the timer zero!!\n");
                }
                else{
                    printf("Error in send\n");
                }

                break;
            }
        }

        sleep(1);
    }

    return 0;
}

// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}
