#include <stdint.h>
#include <stddef.h>  // 用于NULL判断

// 定义队列缓冲区大小（可根据需求调整）
#ifndef BUF_SIZE
#define BUF_SIZE 128
#endif

// 双向循环队列结构体定义（明确成员类型，增强可读性）
typedef struct tty_queue {
    int8_t buf[BUF_SIZE];    // 存储数据的缓冲区
    uint32_t head;           // 队头指针（指向队头元素）
    uint32_t tail;           // 队尾指针（指向队尾下一个位置）
} tty_queue_t;

/**
 * @brief 判断队列是否为空
 * @param queue 队列指针（需非空）
 * @return 1-空，0-非空
 */
uint32_t tty_queue_is_empty(const tty_queue_t* queue) {
    if (queue == NULL) return 1;  // 空指针视为空队列
    return (queue->head == queue->tail);
}

/**
 * @brief 判断队列是否为满
 * @param queue 队列指针（需非空）
 * @return 1-满，0-非满
 */
uint32_t tty_queue_is_full(const tty_queue_t* queue) {
    if (queue == NULL) return 1;  // 空指针视为满队列（避免越界）
    return ((queue->tail + 1) % BUF_SIZE) == queue->head;
}

/**
 * @brief 队尾入队（兼容原enqueue函数）
 * @param queue 队列指针
 * @param c 要入队的字符
 * @return 0-成功，1-失败（队列满或参数错误）
 */
int32_t enqueue_tail(tty_queue_t* queue, int8_t c) {
    if (queue == NULL) return 1;  // 参数校验

    if (!tty_queue_is_full(queue)) {
        queue->buf[queue->tail] = c;
        queue->tail = (queue->tail + 1) % BUF_SIZE;
        return 0;
    }
    return 1;
}

/**
 * @brief 队头入队
 * @param queue 队列指针
 * @param c 要入队的字符
 * @return 0-成功，1-失败（队列满或参数错误）
 */
int32_t enqueue_head(tty_queue_t* queue, int8_t c) {
    if (queue == NULL) return 1;  // 参数校验

    if (!tty_queue_is_full(queue)) {
        // 队头指针向前移动（循环处理）
        queue->head = (queue->head - 1 + BUF_SIZE) % BUF_SIZE;
        queue->buf[queue->head] = c;
        return 0;
    }
    return 1;
}

/**
 * @brief 队头出队（兼容原dequeue函数）
 * @param queue 队列指针
 * @param c 存储出队字符的指针
 * @return 0-成功，1-失败（队列空或参数错误）
 */
int32_t dequeue_head(tty_queue_t* queue, int8_t* c) {
    if (queue == NULL || c == NULL) return 1;  // 参数校验

    if (!tty_queue_is_empty(queue)) {
        *c = queue->buf[queue->head];
        queue->head = (queue->head + 1) % BUF_SIZE;
        return 0;
    }
    return 1;
}

/**
 * @brief 队尾出队
 * @param queue 队列指针
 * @param c 存储出队字符的指针
 * @return 0-成功，1-失败（队列空或参数错误）
 */
int32_t dequeue_tail(tty_queue_t* queue, int8_t* c) {
    if (queue == NULL || c == NULL) return 1;

    if (!tty_queue_is_empty(queue)) {
        // 队尾指针向前移动（循环处理）
        queue->tail = (queue->tail - 1 + BUF_SIZE) % BUF_SIZE;
        *c = queue->buf[queue->tail];
        return 0;
    }
    return 1;
}

// 兼容原函数的宏定义（无需修改旧代码）
#define enqueue queue, c enqueue_tail(queue, c)
#define dequeue queue, c dequeue_head(queue, c)
