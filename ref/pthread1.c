#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
/*abcd....*/
	void *print_message_functionA( void *ptr );
	void *print_message_functionB( void *ptr );
	void *print_message_functionC( void *ptr );
	void *print_message_functionD( void *ptr );
 
void pseudo_mutex_lock(int *mutex);
void pseudo_mutex_unlock(int *mutex);
	 
int CNT =0;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
 int mut1 = 0;
 int mut2 = 0;




	main()
	{
	  pthread_t thread1, thread2, thread3, thread4;
	     const char *message1 = "Thread 1";
	     const char *message2 = "Thread 2";
	     const char *message3 = "Thread 3";
	     const char *message4 = "Thread 4";
	     int  iret1, iret2, iret3, iret4;
	 
	    /* Create independent threads each of which will execute function */
	 
	     iret1 = pthread_create( &thread1, NULL, print_message_functionA, (void*) message1);
	     if(iret1)
	     {
	         fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
	         exit(EXIT_FAILURE);
	     }
	 
	     iret2 = pthread_create( &thread2, NULL, print_message_functionB, (void*) message2);
	     if(iret2)
	       {
	         fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
	         exit(EXIT_FAILURE);
	       }

	     iret3 = pthread_create( &thread3, NULL, print_message_functionC, (void*) message3);
	     if(iret3)
	       {
		 fprintf(stderr,"Error - pthread_create() return code: %d\n",iret3);
		 exit(EXIT_FAILURE);
	       }

	     iret4 = pthread_create( &thread4, NULL, print_message_functionD, (void*) message4);
	     if(iret4)
	       {
		 fprintf(stderr,"Error - pthread_create() return code: %d\n",iret4);
		 exit(EXIT_FAILURE);
	       }
	 
	     printf("pthread_create() for thread 1 returns: %d\n",iret1);
	     printf("pthread_create() for thread 2 returns: %d\n",iret2);
	     printf("pthread_create() for thread 3 returns: %d\n",iret3);
	     printf("pthread_create() for thread 4 returns: %d\n",iret4);
	 
	     /* Wait till threads are complete before main continues. Unless we  */
	     /* wait we run the risk of executing an exit which will terminate   */
	     /* the process and all threads before the threads have completed.   */
	 
	     pthread_join( thread1, NULL);
	     pthread_join( thread2, NULL);
     	     pthread_join( thread3, NULL);
     	     pthread_join( thread4, NULL);
	 
	     exit(EXIT_SUCCESS);
	}
	 
	void *print_message_functionA( void *ptr )
	{
	     char *message;
	     int i;
	     message = (char *) ptr;
	     sleep(2); //
	     
	     //pthread_mutex_lock( &mutex1 );
	     //sleep(1); //
	     //pthread_mutex_lock( &mutex2 );
	     pseudo_mutex_lock( &mut1 );

	     CNT+=4;
	     pthread_mutex_unlock( &mutex1 );
	     //for(i=CNT; i>=0; i--) {
	     for(i=0; i<=CNT; i++) {
	       printf("%s :A: %d (%d)\n", message,i,CNT);
	       sleep(2);
	     };
	     //pthread_mutex_unlock( &mutex2 );
	     //pthread_mutex_unlock( &mutex1 );
	     pseudo_mutex_unlock( &mut1 );
	}

	void *print_message_functionB( void *ptr )
	{
	     char *message;
	     int i;
	     message = (char *) ptr;
	     sleep(2); //

	     //pthread_mutex_lock( &mutex1 );
	     //	     sleep(1); //
	     //  pthread_mutex_lock( &mutex2 );
	     pseudo_mutex_lock( &mut1 );

	     CNT+=4;
	     //pthread_mutex_unlock( &mutex1 );
	     //for(i=CNT; i>=0; i--) {
	         for(i=0; i<=CNT; i++) {
	       printf("%s :B: %d (%d)\n", message,i,CNT);
	       sleep(1);
	     };
		 // pthread_mutex_unlock( &mutex2 );
		 //pthread_mutex_unlock( &mutex1 );
	     pseudo_mutex_unlock( &mut1 );
	}


	void *print_message_functionC( void *ptr )
	{
	     char *message;
	     int i;
	     message = (char *) ptr;
	     sleep(2); //


	     //pthread_mutex_lock( &mutex1 );
	     pseudo_mutex_lock( &mut1 );

	     CNT+=4;
	     //pthread_mutex_unlock( &mutex1 );
	     //for(i=CNT; i>=0; i--) {
	       for(i=0; i<=CNT; i++) {
	       printf("%s :C: %d (%d)\n", message,i,CNT);
	       sleep(1);
	     };
	       //pthread_mutex_unlock( &mutex1 );
	     pseudo_mutex_unlock( &mut1 );
 
	}




	void *print_message_functionD( void *ptr )
	{
	     char *message;
	     int i;
	     message = (char *) ptr;
	     sleep(2);  //

	     //pthread_mutex_lock( &mutex1 );
	     pseudo_mutex_lock( &mut1 );

	     CNT+=4;
	     //pthread_mutex_unlock( &mutex1 );
	     //for(i=CNT; i>=0; i--) {
	       for(i=0; i<=CNT; i++) {
	       printf("%s :D: %d (%d)\n", message,i,CNT);
	       sleep(1);
	     };
	       //pthread_mutex_unlock( &mutex1 );
	     pseudo_mutex_unlock( &mut1 );
 
	}


void pseudo_mutex_lock(int *mutex) 
     {
      struct timespec ntime1;
      struct timespec ntime2;
      ntime1.tv_sec=0;
      ntime1.tv_nsec=0;
                      nanosleep(&ntime1,&ntime2);

      while(*mutex) {
      }
      //   nanosleep(&ntime1,&ntime2);
      *mutex = 1;
}

void pseudo_mutex_unlock(int *mutex) {
  *mutex = 0;
}
