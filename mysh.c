#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct proc {
  int proc_id;
  int jid;
  char *proc_name;
  int is_bg;
  struct proc *next;
}
proc;

proc *head = NULL;

int is_built_in_command(char *command) {
  return (strcmp(command, "j") == 0 || strstr(command, "myw") != NULL);
}

char *form_error_message(char msg1[], const char* msg2) {
  int msg1_len = strlen(msg1);
  int msg2_len = strlen(msg2);
  char *message = NULL;

  // Allocating memory to message
  message = (char *) malloc((msg1_len + msg2_len) * sizeof(char));

  strcpy(message, msg1);
  strcat(message, msg2);
  strcat(message, "\n");

  return message;
}

void add_process_to_list(int pid, int job_id, char **pname,
       int is_background_proc) {
  int i;
  proc *proc_node = (proc *) malloc(sizeof(proc));
  proc_node -> proc_id = pid;
  proc_node -> jid = job_id;
  proc_node -> proc_name = strdup(pname[0]);
  for (i = 1; pname[i] != NULL; i++) {
    strcat(proc_node -> proc_name, " ");
    strcat(proc_node -> proc_name, pname[i]);
  }
  proc_node -> is_bg = is_background_proc;
  proc_node -> next = NULL;

  if (head == NULL) {
    head = proc_node;
  } else {
    proc *temp = head;
    while (temp -> next != NULL) {
      temp = temp -> next;
    }
    temp -> next = proc_node;
  }
}

void write_to_stdout(char *message) {
  write(STDOUT_FILENO, message, strlen(message));
}

void write_to_stderr(char *message) {
  write(STDERR_FILENO, message, strlen(message));
}

int main(int argc, char *argv[]) {
  char line[512];
  char *background_indicator;
  		const char ampersand = '&';

  pid_t child_id, end_id;
  struct timespec start_time, end_time, wait_time;
  int i, j = 0, num_cmd_args, j_with_args, status;
  int is_bg_process = 0;  // To identify a background process

  FILE *fp;

  if (argc == 1) {
    fp = stdin;
  } else if (argc == 2) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      char msg1[] = "Error: Cannot open file ";
      const char *msg2 = (const char *) argv[1];
      char *message = form_error_message(msg1, msg2);
      write_to_stderr(message);
      free(message);
      exit(EXIT_FAILURE);
    }
  } else {
	  write_to_stderr("Usage: mysh [batchFile]\n");
	  exit(EXIT_FAILURE);
  }

  // Exit if Ctrl-D (EOF) is pressed
  if (feof(stdin)) {
    exit(EXIT_FAILURE);
  }

  if (fp == stdin) {
    write_to_stdout("mysh> ");
  }

  // Interactive mode
  // Reading the command from user
  while (fgets(line, sizeof(line), fp) != NULL) {

	  if (strcmp(line, "\n") == 0) {
	    if(fp == stdin) {
          write_to_stdout("mysh> ");
	    } else {
	    	write_to_stdout("\n");
	    }
        continue;
    }

	  if(fp != stdin) {
	    write_to_stdout(line);
	  }

    // Searching for the "&" character at the end and removing it in case
        // of a background process
        if ((background_indicator = strchr(line, ampersand)) != NULL)
    	{
    		is_bg_process = 1;
    		int src = 0, dest = 0;
    		while (line[src] != '\0' )
    		{
    			if (isalnum(line[src]) || line[src] == '-' || line[src] == ' ')
    			{
    				line[dest] = line[src];
    				dest++;
    			}
    			src++;
    		}
    		line[dest] = '\0';
    	}
//        else {
//          for (i = ampersand_index + 1; i < strlen(line); i++) {
//            line[i] = '\0';
//          }
//        }

    // Splitting the command based on space and \n
    char *cmd[512];
    char **next = cmd;
    char *temp = strtok(line, " \n");
    num_cmd_args = -1;
    while (temp != NULL) {
      *next++ = temp;
      num_cmd_args++;  // Keeping track of number of arguments
      temp = strtok(NULL, " \n");
    }
    *next = NULL;

//    if (fp != stdin) {
//      for (i = 0; cmd[i] != NULL; i++) {
//        write_to_stdout(cmd[i]);
//        if (cmd [i + 1] != NULL) {
//          write_to_stdout(" ");
//        }
//      }
//      write_to_stdout("\n");
//    }

    //printf("cmd[%d]: %s", num_cmd_args, cmd[num_cmd_args]);
//    if (strchr(cmd[num_cmd_args], '&') != NULL) {
//      if (strcmp(cmd[0], "exit") != 0) {
//        is_bg_process = 1;
//        cmd[num_cmd_args][strlen(cmd[num_cmd_args]) - 1] = '\0';
//
//        cmd[num_cmd_args][strlen(cmd[num_cmd_args])] = '\0';
//      }
//
//      printf("strlen: %ld", strlen(cmd[num_cmd_args]));
////      printf("cmd[1]:%s", cmd[2]);
//      //printf("cmd[%d]: %s", num_cmd_args, cmd[num_cmd_args]);
//      for (i = 0; cmd[i] != NULL; i++) {
//         printf("%s ",cmd[i]);
//      }
//      //cmd[num_cmd_args] = NULL;
//    }

//    int length = strlen(cmd[num_cmd_args]);
//    cmd[num_cmd_args][strlen(cmd[num_cmd_args]) - 1] = '\0';
//    printf("hello -> %d : %c", length, cmd[num_cmd_args][strlen(cmd[num_cmd_args]) - 1]);
//    int k;
//        for(k = 0; cmd[k] != NULL; k++)
//        {
//        	printf("%d: %s", k, cmd[k]);
//        }

    if (!is_bg_process && !is_built_in_command(cmd[0])) {
      j++;  // Incrementing jid for non-background process
    }

    // Stop execution if "exit" is encountered
    if (strcmp(cmd[0], "exit") == 0) {
      exit(EXIT_SUCCESS);
    }

    // Print all background processes if built-in shell
    // command 'j' is executed
    if (strcmp(cmd[0], "j") == 0  && cmd[1] == NULL) {
      proc *temp = head;
      while (temp != NULL) {
	  end_id = waitpid(temp -> proc_id, & status, WNOHANG);
	  if (end_id == 0) {
		printf("%d : %s\n", temp -> jid, temp -> proc_name);
		fflush(NULL);
	  }
	  temp = temp -> next;
	}
	if (fp == stdin) {
      write_to_stdout("mysh> ");
	}
	continue;
  } else if (strcmp(cmd[0], "j") == 0 && cmd[1] != NULL) {
	j_with_args = 1;
   }

	 // Print the status of a process if built-in shell
	 // command 'myw <jid>' is executed
	 if (strcmp(cmd[0], "myw") == 0) {
	   proc *temp = head;
	   if (cmd[1] != NULL) {
		 while (temp != NULL && temp -> jid != atoi(cmd[1])) {
		   temp = temp -> next;
		 }
		 if (temp == NULL) {
		   char msg1[] = "Invalid jid ";
		   const char *msg2 = (const char *) cmd[1];
		   char *message = form_error_message(msg1, msg2);
		   write_to_stderr(message);

		   wait_time.tv_sec = 0;
		   wait_time.tv_nsec = 100000000;
		   nanosleep(&wait_time, NULL);

		   free(message);
		   if (fp == stdin) {
			 write_to_stdout("mysh> ");
		   }
		   continue;
		 } else {
		   clock_gettime(CLOCK_MONOTONIC_RAW, & start_time);
		   end_id = waitpid(temp -> proc_id, & status, WUNTRACED);
		   clock_gettime(CLOCK_MONOTONIC_RAW, & end_time);
		   uint64_t time_elapsed = (end_time.tv_sec - start_time.tv_sec)
		          * 1000000 + (end_time.tv_nsec - start_time.tv_nsec) / 1000;

		   if (end_id == -1) {
			 perror("Waitpid error");
		   } else if (end_id == child_id) {
			 printf("%ld : Job %d terminated\n", time_elapsed, temp -> jid);
			 fflush(NULL);
		   }
		 }
	} else {
		 char *msg1 = NULL;
		 msg1 = (char *) malloc(strlen(cmd[0]));
		 const char *msg2 = ": Command not found";
		 char *message = form_error_message(msg1, msg2);
		 write_to_stderr(message);
		 free(msg1);
		 free(message);
		 if (fp == stdin) {
		   write_to_stdout("mysh> ");
		 }
		 continue;
	 }
  }
    if ((!is_built_in_command(cmd[0]) || j_with_args) && (child_id = fork()) == -1) {
      perror("Fork error");
      exit(EXIT_FAILURE);
    } else if (child_id == 0) {
      // Child process
      if (!is_built_in_command(cmd[0]) || j_with_args) {
        if (execvp(cmd[0], cmd) == -1) {
          char *message = cmd[0];
          const char *error = ": Command not found\n";
          strcat(message, error);
          write_to_stderr(message);
          exit(EXIT_FAILURE);
        }
      }
    } else if (child_id > 0 || is_built_in_command(cmd[0])) {
      // Parent process

      // Resetting j_with_args
      if (j_with_args) {
    	  j_with_args = 0;
      }
      if (is_bg_process && !is_built_in_command(cmd[0]) && strcmp(cmd[0], "exit") != 0) {
        // Adding background processes to process list
        // Not creating nodes for built-in commands - j, myw and exit
        add_process_to_list(child_id, ++j, cmd, is_bg_process);
        is_bg_process = 0;
        if (fp == stdin) {
          wait_time.tv_sec = 0;
          wait_time.tv_nsec = 100000000;

          // Waiting for the command to display the output
          // before displaying the prompt (in case of ls and pwd)
          // This is to avoid displaying the prompt before the
          // output of the command gets displayed
          nanosleep(& wait_time, NULL);
          write_to_stdout("mysh> ");
          continue;
        }
      }

      // Process is not a background process. Wait for it
      // to finish and then return the prompt
      if (!is_bg_process && !is_built_in_command(cmd[0])) {
        end_id = waitpid(child_id, & status, 0);
        if (end_id == -1) {
          perror("Waitpid error");
          exit(EXIT_FAILURE);
        }
      }
    } // End of else if
    if (fp == stdin) {
	 wait_time.tv_sec = 0;
	 wait_time.tv_nsec = 100000000;
	 nanosleep(&wait_time, NULL);
     write_to_stdout("mysh> ");
    }
  } // End of while
  free(head);
  return 0;
}
