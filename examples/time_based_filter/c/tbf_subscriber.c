/*******************************************************************************
 (c) 2005-2014 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 RTI grants Licensee a license to use, modify, compile, and create derivative
 works of the Software.  Licensee has the right to distribute object form only
 for use with RTI products.  The Software is provided "as is", with no warranty
 of any type, including any warranty for fitness for any purpose. RTI is under
 no obligation to maintain or support the Software.  RTI shall not be liable for
 any incidental or consequential damages arising out of the use or inability to
 use the software.
 ******************************************************************************/
/* tbf_subscriber.c

   A subscription example

   This file is derived from code automatically generated by the rtiddsgen 
   command:

   rtiddsgen -language C -example <arch> tbf.idl

   Example subscription of type tbf automatically generated by 
   'rtiddsgen'. To test them, follow these steps:

   (1) Compile this file and the example publication.

   (2) Start the subscription with the command
       objs/<arch>/tbf_subscriber <domain_id> <sample_count>

   (3) Start the publication with the command
       objs/<arch>/tbf_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and 
       multicast receive addresses via an environment variable or a file 
       (in the current working directory) called NDDS_DISCOVERY_PEERS. 
       
   You can run any number of publishers and subscribers programs, and can 
   add and remove them dynamically from the domain.
              
                                   
   Example:
        
       To run the example application on domain <domain_id>:
                          
       On UNIX systems: 
       
       objs/<arch>/tbf_publisher <domain_id> 
       objs/<arch>/tbf_subscriber <domain_id> 
                            
       On Windows systems:
       
       objs\<arch>\tbf_publisher <domain_id>  
       objs\<arch>\tbf_subscriber <domain_id>   
       
       
modification history
------------ -------       
*/

#include <stdio.h>
#include <stdlib.h>
#include "ndds/ndds_c.h"
#include "tbf.h"
#include "tbfSupport.h"

#define NANOSECOND 1000000000.0

void tbfListener_on_requested_deadline_missed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedDeadlineMissedStatus *status)
{
}

void tbfListener_on_requested_incompatible_qos(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedIncompatibleQosStatus *status)
{
}

void tbfListener_on_sample_rejected(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleRejectedStatus *status)
{
}

void tbfListener_on_liveliness_changed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_LivelinessChangedStatus *status)
{
}

void tbfListener_on_sample_lost(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleLostStatus *status)
{
}

void tbfListener_on_subscription_matched(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SubscriptionMatchedStatus *status)
{
}

void tbfListener_on_data_available(
    void* listener_data,
    DDS_DataReader* reader)
{
    tbfDataReader *tbf_reader = NULL;
    struct tbfSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i;
    double source_timestamp;
    tbf* data;
    struct DDS_SampleInfo *sample_info;

    tbf_reader = tbfDataReader_narrow(reader);
    if (tbf_reader == NULL) {
        printf("DataReader narrow error\n");
        return;
    }

    retcode = tbfDataReader_take(
        tbf_reader,
        &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
        DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
    if (retcode == DDS_RETCODE_NO_DATA) {
        return;
    } else if (retcode != DDS_RETCODE_OK) {
        printf("take error %d\n", retcode);
        return;
    }

    for (i = 0; i < tbfSeq_get_length(&data_seq); ++i) {
        sample_info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
        if (sample_info->valid_data) {
            data = tbfSeq_get_reference(&data_seq, i);

            /* Here we get source timestamp of the sample using the sample info.
             *
             * info_seq[i].source_timestamp returns DDS_Time_t
             * ({seconds,nanoseconds}). We convert nanoseconds to seconds to get
             * the decimal part of the timestamp.
             */
            source_timestamp = sample_info->source_timestamp.sec +
                    sample_info->source_timestamp.nanosec/NANOSECOND;

            printf("%f\t%d\t\t%d\n",
                    source_timestamp,
                    data->code, data->x);
        }
    }

    retcode = tbfDataReader_return_loan(
        tbf_reader,
        &data_seq, &info_seq);
    if (retcode != DDS_RETCODE_OK) {
        printf("return loan error %d\n", retcode);
    }
}

/* Delete all entities */
static int subscriber_shutdown(
    DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides the finalize_instance() method on
       domain participant factory for users who want to release memory used
       by the participant factory. Uncomment the following block of code for
       clean destruction of the singleton. */
/*
    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        printf("finalize_instance error %d\n", retcode);
        status = -1;
    }
*/

    return status;
}

static int subscriber_main(int domainId, int sample_count)
{
    DDS_DomainParticipant *participant = NULL;
    DDS_Subscriber *subscriber = NULL;
    DDS_Topic *topic = NULL;
    struct DDS_DataReaderListener reader_listener =
        DDS_DataReaderListener_INITIALIZER;
    DDS_DataReader *reader = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    struct DDS_Duration_t poll_period = {4,0};
    /* To change the time-based filter programmatically you will need to declare
     * the DataReaderQoS */
    struct DDS_DataReaderQos datareader_qos = DDS_DataReaderQos_INITIALIZER;


    /* To customize participant QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant(
        DDS_TheParticipantFactory, domainId, &DDS_PARTICIPANT_QOS_DEFAULT,
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize subscriber QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    subscriber = DDS_DomainParticipant_create_subscriber(
        participant, &DDS_SUBSCRIBER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        printf("create_subscriber error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = tbfTypeSupport_get_type_name();
    retcode = tbfTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    topic = DDS_DomainParticipant_create_topic(
        participant, "Example tbf",
        type_name, &DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Set up a data reader listener */
    reader_listener.on_requested_deadline_missed  =
        tbfListener_on_requested_deadline_missed;
    reader_listener.on_requested_incompatible_qos =
        tbfListener_on_requested_incompatible_qos;
    reader_listener.on_sample_rejected =
        tbfListener_on_sample_rejected;
    reader_listener.on_liveliness_changed =
        tbfListener_on_liveliness_changed;
    reader_listener.on_sample_lost =
        tbfListener_on_sample_lost;
    reader_listener.on_subscription_matched =
        tbfListener_on_subscription_matched;
    reader_listener.on_data_available =
        tbfListener_on_data_available;


    /* To customize data reader QoS, use 
       the configuration file USER_QOS_PROFILES.xml */
    reader = DDS_Subscriber_create_datareader(
        subscriber, DDS_Topic_as_topicdescription(topic),
        &DDS_DATAREADER_QOS_DEFAULT, &reader_listener, DDS_STATUS_MASK_ALL);
    if (reader == NULL) {
        printf("create_datareader error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* If you want to change the time-based filter programmatically (e.g., to
     * 2 seconds) rather than using the XML file, you will need to add the
     * following lines to your code and comment out the create_datareader
     * call above. */

       /*datareader_qos.time_based_filter.minimum_separation.sec = 2;
       datareader_qos.time_based_filter.minimum_separation.nanosec = 0;
       reader = DDS_Subscriber_create_datareader(
            subscriber, DDS_Topic_as_topicdescription(topic),
            &datareader_qos, &reader_listener, DDS_STATUS_MASK_ALL);
        if (reader == NULL) {
            printf("create_datareader error\n");
            subscriber_shutdown(participant);
            return -1;
        }*/

    /* Results table header: (1) source timestamp of the sample received;
     * (2) instance id (instance.code value); and (3) value of x (instance.x).
     */
    printf("================================================\n");
    printf("Source Timestamp\tInstance\tX\n");
    printf("================================================\n");

    /* Main loop */
    for (count=0; (sample_count == 0) || (count < sample_count); ++count) {
        /*printf("tbf subscriber sleeping for %d sec...\n",
               poll_period.sec);*/
        NDDS_Utility_sleep(&poll_period);
    }

    /* Cleanup and delete all entities */ 
    return subscriber_shutdown(participant);
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t** argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */
    
    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API, 
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
    
    return subscriber_main(domainId, sample_count);
}
#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API, 
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */
    
    return subscriber_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char* __ctype = NULL;

void usrAppInit ()
{
#ifdef  USER_APPL_INIT
    USER_APPL_INIT;         /* for backwards compatibility */
#endif
    
    /* add application specific code here */
    taskSpawn("sub", RTI_OSAPI_THREAD_PRIORITY_NORMAL, 0x8, 0x150000, (FUNCPTR)subscriber_main, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   
}
#endif
