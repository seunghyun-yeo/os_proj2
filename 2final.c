/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h> 
#include <time.h>

#include "queues.h"

#define time_quantum 2
#define maxproc 3
#define maxcpuburst 4

void signal_handler(int signo);
void child_handler(int signo);
void tiktok(int, int);
void reduceall();
void mymovqueue(queue*,queue*,int,int);

pid_t pid;
int tq;
int dio_time;
int dcpu_time;
int cpu_time;
int io_time;
int ret;
int key;
int globaltik=0;

int msgq;
struct sigaction old_sa;
struct sigaction new_sa;
struct itimerval new_itimer, old_itimer;

queue* rqueue;
queue* ioqueue;

typedef struct{
	int mtype;
	pid_t pid;
	int io_time;
} msgbuf;

typedef struct{
	pid_t pid;
	int io_time;
	int cpu_time;
	int tq;
} pcb;

msgbuf msg;
pcb* pcbdata;
int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	//need to make SIGALRM every 1sec

	rqueue = createqueue();
	ioqueue = createqueue();

	for(int i=0; i<maxproc; i++)
	{
		pid=fork();
		if(pid==0)
		{
			key=0x142735;
			msgq = msgget(key,IPC_CREAT|0666);

			srand(time(NULL));

			dcpu_time = 6;
			dio_time =6;
			cpu_time=dcpu_time;
			msg.mtype=0;
			msg.pid=getpid();
			msg.io_time=dio_time;
			new_sa.sa_handler = &child_handler;
			sigaction(SIGUSR1,&new_sa,&old_sa);
			while(1);
		}
		else if(pid>0)
		{
			pcbdata = (pcb*)malloc(sizeof(pcb));
			pcbdata->pid=pid;
			pcbdata->io_time=-1;
			pcbdata->cpu_time=0;
			pcbdata->tq=time_quantum;
			enqueue(rqueue,pcbdata);
		}
		else printf("fork error\n");
	}

	tiktok(1,0);

	key=0x142735;
	msgq = msgget(key,IPC_CREAT|0666);

	while (1)
	{
		while((ret=msgrcv(msgq, &msg, sizeof(msgbuf),0,IPC_NOWAIT))==-1);
		mymovqueue(rqueue,ioqueue,msg.pid,msg.io_time);
	}
	return 0;
}

void signal_handler(int signo)
{
	globaltik++;

	pcb *pcbptr = NULL;
	printf("------------------------------------------\nat %d\n\n",globaltik);
	queuenode * ppre;
	printf("ready queue : ");
	for(ppre=rqueue->front;ppre!=NULL;ppre=ppre->next){
		pcbptr=ppre->dataptr;
		printf("%d\t",pcbptr->pid);
	}
	printf("\n\t");
	for(ppre=rqueue->front;ppre!=NULL;ppre=ppre->next){
		pcbptr=ppre->dataptr;
		printf("\t%d",pcbptr->tq);
	}
	printf("\n");
	printf("io queue :    ");
	for(ppre=ioqueue->front;ppre!=NULL;ppre=ppre->next){
		pcbptr=ppre->dataptr;
		printf("%d\t",pcbptr->pid);
	}
	printf("\n\t");

	for(ppre=ioqueue->front;ppre!=NULL;ppre=ppre->next){
		pcbptr=ppre->dataptr;
		printf("\t%d",pcbptr->io_time);
	}
	printf("\n");

	if(globaltik==30)
	{
		while(!emptyqueue(rqueue))
		{
			dequeue(rqueue,(void**)&pcbptr);
			kill(pcbptr->pid,SIGKILL);
			free(pcbptr);
		}

		while(!emptyqueue(ioqueue))
		{
			dequeue(ioqueue,(void**)&pcbptr);
			kill(pcbptr->pid,SIGKILL);
			free(pcbptr);
		}
		exit(0);
	}

	if(!emptyqueue(ioqueue))
	{
		reduceall();
		printf("in");
		while(1)
		{
			if(emptyqueue(ioqueue)) break;

			queuefront(ioqueue,(void**)&pcbptr);

			if(pcbptr->io_time==0)
			{
				ppre=ioqueue->front;
				ioqueue->front=ppre->next;
				ppre->next=NULL;
				ioqueue->count--;
				if(rqueue->front==NULL)
				{
					rqueue->front=ppre;
					rqueue->rear=ppre;
					rqueue->count++;
				}
				else
				{
					rqueue->rear->next=ppre;
					rqueue->rear = ppre;
					rqueue->count++;
				}
			}
			else
			{
				break;
			}
		}
	}

	if(!emptyqueue(rqueue))
	{
		queuefront(rqueue,(void**)&pcbptr);
		pcbptr->tq--;
		pcbptr->cpu_time++;
		kill(pcbptr->pid,SIGUSR1);
		if(pcbptr->tq==0)
		{
			pcbptr->tq=time_quantum;
			if(queuecount(rqueue)>1)requeue(rqueue);
		}


	}
}

void searchqueue(queue* targetqueue, queuenode **ppre ,queuenode **pploc, int iotime)
{
	pcb * pcbptr;
	for(*ppre=NULL,*pploc=targetqueue->front;*pploc!=NULL;*ppre=*pploc,*pploc=(*pploc)->next)
	{
		pcbptr=(*pploc)->dataptr;
		if(pcbptr->io_time>iotime)
			break;
	}
}

void insertqueue(queue* targetqueue, queuenode *ppre, queuenode *ploc, queuenode* pploc)
{
	if(ppre==NULL)//ploc is the first
	{
		if(!emptyqueue(targetqueue)){
			ploc->next = targetqueue->front;
			targetqueue->front=ploc;
		}
		else{
			targetqueue->front=ploc;
			targetqueue->rear=ploc;
		}
	}
	else
	{	
		if(pploc == NULL){//ploc is the end
			ppre->next = ploc;
			targetqueue->rear = ploc;
			targetqueue->count++;
			return;
		}
		ploc->next=ppre->next;
		ppre->next= ploc;
	}
	targetqueue->count++;
}

void mymovqueue(queue* sourceq, queue* destq, int pid, int iotime)
{
	queuenode *ppre=NULL;
	queuenode *ploc=NULL;
	queuenode *pploc= NULL;
	pcb* pcbptr;
	for(ppre=NULL,ploc=sourceq->front; ploc!=NULL;ppre=ploc,ploc=ploc->next){
		pcbptr = ploc->dataptr;

		if(pcbptr->pid == pid){
			if(ppre != NULL)
				ppre->next = ploc->next;
			else if(ppre ==NULL)
				sourceq->front = ploc->next;

			if(ploc->next==NULL)
			{
				sourceq->rear=ppre;
			}

			ploc->next = NULL;
			sourceq->count--;
			pcbptr->io_time = iotime;
			break;
		}
	}
	searchqueue(destq,&ppre,&pploc,iotime);
	insertqueue(destq,ppre,ploc,pploc);
}
void child_handler(int signo)
{
	cpu_time--;
	if(cpu_time<1)
	{
		msg.io_time=dio_time;
		ret = msgsnd(msgq, &msg,sizeof(msg),NULL);
		cpu_time=dcpu_time;

		return;
	}

}
void reduceall()
{
	pcb* pcbptr;
	queuenode *traverse;
	for(traverse = ioqueue->front;traverse!=NULL;traverse=traverse->next)
	{
		pcbptr=traverse->dataptr;
		pcbptr->io_time--;
	}
}
void tiktok(int a, int b)
{
	new_itimer.it_interval.tv_sec = a;
	new_itimer.it_interval.tv_usec = b;
	new_itimer.it_value.tv_sec = a;
	new_itimer.it_value.tv_usec = b;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}
