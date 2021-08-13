/**
 * Reading from multiple sockets
 * This version uses zmq_poll()
 * The approach with poll will not work,
 * since myServerIP will send the msg twice to the first who connects thus the 3rd will starve.
 * I tried with PUB-SUB but i couldn't get it to work.
 */
#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main(int argc,char *argv[])
{
	void *context = zmq_ctx_new();

	if (argc != 4)
	{
		printf("not enough arguments\n");
		return -1;
	}

	char myPort[10]= {0};
	char serverPort2[10]= {0};
	char serverPort3[10]= {0};
	char myServerIP[256]= {0};
	char serverIP2[256]= {0};
	char serverIP3[256]= {0};
	int counter1 = 0;
	int counter2 = 0;

	printf("ports:[%s] [%s] [%s]\n", argv[1], argv[2], argv[3]);

	memcpy(myPort, argv[1], strlen(argv[1]));
	memcpy(serverPort2, argv[2], strlen(argv[2]));
	memcpy(serverPort3, argv[3], strlen(argv[3]));

	sprintf(myServerIP, "tcp://*:%s", myPort);
	sprintf(serverIP2, "tcp://localhost:%s", serverPort2);
	sprintf(serverIP3, "tcp://localhost:%s", serverPort3);

    void *responder = zmq_socket(context, ZMQ_PUSH);
    int rc = zmq_bind(responder, myServerIP);
    assert (rc == 0);

    void *reqServer2 = zmq_socket(context, ZMQ_PULL);
    void *reqServer3 = zmq_socket(context, ZMQ_PULL);
    zmq_connect(reqServer2, serverIP2);
    zmq_connect(reqServer3, serverIP3);

    // Initialize random number generator
    srandom ((unsigned) time (NULL));
	int myRandomNum = rand() % 500;

	char sendBuffer [10] = {0};
	sprintf(sendBuffer, "%d", myRandomNum);
	printf("myRandomNum is: [%d]\n", myRandomNum);

    zmq_pollitem_t items [] = {
        { reqServer2, 0, ZMQ_POLLIN, 0 },
        { reqServer3, 0, ZMQ_POLLIN, 0 }
    };

	printf("here1\n");
	zmq_send(responder, sendBuffer, 5, 0);
	printf("here as well\n");
	zmq_send(responder, sendBuffer, 5, 0);

    //  Process messages from both sockets
    while(1)
	{
        char msg [256];

        int rc = zmq_poll(items, 2, -1);
		assert (rc >= 0);

        if(items[0].revents & ZMQ_POLLIN)
		{
            int size = zmq_recv(reqServer2, msg, 255, 0);
            if(size != -1)
			{
				printf("from reqServer2: [%s]\n", msg);
				myRandomNum += atoi(msg);
				memset(msg, 0, sizeof(msg));
				counter1++;
            }
        }

        if(items[1].revents & ZMQ_POLLIN)
		{
            int size = zmq_recv(reqServer3, msg, 255, 0);
            if(size != -1)
			{
				printf("from reqServer3: [%s]\n", msg);
				myRandomNum += atoi(msg);
				memset(msg, 0, sizeof(msg));
				counter2++;
            }
        }
		if (counter1 && counter2 ) break;
    }

	printf("The final number is : [%d]\n", myRandomNum);
    zmq_close(reqServer2);
    zmq_close(reqServer3);
    zmq_close(responder);
    zmq_ctx_destroy(context);
    return 0;
}
