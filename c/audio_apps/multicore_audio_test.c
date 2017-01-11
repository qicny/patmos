#include "libaudio/audio.h"
#include "libaudio/audio.c"

const int LIM = 1000;
//master core
const int NOC_MASTER = 0;

int allocFX(struct AudioFX *FXp, int cpuid) {

    int FX_HERE = 0; //amount of effects in this core

    //read current FX_SCHED, SEND_ARRAY and RECV_ARRAY
    int FX_SCHED[MAX_FX][8];
    int SEND_ARRAY[MAX_FX][CHAN_AMOUNT];
    int RECV_ARRAY[MAX_FX][CHAN_AMOUNT];
    for(int fx=0; fx<FX_AMOUNT[current_mode]; fx++) {
        for(int col=0; col<8; col++) { //FX_SCHED first
            FX_SCHED[fx][col] = *((FX_SCHED_P[current_mode]) + fx*8 + col);
        }
        for(int ch=0; ch<CHAN_AMOUNT; ch++) { //then, SEND_ARRAY and RECV_ARRAY
            SEND_ARRAY[fx][ch] = *((SEND_ARRAY_P[current_mode]) + fx*CHAN_AMOUNT + ch);
            RECV_ARRAY[fx][ch] = *((RECV_ARRAY_P[current_mode]) + fx*CHAN_AMOUNT + ch);
        }
    }


    printf("FX_SCHED[%d][8] = {\n", FX_AMOUNT[current_mode]);
    for(int fx=0; fx<FX_AMOUNT[current_mode]; fx++) {
        printf("  { ");
        for(int col=0; col<8; col++) { //FX_SCHED first
            printf("%d, ", FX_SCHED[fx][col]);
        }
        printf(" },\n");
    }
    printf("};\n");

    printf("SEND_ARRAY[%d][%d] = {\n", FX_AMOUNT[current_mode], CHAN_AMOUNT);
    for(int fx=0; fx<FX_AMOUNT[current_mode]; fx++) {
        printf("  { ");
        for(int ch=0; ch<CHAN_AMOUNT; ch++) { //FX_SCHED first
            printf("%d, ", SEND_ARRAY[fx][ch]);
        }
        printf(" },\n");
    }
    printf("};\n");

    printf("RECV_ARRAY[%d][%d] = {\n", FX_AMOUNT[current_mode], CHAN_AMOUNT);
    for(int fx=0; fx<FX_AMOUNT[current_mode]; fx++) {
        printf("  { ");
        for(int ch=0; ch<CHAN_AMOUNT; ch++) { //FX_SCHED first
            printf("%d, ", RECV_ARRAY[fx][ch]);
        }
        printf(" },\n");
    }
    printf("};\n");


    //struct parameters:
    int fx_id;
    fx_t fx_type;
    int xb_size, yb_size, p;
    con_t in_con, out_con;
    int recv_am, send_am;

    // READ FROM SCHEDULER
    int fx_ind = 0;
    for(int n=0; n<FX_AMOUNT[current_mode]; n++) {

        if(FX_SCHED[n][1] == cpuid) { //same core
            //one more FX on this core
            FX_HERE++;
            //assign parameters from SCHEDULER
            fx_id   =         FX_SCHED[n][0];
            fx_type = (fx_t)  FX_SCHED[n][2];
            xb_size =         FX_SCHED[n][3];
            yb_size =         FX_SCHED[n][4];
            p       =         FX_SCHED[n][5];
            in_con  = (con_t) FX_SCHED[n][6];
            out_con = (con_t) FX_SCHED[n][7];
            recv_am = 0;
            send_am = 0;
            for(int ch=0; ch<CHAN_AMOUNT; ch++) {
                if(RECV_ARRAY[n][ch] == 1) {
                    recv_am++;
                }
                if(SEND_ARRAY[n][ch] == 1) {
                    send_am++;
                }
            }
            if(recv_am == 0) { recv_am = 1; } //there is always at least 1
            if(send_am == 0) { send_am = 1; } //there is always at least 1
            //allocate
            alloc_audio_vars(&FXp[fx_ind], fx_id, fx_type, in_con,
                out_con, recv_am, send_am, xb_size, yb_size, p);
            fx_ind++;
        }
    }

    printf("FX_HERE: %d\n", FX_HERE);

    //CONNECT EFFECTS
    for(int n=0; n<FX_HERE; n++) {
        //print input effects
        if( (cpuid == 0) && (*FXp[n].in_con == FIRST) ) {
            printf("FIRST: ID=%d\n", *FXp[n].fx_id);
        }

        // OUTPUT
        int recvChanID[*FXp[n].recv_am]; // channel ID to connect from
        int sendChanID[*FXp[n].send_am]; // channel ID to connect to
        int recv_pnt = 0; //amount of receive channels (for joins)
        int send_pnt = 0; //amount of send    channels (for forks)
        for(int ch=0; ch<CHAN_AMOUNT; ch++) {
            if(RECV_ARRAY[*FXp[n].fx_id][ch] == 1) {
                recvChanID[recv_pnt] = ch; //found channel ID to connect from
                recv_pnt++;
            }
            if(SEND_ARRAY[*FXp[n].fx_id][ch] == 1) {
                sendChanID[send_pnt] = ch; //found channel ID to connect to
                send_pnt++;
            }
        }
        //NoC
        if(*FXp[n].in_con == NOC) {
            for(int r=0; r<*FXp[n].recv_am; r++) {
                audio_connect_from_core(recvChanID[r], &FXp[n], r);
                if(cpuid == 0) {
                    printf("Connected NoC Chanel ID %d to FX ID %d\n", recvChanID[r], *FXp[n].fx_id);
                }
            }
        }
        if(*FXp[n].out_con == NOC) {
            for(int s=0; s<*FXp[n].send_am; s++) {
                audio_connect_to_core(&FXp[n], sendChanID[s], s);
                if(cpuid == 0) {
                    printf("Connected FX ID %d to NoC Chanel ID %d\n", *FXp[n].fx_id, sendChanID[s]);
                }
            }
        }
        //same core
        if(*FXp[n].out_con == SAME) {
            //search for effect in this core with same receive channel
            int con_same_bool = 0;
            for(int m=0; m<FX_HERE; m++) {
                if( (m != n) && (*FXp[m].in_con == SAME) &&
                    (RECV_ARRAY[*FXp[m].fx_id][sendChanID[0]] == 1) ) { //there is just one output
                    con_same_bool = 1; //connection found!
                    audio_connect_same_core(&FXp[n], &FXp[m]);
                    if(cpuid == 0) {
                        printf("Connected FX ID %d to FX ID %d\n", *FXp[n].fx_id, *FXp[m].fx_id);
                    }
                    break;
                }
            }
            if( (cpuid == 0) && (con_same_bool == 0) ) {
                printf("ERROR: Same core connection mismatch: no destination found\n");
            }
        }

        //print output effects
        if( (cpuid == 0) && (*FXp[n].out_con == LAST) ) {
            printf("LAST: ID=%d\n", *FXp[n].fx_id);
        }
    }

    return FX_HERE;
}


void threadFunc(void* args) {
    volatile _UNCACHED int **inArgs = (volatile _UNCACHED int **) args;
    volatile _UNCACHED int *exitP      = inArgs[0];
    volatile _UNCACHED int *allocsDoneP = inArgs[1];


    /*
      AUDIO STUFF HERE
    */

    int cpuid = get_cpuid();

    // -------------------ALLOCATE FX------------------//

    //create structs
    //struct AudioFX FXp[MAX_FX];
    struct AudioFX *FXp = malloc(sizeof(struct AudioFX) * MAX_FX);

    int FX_HERE = allocFX(FXp, cpuid);

    // wait until all cores are ready
    allocsDoneP[cpuid] = 1;
    for(int i=0; i<AUDIO_CORES; i++) {
        while(allocsDoneP[i] == 0);
    }


    // Initialize the communication channels
    int nocret = mp_init_ports();

    //loop
    //audioValuesP[0] = 0;
    //int i=0;
    while(*exitP == 0) {
    //i++;
    //for(int i=0; i<DEBUG_LOOPLENGTH; i++) {

        for(int n=0; n<FX_HERE; n++) {
            if (audio_process(&FXp[n]) == 1) {
                //timeout stuff here
            }
        }

    }

    //free memory allocation
    for(int n=0; n<FX_HERE; n++) {
        free_audio_vars(&FXp[n]);
    }


    // exit with return value
    int ret = 0;
    corethread_exit(&ret);
    return;
}



int main() {

    //arguments to thread 1 function
    int exit = 0;
    int allocsDone[AUDIO_CORES] = {0};
    volatile _UNCACHED int *exitP = (volatile _UNCACHED int *) &exit;
    volatile _UNCACHED int *allocsDoneP = (volatile _UNCACHED int *) &allocsDone;
    volatile _UNCACHED int (*threadFunc_args[1+AUDIO_CORES]);
    threadFunc_args[0] = exitP;
    threadFunc_args[1] = allocsDoneP;

    printf("starting thread and NoC channels...\n");
    //set thread function and start thread
    corethread_t threads[AUDIO_CORES-1];
    for(int i=0; i<(AUDIO_CORES-1); i++) {
        threads[i] = (corethread_t) (i+1);
        corethread_create(&threads[i], &threadFunc, (void*) threadFunc_args);
        printf("Thread created on core %d\n", i+1);
    }

    #if GUITAR == 1
    setup(1); //for guitar
    #else
    setup(0); //for volca
    #endif

    // enable input and output
    *audioDacEnReg = 1;
    *audioAdcEnReg = 1;

    setInputBufferSize(BUFFER_SIZE);
    setOutputBufferSize(BUFFER_SIZE);


    int cpuid = get_cpuid();

    // -------------------ALLOCATE FX------------------//

    //create structs
    //struct AudioFX FXp[MAX_FX];
    struct AudioFX *FXp = malloc(sizeof(struct AudioFX) * MAX_FX);

    int FX_HERE  = allocFX(FXp, cpuid);

    // wait until all cores are ready
    allocsDoneP[cpuid] = 1;
    for(int i=0; i<AUDIO_CORES; i++) {
        while(allocsDoneP[i] == 0);
        printf("core %d alloc done\n", i);
    }

    printf("gonna initialise NoC channels...\n");

    // Initialize the communication channels
    int nocret = mp_init_ports();
    if(nocret == 1) {
        printf("Thread and NoC initialised correctly\n");
    }
    else {
        printf("ERROR: Problem with NoC initialisation\n");
    }

    /*
    printf("SOME INFO: \n");
    printf("FX_HERE=%d\n", FX_HERE);
    printf("fx_id: %d, cpuid: %d, is_fst:%u, is_lst: %u, in_con: %u, out_con: %u\n", *FXp[0].fx_id, *FXp[0].cpuid, *FXp[0].is_fst, *FXp[0].is_lst, *FXp[0].in_con, *FXp[0].out_con);
    printf("x_pnt=0x%x, y_pnt=0x%x, pt=%u, p=%u, rpr=%u, spr=%u, ppsr=%u, xb_size=%u, yb_size=%u, fx=%u, fx_pnt=0x%x\n", *FXp[0].x_pnt, *FXp[0].y_pnt, *FXp[0].pt, *FXp[0].p, *FXp[0].rpr, *FXp[0].spr, *FXp[0].ppsr, *FXp[0].xb_size, *FXp[0].yb_size, *FXp[0].fx, *FXp[0].fx_pnt);
    */


    //CPU cycles stuff
    int CPUcycles[LIM] = {0};
    unsigned int cpu_pnt = 0;


    //int wait_recv = 18; //amount of loops until audioOut is done
    //int wait_recv = LATENCY;
    //for debugging
    //const int WAIT = wait_recv;

    //short audio_in[LIM][2] = {0};
    //short audio_out[LIM][2] = {0};

    while(*keyReg != 3) {

        for(int n=0; n<FX_HERE; n++) {
            audio_process(&FXp[n]);
            /*
              if(n==0) {
              audio_in[cpu_pnt][0] = FXp[n].x[0];
              audio_in[cpu_pnt][1] = FXp[n].x[1];
              }
              if(n==(FX_HERE-1)) {
              audio_out[cpu_pnt-WAIT][0] = FXp[n].y[0];
              audio_out[cpu_pnt-WAIT][1] = FXp[n].y[1];
              }
            */
        }

        //printf("in: %d, %d       out: %d, %d    %d, %d\n", FXp[0].x[0], FXp[0].x[1], FXp[FX_HERE-1].x[0], FXp[FX_HERE-1].x[1], FXp[FX_HERE-1].y[0], FXp[FX_HERE-1].y[1]);


        //store CPU Cycles
        CPUcycles[cpu_pnt] = get_cpu_cycles();
        cpu_pnt++;
        if(cpu_pnt == LIM) {
            //break;
            cpu_pnt = 0;
        }


    }

    //free memory allocation
    for(int n=0; n<FX_HERE; n++) {
        free_audio_vars(&FXp[n]);
    }

    //exit stuff
    printf("exit here!\n");
    exit = 1;
    printf("waiting for all threads to finish...\n");

    /*
    for(int i=1; i<LIM; i++) {
        printf("%d\n", (CPUcycles[i]-CPUcycles[i-1]));
    }
    */

    /*
    for(int i=0; i<(LIM-WAIT); i++) {
        if( (audio_in[i][0] != audio_out[i][0]) || (audio_in[i][1] != audio_out[i][1]) ){
            printf("CORRUPT: i=%d: x[0]=%d, y[0]=%d   :   x[1]=%d, y[1]=%d\n", i, audio_in[i][0], audio_out[i][0], audio_in[i][1], audio_out[i][1]);
        }
    }
    */

    //join with thread 1

    int *retval;
    for(int i=0; i<(AUDIO_CORES-1); i++) {
        corethread_join(threads[i], (void **)&retval);
        printf("thread %d finished!\n", (i+1));
    }

    return 0;
}
