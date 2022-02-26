#include "data_sketch.h"
#include <stdio.h>

#define LEN 10

int main(){
    sketch* bloom = bloom_init(128, 4), *cm = countmin_init(8, 4);
    if(!cm || !bloom)
        return -1;
    int op = 1, res;
    char str[10] = {0};
    printf("Input format: <opcode> <string>\n");
    printf("Opcode : 1: bloom log, 2: bloom query, 3: count_min log, 4: count_min query, -1: quit.\n");
    while(op != -1){
        printf(">>>");
        scanf("%d",&op);
        if(op!=-1) 
            scanf("%s", str);
        else break;
        switch (op)
        {
        case 1:
            bloom_log(bloom, str, LEN);
            printf("Bloom Filters:\n");
            for(int i=0; i<bloom->width/32; i++)
                printf("%.8x ",bloom->counts[i]);
            printf("\n");
            break;
        case 2:
            res = bloom_query(bloom, str, LEN);
            printf("Bloom filter: I have %s seen this.\n", res?"probably":"never");
            break;
        case 3:
            countmin_log(cm, str, LEN);
            printf("Count-Min Counts:\n");
            for(int i=0; i<cm->depth; i++){
                for(int j=0; j<cm->width; j++)
                    printf("%10d ",cm->counts[i*cm->width + j]);
                printf("\n");
            }
            break;
        case 4:
            res = countmin_query(cm, str, LEN);
            printf("Count-Min: I would say it's %d time(s).\n", res);
            break;
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
