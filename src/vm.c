#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { double* data; size_t top, cap; } Stack;
static void spush(Stack* s, double v){ if(s->top==s->cap){s->cap=s->cap?2*s->cap:16; s->data=realloc(s->data,s->cap*sizeof(double));} s->data[s->top++]=v; }
static double spop(Stack* s){ return s->top? s->data[--s->top] : 0.0; }

int vm_run(const unsigned char* code, size_t size){
    Stack st={0};
    size_t ip=0; int rc=0;
    while(ip<size){
        unsigned char op=code[ip++];
        switch(op){
            case OP_CONST: {
                if (ip+8>size){ rc=1; goto end; }
                double v; memcpy(&v,&code[ip],8); ip+=8; spush(&st,v); break; }
            case OP_ADD: {
                double b=spop(&st), a=spop(&st); spush(&st, a+b); break; }
            case OP_PRINT: {
                double v=spop(&st); printf("%g\n", v); break; }
            case OP_HALT: goto end;
            default: rc=1; goto end;
        }
    }
end:
    free(st.data);
    return rc;
}

int vm_run_file(const char* path){
    FILE* f=fopen(path,"rb"); if(!f){ perror("open"); return 1; }
    fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
    unsigned char* buf=(unsigned char*)malloc(sz);
    fread(buf,1,sz,f); fclose(f);
    int rc=vm_run(buf,(size_t)sz);
    free(buf); return rc;
}
