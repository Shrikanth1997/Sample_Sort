#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

int cmp (const void *x, const void *y) {
   return ( *(int*)x - *(int*)y );
}

void
qsort_floats(floats* xs)
{
    // TODO: man 3 qsort ?
    qsort(xs->data,xs->size,sizeof(int),cmp);
}

floats*
sample(float* data, long size, int P)
{
    // TODO: Randomly sample 3*(P-1) items from the input data.
    int i,j;
    int sample_size = 3 * (P-1);
    floats* rand_items;
    floats* samples;
    
    //Get the 3*P-1 items into an array and sort it
    rand_items = make_floats(0);
    for(i=0;i<sample_size;i++,data++){
	floats_push(rand_items,*(data));
    }
    qsort_floats(rand_items);
    floats_print(rand_items);

    //Get the samples of this array by taking the median of this array
    samples = make_floats(0);
    floats_push(samples,0);
    for(j=1;j<rand_items->size;j+=3){
	floats_push(samples, *(rand_items->data + j));
    }
    floats_push(samples,INT_MAX);
    floats_print(samples);
    
    return (samples);
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    
    //floats* xs = malloc(sizeof(floats));
    floats* xs;
    xs = make_floats(0);
    long i=0;
    // TODO: Copy the data for our partition into a locally allocated array.
    for(i=0;i<size;i++){
    	if(data[i] > samps->data[pnum] && data[i]<samps->data[pnum+1]){
		floats_push(xs,data[i]);
	}
        else if(data[i] == samps->data[pnum+1])
		floats_push(xs,data[i]);
    }
    printf("%d: start %.04f, count %ld\n", pnum, samps->data[pnum], xs->size);
    // TODO: Sort the local array.
    qsort_floats(xs);
    floats_print(xs);
    sizes[pnum] = xs->size;
    
    for(i=0;i<P;i++)
	printf("sizes: %ld ",sizes[i]);
    printf("\n");
    // TODO: Using the shared sizes array, determine where the local
    // output goes in the global data array.
    long start=0,end=0;
    i=0;
    while(i<=pnum-1){
	start+=sizes[i];
	i++;
    }
    
    i = 0;
    while(i<=pnum){
	end+=sizes[i];
	i++;
    }
    printf("start: %ld end:  %ld\n", start,end);
    // TODO: Copy the local array to the appropriate place in the global array.
    int j=0;

    for(i=start,j=0; i<end ;j++,i++){
	data[i]= xs->data[j];
	printf("xs: %f ",xs->data[j]);
	
    }
    printf("\n");
    for(i=0;i<size;i++)
	printf("%.04f ",data[i]);
    printf("\n\n");
   

    // TODO: Make sure this function doesn't have any data races.
    barrier_wait(bb);
    
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    // TODO: Spawn P processes running sort_worker
    int i=0;
    for(i=P-1;i>=0;i--){
	if(fork()==0){
		sort_worker(i,data,size,P,samps,sizes,bb);
		exit(0);
	}
    }
    // TODO: Once all P processes have been started, wait for them all to finish.
    for(i=P-1;i>=0;i--)
	wait(NULL);

}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    // TODO: Sequentially sample the input data.
   /*int i,j;
   for(i=0,j=0;i<size;i++,j++,data++){
	printf("sort check pos: %d data: %f\n",j,*data);
    }*/


    floats* samples;
    samples = make_floats(0);
    samples = sample(data,size,P);
    floats_print(samples);

    // TODO: Sort the input data using the sampled array to allocate work
    // between parallel processes.
    run_sort_workers(data,size,P,samples,sizes,bb);
}

int
main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage:\n");
        printf("\t%s P data.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    const char* fname = argv[2];

    seed_rng();

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    struct stat s;
    if(fstat(fd,&s)<0)
	return -1;
    size_t fsize = s.st_size;

    void* file = mmap(0 ,fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0); // TODO: Use mmap for I/O 

    long count = *(long*)file; // TODO: this is in the file
    float* data = (float*)(file+8); // TODO: this is in the file


    floats* fnum;
    fnum = make_floats(0);
    int i=0;
    for(i=0;i<count;i++)
	    floats_push(fnum,data[i]);
    floats_print(fnum);

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(0, sizes_bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // TODO: This should be shared memory.

    barrier* bb = make_barrier(P);

    /*int j;
    for(i=0,j=0;i<count;i++,j++,data++){
	printf("pos: %d data: %f\n",j,*data);
    }*/

    
    sample_sort(data, count, P, sizes, bb);

    for(i=0;i<count;i++)
	printf("%.04f ",data[i]);
    printf("\n");
    
    free_barrier(bb);

    // TODO: Clean up resources.
    munmap(file,fsize);
    munmap(sizes,sizes_bytes);

    return 0;
}

