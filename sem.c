#include "kernel.h"
#include "utils.h"

struct sem {
    int32_t value;
    struct tcb *sem_head;
};

void *sem_create(int32_t value)
{
    struct sem *sem;

    sem = (struct sem *)kmalloc(sizeof(struct sem));
    if(sem == NULL) {
        return NULL;
    }

    sem->value = value;
    sem->sem_head = NULL;

    return sem;
}

int sem_destroy(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    if(sem->sem_head != NULL) {
        restore_flags(flags);
        return -1;
    }

    restore_flags(flags);

    kfree(sem);
    return 0;
}

int sem_wait(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    sem->value--;
    if(sem->value < 0) {
        if(sem->sem_head == NULL)
            sem->sem_head = g_task_running;
        else {
            struct tcb *p, *q;
            p = sem->sem_head;
            do {
                q = p;
                p = p->sem_next;
            } while(p != NULL);
            q->sem_next = g_task_running;
        }
        g_task_running->sem_next = NULL;
        g_task_running->state = TASK_STATE_BLOCKED;
        schedule();
    }

    restore_flags(flags);
    return 0;
}

int sem_signal(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    sem->value++;
    if(sem->value <= 0) {
        struct tcb *tsk = sem->sem_head;
        if(tsk != NULL)
            sem->sem_head = tsk->sem_next;

        tsk->sem_next = NULL;
        tsk->state = TASK_STATE_READY;
    }

    restore_flags(flags);
    return 0;
}