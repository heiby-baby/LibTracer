#include <stdio.h>
#include "lib.h"


int main(){
    char* pass = "pass";
    char input[20];
    scanf("%s", input);
    int result = (compare(input, pass));
    if(result){
        printf("Wrong pass\n");
    }
    else{
        printf("correct\n");
    }

    printf("%d\n", add(1,2));
}