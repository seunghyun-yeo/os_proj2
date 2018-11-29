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
#include <stdbool.h>


#define sec 0
#define msec 500
#define time_quantum 2
#define maxproc 10
#define maxcpuburst 4
#define gendtik 1000
#define pmemsize 0x40000000
#define kernelmemsize 0x10000000
#define l1mask 0xffc00000
#define l1shift 22
#define l2shift 12
#define l2mask 0x003ff000
#define offsetmask 0xfff
#include "queues.h"
#include "var_2level.h"

int* kernel;
int* usr;
	int ttbr;
	int offset;
	int l1index;
	int l1result;
	int l2index;
	int l2result;
	int pa;


int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	//need to make SIGALRM every 1sec

	rqueue = createqueue();
	ioqueue = createqueue();


	usr = (int*)malloc(sizeof(int)*pmemsize);
	for(int j =0; j<pmemsize;j++)
	{
		usr[j]=0;
	}

//	kernel =(int*)malloc(sizeof(int)*kernelmemsize);
//	memset(kernel,0,sizeof(int)*kernelmemsize);
	
	fusrqueue=createqueue();
	fkernelqueue=createqueue();
	
//	int cal;
	for(int j=0; j<pmemsize;j=j+1024)
	{
//		cal=j;
		if(j<kernelmemsize)
		{
			fpf=(pgfinfo*)malloc(sizeof(pgfinfo));
			fpf->pgfnum=j;
			enqueue(fkernelqueue,fpf);
		}
		else
		{
			fpf=(pgfinfo*)malloc(sizeof(pgfinfo));
			fpf->pgfnum=j;
			enqueue(fusrqueue,fpf);
		}
	}
//	printf("%x\n",(cal+1024));

	for(int i=0; i<maxproc; i++)
	{
		pid=fork();
		if(pid==0)
		{
			key=0x142735;
			msgq = msgget(key,IPC_CREAT|0666);

			srand(getpid());

			dcpu_time = rand()%6;
			dio_time =7;//rand()%6;
			cpu_time=dcpu_time;
			msg.mtype=1;
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
			dequeue(fkernelqueue,(void**)&fpf);
			pcbdata->ttbr=fpf->pgfnum;
			//printf("%08x\n",pcbdata->ttbr);
			pcbdata->tq=time_quantum;
			enqueue(rqueue,pcbdata);
		}
		else printf("fork error\n");
	}






	tiktok(sec,msec);
	key=0x142735;
	msgq = msgget(key,IPC_CREAT|0666);

	while (1)
	{
		ret=msgrcv(msgq, &msg, sizeof(msgbuf),1,IPC_NOWAIT);
		if(ret != -1)
		{
			mymovqueue(rqueue,ioqueue,msg.pid,msg.io_time);
		}

		ret=msgrcv(msgq, &memrequest,sizeof(msgbuf2),2,IPC_NOWAIT);
		while((ret!=-1)&&(!memrequest.copyend));
		if(ret != -1)
		{
			memrequest.copyend=0;
			memrequest_handler();
		}
	}
	return 0;
}

void memrequest_handler()
{

	printf("pid : %d\n",memrequest.pid);
	for(int k=0; k<10;k++)
	{
		if(memrequest.write[k]) printf("write  at : ");
		else printf("read from : ");
		printf("%08x",memrequest.va[k]);
		if(memrequest.va[k]) printf(": %d\n",memrequest.data[k]);
		else printf("\n");
	}

	queuenode * ppre=NULL;
	queuenode * ploc=NULL;
	pcb * pcbptr;
	for(ppre=NULL,ploc=rqueue->front;ploc!=NULL;ppre=ploc,ploc=ploc->next)
	{
		pcbptr=ploc->dataptr;
		if(pcbptr->pid==memrequest.pid)
		{
			break;
		}
	}
	ttbr = pcbptr->ttbr;


	for(int k=0; k<10;k++)
	{

		printf("-------------------------------------------------\n");
		offset= memrequest.va[k] & offsetmask;
		l1index = ttbr + ((memrequest.va[k]&l1mask)>>l1shift);
		//	l1index=l1index>>l1shift;
		printf("%08x\n",l1index);
		if((usr[l1index]&0b1)==0)
		{
			printf("make map between l1 and l2\n");
		//	if(emptyqueue(fkernelqueue))printf("no more res form kernel \n");
		//	else 
				dequeue(fkernelqueue,(void**)&fpf);
			usr[l1index]=fpf->pgfnum<<2;
			l1result=usr[l1index];
			usr[l1index]=usr[l1index]|0b1;
			printf("L1 index : %d : %08x\n",l1index,l1result);
			l2index=l1result+((memrequest.va[k]&l2mask)>>l2shift);
		}
		else
		{
			l1result=usr[l1index];
			l2index=l1result+((memrequest.va[k]&l2mask)>>l2shift);
		}


		if((usr[l2index]&0b1)==0)
		{
			printf("make map between l2 and page frame\n");
			//if(emptyqueue(fusrqueue))printf("no more res from usr\n");
			//else 
			dequeue(fusrqueue,(void**)&fpf);
			usr[l2index]=fpf->pgfnum<<2;
			l2result=usr[l2index];
			usr[l2index]=usr[l2index]|0b1;
			printf("L2 index : %d : 0x%08x\n",l2index,l2result);
		}
		else
		{
			l2result=usr[l2index];
			l2result=l2result&(l1mask|l2mask);
		}

		pa=l2result|offset;
		printf("va : 0x%08x  pa : 0x%08x\n",memrequest.va[k],pa);
		if(memrequest.write[k])
		{
			printf("write data : %d at 0x%08x\n",memrequest.data[k],pa>>2);
			usr[pa/4]=memrequest.data[k];
		}
		else
		{
			printf("load data : %d from 0x%08x\n",usr[pa/4],pa/4);
		}
	}


}


void child_handler(int signo)
{
	cpu_time--;
	memrequest.mtype=2;
	memrequest.pid = getpid();
	for(int k=0; k<5;k++)
	{
//		printf("%d",k);
		memrequest.va[k]=rand()%0x100000000;
		memrequest.va[k]=memrequest.va[k]&0xfffffffc;
		memrequest.va[k+5]=memrequest.va[k];
		memrequest.write[k]=1;
		memrequest.write[k+5]=0;
		memrequest.data[k]=rand();
	}
	memrequest.copyend=1;
	ret=msgsnd(msgq,&memrequest,sizeof(memrequest),NULL);
//	printf("msgsnd done");
	if(cpu_time<1)
	{
		printf("wait request");
		msg.io_time=dio_time;
		ret = msgsnd(msgq, &msg,sizeof(msg),NULL);
		cpu_time=dcpu_time;

		return;
	}

}


