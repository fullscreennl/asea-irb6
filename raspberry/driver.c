#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wiringPi.h>
#include <boilerplate/ancillaries.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>

#define CLOCK 0

RT_TASK sync_task;


void catch_signal(int sig){
}


void sync(){
   while(1){
       rt_task_wait_period(NULL);
       digitalWrite (CLOCK, HIGH);
       rt_task_sleep(500000000);
       digitalWrite (CLOCK, LOW);
       rt_task_set_periodic(&sync_task, TM_NOW, 1000000000);
   }
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    wiringPiSetup();
    pinMode (CLOCK, OUTPUT);
    rt_task_set_periodic(&sync_task, TM_NOW, 1000000);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task,sync, NULL);
    pause();
    rt_task_delete(&sync_task);
    return 0;
}
