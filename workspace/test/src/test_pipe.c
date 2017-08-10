/*
 * test_pipe.c
 *
 *  Created on: 2017. 7. 14.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>


#define TRUE  1
#define FALSE 0

static int _run_cmd (char *cmd)
{
	int res;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		// char buf[1024];
		// while (1) {
		// 	if (fgets(buf, sizeof(buf), fp) == NULL)
		//		break;
		//	printf("----- %s", buf);
		//}
		int status = pclose(fp);
		int exit_code = -1;
		int normal_exit;

		if (WIFEXITED(status)) {
			normal_exit = TRUE;
			exit_code = WEXITSTATUS(status);
			if (exit_code == 0) {
				res = TRUE;
			}
			else if (exit_code >= 128) {
				int sig_num = exit_code - 128;
				if (sig_num == SIGPIPE)
					res = TRUE;
				else
					res = FALSE;
				printf("Yes signaled : %d\n", sig_num);
			}
			else {
				res = FALSE;
			}
			// SIGPIPE		13
		}
		else {
			normal_exit = FALSE;
			res = FALSE;
		}

		printf("%s --> %d  normal:%d  exit:%d (%x) \n", cmd, status, (int)normal_exit, exit_code, exit_code);
	}
	else {
		res = FALSE;
	}

	return res;
}

void test_runcmd()
{
	char	buf[256];
	int	n;

	while (1) {
		fgets(buf, sizeof(buf), stdin);
		n = strlen(buf);
		if (n > 0 && buf[n-1] < 0x020) {
			buf[n-1] = 0;
			n--;
		}
		if (n <= 0)
			break;
		int res = _run_cmd (buf);
		if (res)
			printf("success : %s\n", buf);
		else
			printf("error : %s\n", buf);
		printf("\n");
	}
}

