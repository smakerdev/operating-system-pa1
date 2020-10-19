/***********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include "types.h"
#include "parser.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

/*======================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******         */
/**
 * String used as the prompt (see @main()). You may change this to
 * change the prompt */
static char __prompt[MAX_TOKEN_LEN] = "$";

/**
 * Time out value. It's OK to read this value, but SHOULD NOT CHANGE
 * IT DIRECTLY. Instead, use @set_timeout() function below.
 */
static unsigned int __timeout = 2;

static void set_timeout(unsigned int timeout)
{
	__timeout = timeout;

	if (__timeout == 0) {
		fprintf(stderr, "Timeout is disabled\n");
	} else {
		fprintf(stderr, "Timeout is set to %d second%s\n",
				__timeout,
				__timeout >= 2 ? "s" : "");
	}
}
/*          ****** DO NOT MODIFY ANYTHING UP TO THIS LINE ******      */
/*====================================================================*/
char *command_name;
pid_t pid;

static void handler(int sig_number) {
	fprintf(stderr, "%s is timed out\n", command_name);
	kill(pid, SIGKILL);
}

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static int run_command(int nr_tokens, char *tokens[])
{
	/* This function is all yours. Good luck! */
	bool status;

	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = 0;

	sigaction(SIGALRM, &act, NULL);

	if (strncmp(tokens[0], "exit", strlen("exit")) == 0) {
		return 0;
	}

	if (strncmp(tokens[0], "prompt", strlen("prompt")) == 0) {
		strcpy(__prompt, tokens[1]);
		return 1;
	}

	if (strncmp(tokens[0], "for", strlen("for")) == 0) {
		int count = atoi(tokens[1]);

		for (int i = 0; i < count; i++)
			run_command(atoi(tokens[0]) - 2, tokens + 2);

		return 1;
	}

	if (strncmp(tokens[0], "timeout", strlen("timeout")) == 0) {
		if (tokens[1] == NULL)
			printf("Current timeout is %d second\n", __timeout);
		
		else {
			__timeout = atoi(tokens[1]);
			set_timeout(__timeout);
			status = true;
		}
	}

	if (strncmp(tokens[0], "cd", strlen("cd")) == 0) {
		if (nr_tokens == 1 || !strncmp(tokens[1], "~", strlen("~")))
			chdir(getenv("HOME"));
		
		chdir(tokens[1]);
		return 1;
	}

	command_name = tokens[0];
	pid = fork();

	if (pid == -1) {
		fprintf(stderr, "No such file or directory\n");
		return -1;
	} else if (pid == 0) {
		int value = execvp(tokens[0], tokens);

		if (value < 0) {
			fprintf(stderr, "No such file or directory\n");
			abort();
		}
		
		return 0;
	} else {
		if (status)
			alarm(__timeout);
		waitpid(pid, NULL, 0);
	}

	/*
	fork();
	exec();
	...
	*/

	return 1;
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Callback function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char *argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char *argv[])
{

}


/*======================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******        */

static bool __verbose = true;
static char *__color_start = "[0;31;40m";
static char *__color_end = "[0m";

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char *argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	if (__verbose)
		fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);

	while (fgets(command, sizeof(command), stdin)) {	
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;

		if (parse_command(command, &nr_tokens, tokens) == 0)
			goto more; /* You may use nested if-than-else, however .. */

		ret = run_command(nr_tokens, tokens);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			fprintf(stderr, "Error in run_command: %d\n", ret);
		}

more:
		if (__verbose)
			fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
