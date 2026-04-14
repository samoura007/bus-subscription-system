#include "Server.h"
#include "logic/DataStore.h"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    try {
        bus::DataStore store;
        boost::asio::io_context io;
        bus::Server server(io, 9000, store);
        std::cout << "Press Ctrl+C to stop.\n";
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
