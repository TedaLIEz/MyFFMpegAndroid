//
// Created by Jian Guo on 8/25/20.
//

#include <jni.h>
#include "log.h"
#include <pthread.h>
#include <unistd.h>
#include "NaiveQueue.h"

// 线程锁
pthread_mutex_t mutex_id;
// 条件变量
pthread_cond_t produce_condition_id, consume_condition_id;
// 队列
NaiveQueue<AVPacket*>* queue;
// 生产数量
#define PRODUCE_COUNT 10
// 目前消费数量
int consume_number = 0;


void* produce(void* arg) {
    char* name = (char*) arg;
    for (int i = 0; i < PRODUCE_COUNT; i++) {
        pthread_mutex_lock(&mutex_id);
        while (queue->isFull()) {
            pthread_cond_wait(&produce_condition_id, &mutex_id);
        }
        LOGE("QueueTest, %s produce element: %d", name, i);
        queue->offer((AVPacket*) i);
        pthread_cond_signal(&consume_condition_id);
        pthread_mutex_unlock(&mutex_id);
        sleep(1);
    }
    LOGE("QueueTest, %s produce finish", name);
    return NULL;
}


void* consume(void* arg) {
    char* name = (char*) arg;
    while (1) {
        pthread_mutex_lock(&mutex_id);

        while (queue->isEmpty()) {
            if (consume_number == PRODUCE_COUNT) {
                break;
            }
            pthread_cond_wait(&consume_condition_id, &mutex_id);
        }

        if (consume_number == PRODUCE_COUNT) {
            pthread_cond_signal(&consume_condition_id);

            pthread_mutex_unlock(&mutex_id);
            break;
        }
        auto element = queue->poll();
        consume_number += 1;
        LOGE("QueueTest, %s consume element : %d", name, element);
        pthread_cond_signal(&produce_condition_id);
        pthread_mutex_unlock(&mutex_id);
        sleep(1);
    }
    LOGE("QueueTest, %s consume finish", name);
    return  NULL;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_NativeTest_testThread(JNIEnv *env, jobject thiz) {
    queue = new NaiveQueue<AVPacket*>();
    pthread_t tid1, tid2, tid3;

    pthread_mutex_init(&mutex_id, nullptr);

    pthread_cond_init(&produce_condition_id, nullptr);
    pthread_cond_init(&consume_condition_id, nullptr);

    pthread_create(&tid1, nullptr, produce, (void*) "producer1");
    pthread_create(&tid2, nullptr, consume, (void*) "consumer1");
    pthread_create(&tid3, nullptr, consume, (void*) "consumer2");


    pthread_join(tid1, nullptr);
    pthread_join(tid2, nullptr);
    pthread_join(tid3, nullptr);

    pthread_cond_destroy(&produce_condition_id);
    pthread_cond_destroy(&consume_condition_id);

    pthread_mutex_destroy(&mutex_id);
    delete queue;
    consume_number = 0;
}