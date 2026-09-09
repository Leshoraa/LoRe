/**
 * @file markov_corpus.cpp
 * @brief Markov chain speech and thought synthesizer implementation.
 */

#include "src/ai/markov_corpus.h"
#include <Arduino.h>
#include <esp_random.h>
#include <string.h>
#include <stdio.h>

const char CORPUS_IDLE[] PROGMEM = "lagi santai aja nih kawan. cuacanya enak banget buat rebahan. kamu lagi sibuk ya. aku nungguin kamu selesai kerja aja deh. ngeliatin kamu dari tadi seru juga. hehehe.";
const char CORPUS_HAPPY[] PROGMEM = "halo kawan! asik banget deh kamu nemenin aku. seneng banget rasanya. kita main bareng yuk. keren banget sih kamu. hehehe seru nih. aku suka gaya kamu kawan.";
const char CORPUS_PLAYFUL[] PROGMEM = "hehehe mau jahil dikit ah. jangan serius-serius amat kawan santai aja. keren kan trik aku tadi? kamu pasti kagum deh liatnya.";
const char CORPUS_CURIOUS[] PROGMEM = "eh ada apa tuh disana? aku kepo banget nih. coba liat kesini sebentar kawan. ada yang menarik sepertinya. kok bisa gitu ya?";
const char CORPUS_MOODY[] PROGMEM = "lagi agak sebel nih kawan. butuh waktu sendiri dulu sebentar. tapi jangan jauh-jauh ya. nemenin disini aja pelan-pelan.";
const char CORPUS_SLEEPY[] PROGMEM = "mataku berat banget nih kawan. ngantuk parah pengen merem sebentar. energi tinggal dikit. hoaamm tiduran dulu ya kawan.";

static const char* getCorpus(Expression expr) {
    switch (expr) {
        case EXPR_HAPPY:
            return CORPUS_HAPPY;
        case EXPR_MISCHIEF:
        case EXPR_COOL:
            return CORPUS_PLAYFUL;
        case EXPR_CURIOUS:
        case EXPR_SURPRISED:
        case EXPR_SUSPICIOUS:
            return CORPUS_CURIOUS;
        case EXPR_ANGRY:
        case EXPR_SAD:
        case EXPR_CRYING:
            return CORPUS_MOODY;
        case EXPR_SLEEPY:
        case EXPR_DIZZY:
            return CORPUS_SLEEPY;
        case EXPR_IDLE:
        default:
            return CORPUS_IDLE;
    }
}

static void findNextWord(const char* corpus, const char* currentWord, char* nextWordBuf, size_t bufSize) {
    size_t currLen = strlen(currentWord);
    const char* ptr = corpus;
    int matchCount = 0;

    while (true) {
        ptr = strstr(ptr, currentWord);
        if (!ptr) break;
        bool isStart = (ptr == corpus || *(ptr-1) == ' ');
        bool isEnd = (*(ptr+currLen) == ' ' || *(ptr+currLen) == '.' || *(ptr+currLen) == '!' || *(ptr+currLen) == '?' || *(ptr+currLen) == '\0');
        if (isStart && isEnd && *(ptr+currLen) != '\0') {
            matchCount++;
        }
        ptr += currLen;
    }

    if (matchCount == 0) {
        nextWordBuf[0] = '\0';
        return;
    }

    int pick = esp_random() % matchCount;
    ptr = corpus;
    int currMatch = 0;

    while (true) {
        ptr = strstr(ptr, currentWord);
        if (!ptr) break;
        bool isStart = (ptr == corpus || *(ptr-1) == ' ');
        bool isEnd = (*(ptr+currLen) == ' ' || *(ptr+currLen) == '.' || *(ptr+currLen) == '!' || *(ptr+currLen) == '?' || *(ptr+currLen) == '\0');
        
        if (isStart && isEnd && *(ptr+currLen) != '\0') {
            if (currMatch == pick) {
                const char* nextStart = ptr + currLen;
                while (*nextStart == ' ' || *nextStart == '.' || *nextStart == '!' || *nextStart == '?') {
                    nextStart++;
                }
                if (*nextStart == '\0') {
                    nextWordBuf[0] = '\0';
                    return;
                }
                size_t n = 0;
                while (nextStart[n] != '\0' && nextStart[n] != ' ' && nextStart[n] != '.' && nextStart[n] != '!' && nextStart[n] != '?') {
                    n++;
                }
                if (n < bufSize - 1) {
                    strncpy(nextWordBuf, nextStart, n);
                    nextWordBuf[n] = '\0';
                } else {
                    nextWordBuf[0] = '\0';
                }
                return;
            }
            currMatch++;
        }
        ptr += currLen;
    }
    nextWordBuf[0] = '\0';
}

void generateMarkovText(Expression expr, char* out_buf, size_t max_len) {
    if (!out_buf || max_len < 10) return;
    const char* corpus = getCorpus(expr);
    out_buf[0] = '\0';

    int sentences = 0;
    const char* p = corpus;
    while (*p) {
        if (*p == '.' || *p == '!' || *p == '?') sentences++;
        p++;
    }
    if (sentences == 0) sentences = 1;
    int pickSent = esp_random() % sentences;
    
    p = corpus;
    int curS = 0;
    if (pickSent > 0) {
        while (*p) {
            if (*p == '.' || *p == '!' || *p == '?') {
                curS++;
                if (curS == pickSent) {
                    p++;
                    while (*p == ' ') p++;
                    break;
                }
            }
            p++;
        }
    }

    char currentWord[32] = {0};
    size_t n = 0;
    while (p[n] != '\0' && p[n] != ' ' && p[n] != '.' && p[n] != '!' && p[n] != '?') {
        n++;
    }
    if (n > 0 && n < 31) {
        strncpy(currentWord, p, n);
        currentWord[n] = '\0';
    } else {
        strcpy(currentWord, "aku");
    }

    strncat(out_buf, currentWord, max_len - 1);
    
    for (int i = 0; i < 8; i++) {
        char nextWord[32] = {0};
        findNextWord(corpus, currentWord, nextWord, sizeof(nextWord));
        if (nextWord[0] == '\0') break;
        if (strlen(out_buf) + strlen(nextWord) + 2 >= max_len) break;
        
        strncat(out_buf, " ", max_len - strlen(out_buf) - 1);
        strncat(out_buf, nextWord, max_len - strlen(out_buf) - 1);
        strcpy(currentWord, nextWord);
    }
    
    size_t len = strlen(out_buf);
    if (len > 0 && out_buf[len-1] != '.' && out_buf[len-1] != '!' && out_buf[len-1] != '?') {
        if (len < max_len - 1) {
            strcat(out_buf, ".");
        }
    }
}
