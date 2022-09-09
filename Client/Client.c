#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 10075
#define MAXLINE 1024

extern char buf[1024];
extern char file_name[1024];
extern int clientSocket;
extern char buffer[1024];

void storeCommand();
void passCommand();
void quitCommand();
void getResult();
void abortCommand();
void storeFile(char *dest, char file[1024]);

char buf[MAXLINE];
char file_name[1024];
int clientSocket;
char buffer[1024];

int main(int argc, char const *argv[])
{

	if(argc != 2) {
		printf("Use Following Command to execute Client.\n\t%s <server-hostname> \n\n", argv[0]);
        exit(EXIT_SUCCESS);
	}


	struct sockaddr_in serverAdd;		// to store server socket address 
	/* Socket (create - connect) */
    	/*-------------------------------------------------------------------------------------*/
    	/* CREATE */
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);		// to create socket
	if(clientSocket < 0){						//Error handler
		printf("Error in connection.\n");			//Error Message
		exit(1);						//Exit on error
	}
	printf("\n");
	memset(&serverAdd, '\0', sizeof(serverAdd));			//initialize server address
	serverAdd.sin_family = AF_INET;				//Ipv4 connection
	serverAdd.sin_port = htons(PORT);				//Port 10075
	serverAdd.sin_addr.s_addr = inet_addr(argv[1]);		//set IP by user

	/* CONNECT */
	//ret = connect(clientSocket, (struct sockaddr*)&serverAdd, sizeof(serverAdd));		// create connection
	// connect initiates three-way handshaking

	if(connect(clientSocket, (struct sockaddr*)&serverAdd, sizeof(serverAdd)) < 0){ //connecting and error handling
		printf("Error in connection.\n");	//Error Message
		exit(1);				//Exit on error
	}

	printf("Client Socket is created.\n");	//Acknowledgment message
	printf("Connected to File Transfer server.\n");	//Acknowledgment message

	printf("\n");

	while(1){

		printf("\n-->");
		fgets (buffer, MAXLINE, stdin);	// take user input
		buffer[strlen(buffer)] = '\0';	//define end character
/*------------------------------------------------------------------------------------------------------------*/

		// Send the command to the server
		send(clientSocket, buffer, MAXLINE, 0);
		strcpy(buf,buffer);
		buf[strlen(buf)] = '\0';
		bzero(buffer, sizeof(buffer));

/*------------------------------------------------------------------------------------------------------------*/		
		/* storing the command*/
		if (strncmp(buf,"STOR",4) == 0 || strncmp(buf,"stor",4) == 0) 
			storeCommand();	//function to store command

/*------------------------------------------------------------------------------------------------------------*/
		/*Error handling for command*/
    		bzero(buffer, sizeof(buffer));
		if(recv(clientSocket, buffer, MAXLINE, 0) < 0){
			printf("Error in receiving Line.\n");
			exit(1);
		}
/*------------------------------------------------------------------------------------------------------------*/
		if ((strncmp(buf,"PASS",4) == 0 || strncmp(buf,"pass",4) == 0 )){
			passCommand();			// 
		}
		else if(strncmp(buf,"QUIT",4) == 0 || strncmp(buf,"quit",4) == 0  ){
			quitCommand();			// will work only when QUIT command is called.
		}
		else if(strncmp(buffer,"Reply[610]",10) == 0){
			getResult();			// will work only when RETR command is called.
		}
		else {
			if (strncmp(buf,"ABOR",4) == 0 || strncmp(buf,"abor",4) == 0)
				abortCommand();		// will only when Abort command is called.
			printf("%s\n",buffer);	
		}
		bzero(buffer, sizeof(buffer));
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------*/
void storeCommand() {
		
	char file[1024];		
	char ch;
	char ans[1024];
	int k = 0;
	
	for(int i=5; i < strlen(buf); i++) {
		ans[k] = buf[i];
		k++;
	}
	strtok(ans,"\n");

	FILE *fp;
	if((fp = fopen(ans, "r")) != NULL) {		//open file in read mode
        		
        k = 0;
        ch = fgetc(fp);				//read character by character
    	while (ch != EOF) {				//read till end of file
        	file[k] = ch;				//store in file array
        	k++;					//increase index
        	ch = fgetc(fp);			//get character
    	}
    	file[k] = '\0';				//end of file char
    	fclose(fp);					//close file descriptor
    			
    	printf("Source-File-Path : %s\n", ans);	
    	printf("File-Size        : %ld bytes\n", strlen(file));

    	strcpy(buffer, file);				//copy file to buffer
    	send(clientSocket, buffer,MAXLINE, 0);	//send file to server
    	bzero(buffer, sizeof(buffer));		//clear buffer
    }
    
    else {
			
		if(errno == ENOENT) {			//no entry error
            printf("Invalid Filename: File does not exist.\n");	//error message
        }
        else
        	printf("Error opening file.\n");	
        send(clientSocket, "NULL", 4, 0);		//send null to server if error in opening
    }
}

/*------------------------------------------------------------------------------------------------------------*/
void passCommand() {

	printf("%s\n",buffer );			//get password from user
	if (strncmp(buffer, "Reply[200]", 10) == 0) {	//need password message
			
		bzero(buffer, sizeof(buffer));	//clear buffer
		recv(clientSocket, buffer, 10, 0);	//receive message from server
		
		char cmd[30];			
    	strcpy(cmd,"\e]2;");			
    	strcat(cmd,"Client - ");
    	strcat(cmd,buffer);				
    	strcat(cmd,"\a");
    	printf("%s",cmd);		

    	bzero(buffer, sizeof(buffer));		//clear buffer
	}
	
}

/*------------------------------------------------------------------------------------------------------------*/
void quitCommand() {

	printf("\e]2;FTP-Client\a");
	printf("%s\n",buffer );
	close(clientSocket);			//close socket on quit command
	exit(1);
	
}

/*------------------------------------------------------------------------------------------------------------*/
void getResult() {

	/*Store File at client-site.*/
	if(strncmp(buffer,"Reply[610]",10) == 0) {		//check if file being sent
				
		char res[1024];				
		bzero(buffer, sizeof(buffer));		//clear buffer
				
		recv(clientSocket, buffer, MAXLINE, 0);	//receive file
			
		strcpy(res, buffer);				//file copy in res from buffer 
		bzero(buffer, sizeof(buffer));		//clear buffer

		recv(clientSocket,buffer, MAXLINE,0);		//receive file name
		strcpy(file_name, buffer);			//copy filename from buffer
			
		getcwd(buf,256);				//get current working directory
    	strcat(buf,"/");
		strcat(buf, file_name);			//add filename to cwd with /
		bzero(buffer, sizeof(buffer));		//clear buffer
		storeFile(buf, res);				//store file to client

		printf("File-Name: %s.\n", file_name);	
		printf("File-Size: %ld bytes.\n", strlen(res));
		printf("Received Successfully.\n");
		printf("FILE OK...Transfer Completed.");
		printf("\n");
	}
}

/*------------------------------------------------------------------------------------------------------------*/
void abortCommand() {
	printf("\e]2;FTP-Client\a");	
}

/*------------------------------------------------------------------------------------------------------------*/
void storeFile(char *dest, char file[1024]) {

	FILE *fp1;
	fp1 = fopen(dest, "w");		//open file in write mode
	fprintf(fp1,"%s",file);		//print file content in destination file
	fclose(fp1);				//close file descriptor
}
