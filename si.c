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
    // 1行目を描画
    printf("\r\033[2K");    // クリア
    printf("%s \n", line);
    // 2行目を描画
    if (mode == INSERT) printf("\e[1m-- INSERT --\e[m");    // 太字
    else                printf("\033[2K");  // クリア
    // カーソルを1行上の先頭に移動
    printf("\033[1A\r");
    // cursorPos 回カーソルを進める
    if (cursorPos) printf("\033[%dC", cursorPos);

    fflush(stdout);
}

/**
 * 文字列 LINE をコマンドライン上で編集させる
 */
void editLine(char line[MAX_LINE_LENGTH]) {
    int lineLength = strlen(line);
    int cursorPos = lineLength;
    char command = '\0';
    enum SIMode mode = INSERT;
    int insertFlg = 0;

    enableRawMode();

    while (1) {
        char c;
        showEditor(mode, line, cursorPos);
        // printf("\n(%d, %d)",lineLength, cursorPos);
        
        
        c = getchar();
        // printf("<%c %d %X>", c,c,c);

        if (c == '\x0a') break;     // Enter で終了
        
        /* ノーマルモードのキーバインド */
        if (mode == NORMAL) {
            if (c == '[') {
                command = c; continue;
            } else if (command == '[') {
                int lastPos = insertFlg ? lineLength : lineLength-1;
                if (insertFlg) {    // INSERTモードで矢印を押した
                    cursorPos++;
                    mode = INSERT;
                }
                if (c == 'C' && cursorPos < lastPos) {  // Esc-[-C カーソルを右に移動
                    cursorPos++;     
                } else if (c == 'D' && cursorPos > 0) { // Esc-[-D カーソルを左に移動
                    cursorPos--;
                }
                command = '\0'; continue;
            } else {
                command = '\0'; insertFlg = 0;
            }

            if (c == 'i') {      // i 挿入モード（前）
                mode = INSERT;
            } else if (c == 'a') {      // i 挿入モード（前）
                mode = INSERT;
                cursorPos++;
            } else if (c == 'I') {      // I 挿入モード（行頭）
                mode = INSERT;
                cursorPos = 0;
            } else if (c == 'A') {      // A 挿入モード（行末）
                mode = INSERT;
                cursorPos = lineLength;

            } else if (c == 'h') {      // h カーソルを1つ左へ
                if (cursorPos > 0) cursorPos--;
            } else if (c == 'l') {      // l カーソルを1つ左へ
                if (cursorPos < lineLength-1) cursorPos++;
            } else if (c == '0' || c == '^') {  // 0 ^ カーソルを行頭へ
                cursorPos = 0;
            } else if (c == '$') {      // $ カーソルを行頭へ
                cursorPos = lineLength-1;
            } else if (c == 'w') {      // w CSV次列へ
                while (cursorPos < lineLength-1 && line[cursorPos++] != ',');
            } else if (c == 'b') {      // w CSV前列へ
                int tmp = cursorPos;
                while (cursorPos > 1 && line[--cursorPos-1] != ',');
                if ((line[0] != ',' && cursorPos == 1) ||
                    (line[0] == ',' && tmp == cursorPos)) cursorPos--;
            }

        /* 挿入モードのキーバインド */
        } else if (mode == INSERT) {
            if (c == KEY_ESCAPE || c == '\x1b') {     // Esc Ctrl-[ ノーマルモード
                mode = NORMAL; insertFlg = 1;
                cursorPos--;
                continue;

            } else if (c == KEY_BACKSPACE) {    // バックスペースキーで文字を削除
                if (cursorPos > 0) {
                    int i;
                    for (i=cursorPos; i < lineLength; i++) line[i-1] = line[i];
                    line[lineLength-1] = '\0';
                    cursorPos--;
                    lineLength--;
                }

            } else if (c >= '\x20' && c <= '\x7E') {  // 表示可能なASCII文字が押されたら文字を挿入
                if (lineLength < MAX_LINE_LENGTH - 1) {
                    int i;
                    for (i=lineLength; i > cursorPos; i--) line[i] = line[i-1];
                    line[cursorPos] = c;
                    cursorPos++;
                    lineLength++;
                }
            }
        }
    }

    disableRawMode();
    printf("\n");
}

int main() {
    char line[MAX_LINE_LENGTH] = "Hello, world!";
    editLine(line);
    printf("結果：\"%s\"\n", line);
    return 0;
}