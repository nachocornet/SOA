#include <libc.h>

#if 1

#define SCREEN_WIDTH 80

#define TITLE_ROW 1
#define CONTROL_ROW 2
#define HUD_ROW 3

#define BOARD_LEFT 1
#define BOARD_TOP 4
#define BOARD_INNER_W 52
#define BOARD_INNER_H 14
#define BOARD_W (BOARD_INNER_W + 2)
#define BOARD_H (BOARD_INNER_H + 2)
#define BOARD_RIGHT (BOARD_LEFT + BOARD_W - 1)
#define BOARD_BOTTOM (BOARD_TOP + BOARD_H - 1)

#define PANEL_LEFT 58

#define GOAL_LEFT 18
#define GOAL_RIGHT 34
#define GOAL_CENTER 26

#define MAX_SHOTS 5
#define TARGET_GOALS 3
#define EVENT_MSG_SIZE 48
#define PLAYER_NAME_SIZE 16

typedef struct {
    int running;
    int phase; /* 0 = playing, 1 = win, 2 = lose */
    int goals;
    int saves;
    int shots;
    int command;
    int tick;
    int seed;
    int player_x;
    int keeper_x;
    int keeper_dir;
    int keeper_step;
    int ball_active;
    int ball_x;
    int ball_y;
    int ball_step;
    int slowmo_step;
    int shot_cooldown;
    int shot_pending;
    int event_timer;
    int difficulty;
    int keeper_half_width;
    char player_name[PLAYER_NAME_SIZE];
    char event_msg[EVENT_MSG_SIZE];
} game_state_t;

static game_state_t *game = 0;

static void print(char *msg)
{
    write(1, msg, strlen(msg));
}

static void clear_line(char *line)
{
    int i;
    for (i = 0; i < SCREEN_WIDTH; ++i) line[i] = ' ';
}

static void put_text(char *line, int pos, char *text)
{
    int i;
    for (i = 0; text[i] != 0 && pos + i < SCREEN_WIDTH; ++i) {
        line[pos + i] = text[i];
    }
}

static void put_char(char *line, int pos, char c)
{
    if (pos >= 0 && pos < SCREEN_WIDTH) {
        line[pos] = c;
    }
}

static void put_number(char *line, int pos, int value)
{
    char buffer[16];
    itoa(value, buffer);
    put_text(line, pos, buffer);
}

static void put_centered(char *line, char *text)
{
    int len;
    int pos;

    len = strlen(text);
    pos = (SCREEN_WIDTH - len) / 2;
    put_text(line, pos, text);
}

static unsigned int rng_next(game_state_t *g)
{
    g->seed = g->seed * 1103515245 + 12345;
    return (unsigned int)g->seed;
}

static int rng_range(game_state_t *g, int limit)
{
    return (int)(rng_next(g) % (unsigned int)limit);
}

static void copy_event_msg(game_state_t *g, char *msg)
{
    int i;

    for (i = 0; i < EVENT_MSG_SIZE - 1 && msg[i] != 0; ++i) {
        g->event_msg[i] = msg[i];
    }
    g->event_msg[i] = 0;
    g->event_timer = 8;
}

static void copy_player_name(game_state_t *g, char *name)
{
    int i;

    for (i = 0; i < PLAYER_NAME_SIZE - 1 && name[i] != 0; ++i) {
        g->player_name[i] = name[i];
    }
    g->player_name[i] = 0;
}

static void draw_name_prompt_screen(void)
{
    char line[SCREEN_WIDTH];
    int row;

    for (row = 0; row < 25; ++row) {
        clear_line(line);
        gotoxy(0, row);
        set_color(15, 0);
        write(1, line, SCREEN_WIDTH);
    }

    clear_line(line);
    put_centered(line, "========================================");
    gotoxy(0, 8);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "||          PLAYER NAME               ||");
    gotoxy(0, 10);
    set_color(15, 4);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "||     TYPE IT AND PRESS ENTER        ||");
    gotoxy(0, 12);
    set_color(14, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "========================================");
    gotoxy(0, 14);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);
}

static void draw_difficulty_prompt_screen(void)
{
    char line[SCREEN_WIDTH];
    int row;

    for (row = 0; row < 25; ++row) {
        clear_line(line);
        gotoxy(0, row);
        set_color(15, 0);
        write(1, line, SCREEN_WIDTH);
    }

    clear_line(line);
    put_centered(line, "========================================");
    gotoxy(0, 8);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "||         SELECT DIFFICULTY         ||");
    gotoxy(0, 10);
    set_color(15, 4);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "||    E = EASY  N = NORMAL  H = HARD ||");
    gotoxy(0, 12);
    set_color(14, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "||      PRESS A KEY AND ENTER        ||");
    gotoxy(0, 14);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "========================================");
    gotoxy(0, 16);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);
}

static void apply_difficulty(game_state_t *g)
{
    int min_x;
    int max_x;

    if (g->difficulty <= 0) {
        g->keeper_half_width = 0;
    } else if (g->difficulty == 1) {
        g->keeper_half_width = 1;
    } else {
        g->keeper_half_width = 2;
    }

    min_x = GOAL_LEFT + 1 + g->keeper_half_width;
    max_x = GOAL_RIGHT - 1 - g->keeper_half_width;
    if (max_x < min_x) {
        min_x = GOAL_CENTER;
        max_x = GOAL_CENTER;
    }

    g->keeper_x = min_x + rng_range(g, max_x - min_x + 1);
}

static void reset_game(game_state_t *g)
{
    g->running = 1;
    g->phase = 0;
    g->goals = 0;
    g->saves = 0;
    g->shots = 0;
    g->command = 0;
    g->tick = 0;
    g->seed = gettime() + 17;
    g->player_x = GOAL_CENTER;
    g->keeper_x = GOAL_LEFT + 2 + rng_range(g, GOAL_RIGHT - GOAL_LEFT - 3);
    g->keeper_dir = 1;
    g->keeper_step = 0;
    g->ball_active = 0;
    g->ball_x = g->player_x;
    g->ball_y = BOARD_INNER_H - 3;
    g->ball_step = 0;
    g->slowmo_step = 0;
    g->shot_cooldown = 0;
    g->shot_pending = 0;
    g->event_timer = 10;
    g->difficulty = 1;
    g->keeper_half_width = 1;
    copy_player_name(g, "Player");
    copy_event_msg(g, "Press Enter to shoot.");
}

static void prompt_player_name(game_state_t *g)
{
    char name[PLAYER_NAME_SIZE];
    int bytes_read;
    int i;

    draw_name_prompt_screen();
    bytes_read = read(name, PLAYER_NAME_SIZE - 1);
    if (bytes_read <= 0) {
        return;
    }

    name[PLAYER_NAME_SIZE - 1] = 0;

    for (i = 0; i < bytes_read; ++i) {
        if (name[i] == '\n' || name[i] == '\r') {
            name[i] = 0;
            break;
        }
    }

    if (i == bytes_read && bytes_read < PLAYER_NAME_SIZE) {
        name[bytes_read] = 0;
    }

    if (name[0] == 0) {
        return;
    }

    copy_player_name(g, name);
}

static void prompt_player_difficulty(game_state_t *g)
{
    char choice[4];
    int bytes_read;

    draw_difficulty_prompt_screen();
    bytes_read = read(choice, 3);
    if (bytes_read <= 0) {
        return;
    }

    if (choice[0] == 'e' || choice[0] == 'E') {
        g->difficulty = 0;
    } else if (choice[0] == 'h' || choice[0] == 'H') {
        g->difficulty = 2;
    } else {
        g->difficulty = 1;
    }
}

static void set_event(game_state_t *g, char *msg)
{
    copy_event_msg(g, msg);
}

static int clamp_player_x(int x)
{
    if (x < 2) return 2;
    if (x > BOARD_INNER_W - 3) return BOARD_INNER_W - 3;
    return x;
}

static void try_move_player(game_state_t *g, int dx)
{
    if (g->phase != 0) return;
    if (g->ball_active) return;
    g->player_x = clamp_player_x(g->player_x + dx);
}

static void launch_ball(game_state_t *g)
{
    if (g->phase != 0) return;
    if (g->ball_active) return;
    if (g->shot_cooldown > 0) return;

    g->ball_active = 1;
    g->ball_step = 0;
    g->slowmo_step = 0;
    g->ball_x = g->player_x;
    g->ball_y = BOARD_INNER_H - 3;
    set_event(g, "KICK!");
}

static void try_launch_pending_shot(game_state_t *g)
{
    if (!g->shot_pending) return;
    if (g->phase != 0) return;
    if (g->ball_active) return;
    if (g->shot_cooldown > 0) return;

    g->shot_pending = 0;
    launch_ball(g);
}

static void resolve_shot(game_state_t *g)
{
    int inside_goal;
    int saved;

    inside_goal = (g->ball_x >= GOAL_LEFT + 1 && g->ball_x <= GOAL_RIGHT - 1);
    saved = inside_goal && (g->ball_x >= g->keeper_x - g->keeper_half_width && g->ball_x <= g->keeper_x + g->keeper_half_width);

    g->shots++;
    g->ball_active = 0;
    g->shot_cooldown = 6;

    if (!inside_goal) {
        set_event(g, "TRY AGAIN!");
    } else if (saved) {
        g->saves++;
        set_event(g, "TRY AGAIN!");
    } else {
        g->goals++;
        set_event(g, "GOAL!");
    }

    if (g->shots >= MAX_SHOTS) {
        if (g->goals >= TARGET_GOALS) {
            g->phase = 1;
            set_event(g, "CHAMPIONS!");
        } else {
            g->phase = 2;
            set_event(g, "TRY AGAIN!");
        }
    }
}

static void move_keeper(game_state_t *g)
{
    int next_x;
    int min_x;
    int max_x;

    if (g->keeper_step % 3 != 0) return;

    min_x = GOAL_LEFT + 1 + g->keeper_half_width;
    max_x = GOAL_RIGHT - 1 - g->keeper_half_width;

    next_x = g->keeper_x + g->keeper_dir;
    if (next_x <= min_x || next_x >= max_x) {
        g->keeper_dir = -g->keeper_dir;
        next_x = g->keeper_x + g->keeper_dir;
    }

    if (next_x < min_x) next_x = min_x;
    if (next_x > max_x) next_x = max_x;
    g->keeper_x = next_x;
}

static void advance_ball(game_state_t *g)
{
    if (!g->ball_active) return;

    g->ball_step++;
    if (g->ball_step % 3 != 0) return;

    if (g->ball_y > 0) {
        g->ball_y--;
    }

    if (g->ball_y <= 0) {
        resolve_shot(g);
    }
}

static void advance_game(game_state_t *g)
{
    int slow_motion;

    g->tick++;
    slow_motion = g->ball_active;
    if (slow_motion) {
        g->slowmo_step++;
        if (g->slowmo_step % 2 != 0) {
            return;
        }
    }

    g->keeper_step++;

    if (g->event_timer > 0) {
        g->event_timer--;
    }

    if (g->shot_cooldown > 0) {
        g->shot_cooldown--;
        if (g->shot_cooldown == 0 && g->phase == 0 && !g->ball_active) {
            g->ball_x = g->player_x;
            g->ball_y = BOARD_INNER_H - 3;
        }
    }

    try_launch_pending_shot(g);

    if (g->command != 0) {
        int cmd = g->command;
        g->command = 0;

        if (cmd == 'q') {
            g->running = 0;
            return;
        }

        if (g->phase == 0) {
            if (cmd == 'a') {
                try_move_player(g, -1);
            } else if (cmd == 'd') {
                try_move_player(g, 1);
            } else if (cmd == '\n' || cmd == '\r') {
                if (g->ball_active || g->shot_cooldown > 0) {
                    g->shot_pending = 1;
                    set_event(g, "Enter queued. The keeper is trembling.");
                } else {
                    launch_ball(g);
                }
            }
        }
    }

    if (g->phase != 0) {
        return;
    }

    move_keeper(g);
    advance_ball(g);

    if (!g->ball_active && g->shot_cooldown == 0) {
        g->ball_x = g->player_x;
        g->ball_y = BOARD_INNER_H - 3;
    }
}

static void draw_header(game_state_t *g)
{
    char line[SCREEN_WIDTH];
    char *ball_state;

    clear_line(line);
    put_text(line, 2, "PENALTY PARTY - ");
    put_text(line, 19, g->player_name);
    gotoxy(0, TITLE_ROW);
    set_color(15, 4);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_centered(line, "[A][D] move   [ENTER] shoot   [Q] quit");
    gotoxy(0, CONTROL_ROW);
    set_color(0, 3);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_text(line, 2, "Goals: ");
    put_number(line, 9, g->goals);
    put_text(line, 12, "/3   Shots: ");
    put_number(line, 24, g->shots);
    put_text(line, 27, "/5   Saves: ");
    put_number(line, 39, g->saves);
    put_text(line, 45, "Ball: ");
    if (g->ball_active) ball_state = "IN AIR";
    else if (g->shot_cooldown > 0) ball_state = "RESET";
    else ball_state = "READY";
    put_text(line, 51, ball_state);
    put_text(line, 60, "Player: ");
    put_text(line, 68, g->player_name);
    gotoxy(0, HUD_ROW);
    set_color(15, 0);
    write(1, line, SCREEN_WIDTH);
}

static void draw_panel_row(game_state_t *g, int row, char *line)
{
    if (row == BOARD_TOP) {
        put_text(line, PANEL_LEFT, "+---------------------+");
        put_text(line, PANEL_LEFT + 7, "STATUS");
    } else if (row == BOARD_TOP + 1) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Mode:");
        if (g->phase == 0) put_text(line, PANEL_LEFT + 8, "PLAY");
        else if (g->phase == 1) put_text(line, PANEL_LEFT + 8, "WIN");
        else put_text(line, PANEL_LEFT + 8, "LOSE");
    } else if (row == BOARD_TOP + 2) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Net:");
        put_text(line, PANEL_LEFT + 7, "PENALTY");
    } else if (row == BOARD_TOP + 3) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Keeper:");
        put_number(line, PANEL_LEFT + 10, g->keeper_x);
    } else if (row == BOARD_TOP + 4) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Mood:");
        if (g->phase == 0) put_text(line, PANEL_LEFT + 8, "CALM");
        else if (g->phase == 1) put_text(line, PANEL_LEFT + 8, "GOAT");
        else put_text(line, PANEL_LEFT + 8, "BROKEN");
    } else if (row == BOARD_TOP + 5) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Name:");
        put_text(line, PANEL_LEFT + 8, g->player_name);
    } else if (row == BOARD_TOP + 6) {
        put_text(line, PANEL_LEFT, "|                     |");
        put_text(line, PANEL_LEFT + 2, "Difficulty:");
        if (g->difficulty == 0) put_text(line, PANEL_LEFT + 13, "EASY");
        else if (g->difficulty == 1) put_text(line, PANEL_LEFT + 13, "NORMAL");
        else put_text(line, PANEL_LEFT + 13, "HARD");
    } else if (row == BOARD_TOP + 7) {
        put_text(line, PANEL_LEFT, "+---------------------+");
        put_text(line, PANEL_LEFT + 7, "BUTTONS");
    } else if (row == BOARD_TOP + 8) {
        put_text(line, PANEL_LEFT, "| [A] left            |");
    } else if (row == BOARD_TOP + 9) {
        put_text(line, PANEL_LEFT, "| [D] right           |");
    } else if (row == BOARD_TOP + 10) {
        put_text(line, PANEL_LEFT, "| [ENTER] shoot       |");
    } else if (row == BOARD_TOP + 11) {
        put_text(line, PANEL_LEFT, "| [Q] quit            |");
    } else if (row == BOARD_TOP + 13) {
        put_text(line, PANEL_LEFT, "+---------------------+");
        put_text(line, PANEL_LEFT + 7, "TIP");
    } else if (row == BOARD_TOP + 14) {
        put_text(line, PANEL_LEFT, "| Aim at the keeper   |");
    } else if (row == BOARD_TOP + 15) {
        put_text(line, PANEL_LEFT, "| and make him dance. |");
    } else if (row == BOARD_TOP + 16) {
        put_text(line, PANEL_LEFT, "| 3 goals = victory   |");
    } else if (row == BOARD_BOTTOM) {
        put_text(line, PANEL_LEFT, "+---------------------+");
    }
}

static void draw_pitch_row(game_state_t *g, int row, char *line)
{
    int pitch_y;
    int x;
    int screen_x;
    int cell_y;

    pitch_y = row - BOARD_TOP;

    if (pitch_y == 0 || pitch_y == BOARD_H - 1) {
        put_char(line, BOARD_LEFT, '+');
        for (x = 1; x < BOARD_W - 1; ++x) {
            put_char(line, BOARD_LEFT + x, '-');
        }
        put_char(line, BOARD_RIGHT, '+');
        draw_panel_row(g, row, line);
        return;
    }

    put_char(line, BOARD_LEFT, '|');
    put_char(line, BOARD_RIGHT, '|');

    cell_y = pitch_y - 1;
    for (x = 0; x < BOARD_INNER_W; ++x) {
        char ch;

        if (cell_y == 0) {
            if (x == GOAL_LEFT || x == GOAL_RIGHT) ch = '|';
            else if (x > GOAL_LEFT && x < GOAL_RIGHT) ch = '=';
            else ch = '-';
        } else if (cell_y == 9) {
            ch = (x == GOAL_CENTER) ? '.' : ' ';
        } else if (((x + cell_y) % 8) == 0) {
            ch = '.';
        } else {
            ch = ' ';
        }

        screen_x = BOARD_LEFT + 1 + x;
        put_char(line, screen_x, ch);
    }

    if (row == BOARD_TOP + 1 + g->ball_y && g->ball_active) {
        put_char(line, BOARD_LEFT + 1 + g->ball_x, 'o');
    }

    if (row == BOARD_TOP + 1 + BOARD_INNER_H - 3) {
        put_char(line, BOARD_LEFT + 1 + g->player_x, 'P');
    }

    if (row == BOARD_TOP + 1 || row == BOARD_TOP + 2) {
        int offset;

        for (offset = -g->keeper_half_width; offset <= g->keeper_half_width; ++offset) {
            put_char(line, BOARD_LEFT + 1 + g->keeper_x + offset, 'G');
        }
    }

    draw_panel_row(g, row, line);
}

static void draw_footer(game_state_t *g)
{
    char line[SCREEN_WIDTH];

    clear_line(line);
    put_text(line, 2, "Outcome: ");
    if (g->phase == 0) {
        if (g->event_timer > 0) put_text(line, 12, g->event_msg);
        else put_text(line, 12, "Press Enter to shoot.");
    } else if (g->phase == 1) {
        put_text(line, 12, "CHAMPIONS!");
    } else {
        put_text(line, 12, "TRY AGAIN!");
    }
    gotoxy(0, 20);
    set_color(10, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_text(line, 2, "Un chute por jugada.");
    gotoxy(0, 21);
    set_color(13, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_text(line, 2, "Mira el panel de la derecha.");
    gotoxy(0, 22);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_text(line, 2, "Penalty Party: futbol y caos.");
    gotoxy(0, 23);
    set_color(11, 0);
    write(1, line, SCREEN_WIDTH);

    clear_line(line);
    put_text(line, 2, "Meta: 3 goles en 5 chutes. Q para salir.");
    gotoxy(0, 24);
    set_color(8, 0);
    write(1, line, SCREEN_WIDTH);
}

static void draw_overlay(game_state_t *g)
{
    char line[SCREEN_WIDTH];

    if (g->phase == 1) {
        clear_line(line);
        put_centered(line, "*** VICTORY ***");
        gotoxy(0, 10);
        set_color(15, 2);
        write(1, line, SCREEN_WIDTH);

        clear_line(line);
        put_centered(line, "The keeper has been humiliated. Classic.");
        gotoxy(0, 11);
        set_color(15, 2);
        write(1, line, SCREEN_WIDTH);

        clear_line(line);
        put_centered(line, "Press Q to exit.");
        gotoxy(0, 12);
        set_color(15, 2);
        write(1, line, SCREEN_WIDTH);
    } else if (g->phase == 2) {
        clear_line(line);
        put_centered(line, "*** GAME OVER ***");
        gotoxy(0, 10);
        set_color(15, 4);
        write(1, line, SCREEN_WIDTH);

        clear_line(line);
        put_centered(line, "The keeper won the memes battle.");
        gotoxy(0, 11);
        set_color(15, 4);
        write(1, line, SCREEN_WIDTH);

        clear_line(line);
        put_centered(line, "Press Q to exit.");
        gotoxy(0, 12);
        set_color(15, 4);
        write(1, line, SCREEN_WIDTH);
    }
}

static void draw_screen(game_state_t *g)
{
    int row;
    char line[SCREEN_WIDTH];

    set_color(15, 0);
    display_fps();

    draw_header(g);

    for (row = BOARD_TOP; row <= BOARD_BOTTOM; ++row) {
        clear_line(line);
        draw_pitch_row(g, row, line);
        gotoxy(0, row);
        if (row == BOARD_TOP || row == BOARD_BOTTOM) {
            set_color(15, 0);
        } else if (row == BOARD_TOP + 1) {
            set_color(15, 2);
        } else {
            set_color(15, (row % 2) ? 2 : 10);
        }
        write(1, line, SCREEN_WIDTH);
    }

    draw_footer(g);
    draw_overlay(g);
}

static void renderer_loop(game_state_t *g)
{
    int last_tick;

    last_tick = -1;
    while (g->running) {
        int current_tick;

        current_tick = gettime();
        if (current_tick == last_tick) continue;
        last_tick = current_tick;

        fps_update();
        advance_game(g);
        draw_screen(g);
    }

    shmdt((void *)g);
    exit();
}

static void controller_loop(game_state_t *g)
{
    char key[2];
    int bytes_read;

    while (g->running) {
        bytes_read = read(key, 1);
        if (bytes_read <= 0) continue;

        if (key[0] == 'q' || key[0] == 'Q') {
            g->command = 'q';
            g->running = 0;
            break;
        }

        if (g->phase != 0) continue;

        if (key[0] == 'a' || key[0] == 'A' || key[0] == 'h' || key[0] == 'H') {
            g->command = 'a';
        } else if (key[0] == 'd' || key[0] == 'D' || key[0] == 'l' || key[0] == 'L') {
            g->command = 'd';
        } else if (key[0] == '\n' || key[0] == '\r') {
            g->command = '\n';
        }
    }

    shmrm(0);
    shmdt((void *)g);
    exit();
}

int __attribute__((__section__(".text.main")))
main(void)
{
    int pid;

    print("\n========================================\n");
    print("          ZEOS Milestone 8\n");
    print("        PENALTY PARTY\n");
    print("========================================\n");

    game = (game_state_t *)shmat(0, (void *)0);
    if (game == (void *)-1) {
        print("ERROR: could not allocate shared memory for the game\n");
        exit();
        return 0;
    }

    reset_game(game);
    prompt_player_name(game);
    prompt_player_difficulty(game);
    apply_difficulty(game);

    pid = fork();
    if (pid < 0) {
        print("ERROR: fork failed\n");
        shmrm(0);
        shmdt((void *)game);
        exit();
        return 0;
    }

    if (pid == 0) {
        renderer_loop(game);
    }

    controller_loop(game);
    return 0;
}

#endif
