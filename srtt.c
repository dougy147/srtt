#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <raylib.h>
#include <time.h>
#include <inttypes.h> // print uint64_t (timer)

#define NB_ARROWS    4     // Number of possible items on screen
#define NB_BLOCKS    8     // Blocks are cut with pauses
#define NB_STIMULI   120   // Number of stimuli by block
#define CONTEXT_SIZE 2     // Sequence context
#define SOA          0.250 // Stimulus Onset Asynchrony (in seconds)
#define P_REG        0.85  // Regular sequence probability

#define BG_COLOR      BLACK // Background color
#define ARROWS_COLOR  DARKGRAY
#define STIMULI_COLOR WHITE

///////////////////////////////////////////////////////////
/*
   Check this link for time support lead:
    https://stackoverflow.com/a/37920181/22665005
*/
///////////////////////////////////////////////////////////

enum {
    REGULAR_SEQUENCE,
    IRREGULAR_SEQUENCE,
};

typedef struct Arrow {
    Vector2 size; // vertical line size
    Vector2 pos;  // vertical line position
    Vector2 v1;   // left vertex of the arrow triangle
    Vector2 v2;   // center vertex of the arrow triangle
    Vector2 v3;   // right vertex of the arrow triangle
    int index;
    int dot_padding; // stimuli padding
    int dot_radius;  // stimuli are circles
} Arrow;

void draw_arrows(Arrow *arrows)
{
    for (int i = 0; i < NB_ARROWS; i++) {
        DrawRectangleV(arrows[i].pos, arrows[i].size, ARROWS_COLOR);
        DrawTriangle(arrows[i].v1,arrows[i].v2,arrows[i].v3, ARROWS_COLOR);
    }
}

void draw_dot(Arrow *arrows, int arrow_index, Color sequence_color)
{
    DrawCircle(
	    arrows[arrow_index].pos.x + (arrows[arrow_index].size.x / 2),
	    arrows[arrow_index].pos.y + arrows[arrow_index].size.y + (arrows[arrow_index].v2.y - arrows[arrow_index].v1.y) + arrows[arrow_index].dot_padding + arrows[arrow_index].dot_radius,
	    arrows[arrow_index].dot_radius,
	    sequence_color
	    );
}

int next_expected(int *context, int choice, int *regular_sequence, int *irregular_sequence, size_t size_context, size_t size_sequence) { // given a context and a sequence, find expected value
    int *sequence;
    if (choice == REGULAR_SEQUENCE) { sequence = regular_sequence; }
    else { sequence = irregular_sequence;}
    for (int i = 0; i < size_sequence; i++) {
        for (int j = 0; j < size_context; j++) {
	    if (context[j] != sequence[(i+j) % size_sequence]) { break; }
	    if (j==size_context-1 && context[j] == sequence[(j+i) % size_sequence]) {
		return sequence[(i+j+1) % size_sequence];
	    }
	}
    }
    return -1;
}

void update_context(int *context, size_t size_context, int current_value)
{
    for (int i = 0; i < size_context - 1; i++) {
	context[i] = context[i+1];
    }
    context[size_context - 1] = current_value;
}

int choose_sequence(void) {
	if ( rand() < RAND_MAX * P_REG ) {
	    return REGULAR_SEQUENCE;
	} else {
	    return IRREGULAR_SEQUENCE;
	}
}

uint64_t get_posix_clock_time(void)
{
    struct timespec ts;
    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0) { return (uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000); }
    else { return 0; }
}

int main(void)
{
     //////////////////////////////////////////////////////////
    ///////////////* TIME SUPPORT (LINUX)*////////////////////
    uint64_t timer_start, timer_stop;
    uint64_t time_diff, time_task_start, time_diff_total;
    if (_POSIX_MONOTONIC_CLOCK > 0) {
	// good time supported
    } else if (_POSIX_MONOTONIC_CLOCK == 0) {
	#ifdef _SC_MONOTONIC_CLOCK
    	if (sysconf (_SC_MONOTONIC_CLOCK) > 0) {
    	    /* A monotonic clock is present */
    	} else {
    	    printf("ERROR: no monotonic clock found.\n");
    	    return -1;
    	}
    	#endif
    } else {
	printf("ERROR: no time support.\n");
        return -1;
    }
     //////////////////////////////////////////////////////////
    ////////////////*WINDOW PARAMETERS*///////////////////////
    /*Set up window*/
    int factor = 75;
    int w = 16 * factor;
    int h =  9 * factor;

    /*Set up inside square (graph)*/
    int wo = 0.10 * w; // width offset
    int ho = 0.20 * h; // height offset :

    int gw = w - (2 * wo);// graph width
    int gh = h - (2 * ho);// graph height

    // make "graph"'s width dividble by 8 (4 arrows, centered)
    if ( (w - (2 * wo)) % (NB_ARROWS * 2) != 0) {
	int remaining = (w - (2 * wo)) % 8;
	if (remaining % 2 == 0) {
	    wo += remaining / 2;
	    gw -= remaining;
	} else {
	    wo += (remaining / 2) + 1;
	    gw -= remaining;
	    w  += (remaining / 2) - 1;
	}
    }
     ///////////////////////////////////////////////////////////
    ////////////*ARROWS & DOT PARAMETERS (stimulus)*///////////
    float arrow_thickness = 0.01 * gw;
    int dot_padding = (1.0 / 32.0) * gh; // distance from the end of an arrow
    int dot_radius = ((1.5 * arrow_thickness) + arrow_thickness + (1.5 * arrow_thickness)) / 2; // size of the arrow triangle base (from v1 to v3)
    Arrow arrows[NB_ARROWS]; // Arrows array
    for (int i = 0; i < NB_ARROWS; i++) {
	Arrow a;
	a.index = i;
	/*lines*/
    	a.size.x = arrow_thickness;
    	a.size.y = gh / 2;
    	a.pos.x = wo + ((i+1) * gw / (NB_ARROWS * 2)) + ( i * (gw / (NB_ARROWS * 2))) - (a.size.x / 2);
    	a.pos.y = ho;
	/*triangles*/
	Vector2 v1 = { .x = a.pos.x - 1.5 * a.size.x, .y = a.pos.y + a.size.y};
    	Vector2 v2 = { .x = a.pos.x + (a.size.x / 2), .y = (a.pos.y + a.size.y) + 4 * a.size.x};
    	Vector2 v3 = { .x = a.pos.x  + a.size.x + 1.5 * a.size.x, .y = a.pos.y + a.size.y};
	a.v1 = v1;
	a.v2 = v2;
	a.v3 = v3;
	a.dot_padding = dot_padding;
	a.dot_radius = dot_radius;
	arrows[i] = a;
    }

     //////////////////////////////////////////////////////////
    ////////////////*PREPARE SEQUENCES*///////////////////////
    const int SEQ1[] = {0,1,0,3,2,1,3,0,2,3,1,2}; // learning sequence 1
    const int SEQ2[] = {2,0,3,1,0,2,1,2,3,0,1,3}; // learning sequence 2

    size_t SEQUENCE_SIZE = sizeof(SEQ1) / sizeof(SEQ1[0]); // sequence's size
    int    RSEQ[SEQUENCE_SIZE]; // regular learning sequence (85%)
    int    ISEQ[SEQUENCE_SIZE]; // irregular learning sequence (15%)

    int LEARNING_SEQ = 2; // choose between 1 and 2

    if      (LEARNING_SEQ == 1) {
	for (int i = 0; i < SEQUENCE_SIZE; i++) {
	    RSEQ[i] = SEQ1[i];
	    ISEQ[i] = SEQ2[i];
	}
    }
    else if (LEARNING_SEQ == 2) {
	for (int i = 0; i < SEQUENCE_SIZE; i++) {
	    RSEQ[i] = SEQ2[i];
	    ISEQ[i] = SEQ1[i];
	}
    }

     //////////////////////////////////////////////////////////
    /////////////////*SET CURRENT CONTEXT*////////////////////
    int context[CONTEXT_SIZE];
    for (int i = 0; i < CONTEXT_SIZE; i++) { context[i] = RSEQ[i % SEQUENCE_SIZE]; }

    int current_seq = REGULAR_SEQUENCE; // start with regular sequence (TODO randomize)
    int item        = next_expected(context, current_seq       , RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE);
    int reg_item    = next_expected(context, REGULAR_SEQUENCE  , RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE); // to keep track of what is really expected
    int irreg_item  = next_expected(context, IRREGULAR_SEQUENCE, RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE); // to keep track of what is really expected

    int current_block_nb   = 0;
    int current_stimulus_nb = 0;
    int task_started = 0;
    int waiting_for_answer = 1;
    int answer;

     //////////////////////////////////////////////////////////
    ///////////////* SET UP LOG FILE *////////////////////////

    FILE *fp;
    char logfile_name[] = "logfile.txt"; // TODO: adjust this
    fp = fopen(logfile_name, "a");
    fprintf(fp, "block;item;reaction_time;time_from_start;answer;reg_item;irreg_item;shown_item;seq_shown\n");

     //////////////////////////////////////////////////////////
    /////////////////////*SETUP WINDOW*///////////////////////
    InitWindow(w, h, "Serial Reaction Time Task (probabilistic sequence) - v0.1 - https://github.com/dougy147/srtt");

    /*Start window*/
    while (!WindowShouldClose()) {
	BeginDrawing();

	/*check state of block*/
	if (!task_started) {
	    ClearBackground(BG_COLOR); // Clear previous dot
	    char msg[] = "Ready to suffer? :')\n\n\nPress 'Space' to confirm.";
	    int font_size = 50 * factor / 100;
	    DrawText(msg,
		    (w / 2) - (strlen(msg) / 2 * font_size / 2),
		    h / 2,
		    font_size,
		    WHITE);
	    if (IsKeyDown(KEY_SPACE)) {
		task_started = 1;
		time_task_start = get_posix_clock_time();
	    }
	} else if (current_block_nb < NB_BLOCKS && current_stimulus_nb < NB_STIMULI) {
	    ClearBackground(BG_COLOR);

	    draw_arrows(arrows);
	    draw_dot(arrows,item, STIMULI_COLOR); /* draw current stimulus (color choice for debugging only) */
	    /*draw_dot(arrows,item, current_seq == REGULAR_SEQUENCE ? STIMULI_COLOR : RED);*/

	    if (waiting_for_answer) {
		timer_start = get_posix_clock_time(); // Start timer immediately after displaying stimulus
		waiting_for_answer = 0;
	    }

	    /* Update dots each time an answer*/
	    if (IsKeyDown(KEY_C) || IsKeyDown(KEY_V) || IsKeyDown(KEY_B) || IsKeyDown(KEY_N)) {
		timer_stop      = get_posix_clock_time();       // immediately "stop" timer after participant's answer
		time_diff       = timer_stop - timer_start;     // elapsed time since stimulus
		time_diff_total = timer_stop - time_task_start; // elapsed time since beginning of task
		//printf("%" PRIu64 "\n", time_diff);

		answer = GetKeyPressed();
		if      (answer == KEY_C) { answer = 0; }
		else if (answer == KEY_V) { answer = 1; }
		else if (answer == KEY_B) { answer = 2; }
		else if (answer == KEY_N) { answer = 3; }

		// block; item; time_diff; time_diff_total; answer; expected_reg; expected_irreg; showed; current_seq (reg or irreg)
		fprintf(fp, "%d;%d;%"PRIu64";%"PRIu64";%d;%d;%d;%d;%d\n", current_block_nb, current_stimulus_nb, time_diff, time_diff_total, answer, reg_item, irreg_item, item, current_seq); // write to log file

	        update_context(context, CONTEXT_SIZE, item);
	        reg_item    = next_expected(context, REGULAR_SEQUENCE, RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE);
	        irreg_item  = next_expected(context, IRREGULAR_SEQUENCE, RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE);
	        current_seq = choose_sequence();
	        item = next_expected(context, current_seq, RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE);
	        current_stimulus_nb++;
	        WaitTime(SOA);
		waiting_for_answer = 1;
	    }

	} else if (current_block_nb < NB_BLOCKS - 1 ) {
	    ClearBackground(BG_COLOR); // Clear previous dot
	    char msg[] = "Pause. Press 'Space' to continue.";
	    int font_size = 50 * factor / 100;
	    DrawText(msg,
		    (w / 2) - (strlen(msg) / 2 * font_size / 2),
		    h / 2,
		    font_size,
		    WHITE);
	    if (IsKeyDown(KEY_SPACE)) {
		current_stimulus_nb = 0;
		current_block_nb++;
		waiting_for_answer = 1;
	    }
	} else { // nb blocks >= 8
	    ClearBackground(BG_COLOR); // Clear previous dot
	    char msg[] = "End of SRTT.\n\nThanks for participating :^)";
	    int font_size = 50 * factor / 100;
	    DrawText(msg,
		    w / 2 - (strlen(msg) / 2 * font_size / 2),
		    h / 2,
		    font_size,
		    WHITE);
	}

	EndDrawing();
    }

    fclose(fp); // close log file
    CloseWindow();
    return 0;
}
