#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>

RT_TASK sync_task;

void sync(){
   rt_task_wait_period(NULL);
   printf("clock high\n") 
   rt_task_sleep(100);
   printf("clock low\n") 
   rt_task_set_periodic(&sync_task, TM_NOW, 1000000);
}

int main(int argc, char* argv[])
{
    int err = 0;
    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    rt_task_set_periodic(&sync_task, TM_NOW, 1000000);
    rt_task_create(&sync_task, "sync", 0, 99, 0);
    rt_task_start(&sync_task, &sync, NULL);
    pause();
    rt_task_delete(&sync_task);
    return 0;
}
