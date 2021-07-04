#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <bits/stdc++.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}

	if(strlen(argv[1]) > 10){
		fprintf(stderr, "Not the right size for an ID\n");
		exit(0);
	}


	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	fflush(stdout);

	fd_set read_fds;      //multime de citire folosita in select
	fd_set tmp_fds;       //multime folosita temporar
	int fdmax;            //valoare maxima file descriptor din multimea read_fds

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

  //creare socket pentru a comunica cu serverul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Nu s-a putut deschide socket-ul");
  
  // completare informatii socket TCP
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Adresa IP gresita\n");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Nu s-a putut conecta la server");

	ret = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(ret < 0, "Nu s-a putut trimite catre server");

	int flag = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	DIE(ret < 0, "Nu s-a putut dezactiva algoritmul Nagle");

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;


	while (1) {
  		tmp_fds = read_fds;
  		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
  		DIE(ret < 0, "Eroare la alegere socket");

      //primire mesaj de la stdin
  		if(FD_ISSET(STDIN_FILENO, &tmp_fds)){

  			memset(buffer, 0, BUFLEN);
  			fgets(buffer, BUFLEN - 1, stdin);

        //se va deconecta si inchide clientul
  			if(strncmp(buffer, "exit", 4) == 0){
  				ret = shutdown(sockfd, SHUT_RDWR);
  				DIE(ret < 0, "NU SE POATE DA EXIT");
  				break;
  			} 

  			//daca nu este comanda exit, vom primi comanda subscribe 
  			//sau unsubscribe care sunt urmate de parametrii

  			char delim[] = " ";
  			char* ptr = strtok(buffer, delim);
  			subscription sub;
  			int ok = 0;

        //daca este comanda subscribe, vor mai urma 2 parametrii : topic si sf
  			if(strcmp(ptr, "subscribe") == 0){

          //pt subscribe command = 1
  				sub.command = 1; 

          //in ptr vom salva topicul la care se face abonarea
  				ptr = strtok(nullptr, delim);

          if(strlen(ptr) > 50){

            printf("Lungimea topicului nu este buna!\n");
            ok = 1;

          } else {

            strcpy(sub.topic, ptr);

          }
  				
          //in ptr acum se va afla sf-ul cu care se face abonarea(0 sau 1)
  				ptr = strtok(nullptr, delim);
  				int ptr_converted_to_int = *ptr - '0';

  				if(ptr_converted_to_int != 0  && ptr_converted_to_int != 1){

  					printf("Valoare invalida pentru sf!\n");
            ok = 1;

  				} else {

  					sub.sf = ptr_converted_to_int;

  				}

          // daca este comanda unsubscribe, va fi urmata de un singur parametru
  			} else if(strcmp(ptr, "unsubscribe") == 0){

           //pt unsubscribe command = 0
  				sub.command = 0;

          //in ptr se salveaza topicul de la care clientul TCP se dezaboneaza
  				ptr = strtok(NULL, delim);

  				if(strlen(ptr) > 50){

  					printf("Lungimea topicului nu este buna!\n");
            ok = 1;

  				} else {

  					strcpy(sub.topic, ptr);

  				}
 
  			} else {
  				printf("Comanda tastata nu exista!\n");
  			}

  			if(ok == 0){

          //daca nu a aparut nicio eroare, se trimite mesajul la server
  				ret = send(sockfd,(char*) &sub, sizeof(sub), 0);
  				DIE(ret < 0, "Nu s-a putut trimite mesajul la server");

  				if(sub.command == 1){
  					printf("Subscribed to topic.\n");
  				} else {
  					printf("Unsubscribed from topic.\n");
  				}
  			}
		}

    //se primeste mesaj de la server
		 else if(FD_ISSET(sockfd, &tmp_fds)){

			memset(buffer, 0, (sizeof(struct notification)));
			n = recv(sockfd, buffer, (sizeof(struct notification)), 0);
			DIE(n < 0, "Eroare la primirea mesajului din partea serverului");

      //salvez mesajul de la server intr-o structura de tip notificare
      notification *notf = (notification*)malloc(sizeof(notification));
      memcpy(notf, &buffer, sizeof(notification));

			if(n == 0){
				break;
			}

      //se afiseaza datele
      printf("%s:%hu - %s - %s - %s\n", notf->ip,
        notf->port, notf->topic, notf->data_type,
        notf->content );
	}
}


	close(sockfd);

	return 0;
}
