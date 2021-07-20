//  Reading from multiple sockets
//  This version uses zmq_poll()
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
	int counter = 0;

	printf("ports:[%s] [%s] [%s]\n", argv[1], argv[2], argv[3]);

	memcpy(myPort, argv[1], strlen(argv[1]));
	memcpy(serverPort2, argv[2], strlen(argv[2]));
	memcpy(serverPort3, argv[3], strlen(argv[3]));

	sprintf(myServerIP, "tcp://*:%s", myPort);
	sprintf(serverIP2, "tcp://localhost:%s", serverPort2);
	sprintf(serverIP3, "tcp://localhost:%s", serverPort3);

    void *responder = zmq_socket(context, ZMQ_PULL);
    int rc = zmq_bind(responder, myServerIP);
    assert (rc == 0);

    srandom ((unsigned) time (NULL));
	int myRandomNum = rand() % 500;

	char sendBuffer [10] = {0};
	sprintf(sendBuffer, "%d", myRandomNum);

    void *reqServer2 = zmq_socket(context, ZMQ_PUSH);
    void *reqServer3 = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(reqServer2, serverPort2);
    zmq_connect(reqServer3, serverPort3);

    zmq_pollitem_t items [] = {
        { reqServer2, 0, ZMQ_POLLIN, 0 },
        { reqServer3, 0, ZMQ_POLLIN, 0 }
    };

	printf("here\n");
	zmq_send(reqServer2, sendBuffer, 5, 0);
	printf("here as well\n");
	zmq_send(reqServer3, sendBuffer, 5, 0);

    //  Process messages from both sockets
    while(1)
	{
        char msg [256];

        zmq_poll(items, 2, -1);
        if(items [0].revents & ZMQ_POLLIN)
		{
            int size = zmq_recv(reqServer2, msg, 255, 0);
            if(size != -1)
			{
				myRandomNum += atoi(msg);
				counter++;
            }
        }
        if(items [1].revents & ZMQ_POLLIN)
		{
            int size = zmq_recv(reqServer3, msg, 255, 0);
            if(size != -1) {
				myRandomNum += atoi(msg);
				counter++;
            }
        }
		if (counter == 2) break;
    }

	printf("The final number is : [%d]\n", myRandomNum);
    zmq_close(reqServer2);
    zmq_close(reqServer3);
    zmq_close(responder);
    zmq_ctx_destroy(context);
    return 0;
}
