#include "connection_internal.h"

using namespace simpleipc;

void connection_internal::handle_data_available() {
    while (true) {
        if (buffer_off == buffer.size()) {
            if (buffer_start_off == 0) {
                // not enough space in the buffer to handle the message
                if (buffer.size() < MAX_BUFFER_SIZE) {
                    buffer.resize(std::min(buffer.size() * 2, (size_t) MAX_BUFFER_SIZE));
                } else {
                    // too big message
                    // TODO: disconnect
                    return;
                }
            } else {
                // move the data to the start of the buffer
                memcpy(&buffer[0], &buffer[buffer_start_off], buffer_off - buffer_start_off);
                buffer_off -= buffer_start_off;
                buffer_start_off = 0;
            }
        }
        ssize_t n = read_data(&buffer.data()[buffer_off], buffer.size() - buffer_off);
        if (n < 0)
            return;
        size_t check_start = buffer_off;
        buffer_off += n;
        // process all messages
        while (true) {
            ssize_t c = current_encoding->check_read_message_complete(&buffer.data()[buffer_start_off],
                                                                      buffer_off - buffer_start_off,
                                                                      check_start - buffer_start_off);
            if (c < 0)
                break;
            try {
                current_encoding->read_message(&buffer.data()[buffer_start_off], (size_t) c, current_message);
                on_message(current_message);
            } catch (std::exception& e) {
                printf("Error processing message: %s\n", e.what());
                // TODO: disconnect
            }
            current_message.clear();

            buffer_start_off += c;
            check_start = buffer_off;
        }
    }
}

void connection_internal::on_message(message_container const& msg) {
    switch (msg.type) {
        case message_container::message_type::rpc: {
            printf("Got rpc message: method=%s, data=%s\n", current_message.message_rpc.method().c_str(),
                   current_message.message_rpc.data().dump().c_str());
            handler->handle_message(*this, msg.message_response);
            break;
        }
        case message_container::message_type::response: {
            printf("Got reponse message: data=%s\n", current_message.message_rpc.data().dump().c_str());
            handler->handle_message(*this, msg.message_response);
            break;
        }
        case message_container::message_type::error: {
            printf("Got error message: code=%i, message=%s, data=%s\n", current_message.message_error.error_code(),
                   current_message.message_error.error_text().c_str(),
                   current_message.message_error.data().dump().c_str());
            handler->handle_message(*this, msg.message_error);
            break;
        }
        default:
            throw std::runtime_error("Invalid message type");
    }
}