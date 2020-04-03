/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Project headers
#include "timer.h"

// Standard headers
#include <stdio.h>
#include <stdlib.h>


#define fatal_error(msg, ...) \
{ \
    fprintf(stderr, "FATAL ERROR in %s, line %d: " msg "\n", __func__, __LINE__, ##__VA_ARGS__); \
    exit(-1); \
}


TimerManager *tman = NULL;
char *my_lovely_test_string = "hello";


static void my_timer_handler(unsigned id, void *user_data)
{
    char *user_data_as_string = user_data;
    printf("handler called. id=%d user_data=%s\n", id, user_data_as_string);
    if (id == 111) {
        printf("Setting timer %d duration to 1 second.\n", id);
        if (timer_manager_set_duration(tman, id, 1) != 0)
            fatal_error("Couldn't set timer %d duration", id);
    }

    printf("Restarting timer %d\n", id);
    if (timer_manager_start_timer(tman, id) != 0)
        fatal_error("Couldn't start timer %d", id);
}


#ifdef _MSC_VER
# define sleep(x) Sleep(x * 1000)
# include <windows.h>
#else
# include <unistd.h>
#endif

void run_for_a_while()
{
    for (int i = 0; i < 5; i++) {
        puts("Calling do_tick");
        timer_manager_do_tick(tman);
        sleep(1);
    }
}


int main()
{
    tman = timer_manager_create(my_timer_handler, my_lovely_test_string);
    if (timer_manager_create_timer(tman, 111, 3) != 0)
        fatal_error("Couldn't create timer");

    if (timer_manager_create_timer(tman, 222, 2) != 0)
        fatal_error("Couldn't create timer");

    if (timer_manager_start_timer(tman, 111) != 0)
        fatal_error("Couldn't start timer");

    if (timer_manager_start_timer(tman, 222) != 0)
        fatal_error("Couldn't start timer");

    char *state_string = timer_manager_save_state(tman);
    printf("State string is: %s\n", state_string);

    run_for_a_while();

    if (timer_manager_set_duration(tman, 111, 30) != 0)
        fatal_error("Couldn't set duration of timer 111");   

    if (timer_manager_load_state(tman, state_string) != 0)
        fatal_error("Couldn't load state string");
    
    timer_manager_delete(tman);
    puts("\nStarting again\n");
    tman = timer_manager_create(my_timer_handler, NULL);
    if (timer_manager_load_state(tman, state_string) != -1)
        fatal_error("COULD load state string when it should have failed");

    free(state_string);
    timer_manager_delete(tman);

    return 0;
}
