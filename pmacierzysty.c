#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <signal.h>
#include <paths.h>

#define PROCESS_IS_READY 1  // Predefiniowana wartosc oznaczajaca gotowosc (wykonanie zadania) procesu
#define PROCESS_FAILED -1	// // Jak wyzej dla bledu zwroconego przez proces

int q_id = -1;	// Identyfikator kolejki komunikatów
int p1pid = -1; // Pid procesu 1
int p2pid = -1; // Pid procesu 2
int p3pid = -1; // Pid procesu 3

// Struktura wiadomosci wysylanego komunikatu
struct qmessage {
	long int my_qtype; // Typ komunikatu (obowiazkowe pole)
	int stat_pid[2];	// Przeslany status w polu 0, oraz pid procesu w polu 2
}qdata;

// Prototypy funkcji
void my_exit(int); // funckja wywolana przy wyjsciu
void error_handler(int); // zarzadzanie bledami
void read_msg_q(); // wczytanie wiadomosci z kolejki komunikatow
void SIGUSR1_handler(int); // akcje do podjecia po otrzymaniu sygnalu

int main(void) 
{
	//pid_t p1pid, p2pid, p3pid; // pid potomkow
	int p12fd[2];	// deskryptory plikow dla pipe
	int p23fd[2];	// deskryptory plikow dla pipe
	// pipe here	
	pipe(p12fd);	// potok proces1 --> process2
	pipe(p23fd);	// potok proces2 --> process3
	p1pid = fork();
	switch(p1pid)
	{
		case -1:
			// rozwidlenie nie powiodlo sie
			error_handler(errno);
			break;

		case 0: // Proces1 (potomny)
		{
			// zamykamy niepotrzebne deskryptory potokow
			close(p23fd[0]);
			close(p23fd[1]);
			close(p12fd[0]);
			dup2(p12fd[1], 1); // przekierowujemy stdout procesu 1 do potoku p12fd (stdin dla p2)
			// Uruchamiamy process 1
			execl("./process1", "process1", NULL);
			break;
		}
		default: // Proces macierzysty
		{
			p2pid = fork();
			switch(p2pid)
			{
				case -1:
					// rozwidlenie nie powiodlo sie
					error_handler(errno);
					break;

				case 0: // Proces2 (potomny)
				{
					// zamykamy niepotrzebne deskryptory potokow
					close(p23fd[0]);
					close(p12fd[1]);
					// przekierowyjemy stdout do potoku p23fd (stdin dla p3)
					dup2(p23fd[1], 1);
					// przekierowujemy potok p12fd na stdin (pobieranie od p1)
					dup2(p12fd[0], 0);
					// uruchomienie procesu 2
					execl("./process2", "process2", NULL);
					break;
				}
				default: // Proces macierzysty
				{
					p3pid = fork();
					switch(p3pid)
					{
						case -1:
							// rozwidlenie nie powiodlo sie
							error_handler(errno);
							break;

						case 0: // Process 3 (potomny)
						{
							// zamykamy niepotrzebne deskryptory potokow
							close(p12fd[0]);
							close(p12fd[1]);
							close(p23fd[1]);
							// przekierowujemy p23fd na stdin (pobieranie od p2)
							dup2(p23fd[0], 0);
							// uruchomienie procesu
							execl("./process3", "process3", NULL);
							break;
						}
						default: // Proces macierzysty (final)
						{
							// zamykamy potoki
							close(p12fd[0]);
							close(p12fd[1]);
							close(p23fd[0]);
							close(p23fd[1]);
							// sighandlers
							(void) signal(SIGUSR1, SIGUSR1_handler);
							(void) signal(SIGINT, my_exit); // wyjscie przy otrzymaniu SIGINT
							// stworzenie kolejki komunikatow
							q_id =  msgget((key_t)1234, 0666 | IPC_CREAT); // stworzenie kolejki komunikatow
							if(q_id == -1)
							{
								// utworzenie kolejki komunikatow niepowiodlo sie
								error_handler(errno);
							}
							sleep(1); // spimy 1s aby pozostale procesu zdazyly sie uruchomic (mozna to zrobic lepiej)
							kill(p1pid, SIGUSR1); // wyslanie sygnalu do procesu 1 aby wczytał dane							
							while(1)
							{
								pause(); // czekamy na wybudzenie przez sygnał
							}
							break;
						}
					}
					break;
				}
			}
		}
	}
	return 0; // non reached :)
}

// FUNCKJE PROCESU MACIERZYSTEGO;


// Funkcja wywolywana przy wyjsciu
void my_exit(int stat)
{
	fprintf(stderr, "%s\n", "My exit in main process...");
	// Zabicie potomkow
	// wyskanie kolejno sygnalu SIGINT do kazdego z nich
	kill(p1pid,SIGINT);
	kill(p2pid,SIGINT);
	kill(p3pid,SIGINT);
	// Usuniecie kolejki komunikatow
	msgctl(q_id, IPC_RMID, 0);
	exit(stat);
}

// funkcja obslugi bledow, wypisuje numer bledu i konczy dzialanie wszystkich procesow
void error_handler(int error)
{
	fprintf(stderr, "%s %d\n", "Error occured!", error);
	my_exit(error);
}

void read_msg_q()
{
	fprintf(stderr, "%s\n", "Main process in read_msg_q..."); // Do sledzenie gdzie sie aktualnie znajdujemy
	long int kom_do_odbioru = 0;
	msgrcv(q_id, (void*)&qdata,sizeof(qdata.stat_pid),kom_do_odbioru, 0); // odebranie wiadomosci z kolejki komunikatow
	//fprintf(stderr, "stat_pid[0] = %d\n stat_pid[1] = %d\n", qdata.stat_pid[0], qdata.stat_pid[1]); // wyswietlenie odczytanych danych
	if(qdata.stat_pid[0] == PROCESS_IS_READY) // Jesli odebralismy wiadomosc o gotowosci procesu
	{
		if(qdata.stat_pid[1] == p1pid) // z procesu 1
		{
			kill(p2pid, SIGUSR1); // wysylamy sygnal do procesu 2 aby wykonal swoje zadanie
		} else if (qdata.stat_pid[1] == p2pid) // z procesu 2
		{
			kill(p3pid, SIGUSR1); // wysylamy sygnal do procesu 3 aby wykonal swoje zadanie
		} else if (qdata.stat_pid[1] == p3pid) // z procesu 3
		{
			kill(p1pid, SIGUSR1); // oznacza to ze proces 3 wykonal swoje zadanie, proces 1 znowu moze pobierac dane
		}
	} else {
		error_handler(PROCESS_FAILED); // Odczytano inna wartosc => wystapil blad
	}
}


void SIGUSR1_handler(int sig)	// Odebranie sygnalu SIGUSR1, nalezy odczytac wiadomosc z kolejki komunikatow
{
	fprintf(stderr, "%s\n", "Main process received SIGUSR1...");
	read_msg_q();
}
