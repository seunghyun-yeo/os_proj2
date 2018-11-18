/*	================== queuerear =================
Retrieves data at the rear of the queue
without changing the queue contents.
Pre    queue is pointer to initialized queue
Post   Data passed back to caller
Return true if successful; false if underflow
*/
bool queuerear(queue* pqueue, void** itemptr)
{
	//	Statements 
	if (!pqueue->count)
		return true;
	else
	{
		*itemptr = pqueue->rear->dataptr;
		return false;
	} // else 
}	// queuerear 