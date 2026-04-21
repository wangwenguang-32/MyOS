#ifndef  TERMIOS_H
#define TERMIOS_H


/* 0x54 是魔数（对应 ASCII 码 'T'），用于区分终端控制相关的 ioctl 命令，避免与其他命令冲突 */

// -------------------------- ioctl 终端控制命令宏 --------------------------
// 获取终端属性（termios 结构体，新接口）
#define TCGETS		0x5401	/* Get terminal attributes (struct termios) */
// 设置终端属性（立即生效，struct termios）
#define TCSETS		0x5402	/* Set terminal attributes (immediate, termios) */
// 设置终端属性（等待输出完成后生效）
#define TCSETSW		0x5403	/* Set attributes after output drain (termios) */
// 设置终端属性（丢弃输入队列数据后生效）
#define TCSETSF		0x5404	/* Set attributes after flushing input (termios) */
// 获取终端属性（termio 结构体，兼容旧接口）
#define TCGETA		0x5405	/* Get terminal attributes (struct termio, legacy) */
// 设置终端属性（立即生效，struct termio）
#define TCSETA		0x5406	/* Set terminal attributes (immediate, termio) */
// 设置终端属性（等待输出完成后生效，termio）
#define TCSETAW		0x5407	/* Set attributes after output drain (termio) */
// 设置终端属性（丢弃输入队列数据后生效，termio）
#define TCSETAF		0x5408	/* Set attributes after flushing input (termio) */
// 发送中断信号（break 信号）
#define TCSBRK		0x5409	/* Send break signal to terminal */
// 控制流（XON/XOFF 流量控制）
#define TCXONC		0x540A	/* Control XON/XOFF flow control */
// 刷新终端 I/O 队列
#define TCFLSH		0x540B	/* Flush terminal input/output queues */
// 设置终端为独占模式（禁止其他进程打开）
#define TIOCEXCL	0x540C	/* Set terminal exclusive access mode */
// 取消终端独占模式
#define TIOCNXCL	0x540D	/* Disable terminal exclusive access mode */
// 设置终端为控制终端（与当前进程组关联）
#define TIOCSCTTY	0x540E	/* Set terminal as controlling terminal */
// 获取终端所属的进程组 ID
#define TIOCGPGRP	0x540F	/* Get process group ID associated with terminal */
// 设置终端所属的进程组 ID
#define TIOCSPGRP	0x5410	/* Set process group ID associated with terminal */
// 获取输出队列中未发送的字节数
#define TIOCOUTQ	0x5411	/* Get number of bytes in output queue */
// 向终端输入队列插入一个字符（模拟键盘输入）
#define TIOCSTI		0x5412	/* Insert a character into terminal input queue */
// 获取终端窗口大小
#define TIOCGWINSZ	0x5413	/* Get terminal window size (struct winsize) */
// 设置终端窗口大小
#define TIOCSWINSZ	0x5414	/* Set terminal window size (struct winsize) */
// 获取调制解调器状态（如 DTR、RTS 等信号线）
#define TIOCMGET	0x5415	/* Get modem control signals status */
// 置位调制解调器指定信号（如启用 RTS）
#define TIOCMBIS	0x5416	/* Set specific modem control signals */
// 复位调制解调器指定信号（如禁用 RTS）
#define TIOCMBIC	0x5417	/* Clear specific modem control signals */
// 设置调制解调器信号状态（覆盖所有信号）
#define TIOCMSET	0x5418	/* Set all modem control signals */
// 获取软载波检测状态
#define TIOCGSOFTCAR	0x5419	/* Get soft carrier detect status */
// 设置软载波检测状态
#define TIOCSSOFTCAR	0x541A	/* Set soft carrier detect status */
// 获取输入队列中待读取的字节数
#define TIOCINQ		0x541B	/* Get number of bytes in input queue */

// -------------------------- 终端窗口大小结构体 --------------------------
/* 存储终端窗口的行数、列数及像素尺寸，通过 TIOCGWINSZ/TIOCSWINSZ 命令获取/设置 */
struct winsize {
	unsigned short ws_row;		/* 窗口行数（字符数） */
	unsigned short ws_col;		/* 窗口列数（字符数） */
	unsigned short ws_xpixel;	/* 窗口宽度（像素数，可选） */
	unsigned short ws_ypixel;	/* 窗口高度（像素数，可选） */
};


// -------------------------- 标准终端属性结构体 --------------------------
#define NCCS 17	/* termios 结构体中控制字符数组的长度（支持更多控制字符） */
/* 标准终端属性结构体（POSIX 规范），用于终端 I/O 模式配置 */
typedef  struct
{
	unsigned long c_iflag;		/* 输入模式标志（处理输入数据的规则） */
	unsigned long c_oflag;		/* 输出模式标志（格式化输出数据） */
	unsigned long c_cflag;		/* 控制模式标志（硬件相关配置，如波特率） */
	unsigned long c_lflag;		/* 本地模式标志（终端本地交互行为） */
	unsigned char c_line;		/* 线路规程（通常设为 0，使用默认规程） */
	unsigned char c_cc[NCCS];	/* 控制字符数组（存储各类控制字符的 ASCII 码） */
} termios_t;

// -------------------------- c_cc 控制字符索引宏 --------------------------
/* c_cc 数组的索引，对应不同功能的控制字符，可通过 tcsetattr 修改 */
#define VINTR 0		/* 中断字符（默认 Ctrl+C），触发 SIGINT 信号 */
#define VQUIT 1		/* 退出字符（默认 Ctrl+\），触发 SIGQUIT 信号 */
#define VERASE 2	/* 擦除字符（默认退格键），删除前一个输入字符 */
#define VKILL 3		/* 杀死字符（默认 Ctrl+U），删除整行输入 */
#define VEOF 4		/* 结束符（默认 Ctrl+D），表示输入结束 */
#define VTIME 5		/* 超时时间（非规范模式下，单位：0.1 秒） */
#define VMIN 6		/* 最小读取字节数（非规范模式下，满足该数才返回读取） */
#define VSWTC 7		/* 交换字符（保留，部分系统未使用） */
#define VSTART 8	/* 启动字符（默认 Ctrl+Q），恢复 XON/XOFF 流量控制 */
#define VSTOP 9		/* 停止字符（默认 Ctrl+S），暂停 XON/XOFF 流量控制 */
#define VSUSP 10	/* 挂起字符（默认 Ctrl+Z），触发 SIGTSTP 信号 */
#define VEOL 11		/* 行结束符（补充换行符，默认与 newline 相同） */
#define VREPRINT 12	/* 重印字符（默认 Ctrl+R），重新打印输入队列内容 */
#define VDISCARD 13	/* 丢弃字符（默认 Ctrl+O），切换是否丢弃输出 */
#define VWERASE 14	/* 单词擦除字符（默认 Ctrl+W），删除前一个单词 */
#define VLNEXT 15	/* 字面值字符（默认 Ctrl+V），使下一个字符按原义解析 */
#define VEOL2 16	/* 第二个行结束符（保留，部分系统用于多行输入） */

// -------------------------- c_iflag 输入模式标志 --------------------------
/* 控制输入数据的处理规则，可通过位或（|）组合使用 */
#define IGNBRK	0000001	/* 忽略中断条件（break 信号） */
#define BRKINT	0000002	/* 中断条件触发 SIGINT 信号（与 IGNBRK 互斥） */
#define IGNPAR	0000004	/* 忽略校验错误的输入字符 */
#define PARMRK	0000010	/* 标记校验错误的字符（前缀 0xFF 0x00） */
#define INPCK	0000020	/* 启用输入校验（检查奇偶校验错误） */
#define ISTRIP	0000040	/* 剥离输入字符的第 8 位（仅保留 7 位 ASCII 码） */
#define INLCR	0000100	/* 将输入的换行符（NL）转换为回车符（CR） */
#define IGNCR	0000200	/* 忽略输入的回车符（CR） */
#define ICRNL	0000400	/* 将输入的回车符（CR）转换为换行符（NL） */
#define IUCLC	0001000	/* 将输入的大写字母转换为小写（部分系统未实现） */
#define IXON	0002000	/* 启用输出 XON/XOFF 流量控制（接收方控制发送方） */
#define IXANY	0004000	/* 任意字符均可恢复 XON 流量控制（不仅限于 VSTART） */
#define IXOFF	0010000	/* 启用输入 XON/XOFF 流量控制（发送方控制接收方） */
#define IMAXBEL	0020000	/* 输入队列满时触发蜂鸣（不丢弃多余字符） */

// -------------------------- c_oflag 输出模式标志 --------------------------
/* 控制输出数据的格式化规则，如换行转换、字符填充等 */
#define OPOST	0000001	/* 启用输出处理（关闭则输出原始数据，不格式化） */
#define OLCUC	0000002	/* 将输出的小写字母转换为大写（部分系统未实现） */
#define ONLCR	0000004	/* 将输出的换行符（NL）转换为 CR+NL 组合 */
#define OCRNL	0000010	/* 将输出的回车符（CR）转换为换行符（NL） */
#define ONOCR	0000020	/* 列 0 位置不输出回车符（CR） */
#define ONLRET	0000040	/* 换行符（NL）同时执行回车功能（无需额外 CR） */
#define OFILL	0000100	/* 使用填充字符替代延迟（用于低速终端） */
#define OFDEL	0000200	/* 填充字符为 DEL（0x7F），默认是 NUL（0x00） */
#define NLDLY	0000400	/* 换行延迟掩码（控制 NL 输出后的延迟时间） */
#define   NL0	0000000	/* 无换行延迟 */
#define   NL1	0000400	/* 换行延迟（对应系统预设的延迟时间） */
#define CRDLY	0003000	/* 回车延迟掩码 */
#define   CR0	0000000	/* 无回车延迟 */
#define   CR1	0001000	/* 短回车延迟 */
#define   CR2	0002000	/* 中回车延迟 */
#define   CR3	0003000	/* 长回车延迟 */
#define TABDLY	0014000	/* 制表符延迟掩码 */
#define   TAB0	0000000	/* 无制表符延迟 */
#define   TAB1	0004000	/* 制表符延迟 */
#define   TAB2	0010000	/* 用空格填充制表符（替代延迟） */
#define   TAB3	0014000	/* 制表符转换为空格（与 TAB2 类似） */
#define   XTABS	0014000	/* 等同于 TAB3（兼容旧系统） */
#define BSDLY	0020000	/* 退格延迟掩码 */
#define   BS0	0000000	/* 无退格延迟 */
#define   BS1	0020000	/* 退格延迟 */
#define VTDLY	0040000	/* 垂直制表符延迟掩码 */
#define   VT0	0000000	/* 无垂直制表符延迟 */
#define   VT1	0040000	/* 垂直制表符延迟 */
#define FFDLY	0040000	/* 换页延迟掩码（与 VTDLY 共用掩码位） */
#define   FF0	0000000	/* 无换页延迟 */
#define   FF1	0040000	/* 换页延迟 */

// -------------------------- c_cflag 控制模式标志 --------------------------
/* 控制终端硬件特性，如波特率、数据位、校验位等（硬件相关配置） */
#define CBAUD	0000017	/* 波特率掩码（提取波特率相关位） */
#define  B0	0000000		/* 波特率 0（挂断连接，关闭终端） */
#define  B50	0000001		/* 波特率 50 bps */
#define  B75	0000002		/* 波特率 75 bps */
#define  B110	0000003		/* 波特率 110 bps */
#define  B134	0000004		/* 波特率 134.5 bps */
#define  B150	0000005		/* 波特率 150 bps */
#define  B200	0000006		/* 波特率 200 bps */
#define  B300	0000007		/* 波特率 300 bps */
#define  B600	0000010		/* 波特率 600 bps */
#define  B1200	0000011		/* 波特率 1200 bps */
#define  B1800	0000012		/* 波特率 1800 bps */
#define  B2400	0000013		/* 波特率 2400 bps */
#define  B4800	0000014		/* 波特率 4800 bps */
#define  B9600	0000015		/* 波特率 9600 bps */
#define  B19200	0000016		/* 波特率 19200 bps */
#define  B38400	0000017		/* 波特率 38400 bps */
#define EXTA B19200		/* 扩展波特率 A（等同于 19200 bps，兼容旧代码） */
#define EXTB B38400		/* 扩展波特率 B（等同于 38400 bps，兼容旧代码） */
#define CSIZE	0000060	/* 数据位掩码（控制每个字符的位数） */
#define   CS5	0000000	/* 5 位数据位 */
#define   CS6	0000020	/* 6 位数据位 */
#define   CS7	0000040	/* 7 位数据位 */
#define   CS8	0000060	/* 8 位数据位（最常用，无校验时推荐） */
#define CSTOPB	0000100	/* 2 个停止位（默认 1 个，开启则为 2 个） */
#define CREAD	0000200	/* 启用接收（关闭则无法读取终端输入） */
#define CPARENB	0000400	/* 启用奇偶校验（发送/接收时添加/检查校验位） */
#define CPARODD	0001000	/* 奇校验（与 CPARENB 配合使用，默认偶校验） */
#define HUPCL	0002000	/* 关闭终端时挂断调制解调器（DTR 信号） */
#define CLOCAL	0004000	/* 忽略调制解调器状态线（本地终端，无需调制解调器） */
#define CIBAUD	03600000		/* 输入波特率掩码（部分系统未使用，由硬件自动适配） */
#define CRTSCTS	020000000000	/* 启用 RTS/CTS 硬件流量控制（握手信号） */

// 奇偶校验别名（兼容旧代码，与上面的 CPARENB/CPARODD 等价）
#define PARENB CPARENB	/* 启用奇偶校验（别名） */
#define PARODD CPARODD	/* 奇校验（别名） */

// -------------------------- c_lflag 本地模式标志 --------------------------
/* 控制终端本地交互行为，如回显、信号触发、规范模式等 */
#define ISIG	0000001	/* 启用终端产生的信号（如 VINTR、VSUSP 对应的信号） */
#define ICANON	0000002	/* 启用规范模式（行缓冲，按行读取，支持编辑功能） */
#define XCASE	0000004	/* 大小写转换（仅规范模式下有效，部分系统未实现） */
#define ECHO	0000010	/* 启用输入回显（输入字符后立即显示在终端） */
#define ECHOE	0000020	/* 擦除回显（配合 VERASE，删除字符时同时清除终端显示） */
#define ECHOK	0000040	/* 杀死回显（配合 VKILL，删除整行时输出换行） */
#define ECHONL	0000100	/* 回显换行符（即使 ECHO 关闭，也显示 NL） */
#define NOFLSH	0000200	/* 禁止信号触发的队列刷新（如 SIGINT 后不清除输入队列） */
#define TOSTOP	0000400	/* 后台进程尝试写终端时，发送 SIGTTOU 信号（停止进程） */
#define ECHOCTL	0001000	/* 回显控制字符（如 Ctrl+C 显示为 ^C） */
#define ECHOPRT	0002000	/* 擦除时回显删除路径（配合 VEOL2，部分系统未使用） */
#define ECHOKE	0004000	/* 杀死字符回显（将整行擦除，而非仅显示杀死字符） */
#define FLUSHO	0010000	/* 刷新输出模式（暂时丢弃输出队列，可通过 VDISCARD 切换） */
#define PENDIN	0040000	/* 重新处理输入队列（未读取的输入将被重新解析） */
#define IEXTEN	0100000	/* 启用扩展功能（如 VWERASE、VLNEXT 等扩展控制字符） */

// -------------------------- 调制解调器状态位宏 --------------------------
/* TIOCMGET/TIOCMSET 对应的调制解调器信号线状态（硬件握手信号） */
#define TIOCM_LE	0x001	/* 线路空闲（Line Enable） */
#define TIOCM_DTR	0x002	/* 数据终端就绪（Data Terminal Ready） */
#define TIOCM_RTS	0x004	/* 请求发送（Request To Send） */
#define TIOCM_ST	0x008	/* 次要传输状态（Secondary Transmit） */
#define TIOCM_SR	0x010	/* 次要接收状态（Secondary Receive） */
#define TIOCM_CTS	0x020	/* 清除发送（Clear To Send），RTS 的应答信号 */
#define TIOCM_CAR	0x040	/* 载波检测（Carrier Detect），调制解调器连接状态 */
#define TIOCM_RNG	0x080	/* 振铃指示（Ring Indicator），调制解调器振铃 */
#define TIOCM_DSR	0x100	/* 数据设备就绪（Data Set Ready），调制解调器就绪 */
#define TIOCM_CD	TIOCM_CAR	/* 载波检测别名（CD = Carrier Detect） */
#define TIOCM_RI	TIOCM_RNG	/* 振铃指示别名（RI = Ring Indicator） */

// -------------------------- tcflow/TCXONC 动作宏 --------------------------
/* tcflow() 函数和 TCXONC 命令的动作参数，控制流量控制 */
#define	TCOOFF		0	/* 暂停终端输出 */
#define	TCOON		1	/* 恢复终端输出 */
#define	TCIOFF		2	/* 向终端发送 STOP 字符（暂停输入） */
#define	TCION		3	/* 向终端发送 START 字符（恢复输入） */

// -------------------------- tcflush/TCFLSH 队列选择宏 --------------------------
/* tcflush() 函数和 TCFLSH 命令的队列选择参数，指定要刷新的 I/O 队列 */
#define	TCIFLUSH	0	/* 刷新输入队列（丢弃未读取的输入数据） */
#define	TCOFLUSH	1	/* 刷新输出队列（丢弃未发送的输出数据） */
#define	TCIOFLUSH	2	/* 同时刷新输入和输出队列 */

// -------------------------- tcsetattr 生效时机宏 --------------------------
/* tcsetattr() 函数的 optional_actions 参数，指定属性生效时机 */
#define	TCSANOW		0	/* 立即生效 */
#define	TCSADRAIN	1	/* 等待输出完成后生效（不打断当前输出） */
#define	TCSAFLUSH	2	/* 等待输出完成，且刷新输入队列后生效 */

// -------------------------- 波特率类型定义 --------------------------
typedef int speed_t;	/* 波特率类型（用于 cfgetispeed/cfsetispeed 等函数） */

// // -------------------------- 终端控制函数声明 --------------------------
// /* 获取终端输入波特率 */
// extern speed_t cfgetispeed(struct termios *termios_p);
// /* 获取终端输出波特率 */
// extern speed_t cfgetospeed(struct termios *termios_p);
// /* 设置终端输入波特率 */
// extern int cfsetispeed(struct termios *termios_p, speed_t speed);
// /* 设置终端输出波特率 */
// extern int cfsetospeed(struct termios *termios_p, speed_t speed);
// /* 等待终端输出队列排空（所有数据发送完成） */
// extern int tcdrain(int fildes);
// /* 控制终端流量（暂停/恢复输入/输出） */
// extern int tcflow(int fildes, int action);
// /* 刷新终端 I/O 队列（丢弃未读取/未发送的数据） */
// extern int tcflush(int fildes, int queue_selector);
// /* 获取终端属性（存储到 termios 结构体） */
// extern int tcgetattr(int fildes, struct termios *termios_p);
// /* 发送中断信号（break 信号，持续 duration 毫秒，0 表示无限期） */
// extern int tcsendbreak(int fildes, int duration);
// /* 设置终端属性（按 optional_actions 指定的时机生效） */
// extern int tcsetattr(int fildes, int optional_actions, struct termios *termios_p);

#endif
