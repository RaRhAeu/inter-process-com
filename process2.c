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

#define PROCESS_IS_READY 1
#define PROCESS_FAILED -1

int q_id = -1;
struct qmessage {
	long int my_qtype;
	int stat_pid[2];
}qdata;

char *buf; // bufor dla danych

// prototypy funkcji
void error_handler(const char *, int);
void read_from_process1();
void write_to_proces3();
void send_message_to_parent();
void SIGUSR1_handler(int);
void process2_on_exit(int);


int main(void)
{
	buf = (char*)malloc(1024*sizeof(char)); // alokacja pamieci
	q_id = msgget((key_t)1234, 0666 | IPC_CREAT); // utworzenie/dolaczenie do kolejki komunikatow
	// sighandlers
	(void)signal(SIGUSR1, SIGUSR1_handler);
	(void)signal(SIGINT, process2_on_exit);
	fprintf(stderr, "%s\n", "Process 2 in main...");
	while(1) // czekanie na wybudzenie przez sygnal
	{
		pause();
	}
	return 0;
}

void error_handler(const char* com, int error)
{
	fprintf(stderr, "%s %d\n", com, error);
	qdata.stat_pid[0] = (int)PROCESS_FAILED;
	qdata.stat_pid[1] = (int)getpid();
	msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0);
	kill(getppid(), SIGUSR1);
}

void read_from_process1()
{
	fprintf(stderr, "%s\n", "Process 2 reading from process 1...");
	memset(buf, '\0', 1024);
	read(STDIN_FILENO, buf, 1024);
	fprintf(stderr, "%s\n", buf);

}

void write_to_proces3()
{
	fprintf(stderr, "%s\n", "Process 2 writing to process 3...");
	long int dlugosc = strlen(buf);
	write(STDOUT_FILENO, &dlugosc, sizeof(dlugosc));
}

void send_message_to_parent() // wyslanie komunikatu
{
	fprintf(stderr, "%s\n", "Process 2 senging message...");
	qdata.my_qtype = 1;
	qdata.stat_pid[0] = (int)PROCESS_IS_READY;
	qdata.stat_pid[1] = (int)getpid();
	if(msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0) == -1)
	{
		error_handler("msgsnd failed...", errno);
	}
}

void SIGUSR1_handler(int signo)
{
	fprintf(stderr, "%s\n", "P2 in SIGUSR1_handler...");
	read_from_process1();
	write_to_proces3();
	send_message_to_parent();
	kill(getppid(),SIGUSR1);
}


void process2_on_exit(int signo)
{
	fprintf(stderr, "%s\n", "SIGINT received in process 2, exiting...");
	free(buf); // zwalnianie pamieci
	exit(0);
}
