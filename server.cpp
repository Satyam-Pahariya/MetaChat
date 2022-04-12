#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;
map<char*,char*> m;
struct terminal
{
	int id;
	string name;
	string password;
	int socket;
	thread th;
	vector<int> vid;
	bool mark;
};

vector<terminal> clients;
string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m","\033[36m"};
int seed=0;
mutex cout_mtx,clients_mtx;

string color(int code);
void set_name(int id, char name[],char pass[]);
bool Check_password(string Password,int id);
void synchronous_print(string str, bool endLine);
void broadcast_message(string message,int rev_id);
void broadcast_message(int num,int rev_id);
void broadcast_message_all(string message, int sender_id);
void broadcast_message_all(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);
int Find(char Rec[]);
bool checkflag(int rev_id,int child_id);
int main()
{
	int server_socket;
	if((server_socket=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_port=htons(10000);
	server.sin_addr.s_addr=INADDR_ANY;
	bzero(&server.sin_zero,0);

	if((bind(server_socket,(struct sockaddr *)&server,sizeof(struct sockaddr_in)))==-1)
	{
		perror("bind error: ");
		exit(-1);
	}

	if((listen(server_socket,8))==-1)
	{
		perror("listen error: ");
		exit(-1);
	}

	struct sockaddr_in client;
	int client_socket;
	unsigned int len=sizeof(sockaddr_in);

	cout<<colors[NUM_COLORS-1]<<"\n\t  ~ ~ ~ ~ MetaChat Server ~ ~ ~ ~    "<<endl<<def_col;

	while(1)
	{
		if((client_socket=accept(server_socket,(struct sockaddr *)&client,&len))==-1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		thread t(handle_client,client_socket,seed);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({seed, string("Anonymous"),string(""),client_socket,(move(t))});
	}

	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].th.joinable())
			clients[i].th.join();
	}

	close(server_socket);
	return 0;
}

string color(int code)
{
	return colors[code%NUM_COLORS];
}

// Set name of client
void set_name(int id, char name[],char pass[])
{
	for(int i=0; i<clients.size(); i++)
	{
			if(clients[i].id==id)	
			{
				clients[i].name=string(name);
				clients[i].password=string(pass);
			}
	}	
}

bool Check_password(string Password,int id)
{
	for(int i=0; i<clients.size(); i++)
        {
                        if(clients[i].id==id)
                        {
                        	return Password.compare(clients[i].password);
                        }
        }

}


// For synchronisation of cout statements
void synchronous_print(string str, bool endLine=true)
{	
	lock_guard<mutex> guard(cout_mtx);
	cout<<str;
	if(endLine)
			cout<<endl;
}

// Broadcast message to all clients except the sender
void broadcast_message(string message, int rev_id)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
    	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].id==rev_id)
		{
			send(clients[i].socket,temp,sizeof(temp),0);
			if(clients[i].mark)
			{
				for(int j=0;j<clients[i].vid.size();j++)
				{
					broadcast_message(temp,clients[i].vid[j]);
				}
			}
		}
	}
}

// Broadcast a number to all clients except the sender
void broadcast_message(int num, int rev_id)
{
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].id==rev_id)
		{
			send(clients[i].socket,&num,sizeof(num),0);
			if(clients[i].mark)
			{
				for(int j=0;j<clients[i].vid.size();j++)
				{
					broadcast_message(clients[i].id,clients[i].vid[j]);
				}
			}
		}
	}		
}

void broadcast_message_all(string message, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].id!=sender_id)
		{
			send(clients[i].socket,temp,sizeof(temp),0);
		}
	}

}

// Broadcast a number to all clients except the sender
void broadcast_message_all(int num, int sender_id)
{
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].id!=sender_id)
		{
			send(clients[i].socket,&num,sizeof(num),0);
		}
	}		
}

void end_connection(int id)
{
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].id==id)	
		{
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin()+i);
			close(clients[i].socket);
			break;
		}
	}				
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN],rev[MAX_LEN],password[MAX_LEN];
	char str[MAX_LEN];
	recv(client_socket,name,sizeof(name),0);
	recv(client_socket,password,sizeof(password),0);
	int temp=Find(name);
	if(temp==-1)
	{
		set_name(id,name,password);
		broadcast_message(0,id);
	}
	else
	{
		bool flag=Check_password(string(password),temp);
		checkflag(temp,id);
		while(flag)
		{
			broadcast_message(1,id);
			recv(client_socket,password,sizeof(password),0);
			flag=Check_password(string(password),temp);
		}
		broadcast_message(0,id);
	}	

	// Display welcome message
	string welcome_message=string(name)+string(" entered the verse.");
	broadcast_message_all("#NULL",id);	
	broadcast_message_all(id,id);								
	broadcast_message_all(welcome_message,id);	
	synchronous_print(color(id)+welcome_message+def_col);
	
	while(1)
	{
        	int bytes=recv(client_socket,rev,sizeof(rev),0);
		int bytes_received=recv(client_socket,str,sizeof(str),0);
		if(bytes<=0)
			return;
		cout<<str<<endl;
		if(strcmp(rev,"#SendAll")==0)
		{		
			broadcast_message_all(string(name),id);			
			broadcast_message_all(id,id);						
			broadcast_message_all(str,id);
			synchronous_print(color(id)+name+" --> "+color(id)+"All");							
			continue;
		}
		if(strcmp(rev,"#exit")==0)
		{
			// Display leaving message
			string message=string(name)+string(" left the verse.");		
			broadcast_message_all("#NULL",id);			
			broadcast_message_all(id,id);						
			broadcast_message_all(message,id);
			synchronous_print(color(id)+message+def_col);
			end_connection(id);							
			return;
		}
		int rev_id=Find(rev+1);
		if(rev_id==-1)
		{
			string message=string(rev+1)+string(" not found");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			synchronous_print(color(id)+message+def_col);
			continue;
		}
		broadcast_message(string(name),rev_id);					
		broadcast_message(id,rev_id);		
		broadcast_message(string(str),rev_id);
		synchronous_print(color(id)+name+" --> "+color(rev_id)+(rev+1));		
	}	
}

int Find(char Rec[])
{
    for(int i=0; i<clients.size(); i++)
	{
		char temp[MAX_LEN];
		strcpy(temp,clients[i].name.c_str());
		if(strcmp(Rec,temp)==0)
		{
			return clients[i].id;
		}
	}
    return -1;
}
bool checkflag(int rev_id,int child_id)
{
	for (int  i = 0; i < clients.size(); i++)
	{
		if(clients[i].id==rev_id){
			clients[i].mark=true;
			clients[i].vid.push_back(child_id);
		}
	}
}
