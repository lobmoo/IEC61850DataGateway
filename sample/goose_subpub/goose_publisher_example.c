/*
 * goose_publisher_example.c
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "mms_value.h"
#include "goose_publisher.h"
#include "hal_thread.h"

/* has to be executed as root! */
/*同一个主机应该使用lo而不是网卡*/
int
main(int argc, char **argv)
{
    char *interface;

    if (argc > 1)
        interface = argv[1];
    else
        interface = "eth1";

    printf("Using interface %s\n", interface);

    LinkedList dataSetValues = LinkedList_create();

    LinkedList_add(dataSetValues, MmsValue_newIntegerFromInt32(1234));
    LinkedList_add(dataSetValues, MmsValue_newBinaryTime(false));
    LinkedList_add(dataSetValues, MmsValue_newIntegerFromInt32(5678));

    CommParameters gooseCommParameters;

    gooseCommParameters.appId = 1000;
    uint8_t dstMac[6] = {0x01,0x0c,0xcd,0x01,0x00,0x01};
    memcpy(gooseCommParameters.dstAddress, dstMac, sizeof(dstMac));
    gooseCommParameters.vlanId = 0;
    gooseCommParameters.vlanPriority = 4;

    /*
     * Create a new GOOSE publisher instance. As the second parameter the interface
     * name can be provided (e.g. "eth0" on a Linux system). If the second parameter
     * is NULL the interface name as defined with CONFIG_ETHERNET_INTERFACE_ID in
     * stack_config.h is used.
     */
    GoosePublisher publisher = GoosePublisher_create(&gooseCommParameters, interface);

    if (publisher) {
        GoosePublisher_setGoCbRef(publisher, "simpleIOGenericIO/LLN0$GO$gcbAnalogValues");//simpleIOGenericIO/LLN0$GO$gcbAnalogValues
        GoosePublisher_setConfRev(publisher, 1);
        GoosePublisher_setDataSetRef(publisher, "simpleIOGenericIO/LLN0$AnalogValues");
        GoosePublisher_setTimeAllowedToLive(publisher, 500);

        int i = 0;

        for (i = 0; i < 10000; i++) {
            Thread_sleep(1000);

            if (i == 9999) {
                /* now change dataset to send an invalid GOOSE message */
                LinkedList_add(dataSetValues, MmsValue_newBoolean(true));
                GoosePublisher_publish(publisher, dataSetValues);
            }
            else {
                if (GoosePublisher_publish(publisher, dataSetValues) == -1) {
                    printf("Error sending message!\n");
                }
            }
        }

        GoosePublisher_destroy(publisher);
    }
    else {
        printf("Failed to create GOOSE publisher. Reason can be that the Ethernet interface doesn't exist or root permission are required.\n");
    }

    LinkedList_destroyDeep(dataSetValues, (LinkedListValueDeleteFunction) MmsValue_delete);

    return 0;
}




