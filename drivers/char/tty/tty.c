#include<stdint.h>
#include<tty.h>
#include<vga.h>

tty_t tty;

// 空字符/无意义字符（ASCII 0）
static void ctrl_null(tty_t *tty, int8_t c) 
{
    // 可忽略，或作为 Break 条件处理（参考之前的 BRKINT 逻辑）
    if (I_BRKINT(tty)) {
        // send_signal(current_process, SIGINT);
    }
}

// 光标移动（Ctrl+A/B/E/F：ASCII 1/2/5/6）
static void ctrl_cursor_move(tty_t *tty, int8_t c) 
{
    switch (c) {
        case 0x01: // Ctrl+A：光标移到行首
            // 输出终端控制序列：\033[H（光标移到左上角）
            PUTCH_TAIL(&tty->write, '\033');
            PUTCH_TAIL(&tty->write, '[');
            PUTCH_TAIL(&tty->write, 'H');
            break;
        case 0x02: // Ctrl+B：光标左移 1 位
            PUTCH_TAIL(&tty->write, '\b');
            break;
        case 0x05: // Ctrl+E：光标移到行尾
            // 输出终端控制序列：\033[K（清除从当前位置到行尾的内容）
            // 并移动到行尾
            PUTCH_TAIL(&tty->write, '\033');
            PUTCH_TAIL(&tty->write, '[');
            PUTCH_TAIL(&tty->write, 'K');
            break;
        case 0x06: // Ctrl+F：光标右移 1 位
            // 输出终端控制序列：\033[C（光标右移）
            PUTCH_TAIL(&tty->write, '\033');
            PUTCH_TAIL(&tty->write, '[');
            PUTCH_TAIL(&tty->write, 'C');
            break;
    }
    tty->operation.write(tty);
}

// 信号触发（Ctrl+C/Z/\：ASCII 3/26/28）
static void ctrl_signal(tty_t *tty, int8_t c) 
{
    if (!L_ISIG(tty)) return; // 需启用 ISIG 标志

    switch (c) {
        case 0x03: // Ctrl+C：SIGINT（中断）
            // send_signal(current_process, SIGINT);
            break;
        case 0x1A: // Ctrl+Z：SIGTSTP（挂起）
            // send_signal(current_process, SIGTSTP);
            break;
        case 0x1C: // Ctrl+\：SIGQUIT（退出）
            // send_signal(current_process, SIGQUIT);
            break;
    }
}

// EOF（Ctrl+D：ASCII 4）
static void ctrl_eof(tty_t *tty, int8_t c) 
{
    

    // 规范模式：提交缓冲区数据到主接收缓冲区，唤醒等待进程
    if (L_ECHO(tty)) {
        PUTCH_TAIL(&tty->write, '^');
        PUTCH_TAIL(&tty->write, 'D');
        tty->operation.write(tty);
    }
    //wakeup(&tty->read.wait_head);
}

// 响铃（Ctrl+G：ASCII 7）
static void ctrl_bell(tty_t *tty, int8_t c) 
{
    PUTCH_TAIL(&tty->write, 0x07); // 输出 BEL 字符（响铃）
    tty->operation.write(tty);
}

//退格（Ctrl+H/Backspace：ASCII 8）
static void ctrl_backspace(tty_t *tty, int8_t c) {

    if (!EMPTY(&tty->read)) 
    {
        DELCH_TAIL(&tty->read);
    }
    

    if (L_ECHO(tty)) 
    {
        PUTCH_TAIL(&tty->write, '\b');
        PUTCH_TAIL(&tty->write, ' ');
        PUTCH_TAIL(&tty->write, '\b');
        tty->operation.write(tty);
    }
}

// 制表符（Ctrl+I/Tab：ASCII 9）
static void ctrl_tab(tty_t *tty, int8_t c) 
{
    // 按 TTY 输出模式配置处理制表符（O_TABDLY/XTABS）
    if (O_XTABS(tty)) {
        // 扩展制表符：用 8 个空格替换
        int i=0;
        for (i; i < 8; i++) {
            PUTCH_TAIL(&tty->write, ' ');
        }
    } else {
        PUTCH_TAIL(&tty->write, '\t'); // 直接输出制表符
    }
    tty->operation.write(tty);

    // 存入读取缓冲区
    PUTCH_TAIL(&tty->read, c);
}

// 换行/回车（Ctrl+J/M：ASCII 10/13）
static void ctrl_newline(tty_t *tty, int8_t c) 
{
    PUTCH_TAIL(&tty->read, c);
    tty->read.data++;
    if (L_ECHO(tty)) 
    {
        PUTCH_TAIL(&tty->write, '\n');
        tty->operation.write(tty);
    }
    //wakeup(&tty->read.wait_head);
}

// 清屏（Ctrl+L：ASCII 12）
static void ctrl_clear_screen(tty_t *tty, int8_t c) 
{
    PUTCH_TAIL(&tty->write, '\033');
    PUTCH_TAIL(&tty->write, '[');
    PUTCH_TAIL(&tty->write, '2');
    PUTCH_TAIL(&tty->write, 'J');
    PUTCH_TAIL(&tty->write, '\033');
    PUTCH_TAIL(&tty->write, '[');
    PUTCH_TAIL(&tty->write, 'H');
    tty->operation.write(tty);
}

// 大小写切换（Ctrl+O/N：ASCII 14/15）
static void ctrl_case_switch(tty_t *tty, int8_t c) 
{
    if (c == 0x0E) 
    {
        tty->termios.c_iflag |= IUCLC;
    } 
    else if (c == 0x0F) 
    {
        tty->termios.c_iflag &= ~IUCLC;
    }
}

// 软件流控（Ctrl+Q/S：ASCII 17/19）
static void ctrl_flow_ctrl(tty_t *tty, int8_t c) 
{
    if (!I_IXON(tty)) return; // 需启用 IXON 标志

    static int xoff_sent = 0;
    if (c == 0x11) { // Ctrl+Q：XON，恢复发送
        xoff_sent = 0;
    } else if (c == 0x13) { // Ctrl+S：XOFF，暂停发送
        xoff_sent = 1;
    }
}

// 行编辑（Ctrl+U/W/_：ASCII 21/23/31）
static void ctrl_line_edit(tty_t *tty, int8_t c) {
    switch (c) 
    {

        case 0x15:
            while (!EMPTY(&tty->read)) 
            {
                DELCH_TAIL(&tty->read);
                
            }
            break;
        case 0x17:
            while (!EMPTY(&tty->read) &&LAST(&tty->read) != ' ' &&LAST(&tty->read) != '\n') {
                DELCH_TAIL(&tty->read);
            }
            break;
        case 0x1F:
            while (!EMPTY(&tty->read) &&LAST(&tty->read) != '\n') 
            {
                DELCH_TAIL(&tty->read);
            }
            break;
    }

}

// 粘贴模式（Ctrl+V：ASCII 22）
static void ctrl_paste(tty_t *tty, int8_t c) 
{

}

// 取消操作（Ctrl+X：ASCII 24）
static void ctrl_cancel(tty_t *tty, int8_t c) 
{

    while (!EMPTY(&tty->read)) 
    {
        DELCH_TAIL(&tty->read);
    }

    if (L_ECHO(tty)) 
    {
        PUTCH_TAIL(&tty->write, '\n');
        tty->operation.write(tty);
    }
}

// 撤销擦除（Ctrl+Y：ASCII 25）
static void ctrl_undo_erase(tty_t *tty, int8_t c) 
{
    
}

//  转义序列（Ctrl+[/Esc：ASCII 27）
static void ctrl_escape(tty_t *tty, int8_t c) 
{
    // 终端转义序列起始（如颜色控制、光标移动），需后续解析后续字符
    // 此处简化为忽略，实际驱动需实现转义序列解析器
}

// 保留字符（未使用功能：ASCII 16/18/20/29/30）
static void ctrl_reserved(tty_t *tty, int8_t c) 
{
    // 预留扩展，当前忽略
}

typedef void(*ctrl_func_t)(tty_t*tty,int8_t c);


// 控制字符处理函数数组：索引 = ASCII 码（0~31），值 = 对应处理函数
static ctrl_func_t ctrl_handlers[32] = {
    ctrl_null,          // 0: Ctrl+@
    ctrl_cursor_move,   // 1: Ctrl+A
    ctrl_cursor_move,   // 2: Ctrl+B
    ctrl_signal,        // 3: Ctrl+C
    ctrl_eof,           // 4: Ctrl+D
    ctrl_cursor_move,   // 5: Ctrl+E
    ctrl_cursor_move,   // 6: Ctrl+F
    ctrl_bell,          // 7: Ctrl+G
    ctrl_backspace,     // 8: Ctrl+H/Backspace
    ctrl_tab,           // 9: Ctrl+I/Tab
    ctrl_newline,       // 10: Ctrl+J
    ctrl_reserved,      // 11: Ctrl+K（垂直制表符，保留）
    ctrl_clear_screen,  // 12: Ctrl+L
    ctrl_newline,       // 13: Ctrl+M/Enter
    ctrl_case_switch,   // 14: Ctrl+N
    ctrl_case_switch,   // 15: Ctrl+O
    ctrl_reserved,      // 16: Ctrl+P（DLE，保留）
    ctrl_flow_ctrl,     // 17: Ctrl+Q（XON）
    ctrl_reserved,      // 18: Ctrl+R（DC2，保留）
    ctrl_flow_ctrl,     // 19: Ctrl+S（XOFF）
    ctrl_reserved,      // 20: Ctrl+T（DC4，保留）
    ctrl_line_edit,     // 21: Ctrl+U
    ctrl_paste,         // 22: Ctrl+V
    ctrl_line_edit,     // 23: Ctrl+W
    ctrl_cancel,        // 24: Ctrl+X
    ctrl_undo_erase,    // 25: Ctrl+Y
    ctrl_signal,        // 26: Ctrl+Z
    ctrl_escape,        // 27: Ctrl+[/Esc
    ctrl_signal,        // 28: Ctrl+\ 
    ctrl_reserved,      // 29: Ctrl+]（GS，保留）
    ctrl_reserved,      // 30: Ctrl+^（RS，保留）
    ctrl_line_edit      // 31: Ctrl+_（US）
};

// 字符处理函数，根据模式调用不同的处理逻辑
void tty_handle_char(tty_t *tty, int8_t c) 
{
    
    if (I_IUCLC(tty) && c >= 'A' && c <= 'Z') 
    {
        c += 32;
    }

    if (c < 32) 
    {
        if (!L_ICANON(tty)) 
        {
            PUTCH_TAIL(&tty->read, c);
            //wakeup(&tty->read.wait_head);
            return;
        }
        ctrl_handlers[c](tty, c);
    } 
    else if (c == 127) 
    {
        ctrl_backspace(tty, c);
    }
    else 
    {
        if (I_INLCR(tty) && c == '\n') 
        {
            c = '\r';
        } 
        else if (I_ICRNL(tty) && c == '\r') 
        {
            c = '\n';
        } 
        else if (I_IGNCR(tty) && c == '\r') 
        {
            return;
        }

        if (L_ICANON(tty)) 
        {
            PUTCH_TAIL(&tty->read, c);
            if (L_ECHO(tty)) {
                if (O_OLCUC(tty) && c >= 'a' && c <= 'z') 
                {
                    PUTCH_TAIL(&tty->write, c - 32);
                } 
                else 
                {
                    PUTCH_TAIL(&tty->write, c);
                }
                tty->operation.write(tty);
            }
        } 
        else 
        {
            PUTCH_TAIL(&tty->read, c);
            if (L_ECHO(tty)) 
            {
                if (O_OLCUC(tty) && c >= 'a' && c <= 'z') 
                {
                    PUTCH_TAIL(&tty->write, c - 32);
                } else {
                    PUTCH_TAIL(&tty->write, c);
                }
                tty->operation.write(tty);
            }
            //wakeup(&tty->read.wait_head);
        }
    }
}


void tty_init()
{
    tty.operation.write=vga_write;
    tty.termios.c_iflag=ICRNL;
    tty.termios.c_oflag=OPOST|ONLCR;
    tty.termios.c_cflag=0;
    tty.termios.c_lflag=ISIG | ICANON | ECHO | ECHOCTL | ECHOKE;

    cursor_set_pos(ypos,xpos);
    cursor_enable();

}