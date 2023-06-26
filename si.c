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

/**
 * 文字列 LINE をコマンドライン上で編集させる
 */
void editLine(char line[MAX_LINE_LENGTH]) {
    int lineLength = strlen(line);
    int cursorPosition = lineLength;
    char command = '\0';

    enableRawMode();

    while (1) {
        char c;
        printf("\r%s \033[%dD", line, lineLength-cursorPosition+1);
        // printf("\n(%d, %d)",lineLength, cursorPosition);
        fflush(stdout);
        
        c = getchar();
        // printf("<%c %d %X>", c,c,c);

        if (c == '\x0a') break;     // Enter で終了

        if (c == KEY_ESCAPE) {      // (Esc)
            command = c; continue;
        } else if (command == KEY_ESCAPE && c == '[') {  // (Esc-[)
            command = c; continue;
        } else if (command == '[') {
            if (c == 'C' && cursorPosition < lineLength) {   // Esc-[-C カーソルを右に移動
                cursorPosition++;     
            } else if (c == 'D' && cursorPosition > 0) {     // Esc-[-D カーソルを左に移動
                cursorPosition--;
            }
            command = '\0'; continue;
        } else {
            command = '\0';
        }

        if (c == KEY_BACKSPACE) {  // バックスペースキーで文字を削除
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

    disableRawMode();
    printf("\n");
}

int main() {
    char line[MAX_LINE_LENGTH] = "Hello, world!";
    editLine(line);
    printf("結果：\"%s\"\n", line);
    return 0;
}