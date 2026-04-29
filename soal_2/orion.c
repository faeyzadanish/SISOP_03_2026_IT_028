#include "arena.h"

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
    
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, 1);

    // Inisialisasi awal server
    data->user_count = 0;
    data->pvp_waiting_idx = -1; // Reset matchmaking

    printf("\033[1;32m[SERVER] Orion Arena Ready. Matchmaking System Active.\033[0m\n");

    while (1) {
        // Server tetap jalan untuk menjaga memori tetap hidup
        sleep(10);
    }
    return 0;
}
