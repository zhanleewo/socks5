/**
 * Project page: https://github.com/wangrn/ringbuffer
 * Copyright (c) 2013 Wang Ruining <https://github.com/wangrn>
 * @date 2013/01/16 13:33:20
 * @brief   a simple ringbuffer, DO NOT support dynamic expanded memory
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ringbuffer.h>

#define RING_BUFFER_DEFAULT_SIZE    1024 * 10

#define min(a, b) (a)<(b)?(a):(b)

struct ringbuffer{
    int32_t capacity;
    char *head;
    char *tail;
    char *data;
};

void ringbuffer_tran_set_left(struct ringbuffer_tran *tran, int n) {
    tran->nleft = n;
}

struct ringbuffer* ringbuffer_new(int32_t capacity) {
    
    struct ringbuffer *rb = NULL;

    if(capacity <= 0) 
        capacity = RING_BUFFER_DEFAULT_SIZE;

    rb = malloc(sizeof(*rb) + capacity);
    if (!rb)
        return NULL;
    
    rb->capacity = capacity;
    rb->data = (char*) rb + sizeof(struct ringbuffer);
    rb->head = rb->data;
    rb->tail = rb->data;
    
    return rb;
}

void  ringbuffer_free(struct ringbuffer *rb) {
    if(rb) {
        free(rb);
    }
}

void ringbuffer_clear(struct ringbuffer *rb) {
    rb->head = rb->data;
    rb->tail = rb->data;
}

ssize_t ringbuffer_capacity(struct ringbuffer *rb) {

    if(!rb)
        return -1;
    
    return rb->capacity;
}

ssize_t ringbuffer_can_read(struct ringbuffer *rb) {
    
    if(!rb)
        return -1;
    
    if (rb->head == rb->tail)
        return 0;
    
    if (rb->head < rb->tail)
        return rb->tail - rb->head;
    
    return ringbuffer_capacity(rb) - (rb->head - rb->tail);
}

ssize_t ringbuffer_can_write(struct ringbuffer *rb) {
    
    if(!rb)
        return -1;
    
    return ringbuffer_capacity(rb) - ringbuffer_can_read(rb);
}

ssize_t ringbuffer_transaction_read(struct ringbuffer *rb, void *data, size_t count) {
    
    int nread = 0;
    
    if(!rb || !data)
        return -1;
    
    if(ringbuffer_can_read(rb) == 0) {
        return -EWOULDBLOCK;
    }
    
    if (rb->head < rb->tail)
    {
        nread = min(count, ringbuffer_can_read(rb));
        memcpy(data, rb->head, nread);
        rb->head += nread;
        return nread;
    }
    else
    {
        if (count < ringbuffer_capacity(rb)-(rb->head - rb->data))
        {
            nread = count;
            memcpy(data, rb->head, nread);
            rb->head += nread;
            return nread;
        }
        else
        {
            nread = ringbuffer_capacity(rb) - (rb->head - rb->data);
            memcpy(data, rb->head, nread);
            rb->head = rb->data;
            nread += ringbuffer_read(rb, (char*)data+nread, count-nread);
            return nread;
        }
    }
}


struct ringbuffer *ringbuffer_transaction_begin(struct ringbuffer *rb, struct ringbuffer_tran *tran) {
    memcpy(&tran->rb, rb, sizeof(*rb));
    return &tran->rb;
}

int ringbuffer_transaction_rollback(struct ringbuffer *rb, struct ringbuffer_tran *tran) {
    
//    struct ringbuffer *trb = (struct ringbuffer *)tran;
//    trb->head = rb->head;
    memcpy(&tran->rb, rb, sizeof(*rb));
    return 0;
}

/*int ringbuffer_transaction_commit(struct ringbuffer *rb, struct ringbuffer_tran *tran) {
    memcpy(rb, tran, sizeof(*rb));
    return 0;
}*/
int ringbuffer_transaction_commit(struct ringbuffer *rb, struct ringbuffer_tran *tran) {
    
    if(tran->nleft > 0) {
        if(tran->rb.head <= tran->rb.tail) {
            if(tran->rb.head - tran->nleft >= tran->data) {
                tran->rb.head -= tran->nleft;
            } else {
                // tran->rb.head = tran->rb.data + (ringbuffer_capacity(tran->rb) - (tran->nleft - (tran->head - tran->data)))
                tran->rb.head = tran->rb.data + (ringbuffer_capacity(tran->rb) - tran->nleft + tran->head - tran->data);
            }
        } else {
            if(tran->rb.head - tran->nleft >= tran->tail) {
                tran->rb.head -= tran->nleft;
            }
        }
    }

    memcpy(rb, &tran->rb, sizeof(*rb));
    return 0;
}


ssize_t ringbuffer_read(struct ringbuffer *rb, void *data, size_t count) {

    int nread = 0;
    
    if(!rb || !data)
        return -1;
    
    if (rb->head < rb->tail)
    {
        nread = min(count, ringbuffer_can_read(rb));
        memcpy(data, rb->head, nread);
        rb->head += nread;
        return nread;
    }
    else
    {
        if (count < ringbuffer_capacity(rb)-(rb->head - rb->data))
        {
            nread = count;
            memcpy(data, rb->head, nread);
            rb->head += nread;
            return nread;
        }
        else
        {
            nread = ringbuffer_capacity(rb) - (rb->head - rb->data);
            memcpy(data, rb->head, nread);
            rb->head = rb->data;
            nread += ringbuffer_read(rb, (char*)data+nread, count-nread);
            return nread;
        }
    }
}

ssize_t ringbuffer_write(struct ringbuffer *rb, const void *data, size_t count)
{
    if(!rb || !data)
        return -1;
    
    if (count >= ringbuffer_can_write(rb))
        return -1;
    
    if (rb->head <= rb->tail)
    {
        int nleft = ringbuffer_capacity(rb) - (rb->tail - rb->data);
        if (count <= nleft)
        {
            memcpy(rb->tail, data, count);
            rb->tail += count;
            if (rb->tail == rb->data+ringbuffer_capacity(rb))
                rb->tail = rb->data;
            return count;
        }
        else
        {
            memcpy(rb->tail, data, nleft);
            rb->tail = rb->data;
            return nleft + ringbuffer_write(rb, (char*)data+nleft, count-nleft);
        }
    }
    else
    {
        memcpy(rb->tail, data, count);
        rb->tail += count;
        return count;
    }
}
