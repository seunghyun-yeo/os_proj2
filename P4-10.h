/*	================== destroyqueue =================
Deletes all data from a queue and recycles its
memory, then deletes & recycles queue head pointer.
Pre    queue is a valid queue
Post   All data have been deleted and recycled
Return null pointer
*/
queue* destroyqueue(queue* pqueue)
{
	//	Local Definitions 
	queuenode* deleteptr;

	//	Statements 
	if (pqueue)
	{
		while (pqueue->front != NULL)
		{
			free(pqueue->front->dataptr);
			deleteptr = pqueue->front;
			pqueue->front = pqueue->front->next;
			free(deleteptr);
		} // while 
		free(pqueue);
	} // if 
	return NULL;
}	// destroyqueue 