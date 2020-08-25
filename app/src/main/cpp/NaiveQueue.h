//
// Created by Jian Guo on 8/25/20.
//

#ifndef TESTFFMPEG_NAIVEQUEUE_H
#define TESTFFMPEG_NAIVEQUEUE_H

#include <sys/types.h>
#include <pthread.h>

extern "C" {
#include "libavformat/avformat.h"
};
#define QUEUE_MAX_SIZE 50

class NaiveQueue {

    typedef struct _Node {
        AVPacket* data;
        struct _Node* next;
    } Node;


private:
    int size;
    Node* head;
    Node* tail;

    bool is_block;

    pthread_mutex_t* mutex_id;

    pthread_cond_t* not_empty_condition;
    pthread_cond_t* not_full_condition;


public:
    NaiveQueue();



    bool isEmpty();


    bool isFull();

    void offer(AVPacket* data);

    AVPacket* poll();

    void clear();

    void breakBlock();

    virtual ~NaiveQueue();


};


#endif //TESTFFMPEG_NAIVEQUEUE_H
