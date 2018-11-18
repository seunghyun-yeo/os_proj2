bool requeue(queue* pqueue){
	queuenode* tempptr;

	if(pqueue->count ==0)
		return 0;

	tempptr = pqueue->front;
	pqueue->front = pqueue->front->next;
	pqueue->rear->next = tempptr;
	pqueue->rear=pqueue->rear->next;
	pqueue->rear->next=NULL;
	
}
