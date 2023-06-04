#include "client.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BUF_MAX 1024

void connector_new(connector *self) {
    memset(self, 0, sizeof(connector));
}

void connector_destructor(connector *self) {
    if (self->destructor != NULL && self->p != NULL) {
        self->destructor(self->p);
    }
}

// Contains protocol-agnostic logic of `client`.
// e.g. Input serialization, server response parsing, error handling.
typedef struct client_impl_t {
    char buf[BUF_MAX]; // buffer for serialization
} client_impl;

// Creates a new `client_impl`.
// Returns 0 on success, -1 on failure.
int client_impl_new(client_impl *self) {
    memset(self->buf, 0, BUF_MAX);

    return 0;
}

void client_impl_destructor(__attribute__((unused)) client_impl *self) {
    // do nothing
}

int client_impl_initial_prompt(client_impl *self) {
    memset(self->buf, 0, BUF_MAX);

    printf(">>>Ready to start game? (y/n): ");
    scanf("%s", self->buf);
    self->buf[strcspn(self->buf, "\n")] = 0;

    const char *y_str = "y";
    if (strcmp(self->buf, y_str) != 0) {
        return 1;
    }

    return 0;
}

char client_impl_scan_guess(client_impl *self) {
    while (1) {
        memset(self->buf, 0, BUF_MAX);

        printf(">>>Letter to guess: ");
        scanf("%s", self->buf);
        self->buf[strcspn(self->buf, "\n")] = 0;

        if (strlen(self->buf) < 1) { // Ctrl+D!!
            return -1;
        }

        if (strlen(self->buf) == 1) {
            if (self->buf[0] >= 'A' && self->buf[0] <= 'Z') {
                break;
            }
            if (self->buf[0] >= 'a' && self->buf[0] <= 'z') {
                break;
            }
        }

        // two letters
        printf(">>>Error! Please guess one letter.\n");
    }

    return self->buf[0];
}

client *client_new(connector c) {
    client_impl *p = malloc(sizeof(client_impl));
    if (p == NULL) return NULL;
    client_impl_new(p);

    client *self = malloc(sizeof(client));
    if (self == NULL) {
        free(p);
        return NULL;
    }

    self->c = c;
    self->p = p;

    return self;
}

void client_destructor(client *self) {
    connector_destructor(&self->c);

    if (self->p == NULL) return;

    client_impl_destructor(self->p);
    free(self->p);
    self->p = NULL;
}

typedef struct control_msg_t {
    const char *word;
    char word_len;

    const char *wrong_guesses;
    char wrong_guesses_len;
} control_msg;

control_msg *client_recv_control_msg(client *self) {
    char word_len;
    {
        const char *msg = self->c.recv_func(self->c.p, 1); // word len
        if (msg == NULL) {
            printf("recv word len failed");
            return NULL;
        }
        word_len = msg[0];
    }

    char wrong_guesses_len;
    {
        const char *msg = self->c.recv_func(self->c.p, 1); // wrong guesses
        if (msg == NULL) {
            printf("recv wrong guesses failed");
            return NULL;
        }
        wrong_guesses_len = msg[0];
    }

    char *word = malloc(word_len + 1);
    {
        const char *msg = self->c.recv_func(self->c.p, word_len); // word
        if (msg == NULL) {
            printf("recv word failed");
            return NULL;
        }
        strcpy(word, msg);
        word[word_len] = 0;
    }

    char *wrong_guesses_str = NULL;
    if (wrong_guesses_len > 0) {
        wrong_guesses_str = malloc(wrong_guesses_len + 1);

        {
            const char *msg = self->c.recv_func(self->c.p, wrong_guesses_len); // wrong guesses
            if (msg == NULL) {
                printf("recv wrong guesses failed");
                return NULL;
            }
            strcpy(wrong_guesses_str, msg);
            wrong_guesses_str[wrong_guesses_len] = 0;
        }
    }

    control_msg *res = malloc(sizeof(control_msg));
    if (res == NULL) {
        printf("malloc failed");
        return NULL;
    }

    res->word = word;
    res->word_len = word_len;
    res->wrong_guesses = wrong_guesses_str;
    res->wrong_guesses_len = wrong_guesses_len;

    return res;
}

typedef struct server_msg_t {
    char msg_flag;
    union {
        control_msg *control;
        char *announcement;
    } msg;
} server_msg;

int client_recv_msg(client *self, server_msg *ret) {
    char msg_flag;
    {
        const char *msg = self->c.recv_func(self->c.p, 1); // msg flag
        if (msg == NULL) {
            printf("recv msg flag failed");
            return -1;
        }

        msg_flag = msg[0];
    }

    ret->msg_flag = msg_flag;

    if (msg_flag < 1) { // control msg
        control_msg *msg = client_recv_control_msg(self);
        if (!msg) return -1;

        ret->msg.control = msg;

        return 0;
    }

    char *announcement_str = NULL;
    {
        announcement_str = malloc(msg_flag + 1);
        const char *msg = self->c.recv_func(self->c.p, msg_flag); // announcement
        if (msg == NULL) {
            printf("recv announcement failed");
            return -1;
        }
        strcpy(announcement_str, msg);
        announcement_str[msg_flag] = 0;
    }
    ret->msg.announcement = announcement_str;

    return 0;
}

int client_send_msg(client *self, const char *msg, char msg_len) {
    if (self->c.send_func(self->c.p, &msg_len, 1)) return -1;

    if (msg_len > 0) {
        if (self->c.send_func(self->c.p, msg, msg_len)) return -1;
    }

    return 0;
}

void client_run(client *self) {
    {
        server_msg serv;
        client_recv_msg(self, &serv);
        if (serv.msg_flag > 0) {
            if (strcmp(serv.msg.announcement, "server-overloaded") == 0) {
                printf(">>>%s\n", serv.msg.announcement);
                return; // overloaded
            }
        }
    }

    if (client_impl_initial_prompt(self->p)) {
        printf("Exiting...\n");
        return;
    }

    if (client_send_msg(self, NULL, 0)) {
        printf("send initial msg failed");
        return;
    }

//    printf("initialized, awaiting server...\n");

    while (1) {
        server_msg serv;
        client_recv_msg(self, &serv);
        if (serv.msg_flag > 0) {
            printf(">>>%s\n", serv.msg.announcement);

            if (strcmp(serv.msg.announcement, "server-overloaded") == 0) {
                return; // overloaded
            }

            if (strcmp(serv.msg.announcement, "Game Over!") == 0) {
                return; // completed
            }

            continue;
        }

        if(strchr(serv.msg.control->word, '_') == NULL) { // got correct word
            printf(">>>The word was %s\n", serv.msg.control->word);
            continue;
        }

        {
            printf(">>>");
            for (int i = 0; i < serv.msg.control->word_len; i += 1) {
                printf("%c", serv.msg.control->word[i]);
                if (i < serv.msg.control->word_len - 1) {
                    printf(" ");
                }
            }
            printf("\n");
        }

        if (serv.msg.control->wrong_guesses_len < 1) {
            printf(">>>Incorrect Guesses: \n");
        } else {
            printf(">>>Incorrect Guesses: ");
            for (int i = 0; i < serv.msg.control->wrong_guesses_len; i += 1) {
                printf("%c", serv.msg.control->wrong_guesses[i]);
                if (i < serv.msg.control->wrong_guesses_len - 1) {
                    printf(" ");
                }
            }
            printf("\n");
        }

        printf(">>>\n");

        char guess = client_impl_scan_guess(self->p);
        if (guess == -1) { // Ctrl+D
            client_send_msg(self, NULL, 0);
            printf("\n");
            return;
        }
        if (client_send_msg(self, &guess, 1)) {
            printf("send guess msg failed");
            return;
        }
    }
}
