#include <common.h>
#include <stdio.h>
#include <ringbuffer.h>

int main(int argc, char **argv) {
    
    struct ringbuffer *rb = ringbuffer_new(1024 * 1024);
    struct ringbuffer_tran tran;
    char buf[10];
    ringbuffer_write(rb, "hello", 5);
    
    struct ringbuffer *transacted_rb = ringbuffer_transaction_begin(rb, &tran);
    
    int ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    }
    
    ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    }
    
    ringbuffer_write(rb, "world", 5);
    transacted_rb = ringbuffer_transaction_begin(rb, &tran);
    ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    }
    
    ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    } else {
        ringbuffer_transaction_commit(rb, &tran);
    }
    
    ringbuffer_write(rb, "javac", 5);
    ringbuffer_write(rb, "javax", 5);
    
    transacted_rb = ringbuffer_transaction_begin(rb, &tran);
    ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    }
    
    ret = ringbuffer_transaction_read(transacted_rb, buf, 5);
    if(ret < 0) {
        ringbuffer_transaction_rollback(rb, &tran);
    }
    
	printf("hello world!\n");
	return 0;
}
