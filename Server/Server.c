#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/dir.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define USER 1
#define PASS 2
#define MKD  3
#define CWD  4
#define RMD  5
#define PWD  6
#define RETR 7
#define STOR 8
#define LIST 9
#define ABOR 10
#define QUIT 11
#define NOOP 12
#define DELE 13
#define CDUP 14
#define HELP 15
#define INVALID 0
#define MAX 1024

#define PORT 10075

extern char USERNAME[10][100];
extern char PASSWORD[10][100];
extern struct stat st;
extern char arg[1024];
extern int userCommand;
extern bool logIn;
extern int client;
extern bool userDone;
extern char list[1024];
extern char file_name[100];
extern char file[1024];
extern char pwdbuf[256];
extern int count;

void extractFilename(char *pathname);
char *acknowledge(int status);
int authUsername(char *uName);
int authPassword(char *uPassword);
void commandCheck(char *buf);
int makeDir(char *dir);
int changeDir(char *dir);
int showPwd();
int removeDir(const char *path_name);
int listDir();
int getFile(char * filename);
int putFile(char *pathname);
int exitService();
int abortService();
int commandHandler(char *arg);

/*Username and their corresponding passwords*/
char USERNAME[10][100] = {"ADMIN1", "USER1", "USER2", "USER3" , "USER4", "USER5", "USER6", "USER7", "USER8", "USER9"};
char PASSWORD[10][100] = {"123456", "PASS1", "PASS2", "PASS3" , "PASS4", "PASS5", "PASS6", "PASS7", "PASS8", "PASS9"};

int client;
int count = 0;
int main() {
    
    
    printf("\e]2;FTP-Server\a");

    struct sockaddr_in serveAdd;      // to store server socket address
    struct sockaddr_in newAddr;       // to store client socket address
    socklen_t addr_size;             

    int sd;				//socket descriptor for socket connection
    char buf[MAX];			//buffer to store messages and data for the connection
    
    /* Socket (create - bind - listen - accept) */
    /*-------------------------------------------------------------------------------------*/
    /* CREATE */
    sd = socket(AF_INET, SOCK_STREAM, 0);	//AF_INET and SOCK_STREAM indicated IPV4 connection
    if(sd < 0){				//Error handler
        printf("Error in connection.\n");	//Error message
        exit(1);				//Exit on error
    }
    
    /* define port and protocol IPV4 */
    memset(&serveAdd, '\0', sizeof(serveAdd));	//to initialize server address
    serveAdd.sin_family = AF_INET;                    // IPv4 connection
    serveAdd.sin_port = htons(PORT);			//Port is 10075
    serveAdd.sin_addr.s_addr = inet_addr("127.0.0.1");     //to set IP-address '127.0.0.1'
    //printf("Server Socket is created with IP-address %s.\n", inet_ntoa(serveAdd.sin_addr));	//Acknowledgment message

    /* reusing the port for multiple connection */
    int optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));  //socket option function to define socket option to reuse the port multiple times.
    
    
    /*--------------------------------------------------------------------------------------*/
    /* BIND */    
    if(bind(sd, (struct sockaddr*)&serveAdd, sizeof(serveAdd)) < 0){	//Binding and error handling
        printf("Error in binding.\n");				//Error Message
        exit(1);							//Exit on error
    }
    printf("Bind to port %d\n", PORT);				//Acknowledgment message
    
    
    /*--------------------------------------------------------------------------------------*/
    /* LISTEN */
    if(listen(sd, 10) == 0) {				//Listening to accept connection from client
        printf("Listening....\n");			//Acknowledgment message
    }
    else {
        printf("Error in binding.\n");		//Error message
    }

    while(1){						//loop to accept multiple connections and execute requests
        
        /* ACCEPT */
        client = accept(sd, (struct sockaddr*)&newAddr, &addr_size);     // to accept a new connection from clients
        if(client < 0){						//Error handling
            exit(1);							//Exit on error
        }
            
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));  
        printf("But Client is not yet Logged In.\n");

        if (fork() == 0 ) {		//creating child process to handle client request	
            close(sd);                  // to close socket listening

            bzero(buf, sizeof(buf));
            while(1){				//loop to take commands from client

                bzero(buf, sizeof(buf));
                if(recv(client, buf, MAX, 0) < 0) {    	//to receive message from client 
                   char tempBuff[MAX];		//buffer to store message for connection	
                    *tempBuff = '\0';		//initializing the buffer
                    strcat(tempBuff, "Reply[225]: Data Connection Open but No Transfer in progress.");		//Copying message to tem to send client
                    send(client, tempBuff, MAX, 0);	//sending client achnowledgment message
                    continue;
                }

		/*--------------------------------------------------------------------------------------*/
                commandCheck(buf);                    	//to check command case and define actual command      
                int status = commandHandler(arg);		//to get command output status
		/*--------------------------------------------------------------------------------------*/
		/* Pass command pre handling */
                if (userCommand == PASS){				//PASSWORD 	
                    strcpy(buf, acknowledge(status));	//get acknowledgment status to buffer
                    send(client, buf, MAX, 0);		//sending client message
                    bzero(buf, sizeof(buf));			//clear buffer
                    if (status == 333) {			//if command is okay
                        char tempBuff[10];			//temporary buffer to store username
                        strcpy(tempBuff, USERNAME[count]);	//storing username in buffer 
                        send(client, tempBuff, 10, 0);	
                        printf("Client : %s Logged In.\n", USERNAME[count]);    //Acknowledge user login
                    }     
                }
		 /*--------------------------------------------------------------------------------------*/
                /* Retr command pre handling */
                else if (userCommand == RETR && status == 0) {	//RETRIVE
                    char tempBuff[1024];			//temparary buffer to store file and message
                    strcpy(tempBuff, "Reply[610]: File is being sent on to the Data Connection.");                   //Acknowledgment message
                    send(client, tempBuff, MAX, 0);                  	//send client message
                    send(client, file, MAX, 0);				//send File 
                    printf("File Sent Successfully.\n");                  //Acknowledgment message
                    strtok(arg, "\n");					//separating string with \n
                    send(client, arg, MAX, 0);				//sending separated filename
                }
                /*--------------------------------------------------------------------------------------*/
                else {
                    bzero(buf, sizeof(buf));		//clear buffer
                /*--------------------------------------------------------------------------------------*/
                    /* Stor command pre handling */     
                    if (userCommand == STOR && status == 0) {	//STORE
                        strcpy(buf, "Reply[227]: File successfully stored in remote host.");	//Message copy to buffer
                    }
		 /*--------------------------------------------------------------------------------------*/
                    /* List command pre handling */
                    else if (userCommand == LIST && status == 0) {	//LIST                        
                        if (list[strlen(list)-1] == '\n')		//changing \n with \0
                            list[strlen(list)-1] = '\0';
                        strcpy(buf, list);				//copy list to buffer for sending
                    }
		 /*--------------------------------------------------------------------------------------*/                    
                    /* Abor command pre handling */
                    else {
                        strcpy(buf, acknowledge(status));		//get acknowledgment status to buffer
                    }                      
                    send(client, buf, MAX, 0);			
		 /*--------------------------------------------------------------------------------------*/                    
                    /* Quit command pre handling */
                    if (userCommand == QUIT ) {				//QUIT
                        printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));		//Disconnect message
                        break;
                    }
                }
                bzero(buf, sizeof(buf));		//to clear buffer
            }
        }
    }
    /* to let other clients connect we disconnect to current client */
    close(client);		//close Client descriptor for parent process
    return 0;		
}

int userCommand = INVALID;		//initializing command to invalid (if no command is stored in this variable)
char arg[1024];
/*--------------------------------------------------------------------------------------*/		
void commandCheck(char *buf) {	//function to check command and set command to specific format
    
    userCommand = INVALID;		

	while(*buf == ' ' || *buf == '\t') {	//if space is present
        buf++;					//increase buffer to first letter of word
    }
    
    /* to set specific format command */

	if( strncmp(buf,"USER ",5) == 0  || strncmp(buf,"user ",5) == 0  ||
    	strncmp(buf,"USER\t",5) == 0 || strncmp(buf,"user\t",5) == 0 ||
    	strncmp(buf,"USER\n",5) == 0 || strncmp(buf,"user\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = USER;
    }  

    else if(strncmp(buf,"PASS ",5) == 0  || strncmp(buf,"pass ",5) == 0  ||
    		strncmp(buf,"PASS\t",5) == 0 || strncmp(buf,"pass\t",5) == 0 ||
    		strncmp(buf,"PASS\n",5) == 0 || strncmp(buf,"pass\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = PASS;
    }  

    else if(strncmp(buf,"MKD ",4) == 0  || strncmp(buf,"mkd ",4) == 0  ||
    		strncmp(buf,"MKD\t",4) == 0 || strncmp(buf,"mkd\t",4) == 0 ||
    		strncmp(buf,"MKD\n",4) == 0 || strncmp(buf,"mkd\n",4) == 0  )  {
        	
        	buf = buf + 3;
        	userCommand = MKD;
    } 

    else if(strncmp(buf,"CWD ",4) == 0  || strncmp(buf,"cwd ",4) == 0  ||
    		strncmp(buf,"CWD\t",4) == 0 || strncmp(buf,"cwd\t",4) == 0 ||
    		strncmp(buf,"CWD\n",4) == 0 || strncmp(buf,"cwd\n",4) == 0  )  {
        	
        	buf = buf + 3;
        	userCommand = CWD;
    }  

    else if(strncmp(buf,"RMD ",4) == 0  || strncmp(buf,"rmd ",4) == 0  ||
    		strncmp(buf,"RMD\t",4) == 0 || strncmp(buf,"rmd\t",4) == 0 ||
    		strncmp(buf,"RMD\n",4) == 0 || strncmp(buf,"rmd\n",4) == 0  )  {
        	
        	buf = buf + 3;
        	userCommand = RMD;
    } 

    else if(strncmp(buf,"PWD ",4) == 0  || strncmp(buf,"pwd ",4) == 0  ||
    		strncmp(buf,"PWD\t",4) == 0 || strncmp(buf,"pwd\t",4) == 0 ||
    		strncmp(buf,"PWD\n",4) == 0 || strncmp(buf,"pwd\n",4) == 0  )  {
        	
        	buf = buf + 3;
        	userCommand = PWD;
    } 

    else if(strncmp(buf,"RETR ",5) == 0  || strncmp(buf,"retr ",5) == 0  ||
    		strncmp(buf,"RETR\t",5) == 0 || strncmp(buf,"retr\t",5) == 0 ||
    		strncmp(buf,"RETR\n",5) == 0 || strncmp(buf,"retr\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = RETR;
    } 

    else if(strncmp(buf,"STOR ",5) == 0  || strncmp(buf,"stor ",5) == 0  ||
    		strncmp(buf,"STOR\t",5) == 0 || strncmp(buf,"stor\t",5) == 0 ||
    		strncmp(buf,"STOR\n",5) == 0 || strncmp(buf,"stor\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = STOR;
    } 

	else if(strncmp(buf,"LIST ",5) == 0  || strncmp(buf,"list ",5) == 0  ||
    	    strncmp(buf,"LIST\t",5) == 0 || strncmp(buf,"list\t",5) == 0 ||
    	    strncmp(buf,"LIST\n",5) == 0 || strncmp(buf,"list\n",5) == 0  )  {
        	
            buf = buf + 4;
        	userCommand = LIST;
    }   

    else if(strncmp(buf,"ABOR ",5) == 0  || strncmp(buf,"abor ",5) == 0  ||
    		strncmp(buf,"ABOR\t",5) == 0 || strncmp(buf,"abor\t",5) == 0 ||
    		strncmp(buf,"ABOR\n",5) == 0 || strncmp(buf,"abor\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = ABOR;
    } 

    else if(strncmp(buf,"QUIT ",5) == 0  || strncmp(buf,"quit ",5) == 0  ||
    		strncmp(buf,"QUIT\t",5) == 0 || strncmp(buf,"quit\t",5) == 0 ||
    		strncmp(buf,"QUIT\n",5) == 0 || strncmp(buf,"quit\n",5) == 0  )  {
        	
        	buf = buf + 4;
        	userCommand = QUIT;
    }
    else if(strncmp(buf,"DELE",4) == 0)
        userCommand = DELE;

    else if(strncmp(buf,"HELP",4) == 0)
        userCommand = HELP;

    else if(strncmp(buf,"CDUP",4) == 0)
        userCommand = CDUP;

    else if(strncmp(buf,"NOOP",4) == 0)
        userCommand = NOOP;

    while(*buf == ' ' || *buf == '\t') {
        buf++;
    }
    
    strcpy(arg,buf);		//copy buffer to arguement (get arguement of the command)
}
/*--------------------------------------------------------------------------------------*/
bool userDone = false;			//to check if username is already entered or not
bool logIn = false;			//to check if user is already logged in or not

int commandHandler(char *arg) {

/* command handling and sending status */
    int status = 503;
    switch(userCommand) {
        case USER:
            if (logIn) 
                status = 332;                
            else {
                char uName[100];
                strcpy(uName,arg);
                status = authUsername(uName);
            }
            break;

        case PASS:
            if (logIn)
                status = 332;            
            else{ 
                if (userDone)
                    status = authPassword(arg);
                else
                    status = 335;
            }
            break;

        case MKD:
            if (logIn) 
                status = makeDir(arg);            
            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;

        case CWD:
            if (logIn) {
                status = changeDir(arg);
                if (status == 0) {
                    status = showPwd();
                }
            }

           else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;

        case RMD:
            if (logIn) {
                status = removeDir(arg);
                if (status == 0)
                    status = 344;
                else if (status == -1)
                    status = 345;
            }

            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;

        case PWD:
            if (logIn) 
                status = showPwd();
            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;

	case LIST:
		if (logIn)
			status = listDir();
		else {
        	        if (userDone)
        	            status = 600;
        	        else
        	            status = 530;
       	     }
		break;

        case RETR:
            if (logIn) {
                status = getFile(arg);
            }
            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;  

        case STOR:
            if (logIn) {
                if (*arg == '\n' || *arg == '\0')
                    status = 347;
                else
                    status = putFile(arg);
            }
            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;     

        case ABOR:
            if (logIn)
                status = abortService();
            else {
                if (userDone)
                    status = 600;
                else
                    status = 530;
            }
            break;    

        case QUIT:
            if(logIn)
                status = exitService();
            else {
                status = 535;
            }
            break;

        case CDUP:
            status = 502;
            break;

        case DELE:
            status = 502;
            break;

        case HELP:
            status = 502;
            break;

        case NOOP:
            status = 502;
            break;    
    }
    return status;
}

/*--------------------------------------------------------------------------------------*/
char *acknowledge(int status) {

    if(status == -1)
        return "Reply[450]: Invalid Arguments.";

    if (status == 300)
        return "Reply[300]: Username is not correct.";
    
    if (status == 331)
        return "Reply[200]: Command is Okay.\nReply[331]: Username Okay, Need Password.";

    if (status == 332)
        return "Reply[332]: User already logged in.";

    if (status == 333)
        return "Reply[200]: Command is Okay.\nReply[333]: Password Okay, You are Logged In.";

    if (status == 334)
        return "Reply[334]: Incorrect Password.";

    if (status == 335)
        return "Reply[335]: Need Username before Password.";

    if (status == 336)
        return "Reply[200]: Command is Okay.\nReply[336]: Directory is successfully created.";

    if (status == 337)
        return "Reply[337]: Directory already exists.";

    if (status == 338)
        return "Reply[338]: Directory can't be created. Try again.";

    if (status == 503)
        return "Reply[503]: Bad sequence of Commands.";

    if (status == 530)
        return "Reply[530]: No user logged in.";

    if (status == 339)
        return "Reply[339]: EACCES: Permission Denied.";

    if (status == 340)
        return "Reply[340]: ENOTDIR OR ENOENT: Invalid Directory";

    if (status == 341)
        return "Reply[341]: Could not change Directories.";

    if (status == 342)
        return "Reply[342]: Error in getting the current working directory.";

    if (status == 343) {
        
        static char buf[256];
        char mssg[256] = "Reply[200]: Command is Okay.\nReply[343]: Remote Working Directory: ";
        extractFilename(pwdbuf);
        strcat(mssg,pwdbuf);
        strcpy(buf, mssg);
        strcat(buf, "\nCurrent-Working-Directory-Name is : ");
        strcat(buf, file_name);
        return buf;
    }

    if (status == 344)
        return "Reply[200]: Command is Okay.\nReply[344]: Directory successfully removed.";

    if (status == 345)
        return "Reply[345]: Directory can't be removed(Directory may not exists).";

    if (status == 346)
        return "Reply[346]: Error in opening directory.";

    if (status == 347)
        return "Reply[347]: Invalid Filename: File does not exist.";

    if (status == 348)
        return "Reply[348]: Error opening file.";

    if (status == 349)
        return "Reply[349]: Error writing to data socket.";

    if (status == 350)
        return "Reply[350]: Error reading from file.";

    if (status == 226)
        return "Reply[100]: User Logged Out.\nReply[226]: Abort command is successfully processed.";

    if (status == 426)
        return "Reply[551]: Request Action Aborted.\nReply[100]: User Logged Out.\nReply[426]: Service requested aborted abnormally.\nReply[226]: Abort command is successfully processed.";

    if (status == 351)
        return "Reply[351]: Terminate Successfully...Logged Out...Good Bye!!!"; 

    if (status == 352)
        return "Reply[352]: Data Transfer In Progress.\nWaiting...\nWaiting...\nWaiting...\nReply[351]: Terminate Successfully...Logged Out...Good Bye!!!";

    if (status == 502)  
        return "Reply[502]: Command not implemented.";

    if (status == 600)
        return "Reply[600]: Need Password.";

    if (status == 535)
        return "Reply[530]: No user logged in.\nReply[535]: Terminating Session !!!";

    return "Reply[504]: Command not implemented for this parameter.";

}

/*--------------------------------------------------------------------------------------*/
int authUsername(char *uName) {		//Authenticating user (if added already)

    for (int i = 0; i < 10; i++) {		//loop for username array
        if (strncmp(uName,USERNAME[i],6) == 0) {	//comparing string with predefined usernames
            userDone = true;		//username is already entered and varified
            count = i;			//increase count
            return 331;		//return status
        }
    }
    return 300; 	//return status
}

/*--------------------------------------------------------------------------------------*/
int authPassword(char *uPassword) {		//Authenticate password
    if (strncmp(uPassword, PASSWORD[count],6) == 0) {	//compare password with predefined passwords
        logIn = true;	//login status after authentication
        return 333;		//return status
    }
    return 334; 		//return status
}

/*--------------------------------------------------------------------------------------*/
struct stat st = {0};

int makeDir(char *dir) {

    strtok(dir, "\n");    // to remove trailing '\n'
    int status = stat(dir, &st);
    
    if (status == -1) {
        mkdir(dir, 0700);	//make directory with full permission
        return 336;		//status
    }

    if (status == 0) 
        return 337;		//status

    return 338;		//status
}

/*--------------------------------------------------------------------------------------*/
int changeDir(char *dir) {	//Change directory function

    strtok(dir, "\n");		//separate string with \n
    if(chdir(dir) == -1) {	//change directory to dir
        if(errno == EACCES) 		//access error
            return 339;		//error status
        else if(errno == ENOTDIR || errno == ENOENT)	//if not a directory or no entry to directory allowed 
            return 340;	//error status
        
        else 
            return 341;	//error status
    }
    return 0;		
}

/*--------------------------------------------------------------------------------------*/
int removeDir(const char *path_name) {

	char path[256];
	strcpy(path, path_name);
	strtok(path, "\n");
	
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d) {
		struct dirent *p;

		r = 0;

		while (!r && (p=readdir(d)))
		{
			int r2 = -1;
			char *buf;
			size_t len;

			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
				continue;

			len = path_len + strlen(p->d_name) + 2; 
			buf = malloc(len);

			if (buf)
			{
				struct stat statbuf;
				snprintf(buf, len, "%s/%s", path, p->d_name);

				if (!stat(buf, &statbuf))
				{
					/* Check if current item is a directory or not. */
					if (S_ISDIR(statbuf.st_mode))
						r2 = removeDir(buf);
					else
						r2 = unlink(buf);
				}
				else {
					if(errno == EACCES) 
						return 339;
					else if(errno == ENOTDIR || errno == ENOENT) 
						return 340;
					else 
						return 341;
				}
				
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}

	if (!r)
		r = rmdir(path);

   return r;
}

/*--------------------------------------------------------------------------------------*/
char pwdbuf[256];

int showPwd() {

     if(getcwd(pwdbuf, 256) == NULL) 
        return 342;
    strtok(pwdbuf, "\n");    
    return 343;
}

/*--------------------------------------------------------------------------------------*/
char list[1024];
int listDir() {	//List function

        DIR * dir;	
        struct dirent * entry;
        *list = '\0';

        if (strlen(arg) > 1) {
            strtok(arg, "\n");
            if ((dir = opendir(arg)) == NULL)		//open directory
                return 346;	//status
        }
        else {
            if((dir = opendir("./")) == NULL) {	
                return 346;
            }
        }	
	    while((entry = readdir(dir)) != NULL) {
            if((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
                //strcat(list, "-> ");
                strcat(list, entry->d_name);
                strcat(list,"\n");
            }
        }

        closedir(dir);
    
    return 0;
}

/*--------------------------------------------------------------------------------------*/
char file[1024];

int getFile(char * filename) {

     strtok(filename, "\n");
    
    char ch;
	FILE *fp1;
    
    if((fp1 = fopen(filename, "r")) == NULL) {
        if(errno == ENOENT) {
            return 347;
        }
        return 348;
    }
    
    int k = 0;
    ch = fgetc(fp1);
    while (ch != EOF) {
        file[k] = ch;
        k++;
        ch = fgetc(fp1);
    }
    file[k] = '\0';

    return 0;
}

/*--------------------------------------------------------------------------------------*/
int putFile(char *pathname) {

    char buf[1024];
    char res[1024];
    recv(client, buf, MAX, 0);

    if (strncmp(buf,"NULL",4) == 0) {
        printf("Invalid File-Name to Store.\n");
        return -1;
    }

    strcpy(res, buf);
    bzero(buf, sizeof(buf));

    extractFilename(pathname);
    strtok(pathname,"\n");
    char dest[256];
    getcwd(dest,256);
    strcat(dest,"/");
    strtok(file_name,"\n");
    strcat(dest, file_name);
    dest[strlen(dest)] = '\0';

    FILE *fp1;
    fp1 = fopen(dest, "w");
    fprintf(fp1,"%s",res);
    fclose(fp1);
    
    printf("\n-----------------------------------------------------------------------");
    printf("\nFile Received Successfully by client - %s.", USERNAME[count]);
    printf("\nFile Saved in Path %s",dest);
    printf("\nFile Name: %s",file_name);
    printf("\nFile Size: %ld bytes\n", strlen(res));
    printf("-----------------------------------------------------------------------\n");
    
    return 0;
}

/*--------------------------------------------------------------------------------------*/
char file_name[100];
void extractFilename(char *pathname) {

     char splitStrings[10][10]; 
    int i,j,cnt;
    
    j=0; cnt=0;
    for(i=0;i<=(strlen(pathname));i++)
    {
        if( pathname[i]=='/'||pathname[i]=='\0' )
        {
            splitStrings[cnt][j]='\0';
            cnt++;  //for next word
            j=0;    //for next word, init index to 0
        }
        else
        {
            splitStrings[cnt][j]=pathname[i];
            j++;
        }
    }
    strcpy(file_name, splitStrings[cnt-1]);
    file_name[strlen(file_name)] = '\0';
}

/*--------------------------------------------------------------------------------------*/
int abortService() {

    srand(time(NULL)); 
    int no = (rand()%2);
    if (logIn)
        printf("Client %s is abnormally logging out.\n", USERNAME[count]);

    logIn = false;
    userDone = false;
    if (no == 0) 
        return 226;

    return 426;
}

/*--------------------------------------------------------------------------------------*/
int exitService() {

    srand(time(NULL)); 
    int no = (rand()%2);
    if (logIn)
        printf("Client %s is successfully logged out.\n", USERNAME[count]);

    logIn = false;
    userDone = false;
    if (no == 0) 
        return 351;
    return 352;
    
}


