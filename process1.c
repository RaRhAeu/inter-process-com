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
#include <paths.h>

#define PROCESS_IS_READY 1
#define PROCESS_FAILED -1

// Struktura wiadomosci wysylanego komunikatu,
// taka jak w procesie macierzystym
struct qmessage {
	long int my_qtype;
	int stat_pid[2];
}qdata;

int q_id = -1; // identyfikator kolejki komunikatow
char *inbuf; // bufor do ktorego wczytujemy dane



void send_message_to_parent();	// wyslanie komunikatu do rodzica
void read_from_stdin();			// czytanie ze standardowego wejscia
void write_to_process2();		// zapis do procesu 2
void process1_on_exit(int);		// przy wyjsciu
void SIGUSR1_handler(int);		// przechwytywanie sygnalu SIGUSR1
void error_handler(const char*,int);		// zarzadzanie bledami




int main(void)
{	
	inbuf = (char*)malloc(1024*sizeof(char));	// zaalokowanie 1024 bajtow w buforze, do wczytania danych
	memset(inbuf, '\0', 1024);					// ustawienie wszystkich bajtow bufora na bajt zerowy (mozna tez po prostu calloc())
	// KOLEJKA KOMUNIKATOW HERE
	q_id = msgget((key_t)1234, 0666 | IPC_CREAT);	// utworzenie/polaczenie sie z kolejka komunikatow
	if(q_id == -1)
	{
		// msgget failed
		error_handler("msgget failed!", errno);
	}
	// sighandlery
	(void)signal(SIGUSR1, SIGUSR1_handler);
	(void)signal(SIGINT, process1_on_exit);
	fprintf(stderr, "%s\n", "Process 1 in main...");
	while(1) // oczekiwanie na wybudzenie przez sygnal
	{
		pause();
	}
	return 0;
}

void send_message_to_parent()
{
	fprintf(stderr, "%s\n", "Process 1 in sending message...");
	qdata.my_qtype = 1;
	int my_pid = getpid(); // pobranie pidu aktualnego procesu
	qdata.stat_pid[0] = (int)PROCESS_IS_READY; // ustawienie wartosci komunikatu na PROCESS_IS_READY
	qdata.stat_pid[1] = my_pid; // swoj pid do 2 elementu tablicy
	if(msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0) == -1) // wyslanie komunikatu
	{
		error_handler("msgsnd failed!",errno);
	}
}


void read_from_stdin() // funkcja wczytania linii do bufora ze standardowego wejscia
{
	fprintf(stderr, "%s\n", "Process 1 reading from stdin...");
	memset(inbuf, '\0', 1024); // wyzerowanie bufora
	char c; // zmienna pomocnicza do wczytywania znakow
	int i = 0; // licznik petli
	while((c = getc(stdin)) != '\n')
	{
		inbuf[i] = c;
		i++;
		if(i>1023)
		{ // przerwanie jesli wczytamy 1024 znaki
			break;
		}
	}
	if(strlen(inbuf) == 0)
	{
		read_from_stdin();
	}
	fprintf(stderr, "String writed: %s\n", inbuf);
}

void write_to_process2() // zapis danych do procesu 2
{
	fprintf(stderr, "%s\n", "Process 1 writing to process 2...");
	write(STDOUT_FILENO, inbuf, strlen(inbuf));
}

void process1_on_exit(int signo) // funkcja on_exit()
{
	fprintf(stderr, "%s\n", "SIGINT received in process 1 exiting...\n");
	free(inbuf); // zwalnianie pamieci
	exit(0);	// wyjscie
}

void SIGUSR1_handler(int signo) // Przy otrzymaniu sygnalu
{
	fprintf(stderr, "%s\n", "P1 in SIGUSR1_handler...");
	read_from_stdin();
	write_to_process2();
	send_message_to_parent();
	kill(getppid(),SIGUSR1);
}



void error_handler(const char *com,int error)
{
	fprintf(stderr, "%s %d\n", com, error);
	qdata.stat_pid[0] = (int)PROCESS_FAILED;
	qdata.stat_pid[1] = (int)getpid();
	msgsnd(q_id, (void*)&qdata, sizeof(qdata.stat_pid), 0);
	kill(getppid(),SIGUSR1);
}