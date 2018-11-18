/*	================== queuefront =================
This algorithm retrieves data at front of the queue
queue without changing the queue contents.
Pre    queue is pointer to an initialized queue
Post   itemptr passed back to caller
Return true if successful; false if underflow
*/
bool queuefront(queue* pqueue, void** itemptr)
{
	//	Statements 
	if (!pqueue->count)
		return false;
	else
	{
		*itemptr = pqueue->front->dataptr;
		return true;
	} // else 
}	// queuefront 