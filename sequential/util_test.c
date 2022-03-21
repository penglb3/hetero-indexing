#include "data_sketch.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

#define LEN 10

void printqueue(queue* q){
    for(int i=0; i<q->size; i++){
        printf("%5.5lu->", (uintptr_t)q->data[i]);
    }
    printf("\n");
    for(int i=0; i<q->size; i++){
        if(i == q->back || i == q->front)
            printf("^^^^^  ");
        else    
            printf("       ");
    }
    printf("\n");
    for(int i=0; i<q->size; i++){
        if(i == q->back)
            printf("back   ");
        else if(i == q->front)   
            printf("front  ");
        else printf("       ");
    }
    printf("\n");
}

int main(){
    sketch* bloom = bloom_construct(128, 4, 0), *cm = countmin_construct(8, 4, 0);
    queue* q = queue_construct(8);
    if(!cm || !bloom || !q)
        return -1;
    int op = 1, res;
    char str[10] = {0};
    printf("Input format: <opcode> <string>\n");
    printf("Opcode : 1: bloom log, 2: bloom query, 3: count_min log, 4: count_min query, 5: queue push, 6: queue pop, -1: quit.\n");
    while(op != -1){
        printf(">>>");
        scanf("%d",&op);
        if(op!=-1 && op!=6) 
            scanf("%s", str);
        else break;
        switch (op)
        {
        case 1:
            bloom_add(bloom, str, LEN);
            printf("Bloom Filters:\n");
            for(int i=0; i<bloom->width/32; i++)
                printf("%.8x ",bloom->counts[i]);
            printf("\n");
            break;
        case 2:
            res = bloom_exists(bloom, str, LEN);
            printf("Bloom filter: I have %s seen this.\n", res?"probably":"never");
            break;
        case 3:
            countmin_inc(cm, str, LEN);
            printf("Count-Min Counts:\n");
            for(int i=0; i<cm->depth; i++){
                for(int j=0; j<cm->width; j++)
                    printf("%10d ",cm->counts[i*cm->width + j]);
                printf("\n");
            }
            break;
        case 4:
            res = countmin_count(cm, str, LEN);
            printf("Count-Min: I would say it's %d time(s).\n", res);
            break;
        case 5:
            res = atoi(str);
            queue_push(q, (void*)(uintptr_t)res);
            printf("PUSH %u\n", res);
            printqueue(q);
            break;
        case 6:
            printf("POP %lu\n", (uintptr_t) queue_pop(q));
            printqueue(q);
        default:
            printf("Bad opcode. Plz try again\n");
            break;
        }
    }
    countmin_destroy(cm);
    bloom_destroy(bloom);
    printf("Bye~\n");
    return 0;
}
