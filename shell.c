#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARG 20
#define HOST_NAME_MAX 30
#define PATH_MAX 100
#define BOLDCYAN "\033[1m\033[36m"
#define BOLDWHITE "\033[1m\033[37m"
#define RESET "\033[0m"

char states[2][10] = {"Running", "Stopped"};

typedef struct proc
{
	int pid;
	char *name;
} proc;

typedef struct procinfo
{
	int pid;
	int state;
	int job_id;
	char *name;
	struct procinfo *next;
	struct procinfo *prev;
} procinfo;

proc *child;

typedef struct joblist
{
	procinfo *start;
	procinfo *last;
} joblist;
joblist *j, *run;

int jid = 1;

int tokenize_input(char buf[], char *args[], char delim[])
{
	char *string, *token;
	int arg_cnt = 0;
	if (buf == NULL)
	{
		// Ctrl-D pressed
		printf("\n");
		exit(0);
		return 0;
	}
	string = strdup(buf);

	token = strtok(string, delim);
	int i = 0;
	while (token)
	{
		args[i] = strdup(token);
		++i;
		token = strtok(NULL, delim);
	}
	args[i] = NULL;
	arg_cnt = i;

	return arg_cnt;
}
void execute_inredir(char *cmdargs[], char *file_in[])
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
		close(0);
		int fd = open(file_in[0], O_RDONLY);

		// Error checking of file
		if (fd == -1)
		{
			printf("Error Number %d\n", errno);
			return;
		}

		if (execvp(cmdargs[0], cmdargs) == -1)
		{
			printf("shell: %s: command not found\n", cmdargs[0]);
			exit(0);
		}
	}
	else
	{
		wait(0);
	}
}
void execute_outredir(char *cmdargs[], char *file_out[])
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
		close(1);
		int fd = open(file_out[0], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

		// Error checking of file
		if (fd == -1)
		{
			printf("Error Number %d\n", errno);
			return;
		}
		if (execvp(cmdargs[0], cmdargs) == -1)
		{
			printf("shell: %s: command not found\n", cmdargs[0]);
			exit(0);
		}
	}
	else
	{
		wait(0);
	}
}
void execute_in_out_redir(char *cmdargs[], char *file_in[], char *file_out[])
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
		close(0);
		int fd = open(file_in[0], O_RDONLY);

		// Error checking of file
		if (fd == -1)
		{
			printf("Error Number %d\n", errno);
			return;
		}
		close(1);
		int fd2 = open(file_out[0], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		
		// Error checking of file
		if (fd2 == -1)
		{
			printf("Error Number %d\n", errno);
			return;
		}

		if (execvp(cmdargs[0], cmdargs) == -1)
		{
			printf("shell: %s: command not found\n", cmdargs[0]);
			exit(0);
		}
	}
	else
	{
		wait(0);
	}
}
int tokenize_execute_in_out_redir(char buf[])
{
	char *args[MAXARG], delim[] = " ";
	char err_msg[] = "shell: syntax error: unexpected token\n";
	int arg_cnt = 0, j = 0, inredir = 0, outredir = 0;
	arg_cnt = tokenize_input(buf, args, delim);
	char *inout_args[arg_cnt], *file_in_arg[1], *file_out_arg[1];

	if (arg_cnt <= 2)
	{
		printf("%s", err_msg);
		return 2;
	}

	for (int index = 0; index < arg_cnt; index++)
	{
		inredir = 0, outredir = 0;
		inredir = !strcmp(args[index], "<");
		outredir = !strcmp(args[index], ">");
		if (inredir || outredir)
		{
			if (index + 1 == arg_cnt)
			{
				// If last argument is redirection operator
				printf("%s", err_msg);
				return 2;
			}
			else
			{
				if (inredir)
				{
					file_in_arg[0] = args[index + 1];
				}
				else
				{
					file_out_arg[0] = args[index + 1];
				}
				index++;
				// To skip file argument from storing in command arg
			}
		}
		else
		{
			inout_args[j++] = args[index];
		}
	}

	inout_args[j] = NULL;

	execute_in_out_redir(inout_args, file_in_arg, file_out_arg);
	return 1;
}
int tokenize_execute_inredir(char buf[])
{
	char *args[MAXARG], delim[] = " ";
	char err_msg[] = "shell: syntax error: unexpected token\n";
	int arg_cnt = 0, j = 0;
	arg_cnt = tokenize_input(buf, args, delim);
	char *in_args[arg_cnt], *file_arg[1];

	if (arg_cnt == 1)
	{
		// If only redirection operator is given as input
		printf("%s", err_msg);
		return 2;
	}

	for (int index = 0; index < arg_cnt; index++)
	{
		if (!strcmp(args[index], "<"))
		{
			if (index + 1 == arg_cnt)
			{
				// If last argument is redirection operator
				printf("%s", err_msg);
				return 2;
			}
			else
			{
				file_arg[0] = args[index + 1];
			}
			index++;
			// To skip file argument from storing in command arg
		}
		else
		{
			in_args[j++] = args[index];
		}
	}
	in_args[j] = NULL;

	execute_inredir(in_args, file_arg);
	return 1;
}
int tokenize_execute_outredir(char buf[])
{
	char *args[MAXARG], delim[] = " ";
	char err_msg[] = "shell: syntax error: unexpected token\n";
	int arg_cnt = 0, j = 0;
	arg_cnt = tokenize_input(buf, args, delim);
	char *out_args[arg_cnt], *file_arg[1];

	if (arg_cnt == 1)
	{
		// If only redirection operator is given as input
		printf("%s", err_msg);
		return 2;
	}

	for (int index = 0; index < arg_cnt; index++)
	{
		if (!strcmp(args[index], ">"))
		{
			if (index + 1 == arg_cnt)
			{
				// If last argument is redirection operator
				printf("%s", err_msg);
				return 2;
			}
			else
			{
				file_arg[0] = args[index + 1];
			}
			index++;
			// To skip file argument from storing in command arg
		}
		else
		{
			out_args[j++] = args[index];
		}
	}
	out_args[j] = NULL;

	execute_outredir(out_args, file_arg);

	return 1;
}
int tokenize_pipe_redir(char buf[], char *pipe_args[], char *redir_args[])
{
	// Indexes for redir_args (4 strings)
	// 0 - indir exists
	// 1 - outdir exists
	// 2 - input file
	// 3 - output file

	char *args[MAXARG];
	char err_msg[] = "shell: syntax error: unexpected token\n";
	int arg_cnt = 0, j = 0, inredir = 0, outredir = 0;
	arg_cnt = tokenize_input(buf, args, " ");

	redir_args[0] = "0";
	redir_args[1] = "0";

	if (arg_cnt <= 2)
	{
		printf("%s", err_msg);
		return 2;
	}

	for (int index = 0; index < arg_cnt; index++)
	{
		inredir = 0, outredir = 0;
		inredir = !strcmp(args[index], "<");
		outredir = !strcmp(args[index], ">");
		if (inredir || outredir)
		{
			if (index + 1 == arg_cnt)
			{
				// If last argument is redirection operator
				printf("%s", err_msg);
				return 2;
			}
			else
			{
				if (inredir)
				{
					redir_args[0] = "1";
					redir_args[2] = args[index + 1];
				}
				else
				{
					redir_args[1] = "1";
					redir_args[3] = args[index + 1];
				}
				index++;
				// To skip file argument from storing in command arg
			}
		}
		else
		{
			pipe_args[j++] = args[index];
		}
	}

	pipe_args[j] = NULL;

	return 1;
}
void execute_pipe(char *args[][MAXARG], int cmd_cnt, char *redir_args[][4], int redir_op)
{
	int pipefd[cmd_cnt - 1][2];
	pid_t pid[cmd_cnt + 1];

	for (int t = 0; t < cmd_cnt + 1; t++)
	{
		pid[t] = 1;
	}

	// Number of pipes = cmd_cnt - 1
	for (int t = 0; t < cmd_cnt - 1; t++)
	{
		pipe(pipefd[t]);
	}

	int i = -1;
	while (i + 1 < cmd_cnt)
	{
		// no of pipes = cmd_cnt - 1
		// Number of pipes = '|'

		pid[i + 1] = fork();

		if (pid[i + 1] == 0)
		{
			// Starting cmd cannot take input from pipe

			for (int j = 0; j <= i; j++)
			{
				close(pipefd[j][1]);
			}
			if (redir_op && redir_args[i + 1][0][0] == '1')
			{
				int fd = open(redir_args[i + 1][2], O_RDONLY);

				dup2(fd, 0);
			}
			else
			{
				dup2(pipefd[i][0], 0);
				close(pipefd[i][0]);
			}

			if (i + 2 < cmd_cnt)
			{
				for (int j = 0; j <= i + 1; j++)
				{
					close(pipefd[j][0]);
				}

				// Condition for output redirection (if present)
				if (redir_op && redir_args[i + 1][1][0] == '1')
				{
					int fd = open(redir_args[i + 1][3], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

					dup2(fd, 1);
				}
				else
				{
					dup2(pipefd[i + 1][1], 1);
					close(pipefd[i + 1][1]);
				}
			}

			if (execvp(args[i + 1][0], args[i + 1]) == -1)
			{
				printf("shell: %s: command not found\n", args[i + 1][0]);
				exit(0);
			}
		}
		else
		{
			if (i + 2 < cmd_cnt)
			{
				pid[i + 2] = fork();
			}
			if (pid[i + 2] == 0)
			{

				for (int j = 0; j <= i + 1; j++)
				{
					close(pipefd[j][1]);
				}
				if (redir_op && redir_args[i + 2][0][0] == '1')
				{
					int fd = open(redir_args[i + 2][2], O_RDONLY);

					dup2(fd, 0);
				}
				else
				{
					dup2(pipefd[i + 1][0], 0);
					close(pipefd[i + 1][0]);
				}
				// For reading (above) and writing (below)
				if (i + 2 < cmd_cnt)
				{
					if (redir_op && redir_args[i + 2][1][0] == '1')
					{
						int fd = open(redir_args[i + 2][3], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

						dup2(fd, 1);
					}
					else
					{
						dup2(pipefd[i + 2][1], 1);
						close(pipefd[i + 2][1]);
					}
				}

				if (execvp(args[i + 2][0], args[i + 2]) == -1)
				{
					printf("shell: %s: command not found\n", args[i + 2][0]);
					exit(0);
				}
			}
		}
		i = i + 2;
	}
	for (int t = 0; t < cmd_cnt - 1; t++)
	{
		close(pipefd[t][0]);
		close(pipefd[t][1]);
	}
	for (int t = 0; t < cmd_cnt; t++)
	{
		wait(0);
	}
}
int check_redir(char buf[])
{
	if (strchr(buf, '>') || strchr(buf, '<'))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
int tokenize_execute_pipe(char buf[], int redir_op)
{
	char *args[MAXARG], delim[] = "|";
	int cmd_cnt = 0;
	cmd_cnt = tokenize_input(buf, args, delim);
	// 2D Array of Strings
	char *pipe_args[cmd_cnt][MAXARG];
	char *redir_args[cmd_cnt][4];

	for (int index = 0; index < cmd_cnt; index++)
	{
		pipe_args[index][0] = args[index];
		// Tokenize cmd into arguments

		// Condition for the entire shell input
		if (redir_op)
		{
			// Condition for individual cmds separated by pipe
			if (check_redir(args[index]))
			{
				tokenize_pipe_redir(pipe_args[index][0], pipe_args[index], redir_args[index]);
			}
			else
			{
				(void)tokenize_input(pipe_args[index][0], pipe_args[index], " ");
				redir_args[index][0] = "0";
				redir_args[index][1] = "0";
			}
		}
		else
		{
			(void)tokenize_input(pipe_args[index][0], pipe_args[index], " ");
		}
	}

	execute_pipe(pipe_args, cmd_cnt, redir_args, redir_op);

	return 1;
}
void execute_sys(char *cmdargs[], char buf[])
{
	int pid;
	int status;
	pid = fork();
	if (pid == 0)
	{
		if (execvp(cmdargs[0], cmdargs) == -1)
		{
			printf("shell: '%s': command not found\n", cmdargs[0]);
			exit(0);
		}
	}
	else
	{
		child->pid = pid;
		child->name = (char *)malloc(strlen(buf));
		strcpy(child->name, buf);

		waitpid(pid, &status, WUNTRACED);
	}
}
int check_operator(char buf[])
{
	int ret = 0;
	char *pipe_op = strchr(buf, '|');
	char *outdir = strchr(buf, '>');
	char *indir = strchr(buf, '<');

	if (pipe_op)
	{
		// Can contain '|' and redir operator both
		if (outdir || indir)
			ret = tokenize_execute_pipe(buf, 1);
		else
			ret = tokenize_execute_pipe(buf, 0);
	}
	else if (outdir || indir)
	{
		if (outdir && indir)
		{
			// If both input and output redirection
			ret = tokenize_execute_in_out_redir(buf);
		}
		else if (outdir)
		{
			// Output redirection
			ret = tokenize_execute_outredir(buf);
		}
		else
		{
			// Input redirection
			ret = tokenize_execute_inredir(buf);
		}
	}
	return ret;
}
void show_jobs()
{
	procinfo *temp = j->start;
	while (temp != NULL)
	{
		if (temp == j->last->prev)
		{
			printf("[%d]-\t%d\t%s\t%s", temp->job_id, temp->pid, states[temp->state], temp->name);
		}
		else if (temp == j->last)
		{
			printf("[%d]+\t%d\t%s\t%s", temp->job_id, temp->pid, states[temp->state], temp->name);
		}
		else
		{
			printf("[%d]\t%d\t%s\t%s", temp->job_id, temp->pid, states[temp->state], temp->name);
		}
		printf("\n");
		temp = temp->next;
	}
}
int get_pid_job(int jobno, int state)
{
	procinfo *temp = j->start;
	while (temp != NULL)
	{
		if (temp->job_id == jobno)
		{
			temp->state = state;
			return temp->pid;
		}
		temp = temp->next;
	}
	return -1;
}
int get_jobno(char *arg)
{
	char *ptr = arg;
	while (*ptr)
	{
		if (isdigit(*ptr))
		{
			int val = (int)strtol(ptr, &ptr, 10);
			return val;
		}
		else
		{
			ptr++;
		}
	}
	return 1;
}
void free_proc(int procid)
{
	procinfo *temp = j->start;
	while (temp != NULL)
	{
		if (temp->pid == procid)
		{
			if (temp->prev)
				temp->prev->next = temp->next;
			if (temp->next)
				temp->next->prev = temp->prev;
			if (temp == j->start)
			{
				j->start = temp->next;
				if (!j->start)
				{
					j->last = NULL;
				}
			}
			if (temp == j->start)
			{
				j->start = temp->next;
				if (!j->start)
				{
					j->last = NULL;
				}
			}
			if (temp == j->last)
			{
				j->last = temp->prev;
				if (!j->last)
				{
					j->start = NULL;
				}
			}
			temp->prev = NULL;
			temp->next = NULL;
			break;
		}
		temp = temp->next;
	}
	free(temp);
}
int check_resume_proc(int procid)
{
	procinfo *temp = j->last;
	while (temp != NULL)
	{
		if (temp->pid == procid)
		{
			return 1;
		}
		temp = temp->prev;
	}
	return 0;
}
void fg_proc(char *args[], int arg_cnt)
{
	int proc_id;
	// Get pid from job number
	if (arg_cnt == 2)
	{
		int job_no = get_jobno(args[1]);

		proc_id = get_pid_job(job_no, 1);
		// if returned value -1, no such job exists
		if (proc_id == -1)
		{
			printf("fg: %d: no such job\n", job_no);
			// return to shell
			return;
		}
	}
	else
	{
		// else use last suspended job
		if (j->last)
			proc_id = j->last->pid;
		else
			return;
	}

	// Sending SIGCONT to  proc_id
	kill(proc_id, SIGCONT);

	// wait for child
	int w_st;
	pid_t return_pid = waitpid(proc_id, &w_st, WUNTRACED);

	if (return_pid == -1)
	{
		// error 
		free_proc(proc_id);
	}
	else if (return_pid == 0)
	{
		// child is still running 
	}
	else if (return_pid == proc_id)
	{
		// child is finished. exit status in   status 

		kill(proc_id, 0);
		if (errno == ESRCH)
		{
			// For terminated procesess
			// Free the procinfo
			free_proc(proc_id);
			return;
		}
	}
}
void bg_proc(char *args[], int arg_cnt)
{
	int proc_id;
	if (arg_cnt == 2)
	{
		int job_no = get_jobno(args[1]);
		// Get pid from job number

		proc_id = get_pid_job(job_no, 0);

		// if returned value -1, no such job exists
		if (proc_id == -1)
		{
			printf("bg: %d: no such job\n", job_no);
			// return to shell
			return;
		}
	}
	else
	{
		// else use last suspended job
		if (j->last)
			proc_id = j->last->pid;
		else
			return;
	}

	// Sending SIGCONT to  proc_id

	kill(proc_id, SIGCONT);
}
int check_built_in_cmd(char *args[], int arg_cnt)
{
	int ret = 0;
	int result = 0;
	if (strcmp(args[0], "cd") == 0)
	{
		ret = 1;
		if (arg_cnt > 2)
		{
			fprintf(stderr, "shell: cd: %s: too many arguments\n", args[1]);
			return ret;
		}
		result = chdir(args[1]);

		if (result == 0)
			;
		else
		{
			fprintf(stderr, "shell: cd: %s: ", args[1]);
			perror("");
		}
	}
	else if (strcmp(args[0], "exit") == 0)
	{
		ret = 1;
		exit(0);
	}
	else if (strcmp(args[0], "help") == 0)
	{
		ret = 1;

		printf("My Shell version 0.0.1(1)-release (x86_64-pc-linux-gnu)\n");
		printf("Execute commands here\n");
	}
	else if (strcmp(args[0], "jobs") == 0)
	{
		ret = 1;
		show_jobs();
	}
	else if (strcmp(args[0], "fg") == 0)
	{
		ret = 1;
		fg_proc(args, arg_cnt);
	}
	else if (strcmp(args[0], "bg") == 0)
	{
		ret = 1;
		bg_proc(args, arg_cnt);
	}
	return ret;
}
char *getPrompt()
{
	char target[HOST_NAME_MAX + PATH_MAX + 100];
	char *buf;
	char hostname[HOST_NAME_MAX + 1];
	char *uname = getenv("USER");
	char cwd[PATH_MAX];

	gethostname(hostname, HOST_NAME_MAX + 1);

	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		snprintf(target, sizeof(target), BOLDWHITE "%s@%s:" BOLDCYAN "%s" RESET "$ ", uname, hostname, cwd);
	}
	else
	{
		snprintf(target, sizeof(target), BOLDWHITE "%s@%s:" RESET "$ ", uname, hostname);
	}
	buf = malloc(strlen(target) * sizeof(char));
	strcpy(buf, target);
	return buf;
}
void sigint_handler(int signo)
{
}
void sigstp_handler(int signo)
{
	if (!(child->pid))
	{
		// When initially no processes are executed
		return;
	}
	kill(child->pid, 0);
	if (errno == ESRCH)
	{
		// proc doesn't exist
		// For terminated procesess, no need for fg bg or for invalid process
		return;
	}
	if (check_resume_proc(child->pid))
	{
		return;
	}

	procinfo *p1 = (procinfo *)malloc(sizeof(struct procinfo));
	p1->pid = child->pid;
	p1->name = child->name;
	p1->job_id = jid++;
	p1->state = 1;

	if (j->start == NULL)
	{
		j->start = j->last = p1;
		p1->prev = NULL;
		p1->next = NULL;
	}
	else
	{
		// Adding at the end
		p1->prev = j->last;
		j->last->next = p1;
		j->last = p1;
		p1->next = NULL;
	}
	printf("\nStopped\t%s\n", p1->name);
}
int main()
{
	signal(SIGINT, sigint_handler);
	signal(SIGTSTP, sigstp_handler);

	j = (joblist *)malloc(sizeof(struct joblist));
	j->start = j->last = NULL;

	run = (joblist *)malloc(sizeof(struct joblist));
	run->start = run->last = NULL;

	child = (proc *)malloc(sizeof(struct proc));

	while (1)
	{
		int ret_blt_in = 0, ret_op = 0, arg_cnt = 0;
		char *buffer, *cmdargs[MAXARG], *prompt_buf, delim[] = " ";

		prompt_buf = getPrompt(prompt_buf);

		buffer = readline(prompt_buf);
		arg_cnt = tokenize_input(buffer, cmdargs, delim);

		if (arg_cnt == 0)
		{
			// If enter, whitespace, etc given as input
			continue;
		}
		else
		{
			add_history(buffer);
		}
		ret_blt_in = check_built_in_cmd(cmdargs, arg_cnt);

		if (!ret_blt_in)
		{
			ret_op = check_operator(buffer);
		}

		if (!ret_op && !ret_blt_in)
		{
			execute_sys(cmdargs, buffer);
		}
	}
	return 0;
}