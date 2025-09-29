// pmergesort_ms.c
// gcc -O3 -pthread pmergesort_ms.c -o pmergesort_ms
// ./pmergesort_ms <num_elements> <num_threads>

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef INSERTION_CUTOFF
#define INSERTION_CUTOFF 64
#endif

// ----------- token pool for limiting threads -----------
static int tokens = 0;
static pthread_mutex_t tokens_mtx = PTHREAD_MUTEX_INITIALIZER;
static int try_acquire_token(void){int ok=0;pthread_mutex_lock(&tokens_mtx);if(tokens>0){tokens--;ok=1;}pthread_mutex_unlock(&tokens_mtx);return ok;}
static void release_token(void){pthread_mutex_lock(&tokens_mtx);tokens++;pthread_mutex_unlock(&tokens_mtx);}

// ----------- timing helper in ms -----------
static inline double now_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec*1000.0 + ts.tv_nsec/1e6;
}

// ----------- sorting helpers -----------
static void insertion_sort(int *a,int l,int r){
    for(int i=l+1;i<=r;i++){
        int key=a[i],j=i-1;
        // Unroll the inner while loop by 4
        while(j>=l+3 && a[j]>key && a[j-1]>key && a[j-2]>key && a[j-3]>key){
            a[j+1]=a[j];a[j]=a[j-1];a[j-1]=a[j-2];a[j-2]=a[j-3];
            j-=4;
        }
        // Handle remaining elements
        while(j>=l && a[j]>key){a[j+1]=a[j];j--;}
        a[j+1]=key;
    }
}
static void merge(int *a,int *tmp,int l,int m,int r){
    int i=l,j=m+1,k=l;
    
    // Unroll the main merge loop by 4
    while(i<=m-3 && j<=r-3){
        // Process 4 elements at a time
        if(a[i]<=a[j]){tmp[k++]=a[i++];}else{tmp[k++]=a[j++];}
        if(a[i]<=a[j]){tmp[k++]=a[i++];}else{tmp[k++]=a[j++];}
        if(a[i]<=a[j]){tmp[k++]=a[i++];}else{tmp[k++]=a[j++];}
        if(a[i]<=a[j]){tmp[k++]=a[i++];}else{tmp[k++]=a[j++];}
    }
    
    // Handle remaining elements in main merge
    while(i<=m && j<=r){tmp[k++]= (a[i]<=a[j]? a[i++]:a[j++]);}
    
    // Unroll the remaining copy loops
    while(i<=m-3){tmp[k++]=a[i++];tmp[k++]=a[i++];tmp[k++]=a[i++];tmp[k++]=a[i++];}
    while(i<=m) tmp[k++]=a[i++];
    
    while(j<=r-3){tmp[k++]=a[j++];tmp[k++]=a[j++];tmp[k++]=a[j++];tmp[k++]=a[j++];}
    while(j<=r) tmp[k++]=a[j++];
    
    memcpy(a+l,tmp+l,(r-l+1)*sizeof(int));
}

// ----------- threaded mergesort -----------
typedef struct{int *a,*tmp,l,r;}SortArgs;
static void*msort_worker(void*arg);
static void msort_seq(int*a,int*tmp,int l,int r){
    if(r-l+1<=INSERTION_CUTOFF){insertion_sort(a,l,r);return;}
    int m=l+(r-l)/2;
    msort_seq(a,tmp,l,m);
    msort_seq(a,tmp,m+1,r);
    merge(a,tmp,l,m,r);
}
static void*msort_worker(void*arg){
    SortArgs*sa=(SortArgs*)arg;int *a=sa->a,*tmp=sa->tmp,l=sa->l,r=sa->r;
    if(r-l+1<=INSERTION_CUTOFF){insertion_sort(a,l,r);return NULL;}
    int m=l+(r-l)/2;
    pthread_t tid;int spawned=0;
    SortArgs left={a,tmp,l,m},right={a,tmp,m+1,r};
    if(try_acquire_token()){spawned=!pthread_create(&tid,NULL,msort_worker,&left);}
    if(!spawned){msort_worker(&left);}
    msort_worker(&right);
    if(spawned){pthread_join(tid,NULL);release_token();}
    merge(a,tmp,l,m,r);
    return NULL;
}

// ----------- random fill with rand_r -----------
typedef struct{int *a;size_t s,e;unsigned seed;}FillArgs;
static void*fill_worker(void*arg){
    FillArgs*f=(FillArgs*)arg;unsigned s=f->seed;
    size_t i=f->s;
    // Unroll the fill loop by 4
    for(;i+3<f->e;i+=4){
        f->a[i]=rand_r(&s)&0x7fffffff;
        f->a[i+1]=rand_r(&s)&0x7fffffff;
        f->a[i+2]=rand_r(&s)&0x7fffffff;
        f->a[i+3]=rand_r(&s)&0x7fffffff;
    }
    // Handle remaining elements
    for(;i<f->e;i++)f->a[i]=rand_r(&s)&0x7fffffff;
    return NULL;
}

// ----------- check sorted -----------
static int is_sorted(int*a,size_t n){for(size_t i=1;i<n;i++)if(a[i-1]>a[i])return 0;return 1;}

int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: %s <num_elements> <num_threads>\n",argv[0]);return 1;}
    size_t N=atoll(argv[1]);int T=atoi(argv[2]);
    int *a=malloc(N*sizeof(int)),*tmp=malloc(N*sizeof(int));
    if(!a||!tmp){fprintf(stderr,"malloc fail\n");return 1;}

    // parallel fill
    int F=T; if(F>(int)N)F=(int)N;
    pthread_t*tids=malloc(F*sizeof(pthread_t));FillArgs*fa=malloc(F*sizeof(FillArgs));
    double t0=now_ms();
    size_t chunk=(N+F-1)/F;
    for(int i=0;i<F;i++){size_t s=i*chunk,e=s+chunk;if(e>N)e=N;fa[i]=(FillArgs){a,s,e,(unsigned)time(NULL)^0x9e3779b9u*(i+1)};
        pthread_create(&tids[i],NULL,fill_worker,&fa[i]);}
    for(int i=0;i<F;i++)pthread_join(tids[i],NULL);
    double t1=now_ms();

    tokens = (T>1)?T-1:0;
    SortArgs root={a,tmp,0,(int)N-1};
    msort_worker(&root);
    double t2=now_ms();

    printf("Fill:   %.3f ms\nSort:   %.3f ms\nTotal:  %.3f ms  |  %s\n",t1-t0,t2-t1,t2-t0,is_sorted(a,N)?"OK":"FAIL");

    free(a);free(tmp);free(tids);free(fa);
    return 0;
}
