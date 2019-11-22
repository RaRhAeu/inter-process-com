#include <stdio.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/wait.h>

#define PROCESS_IS_READY 1 //Predefiniowana wartosc oznaczajaca gotowosc procesu
#define PROCESS_FAILED -1  // Jak wyzej dla bledu zwroconego przez proces
	
int q_id = -1; // Identyfikator kolejki komunikatow
long int dlugosc; // odebrana liczba

struct qmessage {
	long int my_qtype;
	int stat_pid[2];
}qdata;


void error_handler(const char*, int);
void read_from_process2();
void send_message_to_parent();
void write_to_stdout();
void SIGUSR1_handler(int);
void process3_on_exit();


int main(void)
{
	q_id = msgget((key_t)1234, 0666 | IPC_CREAT);
	(void)signal(SIGUSR1, SIGUSR1_handler);
	(void)signal(SIGINT, process3_on_exit);
	fprintf(stderr, "%s\n", "Process 3 in main...");
	while(1)
	{
		pause();
	}
	return 0;
}

void error_handler(const char *com, int error)
{
	fprintf(stderr, "%s %d\n", com, error);
	qdata.stat_pid[0] = (int)PROCESS_FAILED;
	qdata.stat_pid[1] = (int)getpid();
	msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0);
	kill(getppid(),SIGUSR1);
}
void read_from_process2()
{

	fprintf(stderr, "%s\n", "Process 3 in read_from_process2...");
	read(STDIN_FILENO, &dlugosc, sizeof(dlugosc));
}
void send_message()
{
	fprintf(stderr, "%s\n", "Process 3 sending message...");
	qdata.my_qtype = 1;
	qdata.stat_pid[0] = (int)PROCESS_IS_READY;
	qdata.stat_pid[1] = (int)getpid();
	if((msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0)) == -1)
	{
		error_handler("msgsnd failed", errno);
	}
}

void write_to_stdout()
{
	fprintf(stderr, "%s\n", "Process 3 writing to stdout...");
	fprintf(stdout, "%ld\n", dlugosc);
}

void SIGUSR1_handler(int signo)
{
	fprintf(stderr, "%s\n", "P3 in SIGUSR1_handler...");
	read_from_process2();
	write_to_stdout();
	send_message();
	kill(getppid(),SIGUSR1);
}


void process3_on_exit(int signo)
{

	fprintf(stderr, "%s\n", "SIGINT received in process 3 exiting...\n");
	exit(0);
}