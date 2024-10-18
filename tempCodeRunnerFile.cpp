sendBytes = sendto(sockfd, result, strlen(result), 0, (struct sockaddr *) &cliaddr, addr_len);
        // if(sendBytes < 0){
        //     perror("Error sending data to client: ");
        //     return 0;
        // }