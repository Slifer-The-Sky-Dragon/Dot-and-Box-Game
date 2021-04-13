#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define EXIT 0
#define CONTINUE 1
#define STDIN 0
#define STDOUT 1
#define MAX_PENDING_CON 100
#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"
#define BROADCAST_ADDRESS "255.255.255.255"

#define INPUT_TIME 20

#define MAX_MESSAGE_LEN 100

#define NEXT_TURN_BIT 99

#define ZERO 0
#define ONE 1

#define PORT_PART 0
#define GROUP_PART 1
#define PRIORITY_PART 2

//Signal handler parameters
	int client_sockfd;
	int game_group_cnt;
	struct sockaddr_in broadcast_addr;
	char* game_map;
//

int string_length(char* str){
	int i = 0;
	while(str[i] != '\0'){
		i++;
	}
	return i;
}

void print(char* str){
	write(STDOUT , str , string_length(str));
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

void fill_string_zero(char* str){
	for(int i = 0 ; i < MAX_MESSAGE_LEN ; i++)
		str[i] = '\0';
}

void string_copy(char* map , char* message){
	fill_string_zero(map);

	for(int i = 0 ; i < string_length(message) ; i++){
		if(message[i] == '\n')
			break;
		map[i] = message[i];
	}
}


void create_TCP_socket(int* socket_fd){
	*socket_fd = socket(AF_INET , SOCK_STREAM , 0);
	if(*socket_fd < -1)
		print("Can't connect socket...\n");
	else
		print("TCP socket created succesfully!\n");
}

void create_UDP_socket(int* socket_fd){
	*socket_fd = socket(AF_INET , SOCK_DGRAM , 0);
	if(*socket_fd < -1)
		print("Can't create UDP socket...\n");
	else
		print("UDP socket created succesfully!\n");

}

void initialize_server_struct(struct sockaddr_in* server_address , int server_port){
	server_address -> sin_family = AF_INET;
	(server_address -> sin_addr).s_addr = htonl(INADDR_ANY);
	server_address -> sin_port = htons(server_port);
}

void connect_socket_to_server(int* client_sockfd , struct sockaddr_in* server_address){
	if(connect(*client_sockfd , (struct sockaddr *) server_address , sizeof(*server_address)) < 0)
		print("Can not connect to server...\n");
	else
		print("Connected to server succesfully!\n");
}

int make_reading_list(fd_set* read_sockfd_list , int client_sockfd){
	FD_ZERO(read_sockfd_list);

	FD_SET(client_sockfd , read_sockfd_list);
	FD_SET(STDIN , read_sockfd_list);

	int max_fd = client_sockfd;

	return max_fd;
}

int is_game_start_command(char* message){
	for(int i = 0 ; i < string_length(message) ; i++)
		if(message[i] == '#')
			return ONE;
	return ZERO;
}

void initialize_ingame_variables(char* helper , char* message , int* game_port , int* group_cnt , int* game_priority){
	fill_string_zero(helper);

	int part = PORT_PART;
	int temp_index = 0;

	print("Game info : ");
	for(int i = 0 ; i < string_length(message) ; i++){
		if(message[i] == '#'){
			if(part == PORT_PART){
				print("Port = ");
				*game_port = atoi(helper);
			}
			else if(part == GROUP_PART){
				print("Group Members Count = ");
				*group_cnt = atoi(helper);
			}
			else{
				print("Game Priority = ");
				*game_priority = atoi(helper);
			}
			
			print(helper);
			print(" ");
			fill_string_zero(helper);
			temp_index = 0;
			part++;
			continue;
		}
		helper[temp_index] = message[i];
		temp_index++;
	}
	print("\n");
}


int check_for_recieving_messages(int client_sockfd , fd_set* read_sockfd_list , char* message , 
												char* helper , int* game_port , int* group_cnt , int* game_priority){
	if(FD_ISSET(client_sockfd , read_sockfd_list)){
		fill_string_zero(message);
		if(recv(client_sockfd , message , MAX_MESSAGE_LEN , 0) < 0)
			print("Recieving Error...\n");
		else{
			if(is_game_start_command(message) == ONE){
				initialize_ingame_variables(helper , message , game_port , group_cnt , game_priority);
				return EXIT;
			}
			else{
				print(message);
			}
		}
	}
	else if(FD_ISSET(STDIN , read_sockfd_list)){
		fill_string_zero(message);
		read(STDIN, message , MAX_MESSAGE_LEN);

		if(send(client_sockfd , message , string_length(message) , 0) != string_length(message))
			print("Message didn't send completely!\n");
		else
			print("Message sent!\n");
	}
	return CONTINUE;
}

//In game
void initialize_game_dest_struct(struct sockaddr_in* game_dest_address , int game_port){
	game_dest_address -> sin_family = AF_INET;
	(game_dest_address -> sin_addr).s_addr = htonl(INADDR_ANY);
	game_dest_address -> sin_port = htons(game_port);	
}

void initialize_broadcast_address_struct(struct sockaddr_in* broadcast_addr , int game_port){
	broadcast_addr -> sin_family = AF_INET;
	(broadcast_addr -> sin_addr).s_addr = inet_addr(BROADCAST_ADDRESS);
	broadcast_addr -> sin_port = htons(game_port);	
}

void binding_socket(int socket_fd , struct sockaddr_in* address){
	if(bind(socket_fd , (struct sockaddr *) address , sizeof(*address)) < 0)
		print("Socket Can't Bind...\n");
	else
		print("Socket binded succesfully!\n");
}

void initialize_scores(int* score , int group_cnt){
	for(int i = 0 ; i < group_cnt ; i++)
		score[i] = 0;
}


void initialize_start_map(char* map , int group_cnt){
	fill_string_zero(map);
	int cur_index = 0;
	for(int i = 0 ; i < group_cnt ; i++){
		for(int j = 0 ; j < group_cnt ; j++){
			map[cur_index] = '0';
			cur_index++;
		}
		for(int j = 0 ; j < group_cnt + 1 ; j++){
			map[cur_index] = '0';
			cur_index++;
		}
	}

	for(int i = 0 ; i < group_cnt ; i++){
		map[cur_index] = '0';
		cur_index++;		
	}
	map[cur_index] = '2';
	map[cur_index] = '\n';
}

void print_graphical_map(char* map , int group_cnt){
	int cur_index = 0;
	for(int i = 0 ; i < group_cnt + 1 ; i++){
		print("+");
		for(int j = 0 ; j < group_cnt ; j++){
			if(map[cur_index] == '1')
				print("--");
			else
				print("  ");
			print("+");
			cur_index ++;
		}

		print("\n");
		if(i != group_cnt){
			if(map[cur_index] == '1')
				print("|  ");
			else
				print("   ");

			cur_index++;
			for(int j = 0 ; j < group_cnt ; j++){
				if(map[cur_index] == '1')
					print("|  ");
				else
					print("   ");	
				cur_index++;	
			}
			print("\n");
		}
	}	
}


void print_turn_info(int turn , int game_priority , int* score , int group_cnt){
	
	print("Scores: ");
	for(int i = 0 ; i < group_cnt ; i++){
		print("P");
		print_int(i + 1);
		print(" = ");
		print_int(score[i]);
		print(" ");
	}

	print("\nYour score : ");
	print_int(score[game_priority]);
	print("\n");

	if(turn == game_priority){
		print("It's your turn!\nEnter two points cordinate (You have only 20 secs) :\n");
	}
	else{
		print("waiting for others to play...\n");
	}
}

int absolute_value(int x){
	if(x > 0)
		return x;
	return -x;
}

int find_index_of_points_in_map(int p1x , int p1y , int p2x , int p2y , int group_cnt){
	int index = 0;
	p1x--;
	p2x--;
	p1y--;
	p2y--;
	if(p1x == p2x){
		if(p1y > p2y){
			int help = p1y;
			p1y = p2y;
			p2y = help;
		}

		for(int i = 0 ; i < p1x ; i++){
			index += group_cnt + (group_cnt + 1);
		}
		for(int j = 0 ; j < p1y ; j++){
			index++;
		}
		return index;
	}
	else{
		if(p1x > p2x){
			int help = p1x;
			p1x = p2x;
			p2x = help;
		}

		for(int i = 0 ; i < p1x ; i++){
			index += group_cnt + (group_cnt + 1);
		}
		index += group_cnt;
		for(int j = 0 ; j < p1y ; j++){
			index++;
		}
		return index;
	}
}

int are_points_connectable(int points[3] , int group_cnt , char* map){
	int p1x = points[0];
	int p1y = points[1];
	int p2x = points[2];
	int p2y = points[3];

	int result = ZERO;
	if(p1x == p2x && absolute_value(p1y - p2y) == 1)
		result = ONE;
	if(p1y == p2y && absolute_value(p1x - p2x) == 1)
		result = ONE;

	if(result == ZERO)
		return ZERO;

	int index = find_index_of_points_in_map(p1x , p1y , p2x , p2y , group_cnt);
	if(map[index] == '1')
		return ZERO;
	return ONE;
}

int is_good_game_input(char* message , int points[3] , int group_cnt , char* map){
	int temp_cnt = 0;
	int input_parts_cnt = 0;

	for(int i = 0 ; i < string_length(message) ; i++){
		if(message[i] == ' ' || message[i] == '\n'){
			if(temp_cnt > 1)
				return ZERO;
			if(message[i - 1] < '1' || message[i - 1] > '9')
				return ZERO;
			int message_int = atoi(&message[i - 1]);
			if(message_int < 1 || message_int > group_cnt + 1)
				return ZERO;

			points[input_parts_cnt] = message_int;
			input_parts_cnt++;
			temp_cnt = 0;
		}
		else{
			temp_cnt++;
		}
	}

	if(input_parts_cnt != 4)
		return ZERO;	

	if(are_points_connectable(points , group_cnt , map) == ZERO){
		return ZERO;
	}

	return ONE;
}

void add_line_to_map(int points[3] , int group_cnt , char* map){
	int index = find_index_of_points_in_map(points[0] , points[1] , points[2] , points[3] , group_cnt);
	map[index] = '1';
}

int calculate_added_score(int points[3] , int group_cnt , char* map){
	int p1x = points[0];
	int p1y = points[1];
	int p2x = points[2];
	int p2y = points[3];

	int result = 0;
	if(p1x == p2x){
		if(p1y > p2y){
			int help = p1y;

			p1y = p2y;
			p2y = help;
		}
		if(p1x > 1){
			int index1 = find_index_of_points_in_map(p1x , p1y , p1x - 1 , p1y , group_cnt);
			int index2 = find_index_of_points_in_map(p2x , p2y , p2x - 1 , p2y , group_cnt);
			int index3 = find_index_of_points_in_map(p1x - 1 , p1y , p2x - 1 , p2y , group_cnt);

			if(map[index1] == '1' && map[index2] == '1' && map[index3] == '1')
				result++;
		}
		if(p1x < group_cnt + 1){
			int index1 = find_index_of_points_in_map(p1x , p1y , p1x + 1 , p1y , group_cnt);
			int index2 = find_index_of_points_in_map(p2x , p2y , p2x + 1 , p2y , group_cnt);
			int index3 = find_index_of_points_in_map(p1x + 1 , p1y , p2x + 1 , p2y , group_cnt);

			if(map[index1] == '1' && map[index2] == '1' && map[index3] == '1')
				result++;			
		}
	}	
	else{ //p1y == p2y
		if(p1x > p2x){
			int help = p1x;

			p1x = p2x;
			p2x = help;
		}

		if(p1y > 1){
			int index1 = find_index_of_points_in_map(p1x , p1y , p1x , p1y - 1 , group_cnt);
			int index2 = find_index_of_points_in_map(p2x , p2y , p2x , p2y - 1 , group_cnt);
			int index3 = find_index_of_points_in_map(p1x , p1y - 1 , p2x , p2y - 1 , group_cnt);

			if(map[index1] == '1' && map[index2] == '1' && map[index3] == '1')
				result++;
		}
		if(p1y < group_cnt + 1){
			int index1 = find_index_of_points_in_map(p1x , p1y , p1x , p1y + 1 , group_cnt);
			int index2 = find_index_of_points_in_map(p2x , p2y , p2x , p2y + 1 , group_cnt);
			int index3 = find_index_of_points_in_map(p1x , p1y + 1 , p2x , p2y + 1 , group_cnt);

			if(map[index1] == '1' && map[index2] == '1' && map[index3] == '1')
				result++;
		}
	}
	return result;
}

int is_game_finished(char* map){
	for(int i = 0 ; i < string_length(map) ; i++){
		if(map[i] == '0')
			return ZERO;
	}
	return ONE;
}

void go_to_next_turn(){
	int score_bit = 2 * (game_group_cnt) * (game_group_cnt + 1);

	game_map[score_bit] = '2';

	if(sendto(client_sockfd , game_map , string_length(game_map) , 0 , (struct sockaddr*) &broadcast_addr , 
														sizeof(broadcast_addr)) != string_length(game_map))
		print("Message didn't send completely!\n");
	else
		print("Message sent!\n");
}

void InterruptHandler(int signalType){
	if(signalType == SIGALRM){
		print("Timeout!\n");
		go_to_next_turn();
	}
	else{
		print("Interrupt received. Exiting program.\n");
		exit(1);
	}
}

void setting_alarm_signal(struct sigaction* alarm_handler){
	alarm_handler -> sa_handler = InterruptHandler;
	if(sigfillset(&(alarm_handler -> sa_mask)) < 0)
		print("Can not fill sigaction...\n");
	alarm_handler -> sa_flags = 0;

	if(sigaction(SIGALRM , alarm_handler , 0) < 0)
		print("Can not set SIGALRM...\n");
}


int check_for_ingame_messages(int client_sockfd , struct sockaddr_in* recieving_addr , 
									struct sockaddr_in* broadcast_addr , fd_set* read_sockfd_list , 
									char* message , char* map , int* turn , int game_priority , 
									int group_cnt , int* score){

	if(FD_ISSET(client_sockfd , read_sockfd_list)){
		fill_string_zero(message);

		socklen_t addr_len = sizeof(*recieving_addr);
		if(recvfrom(client_sockfd , message, MAX_MESSAGE_LEN, 0 , (struct sockaddr*) recieving_addr , &addr_len) < 0){
			print("Error in Recieving...\n");
		}
		else{
			int score_bit = 2 * (group_cnt) * (group_cnt + 1);

			if(message[score_bit] == '2')
				*turn = (*turn + 1) % group_cnt;
			else if(message[score_bit] == '3')
				score[*turn] += 1;
			else
				score[*turn] += 2;

			alarm(0); // delete old alarms

			string_copy(map , message);

			if(is_game_finished(map) == ONE)
				return EXIT;

			print_graphical_map(map , group_cnt);
			print_turn_info(*turn , game_priority , score , group_cnt);
			if(*turn == game_priority)
				alarm(INPUT_TIME);
		}
	}
	else if(FD_ISSET(STDIN , read_sockfd_list)){
		fill_string_zero(message);
		read(STDIN, message , MAX_MESSAGE_LEN);

		int points[4] = {0 , 0 , 0 , 0};
		if(*turn != game_priority){
			print("It's not your turn!\nwaiting for others to play...\n");
		}
		else if(is_good_game_input(message , points , group_cnt , map)){
			int added_score = calculate_added_score(points , group_cnt , map);
			add_line_to_map(points , group_cnt , map);

			int score_bit = 2 * (group_cnt) * (group_cnt + 1);

			if(added_score == 2)
				map[score_bit] = '4';
			else if(added_score == 1)
				map[score_bit] = '3';
			else
				map[score_bit] = '2';

			if(sendto(client_sockfd , map , string_length(map) , 0 , (struct sockaddr*) broadcast_addr , 
																sizeof(*broadcast_addr)) != string_length(map))
				print("Message didn't send completely!\n");
			else
				print("Message sent!\n");
		}
		else{
			print("Wrong input! Enter two points cordinate : \n");
		}		
	}
	return CONTINUE;
}

int find_winner_score(int* score , int group_cnt){
	int win_score = 0;
	for(int i = 0 ; i < group_cnt ; i++){
		if(score[i] > win_score)
			win_score = score[i];
	}
	return win_score;
}
void print_winner_of_game(int* score , int group_cnt , int game_priority){
	int winner_score = find_winner_score(score , group_cnt);
	if(score[game_priority] == winner_score)
		print("********You won!  - ");
	else
		print("********You lost! - ");

	print("Your score : ");
	print_int(score[game_priority]);
	print(" ********\n");

	for(int i = 0 ; i < group_cnt ; i++){
		print("P");
		print_int(i + 1);
		print(" : ");
		print_int(score[i]);
		print(" ");
	}
	print("\n");
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
	struct sockaddr_in server_address , game_dest_address , recieving_addr;
	struct sigaction alarm_handler;

	fd_set read_sockfd_list;

	char* message = (char *) malloc(MAX_MESSAGE_LEN * sizeof(char));
	char* helper = (char *) malloc(MAX_MESSAGE_LEN * sizeof(char));
	game_map = (char *) malloc(MAX_MESSAGE_LEN * sizeof(char));

	//in game variables
	int game_port;
	int game_priority;
	int turn = 0;
	int* score = (int*) malloc(game_group_cnt * sizeof(int));

	//initializing variables
	initialize_server_struct(&server_address , server_port);
	setting_alarm_signal(&alarm_handler);

	//make connection with server
	create_TCP_socket(&client_sockfd);
	connect_socket_to_server(&client_sockfd , &server_address);

	//waiting for start game command
	int server_status = CONTINUE;
	while(server_status == CONTINUE){
		int max_fd = make_reading_list(&read_sockfd_list , client_sockfd);
		int activity = select(max_fd + 1 , &read_sockfd_list , NULL , NULL , NULL);

		if(activity < 0)
			print("Select Error...\n");

		server_status = check_for_recieving_messages(client_sockfd , &read_sockfd_list , message , helper ,
																	 &game_port , &game_group_cnt , &game_priority);
	}

	close(client_sockfd);

	//In game
	initialize_game_dest_struct(&game_dest_address , game_port);
	initialize_broadcast_address_struct(&broadcast_addr , game_port);
	create_UDP_socket(&client_sockfd);

	int enable = 1;
	if(setsockopt(client_sockfd , SOL_SOCKET , SO_REUSEPORT , &enable , sizeof(int)) < 0)
		print("Can't set SO_REUSEPORT...\n");
	else
		print("SO_REUSEPORT set succesfully!\n");

	if(setsockopt(client_sockfd , SOL_SOCKET , SO_BROADCAST , &enable , sizeof(int)) < 0)
		print("Can't set SO_BROADCAST...\n");
	else
		print("SO_BROADCAST set succesfully!\n");

	binding_socket(client_sockfd , &game_dest_address);

	initialize_start_map(game_map , game_group_cnt);
	initialize_scores(score , game_group_cnt);
	print_graphical_map(game_map , game_group_cnt);
	print_turn_info(turn , game_priority , score , game_group_cnt);

	int game_status = CONTINUE;

	if(turn == game_priority)
		alarm(20);
	while(game_status == CONTINUE){
		int max_fd = make_reading_list(&read_sockfd_list , client_sockfd);

		int activity = select(max_fd + 1 , &read_sockfd_list , NULL , NULL , NULL);

		game_status = check_for_ingame_messages(client_sockfd , &recieving_addr , &broadcast_addr ,
							&read_sockfd_list , message , game_map , &turn , game_priority , game_group_cnt , score);
	}

	print_winner_of_game(score , game_group_cnt , game_priority);

	close(client_sockfd);
}

