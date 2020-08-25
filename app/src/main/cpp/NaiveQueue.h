//
// Created by Jian Guo on 8/25/20.
//

#ifndef TESTFFMPEG_NAIVEQUEUE_H
#define TESTFFMPEG_NAIVEQUEUE_H

#include <sys/types.h>
#include <pthread.h>
#include "log.h"

extern "C" {
#include "libavformat/avformat.h"
};
#define QUEUE_MAX_SIZE 50

template <typename T>
class NaiveQueue {

    typedef struct _Node {
        T data;
        struct _Node* next;
    } Node;


private:
    int size;
    Node* head;
    Node* tail;

    bool is_block;

    pthread_mutex_t mutex_id;

    pthread_cond_t not_empty_condition;
    pthread_cond_t not_full_condition;


public:
    NaiveQueue() : size(0), head(nullptr), tail(nullptr), is_block(true),
                  mutex_id(),
                  not_empty_condition(),
                  not_full_condition() {
        pthread_mutex_init(&mutex_id, nullptr);
        pthread_cond_init(&not_empty_condition, nullptr);
        pthread_cond_init(&not_full_condition, nullptr);
    }


    NaiveQueue(const NaiveQueue&) = delete;

    NaiveQueue& operator=(NaiveQueue const&) = delete;


    bool isEmpty() {
        return size == 0;
    }


    bool isFull() {
        return size == QUEUE_MAX_SIZE;
    }

    void offer(AVPacket* data) {
        pthread_mutex_lock(&mutex_id);
        while (isFull() && is_block) {
            pthread_cond_wait(&not_full_condition, &mutex_id);
        }

        if (size >= QUEUE_MAX_SIZE) {
            pthread_mutex_unlock(&mutex_id);
            return;
        }
        auto node = new Node;
        node->data = data;
        node->next = nullptr;
        if (tail == nullptr) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        size += 1;
        pthread_cond_signal(&not_empty_condition);
        pthread_mutex_unlock(&mutex_id);

    }

    T poll() {
        pthread_mutex_lock(&mutex_id);
        while (isEmpty() && is_block) {
            pthread_cond_wait(&not_empty_condition, &mutex_id);
        }
        if (head == nullptr) {
            pthread_mutex_unlock(&mutex_id);
            return nullptr;
        }
        auto node = head;
        head = head->next;
        if (head == nullptr) {
            tail = nullptr;
        }
        auto element = node->data;
        free(node);
        size -= 1;
        pthread_mutex_unlock(&mutex_id);
        return element;
    }

    void clear() {
        pthread_mutex_lock(&mutex_id);
        auto node = head;
        while (node != nullptr) {
            head = head->next;
            free(node);
            node = head;
        }
        head = nullptr;
        tail = nullptr;
        size = 0;
        is_block = true;
        pthread_cond_signal(&not_full_condition);
        pthread_mutex_unlock(&mutex_id);

    }

    void breakBlock() {
        is_block = false;
        pthread_cond_signal(&not_empty_condition);
        pthread_cond_signal(&not_full_condition);
    }

    virtual ~NaiveQueue() {
        pthread_mutex_lock(&mutex_id);
        auto node = head;
        while (node != nullptr) {
            head = head->next;
            delete node;
            node = head;
        }
        head = nullptr;
        tail = nullptr;
        is_block = false;
        pthread_mutex_unlock(&mutex_id);
        pthread_mutex_destroy(&mutex_id);
        pthread_cond_destroy(&not_empty_condition);
        pthread_cond_destroy(&not_full_condition);
        LOGD("~NaiveQueue, deconstructor called");
    }


};


#endif //TESTFFMPEG_NAIVEQUEUE_H
