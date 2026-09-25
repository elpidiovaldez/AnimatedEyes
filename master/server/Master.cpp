#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <zmq.hpp>

class EndpointMaster {
public:

    EndpointMaster(const std::string &configFileName) {
        auto configFilePath = std::filesystem::canonical("/proc/self/exe").parent_path() / configFileName;
        load(configFilePath.string());
    }

    void run(zmq::context_t &context) {
        zmq::socket_t socket(context, zmq::socket_type::rep);
        std::string address = getMasterAddress();
        socket.bind(address);

        std::cout << "Endpoint master listening on " << address << '\n';

        while (true) {

            zmq::message_t request;
            auto received = socket.recv(request, zmq::recv_flags::none);
            if (!received) {
                continue;
            }

            std::string name(static_cast<char *>(request.data()), request.size());
            auto reply = lookup(name);
            zmq::message_t response(reply.data(), reply.size());


            socket.send(response, zmq::send_flags::none);
        }
    }

private:

    void flatten(const YAML::Node &node, const std::string &prefix) {
        if (node.IsMap()) {

            for (const auto &entry : node) {
                std::string key = entry.first.as<std::string>();
                std::string new_prefix = prefix.empty() ? key : prefix + "." + key;
                flatten(entry.second, new_prefix);
            }

        } else {

            endpoints[prefix] =  node.as<std::string>();
            std::cout << prefix << " -> " << endpoints[prefix]  << '\n';

        }
    }

    void load(const std::string &filename) {
        YAML::Node config = YAML::LoadFile(filename);
        flatten(config, "");
    }    
    
    std::string lookup(const std::string &name) {

            auto it = endpoints.find(name);
            if (it == endpoints.end()) {
                return "ERROR: unknown endpoint";
            }

            return it->second;
        }

    std::string maskHost(const std::string &address) {

        auto start = address.find("://");
        if(start == std::string::npos)
            return address;

        start += 3;

        auto end = address.find(':', start);
        if(end == std::string::npos)
            end = address.size();

        std::string result = address;
        result.replace(start, end - start, "*");

        return result;
    }

    std::string getMasterAddress() {
        const char *value = std::getenv("MASTER");

        if (!value) {
            throw std::runtime_error("MASTER environment variable not set");
        }

        return std::string(maskHost(value));
    }

    std::unordered_map<std::string, std::string> endpoints;
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " endpoints.yaml\n";
        return 1;
    }

    try {

        zmq::context_t context(1);
        EndpointMaster master(argv[1]);
        master.run(context);

    } catch (const std::exception &e) {

        std::cerr << "Error: " << e.what() << '\n';
        return 1;

    }

    return 0;
}
