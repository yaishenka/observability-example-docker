#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

class Handler : public oatpp::web::server::HttpRequestHandler {
public:
  std::shared_ptr<OutgoingResponse> handle(const std::shared_ptr<IncomingRequest>& request) override {
    std::cerr << "INFO " << "Roll dice called" << std::endl;

    int low = 1;
    int high = 7;
    int random = rand() % (high - low) + low;

    const std::string response = std::to_string(random);

    std::cerr << "INFO " << "Roll dice ready to return " << random << std::endl;

    return ResponseFactory::createResponse(Status::CODE_200, response.c_str());
  }
};

void run() {
  auto router = oatpp::web::server::HttpRouter::createShared();
  router->route("GET", "/rolldice", std::make_shared<Handler>());
  auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);
  auto connectionProvider = oatpp::network::tcp::server::ConnectionProvider::createShared({"0.0.0.0", 8080, oatpp::network::Address::IP_4});
  oatpp::network::Server server(connectionProvider, connectionHandler);

  std::cerr << "INFO " << "Server running on port " <<  static_cast<const char*>(connectionProvider->getProperty("port").getData()) << std::endl;

  server.run();
}

int main() {
  oatpp::base::Environment::init();
  srand((int)time(0));
  run();
  oatpp::base::Environment::destroy();
  return 0;
}
