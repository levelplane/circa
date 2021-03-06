// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <pthread.h>

#include "lo/lo.h"

#include "circa/internal/for_hosted_funcs.h"

using namespace circa;

struct Server
{
    lo_server_thread server_thread;

    // incomingMessages is guarded by a mutex.
    pthread_mutex_t mutex;
    circa::List incomingMessages;

    Server() : server_thread(NULL)
    {
        pthread_mutex_init(&mutex, NULL);
    }
    ~Server() {
        pthread_mutex_destroy(&mutex);
        if (server_thread != NULL)
            lo_server_thread_stop(server_thread);
    }
};

void ServerRelease(caValue* value)
{
    delete (Server*) as_opaque_pointer(value);
}

void AddressRelease(caValue* value)
{
    lo_address_free((lo_address) as_opaque_pointer(value));
}

Name name_Server = name_from_string("Server");
Name name_Address = name_from_string("Address");

void error_callback(int num, const char *m, const char *path)
{
    printf("lo reported error: %d, %s, %s\n", num, m, path);
}

int incoming_message_callback(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
    Server* server = (Server*) user_data;

    List message;
    message.resize(argc + 1);
    set_string(message[0], path);

    for (int i=0; i < argc; i++) {
        char type = types[i];
        caValue* val = message[i + 1];
        if (type == 'i')
            set_int(val, argv[i]->i);
        else if (type == 'f')
            set_float(val, argv[i]->f);
        else if (type == 's')
            set_string(val, &argv[i]->s);
        else {
            printf("osc incoming: couldn't handle type code: %c\n", type);
            return 1;
        }
    }

    int ret = 0;

    ret = pthread_mutex_lock(&server->mutex);
    swap(&message, server->incomingMessages.append());
    ret = pthread_mutex_unlock(&server->mutex);

    return 1;
}

EXPORT CA_FUNCTION(osc__create_server_thread)
{
    int port = INT_INPUT(0);
    char portStr[15];
    sprintf(portStr, "%d", port);

    Server* server = new Server();

    server->server_thread = lo_server_thread_new(portStr, error_callback);

    if (server->server_thread == NULL) {
        RAISE_ERROR("lo_server_thread_new failed");
        set_null(OUTPUT);
        return;
    }

    lo_server_thread_add_method(server->server_thread, NULL, NULL, incoming_message_callback,
            (void*) server);
    lo_server_thread_start(server->server_thread);

    set_handle_value_opaque_pointer(OUTPUT, CALLER->type, server, ServerRelease);
}

EXPORT CA_FUNCTION(osc__read_from_server)
{
    Server* server = (Server*) get_handle_value_opaque_pointer(INPUT(0));

    List incoming;

    pthread_mutex_lock(&server->mutex);
    swap(&incoming, &server->incomingMessages);
    pthread_mutex_unlock(&server->mutex);

    swap(&incoming, OUTPUT);
}

EXPORT CA_FUNCTION(osc__address)
{
    int port = INT_INPUT(1);
    char portStr[20];
    sprintf(portStr, "%d", port);

    lo_address address = lo_address_new(STRING_INPUT(0), portStr);
    set_handle_value_opaque_pointer(OUTPUT, CALLER->type, address, AddressRelease);
}

EXPORT CA_FUNCTION(osc__send)
{
    const char* destination = STRING_INPUT(1);

    const int c_maxArguments = 15;

    if ((NUM_INPUTS + 2) > c_maxArguments)
        return RAISE_ERROR("too many arguments");

    lo_message message = lo_message_new();

    bool failed = false;

    for (int i=2; i < NUM_INPUTS; i++) {
        caValue* val = INPUT(i);

        if (is_int(val))
            lo_message_add(message, "i", as_int(val));
        else if (is_float(val))
            lo_message_add(message, "f", as_float(val));
        else if (is_string(val))
            lo_message_add(message, "s", as_cstring(val));
        else {
            std::stringstream error;
            error << "Don't know how to send value " << val->toString()
                << " (with type " << val->value_type->name << ")";
            RAISE_ERROR(error.str());
            failed = true;
            break;
        }
    }

    if (!failed) {
        lo_address address = get_handle_value_opaque_pointer(INPUT(0));
        int result = lo_send_message(address, destination, message);

        if (result == -1)
            RAISE_ERROR("lo_send_message returned -1");
    }

    lo_message_free(message);
}

EXPORT void on_load(Branch* branch)
{
    Type* server = get_declared_type(branch, "osc:Server");
    Type* address = get_declared_type(branch, "osc:Address");
    setup_handle_type(server);
    setup_handle_type(address);
}
