#include "arena.h"
#include <termios.h>

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

SharedData *data;
int semid;
int current_idx = -1;
char logs[5][100];

void add_log(char *msg) {
    for (int i = 4; i > 0; i--) strcpy(logs[i], logs[i-1]);
    snprintf(logs[0], 100, "%s", msg);
}

void print_logo() {
    printf("\033[1;36m");
    printf("  ______ _______ ______ ______  _____  ____  _   _ \n");
    printf(" |  ____|__   __|  ____|  ____|_   _|/ __ \\| \\ | |\n");
    printf(" | |__     | |  | |__  | |__    | | | |  | |  \\| |\n");
    printf(" |  __|    | |  |  __| |  __|   | | | |  | | . ` |\n");
    printf(" | |____   | |  | |____| |     _| |_| |__| | |\\  |\n");
    printf(" |______|  |_|  |______|_|    |_____|\\____/|_| \\_|\n");
    printf("\033[0m =================================================\n");
}

void print_status() {
    User *u = &data->users[current_idx];
    printf("\033[1;33m[ USER: %s ]\033[0m  ", u->username);
    printf("\033[1;32m[ LVL: %d ]\033[0m  ", u->lvl);
    printf("\033[1;34m[ XP: %d/100 ]\033[0m  ", u->xp);
    printf("\033[1;35m[ GOLD: %dG ]\033[0m\n", u->gold);
    printf(" Weapon: \033[1;36m%s\033[0m\n", u->weapon);
    printf("-------------------------------------------------\n");
}

void battle_logic(char *opp_name) {
    for(int i=0; i<5; i++) strcpy(logs[i], "");
    int m_hp = 100 + (data->users[current_idx].lvl * 10);
    int my_hp = m_hp, o_hp = 100, o_m = 100;

    while(my_hp > 0 && o_hp > 0) {
        system("clear");
        printf("==================== BATTLE FIELD ====================\n");
        printf("\033[1;32m%-15s (YOU)\033[0m | Weapon: %s\n", data->users[current_idx].username, data->users[current_idx].weapon);
        printf("HP: [\033[1;32m%-20.*s\033[0m] %d/%d\n", (my_hp * 20 / m_hp), "####################", my_hp, m_hp);
        printf("\n                 \033[1;33mVS\033[0m\n\n");
        printf("\033[1;31m%-15s (ENEMY)\033[0m\n", opp_name);
        printf("HP: [\033[1;31m%-20.*s\033[0m] %d/%d\n", (o_hp * 20 / o_m), "####################", o_hp, o_m);
        printf("\nCombat Log:\n");
        for(int i=0; i<5; i++) printf("> %s\n", logs[i]);
        printf("------------------------------------------------------\n");
        printf("1. Attack | 2. Run\nChoice: ");
        char c = getch();
        if(c == '1') {
            int extra = 0;
            if(strcmp(data->users[current_idx].weapon, "Wood Sword")==0) extra = 10;
            else if(strcmp(data->users[current_idx].weapon, "Iron Sword")==0) extra = 25;
            else if(strcmp(data->users[current_idx].weapon, "Excalibur")==0) extra = 100;
            int dmg = 15 + extra;
            o_hp -= dmg;
            char m[100]; sprintf(m, "\033[1;32mYou hit for %d!\033[0m", dmg); add_log(m);
            if(o_hp > 0) {
                int edmg = 12 + (rand() % 10);
                my_hp -= edmg;
                sprintf(m, "\033[1;31mEnemy hit for %d!\033[0m", edmg); add_log(m);
            }
        } else break;
    }

    sem_op(semid, -1);
    User *u = &data->users[current_idx];
    int h_idx = u->history_count % 10;
    if(my_hp > 0) {
        printf("\n\033[1;32mVICTORY!\033[0m +150G +50XP\n");
        u->gold += 150; u->xp += 50;
        strcpy(u->history[h_idx].result, "WIN");
    } else {
        printf("\n\033[1;31mDEFEAT!\033[0m +50G +15XP\n");
        u->gold += 50; u->xp += 15;
        strcpy(u->history[h_idx].result, "LOSS");
    }
    strcpy(u->history[h_idx].opponent, opp_name);
    u->history_count++;
    u->lvl = (u->xp / 100) + 1;
    sem_op(semid, 1);
    printf("Press Enter to return..."); getchar();
}

void matchmaking() {
    system("clear"); print_logo();
    printf("\033[1;33m[ MATCHMAKING MODE ]\033[0m\n");
    sem_op(semid, -1);
    if (data->pvp_waiting_idx == -1) {
        data->pvp_waiting_idx = current_idx;
        sem_op(semid, 1);
        for(int i = 35; i > 0; i--) {
            printf("\rWaiting for Player... [%d s] ", i); fflush(stdout);
            sleep(1);
            if (data->pvp_waiting_idx != current_idx) {
                battle_logic("Enemy Player");
                return;
            }
        }
        sem_op(semid, -1);
        data->pvp_waiting_idx = -1;
        sem_op(semid, 1);
        printf("\n\033[1;35mEntering BOT Mode...\033[0m\n"); sleep(1);
        battle_logic("Wild Beast (BOT)");
    } else {
        data->pvp_waiting_idx = -1; 
        sem_op(semid, 1);
        battle_logic("Enemy Player");
    }
}

void armory() {
    while(1) {
        system("clear"); print_logo(); print_status();
        printf("\n1. Wood Sword   (100G)\n2. Iron Sword   (400G)\n3. Excalibur   (2000G)\n4. Back\nChoice: ");
        char c = getch();
        int p=0; char n[50];
        if(c=='1'){p=100; strcpy(n,"Wood Sword");}
        else if(c=='2'){p=400; strcpy(n,"Iron Sword");}
        else if(c=='3'){p=2000; strcpy(n,"Excalibur");}
        else if(c=='4') break;
        if(p > 0 && data->users[current_idx].gold >= p) {
            sem_op(semid,-1); data->users[current_idx].gold -= p;
            strcpy(data->users[current_idx].weapon, n); sem_op(semid,1);
            printf("\n\033[1;32mBought %s!\033[0m", n); sleep(1);
        }
    }
}

void history() {
    system("clear"); print_logo();
    printf("\033[1;33m=== HISTORY ===\033[0m\n");
    User *u = &data->users[current_idx];
    for(int i=0; i<(u->history_count > 10 ? 10 : u->history_count); i++)
        printf("%d. [%s] vs %-15s\n", i+1, u->history[i].result, u->history[i].opponent);
    printf("\nEnter to back..."); getchar();
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    data = shmat(shmid, NULL, 0);
    semid = semget(SEM_KEY, 1, 0666);

    while(1) {
        system("clear"); print_logo();
        printf("1. Register\n2. Login\n3. Exit\nChoice: ");
        char c = getch();
        if(c == '1') {
            sem_op(semid, -1);
            printf("\nUser: "); scanf("%s", data->users[data->user_count].username);
            printf("Pass: "); scanf("%s", data->users[data->user_count].password);
            
            // --- INI PERUBAHANNYA BOS: GOLD JADI 150 ---
            data->users[data->user_count].gold = 150; 
            
            data->users[data->user_count].lvl = 1;
            data->users[data->user_count].xp = 0;
            data->users[data->user_count].history_count = 0;
            strcpy(data->users[data->user_count].weapon, "None");
            data->user_count++;
            sem_op(semid, 1);
            printf("\nSuccess!"); sleep(1);
        } else if(c == '2') {
            char u[50], p[50];
            printf("\nUser: "); scanf("%s", u); printf("Pass: "); scanf("%s", p);
            for(int i=0; i<data->user_count; i++) {
                if(strcmp(data->users[i].username, u) == 0 && strcmp(data->users[i].password, p) == 0) {
                    current_idx = i;
                    while(1) {
                        system("clear"); print_logo(); print_status();
                        printf("1. Battle\n2. Armory\n3. History\n4. Logout\nChoice: ");
                        char s = getch();
                        if(s=='1') matchmaking();
                        else if(s=='2') armory();
                        else if(s=='3') history();
                        else if(s=='4') break;
                    }
                }
            }
        } else if(c == '3') break;
    }
    return 0;
}
