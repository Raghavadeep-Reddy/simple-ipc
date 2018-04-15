#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include "rpc_result.h"

namespace simpleipc {

class rpc_message;
class connection;

class rpc_handler {

public:
    using result_handler = std::function<void (rpc_result)>;

    using call_handler_async = std::function<void (connection& conn, std::string const& method, nlohmann::json const& data, result_handler)>;
    using call_handler_async_data_only = std::function<void (connection& conn, nlohmann::json const& data, result_handler)>;

    using call_handler_sync = std::function<rpc_result (connection& conn, std::string const& method, nlohmann::json const& data)>;
    using call_handler_sync_data_only = std::function<rpc_result (connection& conn, nlohmann::json const& data)>;

private:
    std::map<std::string, call_handler_async> handlers;

public:
    void add_handler(std::string const& method, call_handler_async handler);

    void add_handler(std::string const& method, call_handler_async_data_only handler) {
        add_handler(method, std::bind(handler, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
    }

    void add_handler(std::string const& method, call_handler_sync handler) {
        add_handler(method, [handler](connection& conn, std::string const& method, nlohmann::json const& data, result_handler result) {
            result(handler(conn, method, data));
        });
    }

    void add_handler(std::string const& method, call_handler_sync_data_only handler) {
        add_handler(method, [handler](connection& conn, std::string const& method, nlohmann::json const& data, result_handler result) {
            result(handler(conn, data));
        });
    }


    void invoke(connection& conn, std::string const& method, nlohmann::json const& data, result_handler handler);

    void invoke(connection& conn, rpc_message const& msg);

};

}