//
// Created by Jian Guo on 8/25/20.
//

#include "NaiveQueue.h"

#include "log.h"
#include <mutex>

NaiveQueue::NaiveQueue() : size(0), head(nullptr), tail(nullptr), is_block(true),
                           mutex_id(),
                           not_empty_condition(),
                           not_full_condition() {
    pthread_mutex_init(&mutex_id, nullptr);
    pthread_cond_init(&not_empty_condition, nullptr);
    pthread_cond_init(&not_full_condition, nullptr);
}

NaiveQueue::~NaiveQueue() {
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

bool NaiveQueue::isEmpty() {
    return size == 0;
}

bool NaiveQueue::isFull() {
    return size == QUEUE_MAX_SIZE;
}

void NaiveQueue::offer(AVPacket *data) {
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

AVPacket *NaiveQueue::poll() {
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


void NaiveQueue::breakBlock() {
    is_block = false;
    pthread_cond_signal(&not_empty_condition);
    pthread_cond_signal(&not_full_condition);
}

void NaiveQueue::clear() {
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





