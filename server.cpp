#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <list>


void usage(char *file)
{
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int udp_sockfd, tcp_sockfd, newsockfd, portno, found_client;
	char buffer[BUFLEN];
	struct sockaddr_in tcp_addr, udp_addr, cli_addr;
	int n, i, ret;
	long unsigned int k, j;
	socklen_t clilen;
	//vector in care se retin clientii
	vector <client> clients;
	client new_client;
    socklen_t udp_socket_len = sizeof(sockaddr);

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	fflush(stdout);

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//creare socket TCP
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "Socket-ul TCP nu poate fi creat");

	//creare socket UDP
	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "Socket-ul UDP nu poate fi creat");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	int flag = 1;
	ret = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	DIE(ret < 0, "Nu s-a putut dezactiva algoritmul Nagle");


	//am completat campurile pentru socketi
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(portno);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;


	ret = bind(tcp_sockfd, (struct sockaddr *) &tcp_addr, sizeof(sockaddr_in));
	DIE(ret < 0, "Nu se poate face bind cu socket-ul TCP");

	ret = bind(udp_sockfd, (struct sockaddr *) &udp_addr, sizeof(sockaddr_in));
	DIE(ret < 0, "Nu se poate face bind cu socket-ul UDP");


	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "Nu se poate face listen");


	// se adauga noul file descriptors in multimea read_fds
	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	if(udp_sockfd < tcp_sockfd){
		fdmax = tcp_sockfd;
	} else {
		fdmax = udp_sockfd;
	}

	int stop  = 0;

	while (stop == 0) {

		tmp_fds = read_fds; 
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Nu se poate face select");

		for(i = 0; i <= fdmax; i++){
			if(FD_ISSET(i, &tmp_fds)){
				if(!i){
					//primire comanda de la stdin
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "Eroare la primirea mesajului");

					if(strncmp(buffer, "exit", 4) == 0){
						stop = 1;
						break;
					} else {
						printf("Nu se poate primi nicio alta comanda!\n");
					}
				} else if(i == tcp_sockfd){
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					found_client = 0;

					clilen = sizeof(cli_addr);
					newsockfd = accept(i, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					//se primeste in buffer id-ul clientului TCP care incearca sa se conecteze
					n = recv(newsockfd, buffer, BUFLEN, 0);

					string id_string(buffer);


					for(k = 0; k < clients.size(); k++){
						//daca se gaseste un client cu id-ul primit
						if(clients[k].id == id_string){
							found_client = 1;
							//daca clientul este deconectat
							if(clients[k].connected == 0){
								//clientul se reconecteaza
								clients[k].connected = 1;
								clients[k].socket = newsockfd;

								printf("New client %s connected from %s:%d\n", buffer, 
  								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

								//i se trimit toate mesajele care s-au salvat cat timp 
								//a fost deconectat
  								for(j = 0; j < clients[k].saved_messages.size(); j++){
  									ret = send(newsockfd, (char*)& clients[k].saved_messages[j], sizeof(notification), 0);
  									DIE(ret < 0, "Nu se poate trimite mesajul");
  								}

  								clients[k].saved_messages.clear();
  								//daca clientul este conectat se printeaza mesajul corespunzator
							} else if(clients[k].connected == 1){
								printf("Client %s already connected.\n", buffer);
								close(newsockfd);
  								FD_CLR(newsockfd, &read_fds);
							}
						}
					}
					//daca id-ul respectiv nu a fost gasit, se va construi
					//un client nou care va fi adaugat la lista
					if(found_client == 0){

						string id_string(buffer);
						new_client.id = id_string;
  						new_client.socket = newsockfd;
  						new_client.connected = 1;

  						vector<subscription> s;
  						vector<notification> n;

  						new_client.subscriptions = s;
  						new_client.saved_messages = n;
  							

  						printf("New client %s connected from %s:%d\n", buffer, 
  							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

  	  					clients.push_back(new_client);

					}

				} else if(i == udp_sockfd){
					//se primeste un mesaj de la un client UDP
					message_from_udp *udp_msg = (message_from_udp *)malloc(sizeof(message_from_udp));

					ret = recvfrom(udp_sockfd, udp_msg, sizeof(message_from_udp),0, 
						(sockaddr*)&udp_addr, &udp_socket_len);
					DIE(ret < 0, "Nu s-a primit nimic de la clientul UDP!");

					notification notif;

					strcpy(notif.ip, inet_ntoa(udp_addr.sin_addr));
					notif.port = htons(udp_addr.sin_port);

					//se converteste mesajul udp intr-un mesaj de tip notification care
					//trebuie trimis clientilor
					convert_udp_to_tcp(udp_msg, &notif);

					//dupa construirea mesajului, parcurg toti clientii 
					for(k = 0; k < clients.size(); k++){
						for(long unsigned int jt = 0; jt < clients[k].subscriptions.size(); jt++){
							//daca sunt abonati la topicul respectiv verific ce se intampla cu notificarea
							if(strcmp(notif.topic, clients[k].subscriptions[jt].topic) == 0){
								//daca clientul este deconectat, se salveaza mesajele in vectorul 
								//de notificari corespunzator
								if( clients[k].connected == 0){
									if(clients[k].subscriptions[jt].sf == 1){
										clients[k].saved_messages.push_back(notif);
									}
									//daca clientul este online, i se trimite direct mesajul
								}else if(clients[k].connected == 1){
									ret = send(clients[k].socket, (char*) &notif, sizeof(notification), 0);
									DIE(ret < 0, "Nu se poate trimite mesajul la tcp!");
								}
							}
						}

				   }

				} else {
					//primire comanda de la client
					ret = recv(i, buffer, BUFLEN, 0);
					DIE(ret < 0, "Mesajul nu a putut fi receptionat");

  					int elem;

  					subscription* new_subs = (subscription*)malloc(sizeof(subscription));
  					memcpy(new_subs, &buffer, sizeof(subscription));

  					if(ret > 0){
  					//daca se primeste o comanda de tip subscribe
  					if(new_subs->command == 1){

  						int already = 0;
  						for(k = 0; k < clients.size(); k++){
  							if(clients[k].socket == i){
  								for(long unsigned int jt = 0; jt < clients[k].subscriptions.size(); jt++){
  									//daca clientul respectiv este abonat deja la topic, i se
  									//face update la campul sf
  									if(strcmp(clients[k].subscriptions[jt].topic, new_subs->topic) == 0 ){
  										clients[k].subscriptions[jt].sf = new_subs->sf;
  										already = 1;
  										break;
  									}
  								} 	
  								//daca clientul nu este abonat, i se adauga un nou
  								//abonament in vectorul corespunzator
  								if(already == 0){
  									clients[k].subscriptions.push_back(*new_subs);
  									break;
  								}
  							}  
  						}


					} else if(new_subs->command == 0){
						//daca comanda este unsubscribe, se dezaboneaza clientul de la un topic
						//prin scoaterea acestuia din lista de abonari
  						for(k = 0; k < clients.size(); k++){
  							if(clients[k].socket == i){
  								for(long unsigned int jt = 0; jt < clients[k].subscriptions.size(); jt++){
  									if(strcmp(clients[k].subscriptions[jt].topic, new_subs->topic) == 0){
  										clients[k].subscriptions.erase(clients[k].subscriptions.begin() + jt);
  										break;
  									}
  								}
  							}
  						}
					}
				} else {
					//inchidere conexiune

					for(k = 0; k < clients.size(); k++){
  						if(clients[k].socket == i){
  							clients[k].connected = 0;
  							int lungime = clients[k].id.length();
  							char v[lungime + 1];
  							strcpy(v, clients[k].id.c_str());
  							printf("Client %s disconnected.\n", v );
  							break;
  							}
  						}
  						close(i);
  						FD_CLR(i, &read_fds);

  						int new_max = fdmax;
  						for(int h = fdmax; h > 2; h--){
  							if(FD_ISSET(h, &read_fds)){
  								new_max = h;
  								break;
  							}
  						}

					}

				}
			}
		}
	}

	close(udp_sockfd);
	close(tcp_sockfd);
	close(newsockfd);

	return 0;
}
