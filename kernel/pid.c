#include<pid.h>
#include<section.h>

#define PID_BITMAP_WORD_BITS 32
#define PID_RANGE            (PID_MAX - PID_MIN + 1)
#define PID_BITMAP_WORDS     ((PID_RANGE + PID_BITMAP_WORD_BITS - 1) / PID_BITMAP_WORD_BITS)

static uint32_t pid_bitmap[PID_BITMAP_WORDS] DATA={0};
static int32_t pid_cursor = PID_MIN;

static inline void pid_set_bit(int32_t pid)
{
	int32_t idx = (pid - PID_MIN) / PID_BITMAP_WORD_BITS;
	int32_t bit = (pid - PID_MIN) % PID_BITMAP_WORD_BITS;
	pid_bitmap[idx] |= (1u << bit);
}

static inline void pid_clear_bit(int32_t pid)
{
	int32_t idx = (pid - PID_MIN) / PID_BITMAP_WORD_BITS;
	int32_t bit = (pid - PID_MIN) % PID_BITMAP_WORD_BITS;
	pid_bitmap[idx] &= ~(1u << bit);
}

static inline int pid_test_bit(int32_t pid)
{
	int32_t idx = (pid - PID_MIN) / PID_BITMAP_WORD_BITS;
	int32_t bit = (pid - PID_MIN) % PID_BITMAP_WORD_BITS;
	return (pid_bitmap[idx] >> bit) & 1u;
}

void pid_allocator_init(void)
{
	pid_cursor = PID_MIN;
}

int32_t pid_alloc(void)
{
    int32_t scanned = 0;
	for (scanned; scanned < PID_RANGE; ++scanned) {
		int32_t candidate = PID_MIN + ((pid_cursor - PID_MIN + scanned) % PID_RANGE);
		if (!pid_test_bit(candidate)) {
			pid_set_bit(candidate);
			pid_cursor = candidate + 1;
			if (pid_cursor > PID_MAX) {
				pid_cursor = PID_MIN;
			}
			return candidate;
		}
	}
	return PID_INVALID;
}

void pid_free(int32_t pid)
{
	if (pid < PID_MIN || pid > PID_MAX) {
		return;
	}
	if (pid_test_bit(pid)) {
		pid_clear_bit(pid);
		if (pid < pid_cursor) {
			pid_cursor = pid;
		}
	}
}

int pid_is_in_use(int32_t pid)
{
	if (pid < PID_MIN || pid > PID_MAX) {
		return 0;
	}
	return pid_test_bit(pid);
}

