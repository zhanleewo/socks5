/**
 * Project page: https://github.com/wangrn/ringbuffer
 * Copyright (c) 2013 Wang Ruining <https://github.com/wangrn>
 * @date 2013/01/16 13:33:20
 * @brief   a simple ringbuffer, DO NOT support dynamic expanded memory
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdlib.h>
#include <stdint.h>
 
struct ringbuffer;

struct ringbuffer_tran {
    char buf[8];
};

struct ringbuffer* ringbuffer_new(int32_t capacity);
void ringbuffer_free(struct ringbuffer *rb);
void ringbuffer_clear(struct ringbuffer *rb);

ssize_t ringbuffer_capacity(struct ringbuffer *rb);
ssize_t ringbuffer_can_read(struct ringbuffer *rb);
ssize_t ringbuffer_can_write(struct ringbuffer *rb);

ssize_t ringbuffer_read(struct ringbuffer *rb, void *data, size_t count);
ssize_t ringbuffer_write(struct ringbuffer *rb, const void *data, size_t count);

ssize_t ringbuffer_transaction_read(struct ringbuffer *rb, void *data, size_t count);
struct ringbuffer *ringbuffer_transaction_begin(struct ringbuffer *rb, struct ringbuffer_tran *tran);
int ringbuffer_transaction_rollback(struct ringbuffer *rb, struct ringbuffer_tran *tran);
int ringbuffer_transaction_commit(struct ringbuffer *rb, struct ringbuffer_tran *tran);


#endif
