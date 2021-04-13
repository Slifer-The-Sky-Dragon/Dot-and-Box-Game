#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#define STD_IN 0
#define STD_OUT 1
#define ZERO 0
#define ONE 1
#define EMPTY -1
#define MAX_PENDING_CON 100
#define MAX_INT_DIGIT_COUNT 10
#define MIN_GROUP_CNT 2
#define MAX_GROUP_CNT 5
#define MIN_GROUP_CHAR '2'
#define MAX_GROUP_CHAR '4'
#define MAX_MESSAGE_LEN 100
#define MAX_CLIENTS_CNT 20

int string_length(char* str){
	int i = 0;
	while(str[i] != '\0'){
		i++;
	}
	return i;
}

int is_equal(char* str1 , char* str2){
	if(string_length(str1) != string_length(str2))
		return ZERO;

	int length = string_length(str1);
	for(int i = 0 ; i < length ; i++){
		if(str2[i] != str2[i])
			return ZERO;
	}

	return ONE;
}

void fill_string_zero(char* str){
	for(int i = 0 ; i < MAX_MESSAGE_LEN ; i++)
		str[i] = '\0';
}

void convert_int_to_string(char* result , int x){
	int number[MAX_INT_DIGIT_COUNT];
	int cnt = 0;

	while(x > 0){
		int digit = x % 10;
		number[cnt] = digit;
		x = x / 10;
		cnt++;
	}

	for(int i = cnt - 1 ; i >= 0 ; i--){
		char cur_dig = ((char) number[i]) + '0';
		result[cnt - i - 1] = cur_dig;
	}
}

void concatenate_str(char* result , char* str){
	int result_len = string_length(result);

	for(int i = 0 ; i < string_length(str) ; i++){
		result[i + result_len] = str[i];
	}
}

void print(char* str){
	write(STD_OUT , str , string_length(str));
}

void print_int(int x){
	int temp_num[10] = {0};

	if(x == 0){
		print("0");
		return;
	}

	int len = 0;
	while(x != 0){
		temp_num[len] = x % 10;
		x /= 10;
		len++;
	}

	for(int i = len - 1 ; i >= 0 ; i--){
		char temp_char = ((char) temp_num[i]) + '0';
		print(&temp_char);
	}
}


void send_response_to_client(int client_sockfd , char* message){
	if(send(client_sockfd , message , MAX_MESSAGE_LEN , 0) != MAX_MESSAGE_LEN)
		print("Failed to send message...\n");
	else
		print("Message sent succesfully!\n");	
}

int check_recieved_group_number(char* message){
	if(string_length(message) > 2)
		return ZERO;

	int check_good_input = ONE;
	for(int i = 0 ; i < string_length(message) - 1 ; i++){
		if(message[i] < MIN_GROUP_CHAR || message[i] > MAX_GROUP_CHAR){
			check_good_input = ZERO;
			break;
		}
	}
	return check_good_input;
}

void create_TCP_socket(int* socket_fd){
	*socket_fd = socket(AF_INET , SOCK_STREAM , 0);
	if(*socket_fd < -1)
		print("Can't create TCP socket...\n");
	else
		print("TCP Socket created succesfully!\n");
}

void create_UDP_socket(int* socket_fd){
	*socket_fd = socket(AF_INET , SOCK_DGRAM , 0);
	if(*socket_fd < -1)
		print("Can't create UDP socket...\n");
}


void initialize_server_struct(struct sockaddr_in* server_address , int server_port){
	server_address -> sin_family = AF_INET;
	(server_address -> sin_addr).s_addr = htonl(INADDR_ANY);
	server_address -> sin_port = htons(server_port);
}

void binding_socket(int socket_fd , struct sockaddr_in* address){
	if(bind(socket_fd , (struct sockaddr *) address , sizeof(*address)) < 0)
		print("Server socket Can't Bind...\n");
	else
		print("Server socket binded succesfully!\n");	
}

void initializing_game_arrays(int group_cnt[MAX_GROUP_CNT][MAX_GROUP_CNT]){
	for(int i = 0 ; i < MAX_GROUP_CNT ; i++)
		for(int j = 0 ; j < MAX_GROUP_CNT ; j++)
			group_cnt[i][j] = EMPTY;
}


int make_reading_list(fd_set* read_sockfd_list , int server_sockfd , int client_sockfd_list[]){
	FD_ZERO(read_sockfd_list);
	FD_SET(server_sockfd , read_sockfd_list);
	int max_fd = server_sockfd;

	for(int i = 0 ; i < MAX_CLIENTS_CNT ; i++){
		if(client_sockfd_list[i] != EMPTY){
			FD_SET(client_sockfd_list[i] , read_sockfd_list);
		}
		if(max_fd < client_sockfd_list[i])
			max_fd = client_sockfd_list[i];
	}	

	return max_fd;
}

void check_for_new_connection(int server_sockfd , fd_set* read_sockfd_list , int client_sockfd_list[]){
	int client_sockfd;
	struct sockaddr_in client_address;

	if(FD_ISSET(server_sockfd , read_sockfd_list)){
		socklen_t clsize = sizeof(client_address);

		if((client_sockfd = accept(server_sockfd , (struct sockaddr *) &client_address , &clsize)) < 0)
			print("Connection accept error...\n");
		else{
			print("New client has joined...\n");

			send_response_to_client(client_sockfd , "Welcome to Dot and Boxes Game\nEnter your group number : \n");

			for(int i = 0 ; i < MAX_CLIENTS_CNT ; i++){
				if(client_sockfd_list[i] == EMPTY){
					client_sockfd_list[i] = client_sockfd;
					break;
				}
			}
		}
	}
}

void delete_client_from_sockfd_list(int client_sockfd , int client_sockfd_list[]){
	for(int i = 0 ; i < MAX_CLIENTS_CNT ; i++){
		if(client_sockfd_list[i] == client_sockfd){
			client_sockfd_list[i] = EMPTY;
			break;
		}
	}
}

void delete_client_group(int client_sockfd , int group[MAX_GROUP_CNT][MAX_GROUP_CNT]){
	for(int i = MIN_GROUP_CNT ; i < MAX_GROUP_CNT ; i++){
		for(int j = 0 ; j < i - 1 ; j++){
			if(group[i][j] == client_sockfd)
				group[i][j] = EMPTY;
			if(group[i][j] == EMPTY){
				group[i][j] = group[i][j + 1];
				group[i][j + 1] = EMPTY;
			}
		}
		if(group[i][i - 1] == client_sockfd)
			group[i][i - 1] = EMPTY;
	}
}

void manage_groups(int client_sockfd , char* message , int group[MAX_GROUP_CNT][MAX_GROUP_CNT]){
	delete_client_group(client_sockfd , group);

	int group_number = atoi(message);
	for(int j = 0 ; j < group_number ; j++){
		if(group[group_number][j] == EMPTY){
			group[group_number][j] = client_sockfd;
			break;
		}
	}
}

void show_groups_cnt(int group[MAX_GROUP_CNT][MAX_GROUP_CNT]){
	print("******Groups information:\n");
	for(int i = MIN_GROUP_CNT ; i < MAX_GROUP_CNT ; i++){
		print("Group ");
		print_int(i);
		print(" : ");
		int cnt = 0;
		for(int j = 0 ; j < i ; j++){
			if(group[i][j] != EMPTY)
				cnt++;
		}
		print_int(cnt);
		print("\n");	
	}
}

int find_new_UDP_port(int server_port){
	int result_port;

	int test_sockfd;
	create_UDP_socket(&test_sockfd);

	struct sockaddr_in* dest_address = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
	dest_address -> sin_family = AF_INET;
	(dest_address -> sin_addr).s_addr = htonl(INADDR_ANY);

	for(int port_number = 1000 ; port_number < 9000 ; port_number++){
		if(port_number == server_port)
			continue;

		dest_address -> sin_port = htons(port_number);

		if(bind(test_sockfd , (struct sockaddr *) dest_address , sizeof(*dest_address)) < 0)
			continue;

		result_port = port_number;
		close(test_sockfd);
		break;
	}

	free(dest_address);

	return result_port;
}

void start_complete_groups_game(int client_sockfd_list[] , int group[MAX_GROUP_CNT][MAX_GROUP_CNT] ,
									 char* result , int server_port){
	for(int i = MIN_GROUP_CNT ; i < MAX_GROUP_CNT ; i++){
		int cnt = 0;
		for(int j = 0 ; j < i ; j++)
			if(group[i][j] != EMPTY)
				cnt++;
		if(cnt == i){//Ready to start
			char member_cnt[2] = {(char) (i + '0') , '\0'};

			print("A ");
			print(member_cnt);
			print(" member game is going to start...\n");

			int new_port = find_new_UDP_port(server_port);

			for(int j = 0 ; j < i ; j++){
				fill_string_zero(result);
				convert_int_to_string(result , new_port);

				char priority[2] = {(char) (j + '0') , '\0'};

				concatenate_str(result , "#");
				concatenate_str(result , member_cnt);
				concatenate_str(result , "#");
				concatenate_str(result , priority);
				concatenate_str(result , "#");
				concatenate_str(result , "\n");

				send_response_to_client(group[i][j] , result);

				delete_client_from_sockfd_list(group[i][j] , client_sockfd_list);
				group[i][j] = EMPTY;
			}

			print("The game started!\n");
		}
	}
}

void check_for_clients_responses(fd_set* read_sockfd_list , int client_sockfd_list[] , 
									int group[MAX_GROUP_CNT][MAX_GROUP_CNT] , char* result , int server_port){

	for(int i = 0 ; i < MAX_CLIENTS_CNT ; i++){
		if(client_sockfd_list[i] == EMPTY)
			continue;

		if(FD_ISSET(client_sockfd_list[i] , read_sockfd_list)){
			fill_string_zero(result);
			int message_len = recv(client_sockfd_list[i] , result , MAX_MESSAGE_LEN , 0);

			if(message_len < 0)
				print("Recieving error...\n");
			else if(message_len == 0){
				print("A client has diconnected from server! Closing socket...\n");
				close(client_sockfd_list[i]);
				delete_client_group(client_sockfd_list[i] , group);
				client_sockfd_list[i] = EMPTY;
			}
			else{
				if(check_recieved_group_number(result)){
					print("Someone choosed his/her group number!\n");
					manage_groups(client_sockfd_list[i] , result , group);
					
					char group_number = *result;

					fill_string_zero(result);
					concatenate_str(result , "Group number = ");
					concatenate_str(result , &group_number);
					concatenate_str(result , "\n");
					concatenate_str(result , "Wait for others to join...\nYou can Enter another group before game starts : \n");
					send_response_to_client(client_sockfd_list[i] , result);

					show_groups_cnt(group);
				}
				else{
					print("Wrong group number recieved...\n");
					send_response_to_client(client_sockfd_list[i] , "Wrong group number! Enter your group number : \n");
				}
			}

			start_complete_groups_game(client_sockfd_list , group , result , server_port);
		}
	}
}


int main(int argc , char* argv[]){
	int server_port;
	if(argc != 2){
		if(argc < 2)
			print("Server Port is missing...\n");
		else
			print("Wrong Input files...\n");
		return 0;
	}
	else{
		for(int i = 0 ; i < string_length(argv[1]) ; i++){
			if(argv[1][i] < '0' || argv[1][i] > '9'){
				print("Wrong Input files...\n");
				return 0;
			}
		}
		server_port = atoi(argv[1]);
	}
	//variables
	int server_sockfd;
	struct sockaddr_in server_address;

	fd_set read_sockfd_list;
	int client_sockfd_list[MAX_CLIENTS_CNT];

	int group[MAX_GROUP_CNT][MAX_GROUP_CNT];

	char* result = (char*) malloc(MAX_MESSAGE_LEN * sizeof(char));
	//initializing server
	print("Dot and Boxes game server - Server Port : ");
	print(argv[1]);
	print("\n");

	create_TCP_socket(&server_sockfd);
	initialize_server_struct(&server_address , server_port);
	binding_socket(server_sockfd , &server_address);

	initializing_game_arrays(group);

	if(listen(server_sockfd , MAX_PENDING_CON) < 0)
		write(STD_OUT , "Listening error...\n" , 19);

	//initializing client list
	for(int i = 0 ; i < MAX_CLIENTS_CNT ; i++)
		client_sockfd_list[i] = EMPTY;

	//communicate with clients with listen
	while(1){
		int max_fd = make_reading_list(&read_sockfd_list , server_sockfd , client_sockfd_list);

		int activity = select(max_fd + 1 , &read_sockfd_list , NULL , NULL , NULL);
		if(activity < 0)
			print("Select Error...\n");
		
		check_for_new_connection(server_sockfd , &read_sockfd_list , client_sockfd_list);
		check_for_clients_responses(&read_sockfd_list , client_sockfd_list , group , result , server_port);
	}
}
