#include <pthread.h> 
#include <stdio.h> 
#include <unistd.h> 
#include "AsyncProcManager.h"

int main() 
{ 
	AsyncProcManager apm;
	apm.Startup();
		
	sleep(1); 
	apm.Shutdown();

	return 0; 
} 
