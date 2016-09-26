#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include IMPL

#define DICT_FILE "./dictionary/words.txt"
#define NUM_OF_THREADS 5

FILE *fp;
char *not_read_end = "";

struct thread_data {
    entry *pHead;
    entry *e;
};

pthread_mutex_t current_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t finish_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t finish_cond = PTHREAD_COND_INITIALIZER;
int finished;

entry *creat_entry()
{
    entry *e;
    e = (entry *) malloc(sizeof(entry));
    e->pNext = NULL;

    return e;
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

void *read_line(void *args)
{
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];

    struct thread_data *data = (struct thread_data *) args;

    while (1) {
        pthread_mutex_lock(&current_mutex);
        not_read_end = fgets(line, sizeof(line), fp);
        pthread_mutex_unlock(&current_mutex);

        if (not_read_end == NULL)
            break;

        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;

        data->e = append(line, data->e);
    }

    pthread_mutex_lock(&finish_mutex);
    finished++;
    pthread_cond_signal(&finish_cond);
    pthread_mutex_unlock(&finish_mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
    struct timespec start, end;
    double cpu_time1, cpu_time2;

    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (fp == NULL) {
        printf("cannot open the file\n");
        return -1;
    }

    /* build the entry */
    entry *pHead, *e;
    pHead = (entry *) malloc(sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;
    e->pNext = NULL;

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    clock_gettime(CLOCK_REALTIME, &start);

    /* starting to append */
#if defined(OPT)
    finished = 0;
    pthread_t threads[NUM_OF_THREADS];
    struct thread_data thread_data[NUM_OF_THREADS];

    for (i = 0; i < NUM_OF_THREADS; i++) {
        thread_data[i].pHead = creat_entry();
        thread_data[i].e = thread_data[i].pHead;

        pthread_create(&threads[i], NULL, read_line, (void *)&thread_data[i]);
    }

    while (not_read_end) {
        pthread_mutex_lock(&finish_mutex);
        while (finished < NUM_OF_THREADS) {
            pthread_cond_wait(&finish_cond, &finish_mutex);
        }
        pthread_mutex_unlock(&finish_mutex);
    }

    for (i = 0; i < NUM_OF_THREADS; i++) {
        pthread_cancel(threads[i]);
    }

    for (i = 0; i < NUM_OF_THREADS - 1; i++) {
        thread_data[i].e->pNext = thread_data[i + 1].pHead;
    }
    pHead = thread_data[0].pHead;
#else
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }
#endif

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

    /* close file as soon as possible */
    fclose(fp);

    e = pHead;

    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";
    e = pHead;

    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
    findName(input, e);
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(OPT)
    output = fopen("opt.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

    if (pHead->pNext) free(pHead->pNext);
    free(pHead);

    return 0;
}
