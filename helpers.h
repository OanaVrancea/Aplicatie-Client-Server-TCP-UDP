#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/stdc++.h>
#include <list>
using namespace std;


#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


struct message_from_udp
{
	char topic[50];
	uint8_t data_type;
	char content[1501];

};

struct notification
{
	char ip[16];
	unsigned int port;
	char topic[50];
	char data_type[11];
	char content[1501];
};

struct subscription
{
	int sf;
	char topic[50];
	int command;
};

struct client
{
	string id;
	int socket;
	int connected;
	vector <subscription> subscriptions;
	vector <notification> saved_messages;

};

void convert_udp_to_tcp(message_from_udp *udpm,
						notification *notif){

	strcpy(notif->topic, udpm->topic);

	if(udpm->data_type == 0){

		strcpy(notif->data_type, "INT");

		long long helper_number;
		int has_sign = 0;

		if(udpm->content[0]){
			has_sign = 1;
		}

		memcpy(&helper_number, udpm->content + 1, 4);
		helper_number = ntohl(helper_number);

		if(has_sign == 1){
			helper_number *= -1;
		}

		string convert = to_string(helper_number);
		strcpy(notif->content, convert.c_str());


	} else if(udpm->data_type == 1){

		strcpy(notif->data_type, "SHORT_REAL");

		double helper_number;

		memcpy(&helper_number, udpm->content , 2);
		helper_number = (ntohs(helper_number));
		helper_number = helper_number / 100;

		string convert = to_string(helper_number);
		strcpy(notif->content, convert.c_str());

	} else if(udpm->data_type == 2){

		strcpy(notif->data_type, "FLOAT");

		int has_sign = 0;
		double helper_number;

		if(udpm->content[0] == 1){
			has_sign = 1;
		}

		memcpy(&helper_number, udpm->content + 1, 4);
		helper_number = ntohl(helper_number);
		helper_number /= pow(10, udpm->content[5]);

		if(has_sign == 1){
			helper_number = -helper_number;
		} 

		string convert = to_string(helper_number);
		strcpy(notif->content, convert.c_str());


	} else if(udpm->data_type == 3){

		strcpy(notif->data_type, "STRING");
		memcpy(notif->content, udpm->content, strlen(udpm->content) + 1);
		notif->content[strlen(notif->content)] = '\0';

	}

}

#define BUFLEN		1552	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	256	// numarul maxim de clienti in asteptare

#endif
