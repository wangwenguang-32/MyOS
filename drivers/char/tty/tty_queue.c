#include <tty.h>
#include <mm_lock.h>

/* head is the first byte; tail is the next insertion slot.
 * One slot stays unused: capacity is TTY_QUEUE_SIZE - 1.
 * data counts queued newline bytes, not the number of bytes.
 * IRQ exclusion supports this kernel's single-CPU interrupt producer model.
 */
#define NEXT(index) (((index) + 1U) % TTY_QUEUE_SIZE)
#define PREVIOUS(index) (((index) + TTY_QUEUE_SIZE - 1U) % TTY_QUEUE_SIZE)

typedef char queue_index_capacity_check[(TTY_QUEUE_SIZE > 1 && TTY_QUEUE_SIZE <= 65536U) ? 1 : -1];

void tty_queue_init(tty_queue_t *queue)
{
    uint32_t flags;
    if (!queue) return;
    flags = mm_irq_save();
    queue->head = queue->tail = 0;
    queue->data = 0;
    INIT_LIST_HEAD(&queue->wait_head);
    mm_irq_restore(flags);
}

uint32_t tty_queue_is_empty(const tty_queue_t *queue)
{
    uint32_t flags = mm_irq_save();
    uint32_t result = !queue || queue->head == queue->tail;
    mm_irq_restore(flags);
    return result;
}

uint32_t tty_queue_is_full(const tty_queue_t *queue)
{
    uint32_t flags = mm_irq_save();
    uint32_t result = !queue || NEXT(queue->tail) == queue->head;
    mm_irq_restore(flags);
    return result;
}

uint32_t tty_queue_count(const tty_queue_t *queue)
{
    uint32_t flags = mm_irq_save();
    uint32_t result = queue ? (queue->tail + TTY_QUEUE_SIZE - queue->head) % TTY_QUEUE_SIZE : 0;
    mm_irq_restore(flags);
    return result;
}

int32_t tty_queue_peek_tail(const tty_queue_t *queue, int8_t *value)
{
    uint32_t flags = mm_irq_save();
    if (!queue || !value || queue->head == queue->tail) {
        mm_irq_restore(flags);
        return 1;
    }
    *value = queue->buf[PREVIOUS(queue->tail)];
    mm_irq_restore(flags);
    return 0;
}

int8_t tty_queue_last(const tty_queue_t *queue)
{
    int8_t value = 0;
    (void)tty_queue_peek_tail(queue, &value);
    return value;
}

int32_t enqueue_tail(tty_queue_t *queue, int8_t value)
{
    uint32_t flags = mm_irq_save();
    if (!queue || NEXT(queue->tail) == queue->head) {
        mm_irq_restore(flags);
        return 1;
    }
    queue->buf[queue->tail] = value;
    queue->tail = NEXT(queue->tail);
    if (value == '\n') queue->data++;
    mm_irq_restore(flags);
    return 0;
}

int32_t enqueue_head(tty_queue_t *queue, int8_t value)
{
    uint32_t flags = mm_irq_save();
    if (!queue || NEXT(queue->tail) == queue->head) {
        mm_irq_restore(flags);
        return 1;
    }
    queue->head = PREVIOUS(queue->head);
    queue->buf[queue->head] = value;
    if (value == '\n') queue->data++;
    mm_irq_restore(flags);
    return 0;
}

int32_t dequeue_head(tty_queue_t *queue, int8_t *value)
{
    uint32_t flags = mm_irq_save();
    if (!queue || !value || queue->head == queue->tail) {
        mm_irq_restore(flags);
        return 1;
    }
    *value = queue->buf[queue->head];
    queue->head = NEXT(queue->head);
    if (*value == '\n' && queue->data) queue->data--;
    mm_irq_restore(flags);
    return 0;
}

int32_t dequeue_tail(tty_queue_t *queue, int8_t *value)
{
    uint32_t flags = mm_irq_save();
    if (!queue || !value || queue->head == queue->tail) {
        mm_irq_restore(flags);
        return 1;
    }
    queue->tail = PREVIOUS(queue->tail);
    *value = queue->buf[queue->tail];
    if (*value == '\n' && queue->data) queue->data--;
    mm_irq_restore(flags);
    return 0;
}
