#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define KEY_ESCAPE '\x1b'
#define KEY_BACKSPACE '\x7f'

struct termios orig_termios;

/**
 * ターミナルの設定を変更する
 */
void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/**
 * ターミナルの設定を元に戻す
 */
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

enum SIMode { NORMAL, INSERT };

/**
 * 文字列エディタをコマンドライン上に描画する
 */
void showEditor(enum SIMode mode, char line[MAX_LINE_LENGTH], int cursorPos) {
    /* 1行目を描画 */
    printf("\r\033[2K");    /*クリア*/
    printf("%s \n", line);
    /* 2行目を描画 */
    printf("\033[2K");      /*クリア*/
    if (mode == INSERT) printf("\e[1m-- INSERT --\e[m");    /*太字*/
    /* カーソルを1行上の先頭に移動 */
    printf("\033[1A\r");
    /* cursorPos 回カーソルを進める */
    if (cursorPos) printf("\033[%dC", cursorPos);

    fflush(stdout);
}

/**
 * 文字列 LINE をコマンドライン上で編集させる
 * LINE がマルチバイト文字を含む場合は 1 を返す
 */
int editLine(char line[MAX_LINE_LENGTH]) {
    int lineLength = strlen(line);
    int cursorPos = lineLength;
    char command = '\0';
    enum SIMode mode = INSERT;
    int insertFlg = -1;
    char* p;

    p = line;
    while (*p) {
        if (*p & 0x80) return 1;
        p++;
    } 

    enableRawMode();

    while (1) {
        char c;
        showEditor(mode, line, cursorPos);
        if (cursorPos < 0 || cursorPos > lineLength) printf("INVALID %d", cursorPos);
        
        c = getchar();

        /* Enter で終了 */
        if (c == '\x0a') break;
        
        /* ノーマルモードのキーバインド */
        if (mode == NORMAL) {
            int i;

            if (c == '[') {
                command = c; continue;
            } else if (command == '[') {
                int lastPos = lineLength-1;
                if (insertFlg+1) {    /*挿入モードで矢印キーを押した*/
                    cursorPos = insertFlg;
                    lastPos++;
                    mode = INSERT;
                    insertFlg = -1;
                }
                /* Esc-[-C (→) カーソルを1つ右に移動 */
                if (c == 'C' && cursorPos < lastPos) {
                    cursorPos++;
                /* Esc-[-D (←) カーソルを1つ左に移動 */
                } else if (c == 'D' && cursorPos > 0) {
                    cursorPos--;
                }
                command = '\0'; continue;
            }
            command = '\0'; insertFlg = -1;

            switch (c) {
            /* モード切り替え */
            case 'i':   /* 挿入モード(前) */
                mode = INSERT; break;

            case 'a':   /* 挿入モード(後) */
                mode = INSERT; cursorPos++; break;

            case 'I':   /* 挿入モード(行頭) */
                mode = INSERT; cursorPos = 0; break;

            case 'A':   /* 挿入モード(行末) */
                mode = INSERT; cursorPos = lineLength; break;

            /* カーソル移動 */
            case 'h':   /* カーソルを1つ左へ */
                if (cursorPos > 0) cursorPos--;
                break;

            case 'l':   /* カーソルを1つ左へ */
                if (cursorPos < lineLength-1) cursorPos++;
                break;

            case '0':   /* カーソルを行頭へ */
            case '^':
                cursorPos = 0;
                break;

            case '$':   /* カーソルを行末へ */
                cursorPos = lineLength-1;
                break;

            case 'w':   /* カーソルをCSV次列先頭へ */
                while (cursorPos < lineLength-1 && line[cursorPos++] != ',');
                break;

            case 'e':   /* カーソルをCSV次列末尾へ */
                if (cursorPos >= lineLength-1) break;
                while (++cursorPos < lineLength-1 && line[cursorPos+1] != ',');
                break;

            case 'b':   /* カーソルをCSV前列先頭へ */
                if (cursorPos <= 0) break;
                while (--cursorPos > 0 && line[cursorPos-1] != ',');
                break;

            /* 編集 */
            case 'x':   /* 文字を削除 */
                for (i=cursorPos; i < lineLength-1; i++) line[i] = line[i+1];
                if (cursorPos > 0 && cursorPos == lineLength-1) cursorPos--;
                if (lineLength > 0) line[--lineLength] = '\0';
                break;

            default:
                break;
            }

        /* 挿入モードのキーバインド */
        } else if (mode == INSERT) {
            /* Esc Ctrl-[ ノーマルモード */
            if (c == KEY_ESCAPE) {
                mode = NORMAL; insertFlg = cursorPos;
                if (cursorPos > 0) cursorPos--;
                continue;

            /* BackSpace 文字を削除 */
            } else if (c == KEY_BACKSPACE) {
                int i;
                if (cursorPos <= 0) continue;
                for (i=cursorPos; i < lineLength; i++) line[i-1] = line[i];
                line[lineLength-1] = '\0';
                cursorPos--;
                lineLength--;

            /* 表示可能なASCII文字を挿入 */
            } else if (c >= '\x20' && c <= '\x7E') {
                int i;
                if (lineLength >= MAX_LINE_LENGTH - 1) continue;
                for (i=lineLength; i > cursorPos; i--) line[i] = line[i-1];
                line[cursorPos] = c;
                cursorPos++;
                lineLength++;
            }
        }
    }

    disableRawMode();
    printf("\n");
    return 0;
}

int main() {
    char line[MAX_LINE_LENGTH] = "Hello, world!";
    if (editLine(line))
        printf("ERROR\n");
    else
        printf("結果：\"%s\"\n", line);
    return 0;
}

/* ================================[Safe Width]================================= */
