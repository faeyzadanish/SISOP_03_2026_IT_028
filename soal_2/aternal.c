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
    printf("\033[1;33m");
    printf("  _____ _                 _ \n");
    printf(" | ____| |_ ___ _ __ _ __(_) ___  _ __ \n");
    printf(" |  _| | __/ _ \\ '__| '__| |/ _ \\| '_ \\ \n");
    printf(" | |___| ||  __/ |  | |  | | (_) | | | |\n");
    printf(" |_____|\\__\\___|_|  |_|  |_|\\___/|_| |_|\n");
    printf("\033[0m =======================================\n");
}

void draw_arena(int my_hp, int m_hp, int o_hp, int o_m, char *o_name) {
    system("clear");
    printf("==================== ARENA ====================\n");
    printf("%-15s Lvl %d\n", data->users[current_idx].username, data->users[current_idx].lvl);
    printf("[");
    int my_bar = (my_hp * 20 / m_hp);
    for(int i=0; i<20; i++) printf(i < my_bar ? "#" : " ");
    printf("] %d/%d\n\n", my_hp, m_hp);
    
    printf("                 VS\n\n");
    
    printf("%-15s Lvl 1 | Weapon: %s\n", o_name, data->users[current_idx].weapon);
    printf("[");
    int opp_bar = (o_hp * 20 / o_m);
    for(int i=0; i<20; i++) printf(i < opp_bar ? "#" : " ");
    printf("] %d/%d\n", o_hp, o_m);
    
    printf("\nCombat Log:\n");
    for(int i=0; i<5; i++) printf("> %s\n", logs[i]);
    printf("\nCD: Atk(1.0s) | Ult(0.0s)\n");
    printf("===============================================\n");
}

void battle() {
    for(int i=0; i<5; i++) strcpy(logs[i], "");
    int m_hp = 100 + (data->users[current_idx].xp / 10);
    int my_hp = m_hp, o_hp = 100, o_m = 100;

    while(my_hp > 0 && o_hp > 0) {
        draw_arena(my_hp, m_hp, o_hp, o_m, "Wild Beast");
        char c = getch();
        if(c == 'a') {
            int dmg = 10 + (data->users[current_idx].xp / 50);
            o_hp -= dmg;
            char msg[100]; sprintf(msg, "You hit for %d damage!", dmg);
            add_log(msg);
            if(o_hp > 0) {
                my_hp -= 10;
                add_log("Enemy hit for 10 damage!");
            }
        } else if(c == 'q') break;
    }

    sem_op(semid, -1);
    if(my_hp > 0) {
        printf("\nVICTORY! +120 Gold | +50 XP\n");
        data->users[current_idx].gold += 120; data->users[current_idx].xp += 50;
    } else {
        printf("\nDEFEAT! +30 Gold | +15 XP\n");
        data->users[current_idx].gold += 30; data->users[current_idx].xp += 15;
    }
    data->users[current_idx].lvl = (data->users[current_idx].xp / 100) + 1;
    sem_op(semid, 1);
    printf("Press Enter..."); getchar();
}

void armory() {
    system("clear");
    print_logo();
    printf("Your Gold: %d G\n\n", data->users[current_idx].gold);
    printf("1. Wood Sword  (100 G)  [+5 Dmg]\n");
    printf("2. Iron Sword  (400 G)  [+15 Dmg]\n");
    printf("3. Back\nChoice: ");
    int s; scanf("%d", &s);
    if(s == 1 && data->users[current_idx].gold >= 100) {
        sem_op(semid, -1);
        data->users[current_idx].gold -= 100;
        strcpy(data->users[current_idx].weapon, "Wood Sword");
        sem_op(semid, 1);
    } else if(s == 2 && data->users[current_idx].gold >= 400) {
        sem_op(semid, -1);
        data->users[current_idx].gold -= 400;
        strcpy(data->users[current_idx].weapon, "Iron Sword");
        sem_op(semid, 1);
    }
}

void history() {
    system("clear");
    printf("No | Result  | Opponent       | Reward\n");
    printf("------------------------------------------\n");
    printf("1  | WIN     | Wild Beast     | +50 XP\n");
    printf("------------------------------------------\n");
    printf("\nPress Enter to back...");
    getchar(); getchar();
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    data = shmat(shmid, NULL, 0);
    semid = semget(SEM_KEY, 1, 0666);

    while(1) {
        system("clear"); print_logo();
        printf("1. Register\n2. Login\n3. Exit\nChoice: ");
        int c; scanf("%d", &c);
        if(c == 1) {
            sem_op(semid, -1);
            printf("User: "); scanf("%s", data->users[data->user_count].username);
            printf("Pass: "); scanf("%s", data->users[data->user_count].password);
            data->users[data->user_count].gold = 150;
            data->users[data->user_count].lvl = 1;
            strcpy(data->users[data->user_count].weapon, "None");
            data->user_count++;
            sem_op(semid, 1);
        } else if(c == 2) {
            char u[50], p[50];
            printf("User: "); scanf("%s", u);
            printf("Pass: "); scanf("%s", p);
            for(int i=0; i<data->user_count; i++) {
                if(strcmp(data->users[i].username, u) == 0 && strcmp(data->users[i].password, p) == 0) {
                    current_idx = i;
                    while(1) {
                        system("clear"); print_logo();
                        printf("1. Battle\n2. Armory\n3. History\n4. Logout\nChoice: ");
                        int s; scanf("%d", &s);
                        if(s == 1) battle();
                        else if(s == 2) armory();
                        else if(s == 3) history();
                        else if(s == 4) break;
                    }
                }
            }
        } else break;
    }
    return 0;
}
