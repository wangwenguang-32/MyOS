#ifndef  TTY_H
#define TTY_H

#include<stdint.h>
#include<process.h>
#include<list.h>
#include<termios.h>

#define TTY_QUEUE_SIZE 1024U
#define BUF_SIZE TTY_QUEUE_SIZE

typedef struct tty tty_t;

typedef struct
{
    uint16_t head;
    uint16_t tail;
    uint32_t data;
    list_node_t wait_head;
    int8_t buf[BUF_SIZE];
}tty_queue_t;

typedef struct
{
    void (*read)(tty_t*tty);
    void (*write)(tty_t*tty);
}tty_operation_t;


struct tty
{
    termios_t termios;
    tty_operation_t operation;
    tty_queue_t read;
    tty_queue_t write;
};


#define     IFLAG(tty,f)    ((tty)->termios.c_iflag & f)
#define     OFLAG(tty,f)    ((tty)->termios.c_oflag & f)
#define     CFLAG(tty,f)    ((tty)->termios.c_cflag & f)
#define     LFLAG(tty,f)    ((tty)->termios.c_lflag & f)


#define I_IGNBRK(tty)       IFLAG(tty,IGNBRK)
#define I_BRKINT(tty)       IFLAG(tty,BRKINT)
#define I_IGNPAR(tty)       IFLAG(tty,IGNPAR)
#define I_PARMRK(tty)       IFLAG(tty,PARMRK)
#define I_INPCK(tty)        IFLAG(tty,INPCK)
#define I_ISTRIP(tty)       IFLAG(tty,ISTRIP)
#define I_INLCR(tty)        IFLAG(tty,INLCR)
#define I_IGNCR(tty)        IFLAG(tty,IGNCR)
#define I_ICRNL(tty)        IFLAG(tty,ICRNL)
#define I_IUCLC(tty)        IFLAG(tty,IUCLC)
#define I_IXON(tty)         IFLAG(tty,IXON)
#define I_IXANY(tty)        IFLAG(tty,IXANY)
#define I_IXOFF(tty)        IFLAG(tty,IXOFF)
#define I_IMAXBEL(tty)      IFLAG(tty,IMAXBEL)


#define O_OPOST(tty)        OFLAG(tty, OPOST)
#define O_OLCUC(tty)        OFLAG(tty, OLCUC)
#define O_ONLCR(tty)        OFLAG(tty, ONLCR)
#define O_OCRNL(tty)        OFLAG(tty, OCRNL)
#define O_ONOCR(tty)        OFLAG(tty, ONOCR)
#define O_ONLRET(tty)       OFLAG(tty, ONLRET)
#define O_OFILL(tty)        OFLAG(tty, OFILL)
#define O_OFDEL(tty)        OFLAG(tty, OFDEL)
#define O_NLDLY(tty)        OFLAG(tty, NLDLY)
#define O_NL0(tty)          OFLAG(tty, NL0)
#define O_NL1(tty)          OFLAG(tty, NL1)
#define O_CRDLY(tty)        OFLAG(tty, CRDLY)
#define O_CR0(tty)          OFLAG(tty, CR0)
#define O_CR1(tty)          OFLAG(tty, CR1)
#define O_CR2(tty)          OFLAG(tty, CR2)
#define O_CR3(tty)          OFLAG(tty, CR3)
#define O_TABDLY(tty)       OFLAG(tty, TABDLY)
#define O_TAB0(tty)         OFLAG(tty, TAB0)
#define O_TAB1(tty)         OFLAG(tty, TAB1)
#define O_TAB2(tty)         OFLAG(tty, TAB2)
#define O_TAB3(tty)         OFLAG(tty, TAB3)
#define O_XTABS(tty)        OFLAG(tty, XTABS)
#define O_BSDLY(tty)        OFLAG(tty, BSDLY)
#define O_BS0(tty)          OFLAG(tty, BS0)
#define O_BS1(tty)          OFLAG(tty, BS1)
#define O_VTDLY(tty)        OFLAG(tty, VTDLY)
#define O_VT0(tty)          OFLAG(tty, VT0)
#define O_VT1(tty)          OFLAG(tty, VT1)
#define O_FFDLY(tty)        OFLAG(tty, FFDLY)
#define O_FF0(tty)          OFLAG(tty, FF0)
#define O_FF1(tty)          OFLAG(tty, FF1)



#define C_CBAUD(tty)        CFLAG(tty, CBAUD)
#define C_B0(tty)           CFLAG(tty, B0)
#define C_B50(tty)          CFLAG(tty, B50)
#define C_B75(tty)          CFLAG(tty, B75)
#define C_B110(tty)         CFLAG(tty, B110)
#define C_B134(tty)         CFLAG(tty, B134)
#define C_B150(tty)         CFLAG(tty, B150)
#define C_B200(tty)         CFLAG(tty, B200)
#define C_B300(tty)         CFLAG(tty, B300)
#define C_B600(tty)         CFLAG(tty, B600)
#define C_B1200(tty)        CFLAG(tty, B1200)
#define C_B1800(tty)        CFLAG(tty, B1800)
#define C_B2400(tty)        CFLAG(tty, B2400)
#define C_B4800(tty)        CFLAG(tty, B4800)
#define C_B9600(tty)        CFLAG(tty, B9600)
#define C_B19200(tty)       CFLAG(tty, B19200)
#define C_B38400(tty)       CFLAG(tty, B38400)
#define C_EXTA(tty)         CFLAG(tty, EXTA)
#define C_EXTB(tty)         CFLAG(tty, EXTB)
#define C_CSIZE(tty)        CFLAG(tty, CSIZE)
#define C_CS5(tty)          CFLAG(tty, CS5)
#define C_CS6(tty)          CFLAG(tty, CS6)
#define C_CS7(tty)          CFLAG(tty, CS7)
#define C_CS8(tty)          CFLAG(tty, CS8)
#define C_CSTOPB(tty)       CFLAG(tty, CSTOPB)
#define C_CREAD(tty)        CFLAG(tty, CREAD)
#define C_CPARENB(tty)      CFLAG(tty, CPARENB)
#define C_CPARODD(tty)      CFLAG(tty, CPARODD)
#define C_HUPCL(tty)        CFLAG(tty, HUPCL)
#define C_CLOCAL(tty)       CFLAG(tty, CLOCAL)
#define C_CIBAUD(tty)       CFLAG(tty, CIBAUD)
#define C_CRTSCTS(tty)      CFLAG(tty, CRTSCTS)
#define C_PARENB(tty)       CFLAG(tty, PARENB)
#define C_PARODD(tty)       CFLAG(tty, PARODD)




#define L_ISIG(tty)         LFLAG(tty, ISIG)
#define L_ICANON(tty)       LFLAG(tty, ICANON)
#define L_XCASE(tty)        LFLAG(tty, XCASE)
#define L_ECHO(tty)         LFLAG(tty, ECHO)
#define L_ECHOE(tty)        LFLAG(tty, ECHOE)
#define L_ECHOK(tty)        LFLAG(tty, ECHOK)
#define L_ECHONL(tty)       LFLAG(tty, ECHONL)
#define L_NOFLSH(tty)       LFLAG(tty, NOFLSH)
#define L_TOSTOP(tty)       LFLAG(tty, TOSTOP)
#define L_ECHOCTL(tty)      LFLAG(tty, ECHOCTL)
#define L_ECHOPRT(tty)      LFLAG(tty, ECHOPRT)
#define L_ECHOKE(tty)       LFLAG(tty, ECHOKE)
#define L_FLUSHO(tty)       LFLAG(tty, FLUSHO)
#define L_PENDIN(tty)       LFLAG(tty, PENDIN)
#define L_IEXTEN(tty)       LFLAG(tty, IEXTEN)


/* Queue operations return 0 on success, 1 on full/empty/invalid input.
 * Failed dequeue leaves the output byte unchanged. data counts newlines.
 * Initialize only before use; resetting an active wait queue is not supported.
 */
void tty_queue_init(tty_queue_t *queue);
uint32_t tty_queue_count(const tty_queue_t *queue);
int32_t tty_queue_peek_tail(const tty_queue_t *queue, int8_t *value);
int8_t tty_queue_last(const tty_queue_t *queue);
extern uint32_t tty_queue_is_empty(const tty_queue_t* queue);
extern uint32_t tty_queue_is_full(const tty_queue_t* queue);
extern int32_t enqueue_tail(tty_queue_t*queue,int8_t c);
extern int32_t dequeue_tail(tty_queue_t*queue,int8_t*c);
extern int32_t enqueue_head(tty_queue_t*queue,int8_t c);
extern int32_t dequeue_head(tty_queue_t*queue,int8_t*c);

#define EMPTY(queue)              tty_queue_is_empty(queue)
#define FULL(queue)                 tty_queue_is_full(queue)
#define PUTCH_HEAD(queue,c)       enqueue_head(queue,c)
#define GETCH_HEAD(queue,c)       dequeue_head(queue,c)
#define PUTCH_TAIL(queue,c)       enqueue_tail(queue,c)
#define GETCH_TAIL(queue,c)       dequeue_tail(queue,c)
#define DELCH_HEAD(queue)    do{ \
        int8_t tmp;              \
        dequeue_head(queue,&tmp);\
}while(0)
#define DELCH_TAIL(queue)    do{ \
        int8_t tmp;              \
        dequeue_tail(queue,&tmp);\
}while(0)

#define LAST(queue) tty_queue_last(queue)


void tty_init(void);
void tty_handle_char(tty_t *terminal, int8_t c);
extern tty_t tty;


#endif
