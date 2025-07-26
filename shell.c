/* FELIPE OEHLER GUZMÁN */

/**
 * Linux Job Control Shell Project
 *
 * Operating Systems
 * Grados Ing. Informatica & Software
 * Dept. de Arquitectura de Computadores - UMA
 *
 * Some code adapted from "OS Concepts Essentials", Silberschatz et al.
 *
 * To compile and run the program:
 *   $ gcc shell.c job_control.c -o shell
 *   $ ./shell
 *	(then type ^D to exit program)
 **/

#include "job_control.h"   /* Remember to compile with module job_control.c */

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough */

job * my_job_list; /* List of jobs in the background or suspended */

/**
 * Signal handler for SIGCHLD (child process state changes)
 * This function is called when a child process changes state (stopped, continued, or terminated).
 * It iterates through the job list, checks for state changes, and updates or removes jobs accordingly.
 **/
void sigchld_handler(int num_sig) {
	int status, info;
	pid_t pid_wait;
	enum status status_res;
	job_iterator iter = get_iterator(my_job_list); /* Get iterator to go through job list */

	while(has_next(iter)) {   
		job *the_job = next(iter); /* Get next job in the list */

		pid_wait = waitpid(the_job->pgid, &status, WUNTRACED | WCONTINUED | WNOHANG);

		if (pid_wait == the_job->pgid) { /* If the job's state has changed */
			status_res = analyze_status(status, &info); /* Analyze the status of the job */
			printf("Background pid: %d, command: %s, %s, info: %d\n", the_job->pgid, the_job->command, status_strings[status_res], info);
			
			/* Update job state based on its status */
			if(status_res == SUSPENDED) { 			/* The background job was suspended */
				the_job->state = STOPPED; 

			} else if (status_res == CONTINUED) { 	/* The background job was continued */
				the_job->state = BACKGROUND; 

			} else {								/* Job was finished or signaled */
				delete_job(my_job_list, the_job); 
			}

		} else if (pid_wait == -1) {
			perror("Wait error from sigchld_handler");
		}
	} 
}

/**
 * Signal handler for SIGHUP (hangup signal)
 * This function is called when the shell receives a SIGHUP signal.
 * It appends a message to the file "hup.txt" indicating that SIGHUP was received.
 **/
void sighup_handler(int num_sig) {
	FILE *fp;
	fp = fopen("hup.txt", "a"); /* Open file in append mode */
	if (fp) {
		fprintf(fp, "SIGHUP received.\n"); /* Write to the file */
		fclose(fp);
	}
}

/**
 * Parses the command arguments for output append redirection (>>).
 * - Searches for the ">>" token in the args array.
 * - If found, sets *file_out to the filename following ">>".
 * - Removes ">>" and the filename from the args array so execvp won't see them.
 * - If syntax is incorrect (no filename after ">>"), prints an error and clears the command.
 */
void parse_append_redirection(char **args, char **file_out) {
    *file_out = NULL;
    char **args_start = args;
    while (*args) {
        int is_out = !strcmp(*args, ">>");
        if (is_out) {
            args++;
            if (*args){
                if (is_out) *file_out = *args;
                char **aux = args + 1;
                while (*aux) {
                   *(aux-2) = *aux;
                   aux++;
                }
                *(aux-2) = NULL;
                args--;
            } else {
                /* Syntax error */
                fprintf(stderr, "syntax error in redirection\n");
                args_start[0] = NULL; // Do nothing
            }
        } else {
            args++;
        }
    }
}
 
/**
 * MAIN
 **/
int main(void)
 {
	char inputBuffer[MAX_LINE]; /* Buffer to hold the command entered */
	int background;             /* Equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* Command line (of 256) has max of 128 arguments */

	/* Probably useful variables: */
	int pid_fork, pid_wait; 	/* PIDs for created and waited processes */
	int status;             	/* Status returned by wait */
	enum status status_res; 	/* Status processed by analyze_status() */
	int info;					/* Info processed by analyze_status() */
	int newfd_in, oldfd_in;		/* File descriptors used in input redirection */
	int newfd_out, oldfd_out;	/* File descriptors used in output redirection */
	int newfd_out_append, oldfd_out_append; /* File descriptors used in output append redirection */
 
	/* Initialize signal handling and job list */
	ignore_terminal_signals();
	my_job_list = new_list("Job List");	/* List of jobs in the background or suspended */
	signal(SIGCHLD, sigchld_handler);
	signal(SIGHUP, sighup_handler);

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* Get next command */
		
		/* Handle input and output redirection */
		char *file_in, *file_out, *file_out_append;
		parse_redirections(args, &file_in, &file_out);
		parse_append_redirection(args, &file_out_append);
		 
		if(args[0]==NULL) continue;   /* Do nothing if empty command */

		/* 
         * Built-in command: exit
         * Terminates the shell process.
         * Prints a goodbye message and exits with success status.
         */
		if(!strcmp(args[0], "exit")) {
			printf("Bye\n");
			exit(EXIT_SUCCESS);

		/* 
         * Built-in command: cd
         * Changes the current working directory of the shell.
         * If no argument is given, changes to the user's HOME directory.
         * On error, prints an error message.
         */
		} else if(!strcmp(args[0], "cd")) {	
			char* path = (args[1] == NULL) ? getenv("HOME") : args[1];
			int cd_status = chdir(path);
			if(cd_status == -1) {
				perror("cd error");
			} else {
				printf("Current working directory changed to %s\n", path);
			}

		/* 
         * Built-in command: jobs
         * Lists all jobs that are currently in the background or stopped.
         * If there are no jobs, prints a message indicating so.
         */
		} else if (!strcmp(args[0], "jobs")) {
			if(empty_list(my_job_list)) {
				printf("There are no jobs in background or stopped\n");
			} else {
				print_job_list(my_job_list); /* Print list of background jobs */
			}

		/* 
         * Built-in command: fg
         * Brings a background or stopped job to the foreground.
         * If a job position is given as an argument, uses that; otherwise, defaults to position 1.
         * - Finds the job in the job list.
         * - If found, resumes it if stopped, sets terminal control, and waits for it to finish or stop.
         * - Removes the job from the job list and updates its state.
         * - Handles signals and terminal control properly.
         */
		} else if (!strcmp(args[0], "fg")) {
			int pos = (args[1] == NULL) ? 1 : atoi(args[1]);
			block_SIGCHLD();
			job* fg_job = get_item_bypos(my_job_list, pos);

			if(fg_job == NULL) { /* No jobs found */
				printf("There is no job in position %d\n", pos);
				unblock_SIGCHLD();

			} else {
				int fg_job_pgid = fg_job->pgid;
				char fg_job_command[MAX_LINE];
				strcpy(fg_job_command, fg_job->command);

				if(fg_job->state == STOPPED) {
					printf("Resuming job in foreground: [%d] %s\n", pos, fg_job_command);
				} else {
					printf("Bringing job to foreground: [%d] %s\n", pos, fg_job_command);
				}

				set_terminal(fg_job_pgid); /* Set terminal to job's process group */
				fg_job->state = FOREGROUND; /* Change state to foreground */

				int fg_status = killpg(fg_job_pgid, SIGCONT); /* Continue the job */
				if(fg_status == -1) {
					perror("fg error");
					unblock_SIGCHLD();
					set_terminal(getpid());
				}
				delete_job(my_job_list, fg_job); /* Remove job from job list */

				unblock_SIGCHLD();

				pid_wait = waitpid(fg_job_pgid, &status, WUNTRACED); /* Wait for the job */
				set_terminal(getpid());	 /* Set terminal back to shell */
				block_SIGCHLD();
				if (pid_wait == -1) {
					perror("waitpid error");
					unblock_SIGCHLD();

				} else {
					if (WIFSTOPPED(status)) {
						add_job(my_job_list, new_job(fg_job_pgid, fg_job_command, STOPPED));
						printf("Process stopped by signal: %d\n", WSTOPSIG(status));
					} else if (WIFCONTINUED(status)) {
						printf("Process continued\n");
					} else {
						if (WIFEXITED(status)) {
							printf("Process completed with exit code: %d\n", WEXITSTATUS(status));
						} else if (WIFSIGNALED(status)) {
							printf("Process terminated by signal: %d\n", WTERMSIG(status));
						}
					}

					status_res = analyze_status(status, &info);
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", fg_job_pgid, fg_job_command, status_strings[status_res], info);
					unblock_SIGCHLD();
				}
			}
		
		/* 
         * Built-in command: bg
         * Continues a stopped job in the background.
         * If a job position is given as an argument, uses that; otherwise, defaults to position 1.
         * - Finds the job in the job list.
         * - If found, sets its state to BACKGROUND and sends SIGCONT to its process group.
         * - Handles errors if the job does not exist or cannot be continued.
         */
		} else if(!strcmp(args[0], "bg")) {	
			block_SIGCHLD();
			int pos = (args[1] == NULL) ? 1 : atoi(args[1]);
			job* bg_job = get_item_bypos(my_job_list, pos);

			if(bg_job == NULL) { /* No jobs found */
				printf("There is no job in position %d\n", pos);

			} else {
				bg_job->state = BACKGROUND;
				int bg_status = killpg(bg_job->pgid, SIGCONT); /* Continue the background job */
				if(bg_status == -1) {
					perror("bg error");
				}
			}
			unblock_SIGCHLD();

		/* 
         * Built-in command: currjob
         * Prints information about the current (first) job in the job list.
         * If there are no jobs, prints a message indicating so.
         */
		} else if(!strcmp(args[0], "currjob")) {
			block_SIGCHLD();
			job* current_job = get_item_bypos(my_job_list, 1);

			if (current_job == NULL) { /* No jobs found */
				printf("No current job\n");

			} else {
				printf("Current job: PID=%d command=%s\n", current_job->pgid, current_job->command);
			}
			unblock_SIGCHLD();

		/* 
         * Built-in command: deljob
         * Deletes the current (first) job from the job list if it is running in background.
         * - If there are no jobs, prints a message indicating so.
         * - If the current job is stopped (suspended), does not allow deletion and prints a warning.
         * - If the current job is running in background, deletes it from the job list and prints a confirmation.
         */
		} else if(!strcmp(args[0], "deljob")) {
			block_SIGCHLD();
			job* current_job = get_item_bypos(my_job_list, 1);

			if (current_job == NULL) { /* No jobs found */
				printf("No current job\n");

			} else if (current_job->state == STOPPED) { /* The process is suspended */
				printf("Cannot delete suspended background jobs\n");

			} else if (current_job->state == BACKGROUND) { /* The process is running in background */
				printf("Deleting current job from jobs list: PID=%d command=%s\n", current_job->pgid, current_job->command);
				delete_job(my_job_list, current_job);
			}
			unblock_SIGCHLD();
		
		/* 
         * Built-in command: zjobs
         * Lists all zombie child processes whose parent is the shell.
         * - Iterates through /proc to find processes in zombie state (state 'Z').
         * - Prints the PID of each zombie child process whose parent PID matches the shell's PID.
         */
		} else if(!strcmp(args[0], "zjobs")) {
			DIR *d;
			struct dirent *dir;
			char buff[2048];
			d = opendir("/proc"); /* Open the /proc directory to iterate over all processes */
			if (d) {
				while ((dir = readdir(d)) != NULL) {
					/* Build the path to the stat file of each process */
					sprintf(buff, "/proc/%s/stat", dir->d_name);
					FILE *fd = fopen(buff, "r");
					if (fd) {
						long pid;	/* PID */
						long ppid;	/* Parent PID */
						char state;	/* State: State: R (runnable), S (sleeping), T (stopped), Z (zombie) */
					
						/* Read the PID, command name, state and parent PID from /proc/<pid>/stat */
						fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid);
						fclose(fd);

						/* Check if the process is in zombie state and its parent is the shell */
						if ((ppid == getpid()) && (state == 'Z')) { 
							printf("%ld\n", pid);
						}
					}
				} 
				closedir(d); /* Close the /proc directory */
			} else {
				perror("Failed to open the /proc directory");
			}

		/* 
         * Built-in command: bgteam
         * Launches N background jobs running the specified command.
         * Usage: bgteam <N> <command> [args...]
         * - N: Number of background jobs to launch (must be > 0).
         * - command: The command to execute in each background job.
         * For each job:
         *   - Forks a new child process.
         *   - In the parent (shell), sets up the job in the background job list.
         *   - In the child, executes the specified command with its arguments.
         * If arguments are missing or N is not positive, prints an error message.
         */
		} else if(!strcmp(args[0], "bgteam")) {
			if (args[1] == NULL || args[2] == NULL) {
				/* Not enough arguments provided */
				printf("The bgteam command requires two arguments");
			
			} else if (atoi(args[1]) > 0) {
				int n = atoi(args[1]); /* Number of jobs to launch */

				int i = 0;
				while(i < n) { 
					pid_fork = fork(); /* Fork a new process */

					if (pid_fork > 0) { /* We are in the shell */
						new_process_group(pid_fork);
						printf("Background job running... pid: %d, command %s\n", pid_fork, args[2]);
						block_SIGCHLD();
						add_job(my_job_list, new_job(pid_fork, args[2], BACKGROUND));
						unblock_SIGCHLD();

					} else if (pid_fork == 0) { /* We are in the child */
						restore_terminal_signals();
						int j = 1;
						char* new_args[MAX_LINE/2]; /* New arguments for execvp (skip "bgteam" and N) */
						while (args[j] != NULL) {
							j++;
							new_args[j - 2] = args[j];
						}

						execvp(new_args[0], new_args); /* Execute the command */
						printf("Error, command not found: %s\n", new_args[0]);
						exit(EXIT_FAILURE);

					} else { /* There was an error */
						perror("Fork error");
					}
					++i;
				}
			}

		/* 
         * Built-in command: fico
         * Runs the filecount.sh script, optionally with a prefix argument.
         * - If run in the foreground, waits for the script to finish or stop, and prints its status.
         * - If run in the background (with &), adds the job to the background job list.
         * - Handles job control, terminal signals, and job list updates.
         * Usage: fico [prefix]
         *   - If [prefix] is given, passes it to filecount.sh to count files starting with that prefix.
         *   - If no prefix is given, counts all files in the current directory.
         */
		} else if(!strcmp(args[0], "fico")) {
			pid_fork = fork();

			if (pid_fork > 0) { /* We are in the shell */
				new_process_group(pid_fork);

				if (!background) { /* The command was launched in the foreground */
					set_terminal(pid_fork);
					pid_wait = waitpid(pid_fork, &status, WUNTRACED);
					set_terminal(getpid());
					if (pid_wait == pid_fork) {
						status_res = analyze_status(status, &info);
						printf("Foreground pid: %d, command: fico, %s, info: %d\n", pid_fork, status_strings[status_res], info);

						if (WIFSTOPPED(status)) { /* The command was stopped */
							printf("Stopped pid: %d, command:fico, %s, info: %d\n", pid_fork, status_strings[status_res], info);
							block_SIGCHLD();
							add_job(my_job_list, new_job(pid_wait, "fico", STOPPED));
							unblock_SIGCHLD();
						}
					} else if (pid_wait == -1) {
						perror("Wait error");
					}
				
				} else { /* The command was launched in the background */
					printf("Background job running... pid: %d, command fico\n", pid_fork);
					block_SIGCHLD();
					add_job(my_job_list, new_job(pid_fork, "fico", BACKGROUND));
					unblock_SIGCHLD();				}
				
			} else if (pid_fork == 0) { /* We are in the child */
				if (!background) set_terminal(getpid());
				restore_terminal_signals();

				char *args_fico[] = {"./filecount.sh", args[1], NULL};
				execvp(args_fico[0], args_fico); /* Execute the filecount.sh file */
				exit(EXIT_FAILURE);
			
			} else {
				perror("Fork error");
			}
		
		/*
         * Built-in command: mask
         * Allows running a command with certain signals blocked (masked).
         * Usage: mask <signal1> <signal2> ... -c <command> [args...]
         * - The user specifies one or more signal numbers to block, followed by "-c" and the command to run.
         * - The specified signals are blocked in the child process before executing the command.
         * - If run in the foreground, waits for the command to finish or stop, and prints its status.
         * - If run in the background (with &), adds the job to the background job list.
         * - Handles syntax errors, job control, and terminal signals.
         */
		} else if(!strcmp(args[0], "mask")) {	
			int signals[100];		/* Array to store signal numbers to block */
			int num_signals = 0;	/* Size of the array */
			int syntax_error = 0;	/* Flag for syntax error */
			
			/* Parse signal numbers until "-c" is found */
			int i = 1;
			while (args[i] != NULL && strcmp(args[i], "-c")) {
				signals[i - 1] = atoi(args[i]);
				if (signals[i - 1] <= 0) { /* Signal numbers must be positive */
					printf("mask: error de sintaxis\n");
					syntax_error = 1;
				}
				++i;
				num_signals++;
			}

			/* Check for syntax errors: missing "-c" or command after "-c" */
			if ((args [i] == NULL || args[i + 1] == NULL) && syntax_error == 0) {
				printf("mask: error de sintaxis\n");
				syntax_error = 1;

			} else if (syntax_error == 0) {
				++i; /* Move past "-c "*/
				int j = 0;
				char* new_args[MAX_LINE/2]; /* Arguments for the command to execute */

				/* Copy command and its arguments after "-c" */
				while (args[i] != NULL) {
					new_args[j] = args[i];
					++i;
					++j;
				}
				new_args[j] = NULL;

				pid_fork = fork(); /* Create child process */

				if(pid_fork > 0) { /* We are in the shell */
					new_process_group(pid_fork);

					if(!background) { /* The command was launched in the foreground */
						set_terminal(pid_fork);
						pid_wait = waitpid(pid_fork, &status, WUNTRACED);
						set_terminal(getpid());
						if (pid_wait == pid_fork) {
							status_res = analyze_status(status, &info);
							printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, new_args[0], status_strings[status_res], info);
							
							if (WIFSTOPPED(status)) { /* The command was stopped*/
								printf("Stopped pid: %d, command: %s, %s, info: %d\n", pid_fork, new_args[0], status_strings[status_res], info);
								add_job(my_job_list, new_job(pid_wait, new_args[0], STOPPED));
							}
						
						} else if (pid_wait == -1) {
							perror("Wait error");
						}

					} else { /* The job was launched in the background */
						printf("Background job running... pid: %d, command %s\n", pid_fork, new_args[0]);
						block_SIGCHLD();
						add_job(my_job_list, new_job(pid_fork, new_args[0], BACKGROUND));
						unblock_SIGCHLD();
					}

				} else if (pid_fork == 0) { /* We are in the child */
					/* Block each specified signal in the children process */
					int k = 0;
					while (k < num_signals) {
						block_signal(signals[k], 1);
						k++;
					}

					if(!background) set_terminal(getpid()); /* Foreground process */
					restore_terminal_signals();
					execvp(new_args[0], new_args); /* Execute the command */
					printf("Error, command not found: %s\n", new_args[0]);
					exit(EXIT_FAILURE);

				} else { /* There was an error */
					perror("Fork error");
				}
			}

		} else {

			/** The steps are:
			*	 (1) Fork a child process using fork()
			*	 (2) The child process will invoke execvp()
			* 	 (3) If background == 0, the parent will wait, otherwise continue
			*	 (4) Shell shows a status message for processed command
			* 	 (5) Loop returns to get_commnad() function
			**/
	
			pid_fork = fork(); /* Create child process */

			if(pid_fork > 0) { /* We are in the shell */
				new_process_group(pid_fork);

				if(!background) { /* The command was launched in the foreground */
					set_terminal(pid_fork);
					pid_wait = waitpid(pid_fork, &status, WUNTRACED);
					set_terminal(getpid());
					if (pid_wait == pid_fork) {
						status_res = analyze_status(status, &info);
						printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_strings[status_res], info);
						
						if (WIFSTOPPED(status)) { /* The command was stopped*/
							printf("Stopped pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_strings[status_res], info);
							add_job(my_job_list, new_job(pid_wait, args[0], STOPPED));
						}
					
					} else if (pid_wait == -1) {
						perror("Wait error");
					}

				} else { /* The job was launched in the background */
					printf("Background job running... pid: %d, command %s\n", pid_fork, args[0]);
					block_SIGCHLD();
					add_job(my_job_list, new_job(pid_fork, args[0], BACKGROUND));
					unblock_SIGCHLD();
				}
	
			} else if (pid_fork == 0) { /* We are in the child */
				if(!background) set_terminal(getpid()); /* Foreground process */
				restore_terminal_signals();

				if(file_in != NULL || file_out != NULL || file_out_append != NULL) { /* Redirection */

					if(file_in != NULL) { /* Input redirection */
						newfd_in = open(file_in, O_RDONLY);
						oldfd_in = fileno(stdin);
						if(newfd_in == -1) {
							perror("Error when opening input file");
							continue;
						}

						if(dup2(newfd_in, oldfd_in) == -1) {
							perror("Error redirectioning input");
							close(newfd_in);
							continue;
						}
					}

					if(file_out != NULL) { /* Output redirection */
						newfd_out = fileno(stdout);
						oldfd_out = open(file_out, O_RDWR | O_CREAT | O_TRUNC, 0666);
						if(newfd_out == -1) {
							perror("Error when opening output file");
							continue;
						}

						if(dup2(oldfd_out, newfd_out) == -1) {
							perror("Error redirecioning output");
							close(oldfd_out);
							continue;
						}
					}

					if(file_out_append != NULL) { /* Output append redirection */
						newfd_out_append = fileno(stdout);
						oldfd_out_append = open(file_out_append, O_RDWR | O_CREAT | O_APPEND, 0666);
						if(newfd_out_append == -1) {
							perror("Error when opening output file");
							continue;
						}

						if(dup2(oldfd_out_append, newfd_out_append) == -1) {
							perror("Error redirecioning output");
							close(oldfd_out_append);
							continue;
						}
					}

					if(file_in != NULL)  close(newfd_in);
					if(file_out != NULL) close(oldfd_out);
					if(file_out_append != NULL) close(oldfd_out_append);
				}

				execvp(args[0], args); /* Execute the command */
				printf("Error, command not found: %s\n", args[0]);
				exit(EXIT_FAILURE);

			} else { /* There was an error */
				perror("Fork error");
			}
		}
	} /* End while */
 }