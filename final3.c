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
#define endtik 1000
#include "var.h"

#define sec 0
#define msec 500
#define maxproc 10
#define PA_SIZE 0x40000000

int* PA;
int* table;
int ret2;
int VA = 0;
void insertfreequeue(queue*, int*);
void findttbr(queue*, pid_t, int*);
queue* freequeue;

int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	rqueue = createqueue();
	ioqueue = createqueue();
	freequeue = createqueue();
	
	PA = (int*)malloc(sizeof(int)*PA_SIZE);//pa is 4GB, entry is 1G
	table = (int*)malloc(sizeof(int)*0x00A00000);//table is 64MB, entry is 16M
	tlb = (transbuf*)malloc(sizeof(transbuf)*100);
	memset(PA, 0, sizeof(int)*PA_SIZE);
	memset(table, 0, sizeof(int)*0x00A00000);

	for(int i=0; i<100; i++){
		tlb[i].page_index = 0;
		tlb[i].page_fn = 0;
		tlb[i].sc = 0;
	}	

	int* frame;
	frame = (int*)malloc(sizeof(int)); 
	insertfreequeue(freequeue, frame);

	int ttbr_autoincrement=0;
	int ttbr2_autoincrement=0;

	for(int i=0; i<maxproc; i++)
	{
		pid=fork();
		if(pid==0)
		{
			key=0x32144055;
			msgq = msgget(key,IPC_CREAT|0666);

			srand(getpid());

			dcpu_time = rand()%6;
			dio_time = 7; //rand() %6;
			cpu_time=dcpu_time;
			msg.mtype=1;
			msg.pid=getpid();
			printf("pid : %d\n",msg.pid);
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
			pcbdata->ttbr=ttbr_autoincrement;
			pcbdata->ttbr2=ttbr2_aoutoincrement;
			ttbr_autoincrement += 0x100000;
			ttbr2_autoincrement += 10;
			pcbdata->tq=time_quantum;
			enqueue(rqueue,pcbdata);
		}
		else printf("fork error\n");
	}

	tiktok(sec,msec);

	key=0x32144055;
	msgq = msgget(key,IPC_CREAT|0666);

	while (1)
	{
		ret = msgrcv(msgq, &msg, sizeof(msgbuf), 1, IPC_NOWAIT);
		if(ret != -1)
			mymovqueue(rqueue,ioqueue,msg.pid,msg.io_time);

		ret2 = msgrcv(msgq, &raw_page, sizeof(msgbuf2), 2, IPC_NOWAIT);
		if(ret2 != -1){
			int want;
			findttbr(rqueue, raw_page.pid, &want);
			int ttbr = want;

			for(int i=0; i<10; i++){
				int offset;
				int pageidx;
				int real_addr;
				int val;
				int tlb_pointer;
				tlb_pointer = (int*)malloc(sizeof(int))
				
				pageidx = ((raw_page.va[i]&0xFFFFF000)>>12) + ttbr;
				printf("pageidx : 0x%08X ttbr : %X\n",pageidx, ttbr);
				offset = raw_page.va[i] & 0x00000FFF;
				
				if(traversetlb(raw_page.va[i], ttbr2, tlb_pointer)){
					table[pageidx] = tlb[tlb_pointer].page_fn;
					tlb[tlb_pointer].sc = 1;
					printf("TLB Hit!!\n");
					printf("VA : 0x%08X is mapped into page table PA : 0x%08X\n",raw_page.va[i], table[pageidx]);
				}
				else{
					if((table[pageidx]&0xFFF) == 0x0){
						dequeue(freequeue, (void**)&frame);
						table[pageidx] = *frame; //mapping
						printf("VA : 0x%08X is mapped into page table PA : 0x%08X\n",raw_page.va[i], table[pageidx]);
						table[pageidx] = table[pageidx] | 1;

						//function puts mapped one into tlb
					}//not mapped
					else{
						//function find the mapped one and put into tlb
					}
				}//tlb MISS
					real_addr = table[pageidx] | offset;
					if(raw_page.read[i] == 0){//write the data
						PA[real_addr] = raw_page.data[i];
						printf("write :%05d into 0x%08X\n",raw_page.data[i], real_addr);
					}
					if(raw_page.read[i] == 1){//read the data
						val = PA[real_addr];
						printf("read : %05d from 0x%08X\n", PA[real_addr], real_addr );
					}
				
			}//for
		}//if
	}//while
	return 0;
}//main

void child_handler(int signo)
{
	cpu_time--;
	raw_page.mtype=2;
	raw_page.pid=getpid();

	for(int i=0; i<5; i++)
	{
		raw_page.va[i] = rand() % 0x100000000;
		raw_page.va[i+5] = raw_page.va[i];
		raw_page.read[i] = 0;
		raw_page.data[i] = rand() % 100;
		raw_page.read[i+5] = 1;
		//raw_page.data[i+5] = 0;
		
	}
	ret2 = msgsnd(msgq, &raw_page, sizeof(raw_page), NULL);


	if(cpu_time<1)
	{
		msg.io_time=dio_time;
		ret = msgsnd(msgq, &msg,sizeof(msg),NULL);
		cpu_time=dcpu_time;

		return;
	}

}

void insertfreequeue(queue* targetqueue, int* frame)
{
	int frame_end = PA_SIZE & 0xFFFFF000 >> 12;
	int flag = 0x0;
	
	for(int pframe=0x0; pframe<0x40000; pframe++)
	{
		frame = (int*)malloc(sizeof(int));
		*frame = ((pframe << 12) | flag);
		enqueue(targetqueue, frame);	
	}
}

void findttbr(queue* targetqueue, pid_t pid, int* want){
 	pcb* pcbptr;
	queuenode *ppre;
	queuenode *ploc;

        for(ppre=NULL,ploc=targetqueue->front; ploc!=NULL; ppre=ploc,ploc=ploc->next)
        {
                pcbptr = ploc->dataptr;
                if(pcbptr->pid == pid){
                        *want = pcbptr->ttbr;
			break;
		}
        }
}

bool traversetlb(int pagenum, int ttbr, int i){

	for(i=0; i<10; i++){
		if(tlb[i+ttbr].page_index == pagenum)
			return true;
		else
			return false;
	}

}
