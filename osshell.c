#include <stdio.h>
#include <string.h>
#include <stdlib.h>	  // exit()
#include <unistd.h>	  //  for gettingfork(), getpid(), exec()
#include <sys/wait.h> //  for getting wait()
#include <signal.h>	  //  for signal()
#include <fcntl.h>	  // for close(), open() functions

#define Current_dir_size 10000	// for size of current working directory 
#define MAX_COMMANDS 10		// for setting MAX number of strings in command
#define MAX_COMMAND_LENGTH 100 // for setting Max possible length of any command

char **get_input(char *input)
{
	char **args = malloc(MAX_COMMANDS * sizeof(char *));
	char *parser; // storing the parser string after tokensisation
	int ind = 0; //for setting index

	// Split the string using " " as delimiter below
	// to return the first token from the function
	parser = strtok(input, " ");
	// Keep getting tokens while one of the delimiters present in args[]  above declared data .
	while (parser != NULL)
	{
		args[ind] = parser;
		ind++;
		parser = strtok(NULL, " ");
	}

	args[ind] = NULL;
	return args;
}

/////////////////////////////////////////
// The below fucntion checks if the given tokens exists in command to avoid error
int tokenCheck(char **cmds, char *tokens)
{
	
	char **argz = cmds;

    //moving through the strings in the 'argz' and doing a check if string is equal to token
	while (*argz)
	{
		if (strcmp(*argz, tokens) == 0)
			return 1;
		*argz++;
	}
	return 0;
}
///////////////////////////////////////////////
// The below function parses the input string into multiple cmds or a single cmd with argz depending on the delimiter (spaces,>,##,&&).
int parseInput(char **argz)
{
	// Checks if command entered is 'exit'
	if (strcmp(argz[0], "exit") == 0)
	{
		return 0;
	}

	int parallel_flag = tokenCheck(argz, "&&"); // Checks if argz has "&&"
	if (parallel_flag)								 
	{
		return 1;
	}

	int sequential_flag = tokenCheck(argz, "##"); // Checks if argz has "##"
	if (sequential_flag)							   
	{
		return 2;
	}

	int redirections_flag = tokenCheck(argz, ">"); // Checks if argz has ">"
	if (redirections_flag)							  
	{
		return 3;
	}

	return 4; // other normal single cmds
}
////////////////////////////////
void changeDirectory(char **argz)
{
    // Go to the said directory from the current working directory
	const char *path = argz[1];
	if (chdir(path) == -1)
	{
		printf("Shell: Incorrect command\n"); // for the condition when chdir fails to do its expected work
	}
}

////////////////////////////////
void executeCommand(char **argz)
{
    // for the case when user has given no command 
	if (strlen(argz[0]) == 0)
	{
		return;
	}
	else if (strcmp(argz[0], "cd") == 0) // command given is cd
	{
		changeDirectory(argz);
	}
	else if (strcmp(argz[0], "exit") == 0) // command  given is exit
	{
		exit(0);
	}
    // TO execute a command by forking a new process 
	else
	{
		pid_t p_id = fork(); // child process forked
		if (p_id < 0)		// unsuccessful fork
		{
			printf("Shell: Incorrect command\n");
		}
		else if (p_id == 0) // completed fork
		{
			
			// default behaviour restore for Ctrl-Z and Ctrl-C
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			if (execvp(argz[0], argz) < 0) //  wrong command
			{
				printf("Shell: Incorrect command\n");
			}
			exit(0);
		}
		else // parent
		{
			wait(NULL); // wait till child process completes
			return;
		}
	}
}
/////////////////////////////////////
// The below function will run multiple cmds in parallel
void executeParallelCommands(char **argz)
{
	// keeping a copy of 'argz'
	char **temp = argz;
	// storing all cmommands which are supposed to be seperately executed
	char *all_cmds[MAX_COMMANDS][MAX_COMMAND_LENGTH];
	int i = 0, j = 0, count = 1;
	
	pid_t p_id = 1;
	int status; // For waitpid function
	
	// moving through argz while storing it in 'temp' so that without any issue command can be done
	while (*temp)
	{
		
        // no && tells that it is part of the earlier command
		if (*temp && (strcmp(*temp, "&&") != 0))
		{
			all_cmds[i][j++] = *temp;
		}
        // && tells that the earlier command has finished and thus making changes to keep the coming command
		else
		{
			all_cmds[i++][j] = NULL; // set to null
			j = 0;
			count++;
		}
		//If *temp is not equal to null that tells us that commands are still remaining to be modified
		if (*temp)
			*temp++;
	}
	
	all_cmds[i][j] = NULL;
	all_cmds[i + 1][0] = NULL;
	// moving through all the cmds
    // loop should be executed for parent only ,for that we are using condition of process id !=0
	for (int i = 0; i < count && p_id != 0; ++i)
	{
		p_id = fork(); // p_id is process id 
		////////////////////
		if (p_id == 0) // Fork successful condition
		{
			// handling signal
            // Ctrl-c and ctrl-z siganl default restored
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			//Running command
			execvp(all_cmds[i][0], all_cmds[i]);
		}
        else if (p_id < 0)  // fork failed condition
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	}
	while (count > 0)
	{
		waitpid(-1, &status, WUNTRACED);
		count--;
	}
}

////////////////////////////////////
void executeSequentialCommands(char **argz)
{
	
	// This function will run multiple cmds sequentially
	char **temp = argz;
	while (*temp)
	{
		char **one_cmds = malloc(MAX_COMMANDS * sizeof(char *)); //  The commands to be run are stored here
		one_cmds[0] = NULL;
		int j = 0; // serves as index for one_cmds 
		// Moving through argz and keeping data in 'one_cmds' till we get ##
		while (*temp && (strcmp(*temp, "##") != 0))
		{
			one_cmds[j] = *temp++;
			j++;
		}
		executeCommand(one_cmds);
		//  temp is not null check and going ahead 
		if (*temp)
		{
			*temp++;
		}
	}
}

//////////////////////////////////////
void executeCommandRedirection(char **temp)
{
	char **argz = temp;
    // we need to make sure that there is no problem when we are running execvp()
	argz[1] = NULL;

    // to handle the case where name of the file is empty
	if (strlen(temp[2]) == 0)
	{
		printf("Shell: Incorrect command\n");
	}
	else
	{
		pid_t p_id = fork(); // p_id is process id and here we are forking child
	
		if (p_id == 0) // fork successful - child
		{
			
            // Ctrl-z and ctrl-c default behaviour restore
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			// stdout redirected
			close(STDOUT_FILENO);
			open(temp[2], O_CREAT | O_WRONLY | O_APPEND);
			// command executing
			execvp(argz[0], argz);
			return;
		}
        else if (p_id < 0)		// unsuccesful fork
		{
			printf("Shell: Incorrect command\n");
			return;
		}
		else
		{
			wait(NULL);
			return;
		}
	}
}

/////////////////////////////////////////////////

int main()
{
	signal(SIGINT, SIG_IGN);  //  signal interrupt ignored --Ctrl+C
	signal(SIGTSTP, SIG_IGN); //  signal of suspending execution ignored --Ctrl+Z
	char cwd[Current_dir_size];		  // Var for storing working directory that is being used currently
	char *cmd = NULL;	  //  getline function -> lineptr, and if set to null, allocated a buffer to store line
	int cmd_val = 0;	  //  keeping order of the commands to run
	size_t cmd_size = 0;

	while (1)
	{
		// Ex. cd check && ls
		cmd_val = -1;
		char **argz = NULL; // strings stored seperately by command on terminal
		//  getcwd function is used to get the working directory getting used currently
		getcwd(cwd, Current_dir_size);
		printf("%s$", cwd);

        // taking the input using getline()
		getline(&cmd, &cmd_size, stdin);
		// tokens surrounded  delimiter '\n' extracted
		cmd = strsep(&cmd, "\n");
		
		// when no command is entered by user then this condition will be used
		if (strlen(cmd) == 0)
		{
			continue;
		}

		// tokenss surrounded by " " taken out
		argz = get_input(cmd);
		
        // first command is empty when in sequential execution
		if (strcmp(argz[0], "##") == 0)
		{
			continue;
		}

		
		cmd_val = parseInput(argz);

		
		if (cmd_val == 0)
		{
			printf("Exiting shell...\n");
			break;
		}
		
		if (cmd_val == 1)
		{
			executeParallelCommands(argz); // when user desires to execute many commands in parallel && separated command
		}
		
		else if (cmd_val == 2)
		{
			executeSequentialCommands(argz); // when user wants to run multiple commands sequentially ## separated command
		}
		
		else if (cmd_val == 3)
		{
			executeCommandRedirection(argz); // when user desires to output of a single command to be redirected and output file specificed 
		}
		
		else
		{
			executeCommand(argz); // when a single command is to be run this function is used
		}
	}
	// Bash shell done
	return 0;
}