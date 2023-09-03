#ifndef __SEM_DEBUG__
#define __SEM_DEBUG__

#define sem_post(sem) print_debug("sem post %s:%d\n", __FILE__, __LINE__); sem_post(sem);
#define sem_wait(sem) print_debug("sem wait %s:%d\n", __FILE__, __LINE__); sem_wait(sem);

#endif
