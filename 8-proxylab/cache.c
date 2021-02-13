#include "csapp.h"
#include "cache.h"

struct web_obj {
    int active;
    unsigned long obj_size;
    pthread_rwlock_t lock;
    char url[64];
};

static struct web_obj wobj[N_CACHE];

static char *web_cache = NULL;

void init_web_cache()
{
    for (int i = 0; i < N_CACHE; i++) {
        wobj[i].active = -1;
        wobj[i].obj_size = 0;
        memset(wobj[i].url, 0, sizeof(wobj[i].url));
        
        pthread_rwlock_init(&wobj[i].lock, NULL);
    }
    
    web_cache = (char *)malloc(MAX_CACHE_SIZE);
}

/*
 * cache the web object. LRU
 */
void cache_webobj(const char *url, const char *buf, size_t total)
{
    int index = 0, flag = 0;
    for (int i = 0; i < N_CACHE; i++) {
        pthread_rwlock_rdlock(&wobj[i].lock);
        
        if (wobj[i].active != -1 && strcmp(wobj[i].url, url) == 0) {
            index = i;
            flag = 1;
            pthread_rwlock_unlock(&wobj[i].lock);
            break;
        }

        if (wobj[index].active > wobj[i].active) index = i;

        pthread_rwlock_unlock(&wobj[i].lock);
    }

    pthread_rwlock_wrlock(&wobj[index].lock);

    if (flag){
        wobj[index].active++;
    }
    else {
        wobj[index].active = 0;
        strncpy(wobj[index].url, url, sizeof(wobj[index].url));
    }

    wobj[index].obj_size = total;
    memcpy(web_cache + index * MAX_OBJECT_SIZE, buf, total);

    pthread_rwlock_unlock(&wobj[index].lock);
}

/*
 * if already cached the web object which client want to get,
 * then immediatly send it to client
 */
int process_cache(int client, const char *url)
{
    int ret = 0;
    
    for (int i = 0; i < N_CACHE; i++) {
        pthread_rwlock_rdlock(&wobj[i].lock);
        if (wobj[i].active != -1 && strcmp(wobj[i].url, url) == 0) {
            char *buf = web_cache + i * MAX_OBJECT_SIZE;
            
            wobj[i].active++;   // increase active count

            Rio_writen(client, buf, wobj[i].obj_size);   // reply client

            ret = 1;
            pthread_rwlock_unlock(&wobj[i].lock);
            break;
        }
        pthread_rwlock_unlock(&wobj[i].lock);
    }

    return ret;
}

