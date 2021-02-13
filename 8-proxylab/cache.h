#ifndef __CACHE_H_
#define __CACHE_H_

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define N_CACHE (MAX_CACHE_SIZE / MAX_OBJECT_SIZE)

void init_web_cache();
int process_cache(int client, const char *url);
void cache_webobj(const char *url, const char *buf, size_t total);

#endif
