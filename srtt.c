#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <raylib.h>
#include <time.h>
#include <inttypes.h> // print uint64_t (timer)


#define AUTO_GENERATE_SEQUENCES 1      // Not recommanded if NB_LOCATIONS >= 6 (not a smart function)

#define NB_LOCATIONS        4          // Stimuli possible locations
#define NB_BLOCKS    	    8          // Blocks are cut with pauses
#define NB_STIMULI   	    120        // Number of stimuli by block
#define CONDITIONAL_ORDER   2          // Conditional order of sequences
#define SOA          	    0.250      // Stimulus Onset Asynchrony (in seconds)
#define P_REG        	    0.85       // Regular sequence probability
#define P_IRREG        	    1 - P_REG  // Regular sequence probability
#define CONTEXT_SIZE CONDITIONAL_ORDER // Sequence context size depends on conditional order

#define PLACEHOLDERS_TYPE   HANGMAN    // ARROW or HANGMAN
#define BG_COLOR            BLACK      // Background color
#define PLACEHOLDERS_COLOR  DARKGRAY
#define STIMULI_COLOR       WHITE

#define DEBUG 0

///////////////////////////////////////////////////////////
/*
   Check this link for time support lead:
    https://stackoverflow.com/a/37920181/22665005
*/
///////////////////////////////////////////////////////////

enum {
    REGULAR_SEQUENCE,
    IRREGULAR_SEQUENCE,
} Sequence;

/*placeholders can be arrows above stimulus or rectangles under (hangman)*/
enum {
    ARROW,
    HANGMAN,
};

typedef struct Placeholder {
    Vector2 size;     // Vertical line (if arrow) / rect width (if hangman)
    Vector2 pos;      // On screen position
    int index;        // Placeholders are indexed given their position from left to right on screen
    int stimulus_pos; // Relative stimulus' position to placeholder
    int type;         // Placeholder own type (ARROW, HANGMAN, etc.)
    Vector2 v1;       // (for ARROWS only) left vertex of the arrow triangle
    Vector2 v2;       // (for ARROWS only) center vertex of the arrow triangle
    Vector2 v3;       // (for ARROWS only) right vertex of the arrow triangle
} Placeholder;

typedef struct Stimulus {
    int radius; // Stimuli are just circle for now
} Stimulus;

void draw_placeholders(Placeholder *ph) {
    switch (ph[0].type) {
	case ARROW:
	    for (int i = 0; i < NB_LOCATIONS; i++) {
    	        DrawRectangleV(ph[i].pos, ph[i].size,    PLACEHOLDERS_COLOR);
    	        DrawTriangle(ph[i].v1,ph[i].v2,ph[i].v3, PLACEHOLDERS_COLOR);
    	    }
	    break;
	case HANGMAN:
	    for (int i = 0; i < NB_LOCATIONS; i++) {
    	        DrawRectangleV(ph[i].pos, ph[i].size, PLACEHOLDERS_COLOR);
    	    }
	    break;
    }
}

void draw_stimulus(Placeholder *arrows, int arrow_index, Color color, Stimulus stimulus) {
    DrawCircle(
	    arrows[arrow_index].pos.x + (arrows[arrow_index].size.x / 2),
	    arrows[arrow_index].pos.y + arrows[arrow_index].size.y + (arrows[arrow_index].v2.y - arrows[arrow_index].v1.y) + arrows[arrow_index].stimulus_pos,
	    stimulus.radius,
	    color
	    );
}

int next_expected(int *context, int choice, int *regular_sequence, int *irregular_sequence, size_t size_context, size_t size_sequence) {
    /* Given a context and a sequence, find expected value */
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

void update_context(int *context, size_t size_context, int current_value) {
    for (int i = 0; i < size_context - 1; i++) {
	context[i] = context[i+1];
    }
    context[size_context - 1] = current_value;
}

int contexts_collide(int *seq1, int *seq2, size_t size_context, size_t size_sequence) {
    for (int i = 0; i < size_sequence; i++) {
	for (int j = 0; j < size_sequence; j++) {
	    for (int k = 0; k < CONTEXT_SIZE; k++) {
		if (seq1[(i+k) % size_sequence] != seq2[(j+k) % size_sequence]) {break;}
		if (k == CONTEXT_SIZE - 1) {
		    if (seq1[(i+k+1) % size_sequence] == seq2[(j+k+1) % size_sequence]) {
			return -1; // same expectation for same context. mustn't happen
		    }
		}
	    }
	}
    }
    return 0;
}

int choose_sequence(void) {
	if ( rand() < RAND_MAX * P_REG ) {
	    return REGULAR_SEQUENCE;
	} else {
	    return IRREGULAR_SEQUENCE;
	}
}

uint64_t get_posix_clock_time(void) {
    struct timespec ts;
    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0) { return (uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000); }
    else { return 0; }
}

int generate_sequence(int *sequence) {
    /* Dumb function to generate random sequences */
    if (CONDITIONAL_ORDER == 1) {
	int selected_indexes[NB_LOCATIONS];
	int new_sequence[NB_LOCATIONS];
	int added = 0;
	while (added < NB_LOCATIONS) {
	    int random_index = rand() % NB_LOCATIONS;
	    int add = 1;
	    for (int j = 0; j < added; j++) {
		if (selected_indexes[j] == random_index) add = 0;
	    }
	    if (add == 1) {
		selected_indexes[added] = random_index;
		sequence[added] = random_index;
		added++;
	    }
	}
	return 0;
    }
    int  iterations = 0; //arbitrarily stops when iterations are too high (DUMB)
    int  max_nb_context = NB_LOCATIONS * (NB_LOCATIONS - 1);
    char contexts[max_nb_context][CONTEXT_SIZE * CONTEXT_SIZE];
    int  index = 0;
    for (int i = 0; i < NB_LOCATIONS; i++) {
        for (int j = 0; j < NB_LOCATIONS ; j++) {
    	if (i != j) {
    	    char c[CONTEXT_SIZE];
    	    snprintf(c, CONTEXT_SIZE * CONTEXT_SIZE, "%d%d", i, j);
    	    snprintf(contexts[index], CONTEXT_SIZE * CONTEXT_SIZE, c);
    	    index++;
	   }
        }
    }
    char random_arrangement[max_nb_context][CONTEXT_SIZE * CONTEXT_SIZE]; // random arrangement of context (01 -> 12 -> 20 -> 03 ...)
    int  grabbed_indexes[max_nb_context];
    int selected = 0;
    int collision = 0;
    while ( selected < max_nb_context) {
        if (iterations++ > max_nb_context * max_nb_context * max_nb_context) { return -1; };
        int random_index = rand() % max_nb_context;
        for (int i = 0; i < selected; i++) { if (grabbed_indexes[i] == random_index) continue; }
        char s[CONTEXT_SIZE*CONTEXT_SIZE];
        snprintf(s, CONTEXT_SIZE * CONTEXT_SIZE, contexts[random_index]);
        /*check that we haven't stored that context already*/
        for (int i = 0; i < selected; i++) {
	   if (strcmp(s, random_arrangement[i]) == 0) {
    	       collision = 1;
    	       break;
	   }
        }
        if (collision == 1) { collision = 0; continue; }
        /*check this context is linkable with the previous one*/
        if (selected > 0) {
	   if (s[0] != random_arrangement[selected-1][CONTEXT_SIZE-1]) {
    	       collision = 0;
    	       continue;
	   }
        }
        /*context is valid, append it and continue*/
        snprintf(random_arrangement[selected], CONTEXT_SIZE * CONTEXT_SIZE, s);
        collision = 0;
        grabbed_indexes[selected] = random_index;
        selected++;
    }
    for (int i = 0; i < max_nb_context; i++) {
        sequence[i] = random_arrangement[i][0] - '0';
    }
    return 0;
}

int main(void)
{
    srand(time(NULL)); // random seed for random functions
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
    if ( (w - (2 * wo)) % (NB_LOCATIONS * 2) != 0) {
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
    float unit_width = 0.01 * gw; // elementary brick to construct other elements (arbitrarily related to width)
    int stimulus_padding  = (1.0 / 32.0) * gh; // distance from the end of an arrow

    Stimulus stimulus;
    stimulus.radius = ((1.5 * unit_width) + unit_width + (1.5 * unit_width)) / 2; // size of the arrow triangle base (from v1 to v3)

    //int PLACEHOLDERS_TYPE = ARROW;
    Placeholder placeholders[NB_LOCATIONS]; // Arrows array

    switch (PLACEHOLDERS_TYPE) {
	case ARROW:
    	    for (int i = 0; i < NB_LOCATIONS; i++) {
    	        Placeholder a;
		a.type = ARROW;
    	        a.index = i;
    	        /*lines*/
    	    	a.size.x = unit_width;
    	    	a.size.y = gh / 2;
    	    	a.pos.x = wo + ((i+1) * gw / (NB_LOCATIONS * 2)) + ( i * (gw / (NB_LOCATIONS * 2))) - (a.size.x / 2);
    	    	a.pos.y = ho;
    	        /*triangles*/
    	        Vector2 v1 = { .x = a.pos.x - 1.5 * a.size.x, .y = a.pos.y + a.size.y};
    	    	Vector2 v2 = { .x = a.pos.x + (a.size.x / 2), .y = (a.pos.y + a.size.y) + 4 * a.size.x};
    	    	Vector2 v3 = { .x = a.pos.x  + a.size.x + 1.5 * a.size.x, .y = a.pos.y + a.size.y};
    	        a.v1 = v1;
    	        a.v2 = v2;
    	        a.v3 = v3;
    	        a.stimulus_pos = stimulus_padding + stimulus.radius;
    	        placeholders[i] = a;
    	    }
	    break;

	case HANGMAN:
    	    for (int i = 0; i < NB_LOCATIONS; i++) {
    	        Placeholder a;
		a.type = HANGMAN;
    	        a.index = i;
    	        /*rectangles*/
    	    	a.size.x = stimulus.radius * 2;
    	    	a.size.y = stimulus.radius / 4;
    	    	a.pos.x = wo + ((i+1) * gw / (NB_LOCATIONS * 2)) + ( i * (gw / (NB_LOCATIONS * 2))) - (a.size.x / 2);
    	    	a.pos.y = ho + gh - ((1.0 / 4.0) * gh);
    	        a.stimulus_pos = - stimulus_padding - stimulus.radius;
    	        placeholders[i] = a;
    	    }
	    break;
    }

     //////////////////////////////////////////////////////////
    ////////////////*PREPARE SEQUENCES*///////////////////////
    int seq_length = CONDITIONAL_ORDER == 1 ? NB_LOCATIONS : NB_LOCATIONS * (NB_LOCATIONS - 1);
    int SEQ1[seq_length];
    int SEQ2[seq_length];

    if (AUTO_GENERATE_SEQUENCES == 1) {  	// costly function
	while (generate_sequence(SEQ1) < 0) {}; // sequence 1
	while (generate_sequence(SEQ2) < 0) {}; // sequence 2
	do { generate_sequence(SEQ2); } 	// while sequences are equal OR same context gives same output, continue.
	    while (memcmp(SEQ1, SEQ2, sizeof(SEQ1)) == 0 || contexts_collide(SEQ1, SEQ2, CONTEXT_SIZE, seq_length) != 0);
    } else { // use default sequence==s
	int dSEQ1[] = {0,1,0,3,2,1,3,0,2,3,1,2}; // default sequence 1
    	int dSEQ2[] = {2,0,3,1,0,2,1,2,3,0,1,3}; // default sequence 2
	for (int i = 0; i < seq_length; i++) { SEQ1[i] = dSEQ1[i]; SEQ2[i] = dSEQ2[i]; }
    }

    size_t SEQUENCE_SIZE = sizeof(SEQ1) / sizeof(SEQ1[0]); // sequence's size
    int    RSEQ[SEQUENCE_SIZE]; // regular learning sequence (with probability P_REG)
    int    ISEQ[SEQUENCE_SIZE]; // irregular learning sequence (15%)

    //int LEARNING_SEQ = 2; // choose between 1 and 2
    int LEARNING_SEQ = (rand() < RAND_MAX * 0.5) ? 0 : 1; // randomize regular sequence selection

    if      (LEARNING_SEQ == 0) {
	for (int i = 0; i < SEQUENCE_SIZE; i++) {
	    RSEQ[i] = SEQ1[i];
	    ISEQ[i] = SEQ2[i];
	}
    }
    else if (LEARNING_SEQ == 1) {
	for (int i = 0; i < SEQUENCE_SIZE; i++) {
	    RSEQ[i] = SEQ2[i];
	    ISEQ[i] = SEQ1[i];
	}
    }

    /*Prepare sequences strings for logfile*/
    int  length = CONDITIONAL_ORDER == 1 ? NB_LOCATIONS : NB_LOCATIONS * (NB_LOCATIONS - 1);
    char regular_string[length];
    char irregular_string[length];
    for (int i=0; i<length; i++) { sprintf(&regular_string[i],   "%d", RSEQ[i]); }
    for (int i=0; i<length; i++) { sprintf(&irregular_string[i], "%d", ISEQ[i]); }

     //////////////////////////////////////////////////////////
    /////////////////*SET CURRENT CONTEXT*////////////////////
    int context[CONTEXT_SIZE];
    /*randomly select the sequence to start with (according to desired probabilities; e.g. P_REG = .85)*/
    int current_seq = (rand() < RAND_MAX * P_REG) ? REGULAR_SEQUENCE : IRREGULAR_SEQUENCE;

    /*randomly select a context to start with*/
    int random_index = rand() % (SEQUENCE_SIZE + 1);
    for (int i = 0; i < CONTEXT_SIZE; i++) { context[i] = RSEQ[(i + random_index) % SEQUENCE_SIZE]; }

    int item        = next_expected(context, current_seq       , RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE);
    int reg_item    = next_expected(context, REGULAR_SEQUENCE  , RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE); // to keep track of what is really expected
    int irreg_item  = next_expected(context, IRREGULAR_SEQUENCE, RSEQ, ISEQ, CONTEXT_SIZE, SEQUENCE_SIZE); // to keep track of what is really expected

    int current_block_nb    = 0; // from 0 to nb_blocks - 1
    int current_stimulus_nb = 0; // from 0 to stimuli by block - 1
    int task_started        = 0;
    int waiting_for_answer  = 1;
    int answer; // participant's answer

     //////////////////////////////////////////////////////////
    ///////////////* SET UP LOG FILE *////////////////////////

    FILE *fp;

    char date[50]; // YYYY-MM-DD_hh-mm-ss (to store in participant's results' file)
    time_t now = time(NULL);
    strftime(date, sizeof(date)-1, "%Y-%m-%d_%H-%M-%S", localtime(&now));

    int    random_id = rand() % 999999; // a million ID should be enough...

    char   participant_file[50]; // result_RANDOMID_YYYY-MM-DD_hh-mm-ss.txt
    snprintf(participant_file, sizeof(participant_file), "result_%d_%s.txt", random_id, date);

    fp = fopen(participant_file, "a");
    fprintf(fp, "block;item;reaction_time;time_from_start;answer;reg_item;irreg_item;shown_item;seq_shown;%s;%s\n",regular_string,irregular_string);

     //////////////////////////////////////////////////////////
    /////////////////////*SETUP WINDOW*///////////////////////
    InitWindow(w, h, "Serial Reaction Time Task (probabilistic sequence) - v0.1 - https://github.com/dougy147/srtt");

    /*Start window*/
    while (!WindowShouldClose()) {
	BeginDrawing();

	/*check state of block*/
	if (!task_started) {
	    ClearBackground(BG_COLOR); // Clear previous stimulus

	    /*If DEBUG mode, displays REG and IRREG sequences on screen.*/
	    if (DEBUG) {
		DrawText(TextFormat("REG: %s",regular_string), 0, 0, 50 * factor / 100, RED);
		DrawText(TextFormat("IRR: %s",irregular_string), 0, 50 * factor / 100, 50 * factor / 100, RED);
	    }

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

	    draw_placeholders(placeholders);
	    if (DEBUG) {
		draw_stimulus(placeholders,item, current_seq == REGULAR_SEQUENCE ? STIMULI_COLOR : RED, stimulus);
	    } else {
		draw_stimulus(placeholders,item, STIMULI_COLOR, stimulus); /* draw current stimulus (color choice for debugging only) */
	    }

	    if (waiting_for_answer) {
		timer_start = get_posix_clock_time(); // Start timer immediately after displaying stimulus
		waiting_for_answer = 0;
	    }

	    /* Update stimulus each time an answer*/
	    if (IsKeyDown(KEY_C) || IsKeyDown(KEY_V) || IsKeyDown(KEY_B) || IsKeyDown(KEY_N)) {
		timer_stop      = get_posix_clock_time();       // immediately "stop" timer after participant's answer
		time_diff       = timer_stop - timer_start;     // elapsed time since stimulus
		time_diff_total = timer_stop - time_task_start; // elapsed time since beginning of task

		answer = GetKeyPressed();
		if      (answer == KEY_C) { answer = 0; }
		else if (answer == KEY_V) { answer = 1; }
		else if (answer == KEY_B) { answer = 2; }
		else if (answer == KEY_N) { answer = 3; }

		fprintf(fp, "%d;%d;%"PRIu64";%"PRIu64";%d;%d;%d;%d;%d\n",
			current_block_nb, current_stimulus_nb,
			time_diff, time_diff_total,
			answer,
			reg_item, irreg_item,
			item, current_seq); // write to log file

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
	    ClearBackground(BG_COLOR); // Clear previous stimulus
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
	    ClearBackground(BG_COLOR); // Clear previous stimulus
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
