
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/time.h> 

int PATH_MAX = 512;
int HISTORY_MAX = 10;
char arrowPrompt[3] = "->";
char dirPrompt[512];

#define BUFLEN 1024 
#define PORT 6666
#define LISTNUM 20

/*linuxshell_function*/
void mergeFile(char *commands[]){               //merges two files into a third new file （cat文件合并模块）
  FILE *fp1, *fp2, *fp3;
  char c;
  fp1 = fopen(commands[1], "r");
  fp2 = fopen(commands[2], "r");
  fp3 = fopen(commands[4], "w");
  if(fp1 == NULL || fp2 == NULL || fp3 == NULL){       //error opening one, print error and return
    printf("\nError opening files");
     fclose(fp1);
     fclose(fp2);
     fclose(fp3);
    return;
  }
  else{
    c = fgetc(fp1);                                   //cat the first file to the new file
    while(c != EOF){
      fputc(c, fp3);
      c = fgetc(fp1);
    }
    c = fgetc(fp2);                                  //cat the second file to the new file
    while(c != EOF){
      fputc(c, fp3);
      c = fgetc(fp2);
    }
    printf("\n");
  }                                                //close the files
  fclose(fp1);
  fclose(fp2);
  fclose(fp3);
  return;
}

void commandExecute(char *commands[]){          //executes the users command with execvp （用execvp（）执行用户命令） **核心
  int pid;
  int status;
  printf("\n");
  pid = fork();
  if(pid > 0){                         //父进程
    waitpid(pid, &status, WUNTRACED);  //暂时停止目前进程的执行，直到有信号来到或子进程结束
   } 
  else {                                //execute the command, if an error display a message
    int cmdError = execvp(commands[0], commands);  //从PATH 环境变量所指的目录中查找符合参数1的文件名，找到后便执行该文件，然后将参数2传给该欲执行的文件。
    if(cmdError == -1){
      printf("Command entered does not exist.\n");
      exit(1);
    }
  }
}

void tokenize(int *comCnt, int *pipeCheck, char *commands[], char inCommand[]){      //（拆分命令模块）
  char *token;                                 
  *comCnt = 0;
  *pipeCheck = 0;
  token = '\0';
  token = strtok(inCommand, " ");                //以空格切割字符串 可思考使用正则表达式
  while(token) {                                //tokenize the command into a pointer array 将命令标记为指针数组 
    if(strcmp(token, "|") == 0){                //if the command contains the pipe split the commands
      *pipeCheck = 1;
    }
    commands[*comCnt] = token;                  //comCnt
    token = strtok(NULL," ");                   //再次调用需要将字符串设为null            
    *comCnt = *comCnt + 1;
  }
}

void pipeProcess(char *commands[], int *pipeCheck, int comCnt){    //（管道进程通信模块） **核心
  char *pipeCom1[10];
  char *pipeCom2[10];
  int pid, pidb, status, index;
  int pipefd[2];                                          //pipefd管道数组

  printf("\n");
  for(int i=0; i<comCnt; i++){                             //allows a pipe command with operate '|' to be used
    if(strcmp(commands[i], "|") == 0){                     //split the command into left and right of '|'
      index = i;
      *pipeCheck = 0;
    }
    else{
      if(*pipeCheck == 1){                              
	  pipeCom1[i] = commands[i];
      }
      else if(*pipeCheck == 0){
	   pipeCom2[i-index-1] = commands[i];
      }
    }
  }
  pipeCom1[index] = NULL;                                  //null terminate last element of command array  终止
  pipeCom2[comCnt-index] = NULL;

  pipe(pipefd);                                   //管道建立
  pid = fork();

  if(pid == 0){
    close(0);                                 //child process, receives data from pipe for execution
    dup(pipefd[0]);                           //复制管道描述符 读                                     
    close(pipefd[1]);
    execvp(pipeCom2[0], pipeCom2);
  }
  else{
    pidb = fork();
    if(pidb == 0){                      //other child process writes data to pipe for execution                                               
      close(1);
      dup(pipefd[1]);
      close(pipefd[0]);
      execvp(pipeCom1[0], pipeCom1);
    }
    else
      waitpid(0, &status, WUNTRACED);    //parent process has to wait on children to finish before continuing execution
  }
  return;
}

void backspace(char inCommand[]){              //deletes the last character when pressed（退格模块）
  if(strlen(inCommand) != 0)
    {
      inCommand[strlen(inCommand) - 1] = '\0';
      printf("\b");
      printf(" ");
      printf("\b");
    }
  return;
}

int exitProgram(){                             //functionality for exiting the program （退出模块）
  char exitCommand[PATH_MAX];
  int rePrompt = 1;
  int inChar = '\0';
  char cChar[2];

  memset(&exitCommand[0], 0, sizeof(exitCommand));

  while(rePrompt == 1){
    printf("\n%s%sDo you want to exit? y or n:", dirPrompt, arrowPrompt); //prompt user to exit, if yes exit
    while(inChar != '\n'){                                                //if no continue accepting commands                                      
      inChar = getchar();                                                 //if neither, reprompt them
      if(inChar == 0x7f)
	{
	  backspace(exitCommand);
	}
      else{
	printf("%c", inChar);
	sprintf(cChar, "%c", inChar);
	strcat(exitCommand, cChar);
      }
    }
    if(strcmp(exitCommand, "y\n") == 0){
      printf("goodbye");
      return 1;
    }
    else if(strcmp(exitCommand, "n\n") == 0){
      memset(&exitCommand[0], 0, sizeof(exitCommand));
      printf("%s%s", dirPrompt, arrowPrompt);
      inChar = '\0';
      break;
    }
    else{
      printf("Please enter a valid command to exit.");
      memset(&exitCommand[0], 0, sizeof(exitCommand));
      inChar = '\0';
    }
  }
  return 0;
}


 
/*linuxshell*/
void linuxshell(){
  int inChar = '\0';
  int arrowKey = 0;
  char inCommand[256] = "\0";  //命令储存数组
  char cChar[2];
  char temp[64];
  char *commandArr[128];       //存放命令的指针的数组
  char *cmdHistory[HISTORY_MAX];//历史命令储存
  int comCnt;
  int exitState = 0;   //exit 状态
  int cmdError = 0;
  int history_count = 0;
  int pipeCheck = 0;  //管道检查值
  int upDownCount = 0;
  int charCount = 0;
  int last_press = 2;


  getcwd(dirPrompt, sizeof dirPrompt);                   //将当前工作目录的绝对路径复制到参数dirPrompt所指的内存空间中,参数size为dirPrompt的空间大小(512)
 
  //设置为终端输入模式  （思考设置成终端io键盘输入和socket通信受控双模式）
  struct termios origConfig, newConfig;                  //termios函数获取与终端相关的参数（打开的终端文件描述符）在termios结构体
  tcgetattr(0, &origConfig);                             //save IO settings, set no echo, special chars, delete allowed
  newConfig = origConfig;
  newConfig.c_lflag &= ~(ICANON| ECHOE | ECHO);          //输入模式标志，控制终端输入方式
  tcsetattr(0, TCSANOW, &newConfig);                     //TCSANOW：不等数据传输完毕就立即改变属性
  
  
  printf("%s%s", dirPrompt, arrowPrompt);                //显示当前绝对路径
  
  memset(&inCommand[0], 0, sizeof(inCommand));           //所指向的某一块内存中的后n个字节的内容全部设置为ch指定的ASCII值  
  for(int i=0; i<128; i++){   //命令数组全部置零
    commandArr[i] = NULL;         
  }

  while(exitState == 0){
    inChar = getchar();
    charCount++;
    while(!((strcmp(inCommand, "exit") == 0) && (inChar == '\n')))   //continue to accept commands while user hasn't exited
      {
	    if(inChar == '\n')          //command entered（回车）
	    {
	    if(inChar == '\n' && charCount == 1){
	      charCount = 0;
	    }
	    else{                                   //command needs to be added to the history array
	      charCount = 0;
	      upDownCount = 0;
	      if(history_count == 0){              //历史命令积累
		    history_count++;
		    cmdHistory[0] = strdup(inCommand);     //strdup将串拷贝到新建的位置处 返回一个指针,指向为复制字符串分配的空间
	      }
	      else{
		     for(int i=history_count; i>0; i--){     //copy from the last element to the first so data isn't destroyed
		     if(i < 10){
		     cmdHistory[i] = strdup(cmdHistory[i-1]);  //腾出cmdhistory[0]的空间
		       }
		     }
		     strcpy(cmdHistory[0], inCommand);
		     if(history_count <11)
		      history_count++;
	       }
	      //拆分命令 **
	      tokenize(&comCnt, &pipeCheck, commandArr, inCommand); 
	      commandArr[comCnt+1] = NULL;              //set the element after the last to null to indicate command ends设置最后一个到null的元素以指示命令结束 
	      //——————————————————————cd命令 ————————————————————————————————
	       if(strcmp(commandArr[0], "cd") == 0){           //if its a cd command execute chdir, print errors if chdir fails改变当前工作目录
		    if(chdir(commandArr[1]) != 0){
		     fprintf(stderr, "\ncd to /%s failed. ", commandArr[1]);
		     perror("");
		     }
		     else{
		     getcwd(dirPrompt, sizeof dirPrompt);     //复制当前工作的绝对路径
		    }
	      }
	       //——————————————————————cat命令 ————————————————————————————————
	     else if(strcmp(commandArr[0], "merge") == 0){        //user entered a merge command call function  文件合并
		   strcpy(commandArr[0], "\0");
		   strcpy(commandArr[0], "cat");
		   mergeFile(commandArr);
	      }
	      
	       else{
		   if(pipeCheck == 1){                            //pipe command enetered
		   pipeProcess(commandArr, &pipeCheck, comCnt);
		   printf("\x1B[A");
		    }
		   else{
		  commandExecute(commandArr);                  //use execvp to execute the command
		  printf("\x1B[A");
		   }
	      }
	   }
	    
	    for(int i=0; i<20; i++)
	      commandArr[i] = NULL;
	    printf("\n%s%s", dirPrompt, arrowPrompt);
	    memset(&inCommand[0], 0, sizeof(inCommand));    //reset incoming command to empty 命令清零
	}//完整判断输入未完结开始单个字母的判断
	else if(strlen(inCommand) == 255)                   //don't allow overflow of input（命令最多256）
	  inChar = '\b';

	else if(inChar == 27)                               //special key pressed, ignore l/r, allow up/down（历史命令浏览）
	  {                 
	    charCount = 0;                       //TO-DO ** make this a function
	    arrowKey = getchar();
	    arrowKey = getchar();

	    if(history_count > 0){                //user entered up key, check to see they have history, if they do display it
	     if(arrowKey == 65){        
		    if(upDownCount >= 0 && upDownCount <= (history_count - 1)){
		    for(int i=0; i<sizeof(inCommand); i++)
		    backspace(inCommand);
		    upDownCount++;
		    strcpy(inCommand,cmdHistory[upDownCount-1]);            //once they reach the history end, nothing will happen
		    printf("%s", inCommand);
		    last_press = 1;
		  }
	     }

	     else if(arrowKey == 66){                           //user entered the down key, check to see if they have history
		 for(int i=0; i<sizeof(inCommand); i++)             //if they do display it.
		 backspace(inCommand);                              //once they reach the bottom of the history nothing happens
		
		 if(upDownCount > 0 && upDownCount <= (history_count)){	
		   if(upDownCount == 1){
		    for(int j=0; j<sizeof(inCommand); j++)
		      backspace(inCommand);
		    upDownCount--;
		  }
		  else{
		    if(upDownCount > 1 && last_press == 1)
		      upDownCount = upDownCount - 1;
		    else if(upDownCount > 1 && last_press == 0)
		      upDownCount--;
		    strcpy(inCommand, cmdHistory[upDownCount-1]);
		    printf("%s", inCommand);
		  }
		  last_press = 0;
		 }
	   }
	  }
	}

	else  if(inChar == 0x7f)        //delete last character if delete/backspace pressed
	  {               
	    backspace(inCommand);
	    charCount--;
	  }

	else{
	  printf("%c", inChar);         //nothing else to be done, add the character to the command array
	  sprintf(cChar, "%c", inChar);
	  strcat(inCommand, cChar);
	   }
	  inChar = getchar();      //判断全部结束再次输入进行while判断
	  charCount++;
 }//while完整程序-对当前输入的判断
     //历史命令积累
    if(history_count == 0){                       //
      history_count++;                            //for now adds exit command to history
      cmdHistory[0] = strdup(inCommand);
    }
    else{
      for(int i=history_count; i>0; i--){     //copy from the last element to the first so data isn't destroyed                                   
	if(i < 10){
	  cmdHistory[i] = strdup(cmdHistory[i-1]);
	}
      }
      strcpy(cmdHistory[0], inCommand);
      if(history_count <11)
	history_count++;
    }
    
    upDownCount = 0;
    charCount = 0;
    memset(&inCommand[0], 0, sizeof(inCommand));
    exitState = exitProgram();    //call function to see if user wants to exit
}  //while完整程序-整体退出

  tcsetattr(0, TCSANOW, &origConfig);                        //reset the configuration of IO back to normal
  printf("\n");
  //return 0;
}


/*communication*/
  
int tongxing()
{ 
    int sockfd, newfd; 
    struct sockaddr_in s_addr, c_addr; 
    char buf[BUFLEN]; 
    socklen_t len; 
    unsigned int port, listnum; 
    fd_set rfds; 
    struct timeval tv; 
    int retval,maxfd; 
      
    /*建立socket*/ 
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){ 
        perror("socket error"); 
        exit(errno); 
    }else 
        printf("socket create success!\n"); 
    memset(&s_addr,0,sizeof(s_addr)); 
    s_addr.sin_family = AF_INET; 
    s_addr.sin_port = htons(PORT); 
    s_addr.sin_addr.s_addr = htons(INADDR_ANY); 
    
    /*把地址和端口帮定到套接字上*/ 
    if((bind(sockfd, (struct sockaddr*) &s_addr,sizeof(struct sockaddr))) == -1){ 
        perror("bind"); 
        exit(errno); 
    }else 
        printf("bind success!\n"); 
    /*侦听本地端口*/ 
    if(listen(sockfd,listnum) == -1){ 
        perror("listen"); 
        exit(errno); 
    }else 
        printf("the server is listening!\n"); 
    while(1){ 
        printf("*****************聊天开始***************\n"); 
        len = sizeof(struct sockaddr); 
        if((newfd = accept(sockfd,(struct sockaddr*) &c_addr, &len)) == -1){ 
            perror("accept"); 
            exit(errno); 
        }else 
            printf("正在与您聊天的客户端是：%s: %d\n",inet_ntoa(c_addr.sin_addr),ntohs(c_addr.sin_port)); 
        while(1){ 
            FD_ZERO(&rfds); 
            FD_SET(0, &rfds); 
            maxfd = 0; 
            FD_SET(newfd, &rfds); 
            /*找出文件描述符集合中最大的文件描述符*/ 
            if(maxfd < newfd) 
                maxfd = newfd; 
            /*设置超时时间*/ 
            tv.tv_sec = 10; 
            tv.tv_usec = 0; 
            /*等待聊天*/ 
            retval = select(maxfd+1, &rfds, NULL, NULL, &tv); //阻塞
            if(retval == -1){ 
                printf("select出错，与该客户端连接的程序将退出\n"); 
                break; 
            }else if(retval == 0){ 
                printf("waiting...\n"); 
                continue; 
            }else{ 
                /*用户输入信息*/ 
                if(FD_ISSET(0, &rfds)){ 
            
                    /******发送消息*******/ 
                    memset(buf,0,sizeof(buf)); 
                    /*fgets函数：从流中读取BUFLEN-1个字符*/ 
                    fgets(buf,BUFLEN,stdin);  //stdin键盘输入到缓冲区里的
                    /*打印发送的消息*/ 
                    //fputs(buf,stdout); 
                    if(!strncasecmp(buf,"quit",4)){  //比较参数s1和s2字符串前n个字符，比较时会自动忽略大小写的差异。
                        printf("server 请求终止聊天!\n"); 
                        break; 
                    } 
                        len = send(newfd,buf,strlen(buf),0); 
                    if(len > 0) 
                        printf("\t消息发送成功：%s\n",buf); 
                    else{ 
                        printf("消息发送失败!\n"); 
                        break; 
                    } 
                } 
                /*客户端发来了消息*/ 
                if(FD_ISSET(newfd, &rfds)){ 
                    /******接收消息*******/ 
                    memset(buf,0,sizeof(buf)); 
                    /*fgets函数：从流中读取BUFLEN-1个字符*/ 
                    len = recv(newfd,buf,BUFLEN,0); 
                    if(len > 0) 
                        printf("客户端发来的信息是：%s\n",buf); 
                    else{ 
                        if(len < 0 ) 
                            printf("接受消息失败！\n"); 
                        else 
                            printf("客户端退出了，聊天终止！\n"); 
                        break; 
                    } 
                } 
            } 
        } 
        /*关闭聊天的套接字*/ 
        close(newfd); 
        /*是否退出服务器*/ 
        printf("服务器是否退出程序：y->是；n->否? "); 
        bzero(buf, BUFLEN); 
        fgets(buf,BUFLEN, stdin); 
        if(!strncasecmp(buf,"y",1)){ 
            printf("server 退出!\n"); 
            break; 
        } 
    } 
    /*关闭服务器的套接字*/ 
    close(sockfd); 
    return 0; 
}


/*menu*/
 int menu()
{
   int xuanze;
   printf("-----------menu-----------\n");
   printf("     0.exit             \n");
   printf("     1.linuxshell        \n");
   printf("     2.communication\n");
   printf("--------------------------\n");
   printf("     select<0,1,2>: ");
   scanf("%d",&xuanze);
   return xuanze;
}

	
int main()
{
	int code;
        while(1){
		code=menu();//调用菜单
		switch(code) 
		{
			case 0: exit(0);
			case 1:  
			    linuxshell();
			    scanf("%*2c");break;
			case 2:
			    tongxing();
			    scanf("%*2c");break;
		 } 	
       }	
	return 0;
 }
