#include "server.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include "third_party/log.h"

#define THREAD_MAX 3
#define BUF_MAX 1024
#define GUESS_MAX 6

void acceptor_new(acceptor *self) {
    memset(self, 0, sizeof(acceptor));
}

void acceptor_destructor(acceptor *self) {
    if (self->destructor != NULL && self->p != NULL) {
        self->destructor(self->p);
    }
}

void listener_new(listener *self) {
    memset(self, 0, sizeof(listener));
}

void listener_destructor(listener *self) {
    if (self->destructor != NULL && self->p != NULL) {
        self->destructor(self->p);
    }
}

// Contains protocol-agnostic logic of `server` and `server_stateless`.
// e.g. Input serialization, server response parsing, error handling.
typedef struct server_impl_t {
    char buf[BUF_MAX];
} server_impl;

// Creates a new `server_impl`.
// Returns 0 on success, -1 on failure.
int server_impl_new(server_impl *self) {
    memset(self->buf, 0, BUF_MAX);

    return 0;
}

void server_impl_destructor(__attribute__((unused)) server_impl *self) {
    // do nothing
}

// Handle the client's request.
char *server_impl_handle(server_impl *self, const char msg) {
    return NULL;
}

server *server_new(listener l) {
    server_impl *p = malloc(sizeof(server_impl));
    if (p == NULL) return NULL;
    server_impl_new(p);

    server *self = malloc(sizeof(server));
    if (self == NULL) {
        free(p);
        return NULL;
    }

    self->l = l;
    self->p = p;

    return self;
}

void server_destructor(server *self) {
    listener_destructor(&self->l);

    if (self->p == NULL) return;

    server_impl_destructor(self->p);
    free(self->p);
    self->p = NULL;
}

typedef struct client_msg_t {
    char msg_flag;
    char *msg;
} client_msg;

int server_recv_msg(
        acceptor *c,
        client_msg *ret
) {
    char msg_flag;
    {
        const char *msg = c->recv_func(c->p, 1); // msg flag
        if (msg == NULL) {
            log_error("recv msg flag failed");
            return -1;
        }

        msg_flag = msg[0];
    }

    ret->msg_flag = msg_flag;

    if (msg_flag < 1) return 0; // received nothing

    char *msg_str = NULL;
    {
        msg_str = malloc(msg_flag + 1);
        const char *msg = c->recv_func(c->p, msg_flag); // announcement
        if (msg == NULL) {
            log_error("recv msg content failed");
            return -1;
        }
        strcpy(msg_str, msg);
        msg_str[msg_flag] = 0;
    }

    ret->msg = msg_str;

    return 0;
}

int server_send_control_msg(
        acceptor *c,
        const char *word,
        char word_len,
        const char *wrong_guesses,
        char wrong_guesses_len
) {
    char msg_flag = 0;

    if (c->send_func(c->p, &msg_flag, 1)) return -1; // msg flag
    if (c->send_func(c->p, &word_len, 1)) return -1; // word len
    if (c->send_func(c->p, &wrong_guesses_len, 1)) return -1; // incorrect

    if (word_len > 0) {
        if (c->send_func(c->p, word, word_len)) return -1; // word
    }
    if (wrong_guesses_len > 0) {
        if (c->send_func(c->p, wrong_guesses, wrong_guesses_len)) return -1; // wrong_guesses
    }

    return 0;
}

int server_send_announcement(
        acceptor *c,
        const char *msg,
        char msg_len
) {
    if (c->send_func(c->p, &msg_len, 1)) return -1; // msg flag
    if (c->send_func(c->p, msg, msg_len)) return -1; // msg

    return 0;
}

int server_run_p(acceptor *c, const char *word) {
    const char word_len = strlen(word);

    {
        char *msg = c->recv_func(c->p, 1);
        if (msg == NULL) {
            log_error("recv msg failed");
            return -1;
        }

        const char msg_len = msg[0];
        if (msg_len != 0) {
            log_error("initial message should be empty");
            return -1;
        }
    }

    // Start the game

    char *word_buf;
    {
        word_buf = malloc(sizeof(char) * (word_len + 1));
        if (word_buf == NULL) {
            log_error("malloc failed");
            return -1;
        }
        memset(word_buf, '_', word_len);
        word_buf[word_len] = '\0';
    }

    char wrong_guesses[GUESS_MAX] = {0};
    size_t wrong_guesses_len = 0;

    while (wrong_guesses_len < GUESS_MAX) {
        if (server_send_control_msg(
                c,
                word_buf, word_len,
                wrong_guesses, wrong_guesses_len
        )) {
            log_error("send msg failed");
            return -1;
        }

        client_msg msg;
        while (1) {
            if (server_recv_msg(c, &msg)) {
                log_error("recv response failed");
                return -1;
            }
            if (msg.msg_flag < 1) { // client exit
                log_info("client exited");
                return 0;
            }
            if (msg.msg_flag == 1) {
                break; // received a valid guess
            }
        }

        const char guess = msg.msg[0];
//        printf("recv guess: %c\n", guess);

        int found = 0;
        for (int i = 0; i < word_len; i += 1) {
            if (word[i] == guess) {
                word_buf[i] = guess;
                found = 1;
            }
        }

        if (strcmp(word_buf, word) == 0) {
            break; // word is found
        }

        if (found) {
            continue; // guess is correct
        }

        for (int i = 0; i < GUESS_MAX; i += 1) {
            if (wrong_guesses[i] == guess) {
                break;
            }
            if (wrong_guesses[i] == 0) {
                wrong_guesses[i] = guess;
                wrong_guesses_len += 1;
                break;
            }
        }
    }

    log_info("game finished");

    if (server_send_control_msg(
            c,
            word, word_len,
            wrong_guesses, wrong_guesses_len
    )) {
        log_error("send final control msg failed");
        return -1;
    }

    if (wrong_guesses_len == GUESS_MAX) {
        if (server_send_announcement(c, "You Lose!", 9)) {
            log_error("send msg failed");
            return -1;
        }
    } else {
        if (server_send_announcement(c, "You Win!", 8)) {
            log_error("send msg failed");
            return -1;
        }
    }

    if (server_send_announcement(c, "Game Over!", 10)) {
        log_error("send msg failed");
        return -1;
    }

    return 0;
}

typedef struct thread_arg_t {
    acceptor *c;
    const char *word;

    size_t *thread_count;
} thread_arg;

void server_thread_handler(thread_arg* arg) {
    acceptor *c = arg->c;
    const char *word = arg->word;
    size_t *thread_count = arg->thread_count;
    free(arg);

    int ret = server_run_p(c, word);
    if (ret < -1) {
        (*thread_count) -= 1;
        return; // fatal error
    }

    acceptor_destructor(c);

    (*thread_count) -= 1;
}

char* server_rand_word() {
    char* words[20] = {0};

    FILE *fd = fopen("hangman_words.txt", "r");
    if (!fd) return NULL;

    size_t words_len = 0;

    for (int i = 0; i < 16; i += 1) {
        char *buf = malloc(128); /* allocate a memory slot of 128 chars */
        if (fscanf(fd, "%127s", buf) > 0) {
            words[i] = buf;
            words_len += 1;
        } else {
            free(buf);
            break;
        }
    }

    fclose(fd);

    return words[rand() % words_len];
}

void server_run(server *self) {
    size_t thread_count = 0;
    pthread_t thread[THREAD_MAX];

    while (1) {
        acceptor* c = malloc(sizeof(acceptor));
        acceptor_new(c);

        int res = self->l.next_client(self->l.p, c);
        if (res) {
            log_error("accept failed");
            free(c);
            return;
        }

        if (thread_count < THREAD_MAX) {
            server_send_announcement(c, "welcome", 7);

            const char* word = server_rand_word();
            if (!word) word = "default"; // default case
            log_info("chosen word: %s", word);

            thread_arg *arg = malloc(sizeof(thread_arg));
            arg->word = word;
            arg->c = c;
            arg->thread_count = &thread_count;

            pthread_create(
                    &thread[thread_count],
                    NULL,
                    (void *(*)(void *)) &server_thread_handler,
                    arg
            );
            thread_count += 1;

            continue;
        }

        log_warn("too many threads\n");
        server_send_announcement(c, "server-overloaded", 17);

        acceptor_destructor(c);
        free(c);
    }
}
