#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include "include/rf.h"
#include "include/utils.h"
#include "include/rf_receiver.h"
#include "include/rf_transmitter.h"
#include "include/so_transmitter.h"
#include "include/logger.h"
#include "include/NRF24.h"
#include "include/so_receiver.h"
#include "include/gate_in.h"
#include "include/gate_out.h"
#include "libs/collections/include/rings.h"
#include "libs/collections/include/lbq.h"
#include "libs/collections/include/map2.h"
#include "libs/collections/include/list.h"
#include "libs/collections/include/treeset.h"

// ================================ GLOBAL VARIABLES ====================================

char *pid_path = "/var/run/wsd.pid";
LinkedBlockingQueue *gateInQueue;
LinkedBlockingQueue *rfTransmitterQueue;
LinkedBlockingQueue *gateOutQueue;
LinkedBlockingQueue *soTransmitterQueue;
RingBufferDef *soTransmitterRing;
RingBufferDef *rfReceiverRing;
Map *globSoMap;

/** Start domain server. This server listen for incoming bind request. After accept a connection, it create
 *  new client thread with client socket id, and try to accept a new connections */
void startServer() {
    int server_sockfd, client_sockfd;
    int server_len, client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    char *socket_path = "/tmp/wsd.socket";

    unlink(socket_path);
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, socket_path);
    server_len = sizeof(server_address);
    bind(server_sockfd, (struct sockaddr *)&server_address, server_len);

    listen(server_sockfd, 5);

    Logger_info("Server", "Server listen '%s'", socket_path);
    while(1) {
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);

        if (client_sockfd == -1) {
            Logger_fatal("Server", "Unable to open socket '%s'", socket_path);
            exit(-1);
        }

        pthread_t thread;
        SoReceiverThreadArgs *args = malloc(sizeof(SoReceiverThreadArgs));  //Free in cmd_processor
        args->socket = client_sockfd;
        args->globSoMap = globSoMap;
        args->downQueue = gateInQueue;
        pthread_create(&thread, NULL, (void *) SoReceiver_thread, args);
    }
}

void startDaemon() {
	gateInQueue  = new_LQB(128);
	rfTransmitterQueue  = new_LQB(128);
	soTransmitterQueue  = new_LQB(128);
	gateOutQueue = new_LQB(128);
	globSoMap = MAP_new();
	soTransmitterRing  = RINGS_createRingBuffer(128, RINGS_OVERFLOW_SHIFT, true);
	rfReceiverRing  = RINGS_createRingBuffer(1024, RINGS_OVERFLOW_SHIFT, true);
	Map *tidSoMap = MAP_new();

	int ir = RF_init();
	if (ir != 0) {
		Logger_fatal("DaemonRunner/startDaemon", "Unable to initialize rf harware, code %d", ir);
		exit(0);
	}
	Logger_info("DaemonRunner/startDaemon", "Rf hardware was initialized");

	pthread_t gateInThread;
	GateInThreadArgs *gateInArgs = malloc(sizeof(GateInThreadArgs));
	gateInArgs->upQueue = gateInQueue;
	gateInArgs->rfTransmitterQueue = rfTransmitterQueue;
	gateInArgs->soTransmitterQueue = soTransmitterQueue;
	gateInArgs->tidSoMap = tidSoMap;
	pthread_create(&gateInThread, NULL, (void *) GateIn_thread, gateInArgs);

	pthread_t gateOutThread;
	GateOutThreadArgs *gateOutThreadArgs = malloc(sizeof(GateOutThreadArgs));  //Free in cmd_processor
	gateOutThreadArgs->upQueue = gateOutQueue;
	gateOutThreadArgs->soTransmitterQueue = soTransmitterQueue;
	gateOutThreadArgs->tidSoMap = tidSoMap;
	pthread_create(&gateOutThread, NULL, (void *) GateOut_thread, gateOutThreadArgs);

	pthread_t rfTransmitterThread;
	RfTransmitterArgs *rfTransmitterArgs = malloc(sizeof(RfTransmitterArgs));
	rfTransmitterArgs->upQueue = rfTransmitterQueue;
	rfTransmitterArgs->soTransmitterQueue = soTransmitterQueue;
	pthread_create(&gateInThread, NULL, (void *) RfTransmitter_thread, rfTransmitterArgs);

	pthread_t soTransmitterThread;
	SoTransmitterArgs *soTransmitterArgs = malloc(sizeof(SoTransmitterArgs));
	soTransmitterArgs->upQueue = soTransmitterQueue;
	soTransmitterArgs->tidSoMap = tidSoMap;
	soTransmitterArgs->globSoMap = globSoMap;
	pthread_create(&soTransmitterThread, NULL, (void *) SoTransmitter_thread, soTransmitterArgs);

	pthread_t rfReceiverThread;
	RfReceiverArgs *rfReceiverThreadArgs = malloc(sizeof(RfReceiverArgs));
	rfReceiverThreadArgs->downQueue = gateOutQueue;
	pthread_create(&rfReceiverThread, NULL, (void *) RfReceiver_thread, rfReceiverThreadArgs);


	/*pthread_t rfReceiverThread;
	pthread_create(&rfReceiverThread, NULL, (void *) RfReceiver_thread, NULL);
	pthread_t rfTransmitterThread;
	pthread_create(&rfTransmitterThread, NULL, (void *) RfTransmitter_thread, NULL);*/
	startServer();
}

/** Daemon signal handler. This handler handle only one signal SIGUSR1. This signal sent by daemon launcher from
 *  stop logic branch. After receive this signal daemon will terminates */
void signalHandler(int sig) {
    if (sig == SIGUSR1) {
        exit(0);
    }
}

/** Start new daemon instance. It fork the program process and write it pid to the nsd.pid file. Child process
 *  run new domain socket server. */
int start() {
    //check pid file existing
    if (access(pid_path, 0) == 0)  {
        Logger_fatal("DaemonRunner/start", "Daemon already started");

        return -1;
    }

    int pid = fork();

    if (pid == -1) { // если не удалось запустить потомка
        Logger_fatal("DaemonRunner", "Start Daemon failed (%s)", strerror(errno));

        return -1;
    } else if (!pid) { // если это потомок
        umask(0);
        setsid();

        signal(SIGUSR1, signalHandler);
        startDaemon();

        return 0;
    } else { // если это родитель
        int pidf = open(pid_path, O_RDWR | O_CREAT | O_TRUNC, 0644);

        if (pidf == -1) {
            Logger_fatal("DaemonRunner/start", "Unable to create pid file");

            return -1;
        }

        char *pstr = itoa2(pid);
        if (write(pidf, pstr, strlen(pstr)) <= 0) {
            Logger_fatal("DaemonRunner/start", "Unable to write pid file");

            return -1;
        }
        free(pstr);

        Logger_info("DaemonRunner/start", "Daemon has been successfully started with pid '%d'", pid);

        return 0;
    }
}

/** Stop current daemon instance. It is send SIGUSR1 to the daemon process with pid from nsd.pid file and remove thi
 * file */
int stop() {
    //check pid file existing
    if (access(pid_path, 0) != 0)  {
        Logger_fatal("DaemonRunner/stop", "Daemon does not started");

        return -1;
    } else {
        int pidf = open(pid_path, O_RDWR, 0644);

        if (pidf == -1) {
            Logger_fatal("DaemonRunner/stop", "Unable to open pid file");

            return -1;
        }

        lseek(pidf, 0L, SEEK_END);
        size_t fSize = (size_t) lseek(pidf, 0, SEEK_CUR);
        lseek(pidf, 0L, SEEK_SET);

        char *pids = malloc(fSize);
        read(pidf, pids, fSize);
        int pid = atoi(pids);
        free(pids);

        Logger_info("DaemonRunner/stop", "Daemon with pid '%d' has been requested to stopped", pid);

        kill(pid, SIGUSR1); //Send stop signal to the daemon process

        remove(pid_path);

        return 0;
    }
}

/** Stop and Start daemon */
int restart() {
    int r1 = stop();
    int r2 = start();

    if (r1 < 0 || r2 < 0 ) {
        Logger_fatal("DaemonRunner/restart", "Unable to restart daemon");

        return -1;
    }

    return 0;
}

/** Return status of the current daemon instance */
int status() {
    if (access(pid_path, 0) != 0)  {
        Logger_fatal("DaemonRunner/status", "Daemon does not started");

        return 0;
    } else {
        int pidf = open(pid_path, O_RDWR, 0644);

        if (pidf == -1) {
            Logger_fatal("DaemonRunner/status", "Unable to open pid file");

            return -1;
        }

        lseek(pidf, 0L, SEEK_END);
        size_t fSize = (size_t) lseek(pidf, 0, SEEK_CUR);
        lseek(pidf, 0L, SEEK_SET);

        char *pids = malloc(fSize);
        read(pidf, pids, fSize);
        int pid = atoi(pids);
        free(pids);

        Logger_info("DaemonRunner/status", "Daemon run with pid '%d'", pid);
    }

    return 0;
}

/** Parse input arg and run specified subprogram */
int daemonRun(int argc, char* argv[]) {
	if (Logger_init("/var/log/wsd.log") < 0) {
		printf("Unable to initialize logger. Program will be terminated!");
	    fflush(stdout);
	    exit(1);
	}

    if (argc < 2) {
        Logger_fatal("DaemonRunner", "No action specified");

        return -1;
    }

    if (strcmp(argv[1], "start") == 0) {
        return start();
    } else if(strcmp(argv[1], "stop") == 0) {
        return stop();
    } else if(strcmp(argv[1], "restart") == 0) {
        return restart();
    } else if(strcmp(argv[1], "status") == 0) {
        return status();
    } else {
        Logger_fatal("DaemonRunner", "Unknown action '%s'", argv[1]);
    }

    return 0;
}

/** This function used to start program in the developing mode. In this mode, fork does not take place, and execution
 *  immediately begins from startServer function */
int devRun(int argc, char* argv[]) {
	Logger_init("/var/log/wsd.log");

	umask(0);
    startDaemon();

    return 0;
}

int main(int argc, char* argv[]) {
	//volatile uint8_t x = 0x4A;
	//x = x << 4;
	//x = x >> 5;

	return devRun(argc, argv);
	//return daemonRun(argc, argv);

	/*volatile TreeSet *set = TreeSet_new(true);
	TreeSet_put(set, "2", 2);
	TreeSet_put(set, "1", 2);
	TreeSet_put(set, "3", 2);*/
	//TreeSet_put(set, "d", 2);
	//TreeSet_put(set, "e", 2);

	return 0;

}
