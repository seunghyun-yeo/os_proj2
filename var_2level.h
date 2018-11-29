
void signal_handler(int signo);
void child_handler(int signo);
void tiktok(int, int);
void reduceall();
void searchqueue(queue* ,queuenode**,queuenode**,int);
void insertqueue(queue* ,queuenode*,queuenode*,queuenode*);
void mymovqueue(queue*,queue*,int,int);
void memrequest_handler();

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
queue* fusrqueue;
queue* fkernelqueue;

typedef struct{
	long mtype;
	pid_t pid;
	int io_time;
} msgbuf;

typedef struct{
	long mtype;
	pid_t pid;
	int va[10];
	bool write[10];
	int data[10];
	bool copyend;
} msgbuf2;

typedef struct{
	pid_t pid;
	int io_time;
	int cpu_time;
	int tq;
	int ttbr;
} pcb;

typedef struct{
	int pgfnum;
} pgfinfo;

msgbuf msg;
msgbuf2 memrequest;
pcb* pcbdata;
pgfinfo* fpf;




void tiktok(int a, int b)
{
	new_itimer.it_interval.tv_sec = a;
	new_itimer.it_interval.tv_usec = b;
	new_itimer.it_value.tv_sec = a;
	new_itimer.it_value.tv_usec = b;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
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


void signal_handler(int signo)
{
	globaltik++;


	pcb *pcbptr = NULL;
	queuenode * ppre;
	//endtik=gendtik

	printf("================================================\nat %d\n\n",globaltik);
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

	if(globaltik==gendtik)
	{
		//printf("in\n");
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
	//	printf("in");
		while(1)
		{
			if(emptyqueue(ioqueue)) break;

			queuefront(ioqueue,(void**)&pcbptr);

			if(pcbptr->io_time<0)
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

