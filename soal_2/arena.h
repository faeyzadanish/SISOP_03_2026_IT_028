#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define SHM_KEY 1234
#define SEM_KEY 5678

typedef struct {
    char result[10];   // WIN atau LOSS
    char opponent[30]; // Nama musuh (Player/Bot)
    int reward_xp;
} MatchRecord;

typedef struct {
    char username[50];
    char password[50];
    int gold;
    int lvl;
    int xp;
    char weapon[50];
    MatchRecord history[10]; // Riwayat battle
    int history_count;
    int pvp_status;          // 0: Idle, 1: Waiting, 2: In Battle
} User;

typedef struct {
    User users[100];
    int user_count;
    int pvp_waiting_idx;     // Menyimpan index user yang lagi nunggu PVP
} SharedData;

static void sem_op(int semid, int op) {
    struct sembuf sb = {0, op, 0};
    semop(semid, &sb, 1);
}

#endif
