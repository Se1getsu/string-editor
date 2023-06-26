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
void showEditor(enum SIMode mode, char line[MAX_LINE_LENGTH], int cursorPosition) {
    // 1行目を描画
    printf("\r\033[2K");    // クリア
    printf("%s \n", line);
    // 2行目を描画
    if (mode == INSERT) printf("\e[1m-- INSERT --\e[m");    // 太字
    else                printf("\033[2K");  // クリア
    // カーソルを1行上の先頭に移動
    printf("\033[1A\r");
    // cursorPosition 回カーソルを進める
    if (cursorPosition) printf("\033[%dC", cursorPosition);

    fflush(stdout);
}

/**
 * 文字列 LINE をコマンドライン上で編集させる
 */
void editLine(char line[MAX_LINE_LENGTH]) {
    int lineLength = strlen(line);
    int cursorPosition = lineLength;
    char command = '\0';
    enum SIMode mode = INSERT;
    int insertFlg = 0;

    enableRawMode();

    while (1) {
        char c;
        showEditor(mode, line, cursorPosition);
        // printf("\n(%d, %d)",lineLength, cursorPosition);
        
        
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
                    cursorPosition++;
                    mode = INSERT;
                }
                if (c == 'C' && cursorPosition < lastPos) {     // Esc-[-C カーソルを右に移動
                    cursorPosition++;     
                } else if (c == 'D' && cursorPosition > 0) {    // Esc-[-D カーソルを左に移動
                    cursorPosition--;
                }
                command = '\0'; continue;
            } else {
                command = '\0'; insertFlg = 0;
            }

            if (c == 'i') {      // i 挿入モード（前）
                mode = INSERT;
            } else if (c == 'a') {      // i 挿入モード（前）
                mode = INSERT;
                cursorPosition++;
            } else if (c == 'I') {      // I 挿入モード（行頭）
                mode = INSERT;
                cursorPosition = 0;
            } else if (c == 'A') {      // A 挿入モード（行末）
                mode = INSERT;
                cursorPosition = lineLength;

            } else if (c == 'h') {      // h カーソルを1つ左へ
                if (cursorPosition > 0) cursorPosition--;
            } else if (c == 'l') {      // l カーソルを1つ左へ
                if (cursorPosition < lineLength-1) cursorPosition++;
            } else if (c == '0' || c == '^') {  // 0 ^ カーソルを行頭へ
                cursorPosition = 0;
            } else if (c == '$') {      // $ カーソルを行頭へ
                cursorPosition = lineLength-1;
            } else if (c == 'w') {      // w CSV次列へ
                while (cursorPosition < lineLength-1 && line[cursorPosition++] != ',');
            } else if (c == 'b') {      // w CSV前列へ
                int tmp = cursorPosition;
                while (cursorPosition > 1 && line[--cursorPosition-1] != ',');
                if ((line[0] != ',' && cursorPosition == 1) ||
                    (line[0] == ',' && tmp == cursorPosition)) cursorPosition--;
            }

        /* 挿入モードのキーバインド */
        } else if (mode == INSERT) {
            if (c == KEY_ESCAPE || c == '\x1b') {     // Esc Ctrl-[ ノーマルモード
                mode = NORMAL; insertFlg = 1;
                cursorPosition--;
                continue;

            } else if (c == KEY_BACKSPACE) {    // バックスペースキーで文字を削除
                if (cursorPosition > 0) {
                    int i;
                    for (i=cursorPosition; i < lineLength; i++) line[i-1] = line[i];
                    line[lineLength-1] = '\0';
                    cursorPosition--;
                    lineLength--;
                }

            } else if (c >= '\x20' && c <= '\x7E') {  // 表示可能なASCII文字が押されたら文字を挿入
                if (lineLength < MAX_LINE_LENGTH - 1) {
                    int i;
                    for (i=lineLength; i > cursorPosition; i--) line[i] = line[i-1];
                    line[cursorPosition] = c;
                    cursorPosition++;
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