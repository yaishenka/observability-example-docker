#include "log.hpp"

#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"


#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/provider.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"

#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

namespace trace_api      = opentelemetry::trace;
namespace trace_sdk      = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

class TracerGuard {
public:
  TracerGuard() {
    InitTracer();
  }

  ~TracerGuard() {
    CleanupTracer();
  }

private:
  void InitTracer() {
    auto exporter  = trace_exporter::OStreamSpanExporterFactory::Create();
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    std::shared_ptr<trace_api::TracerProvider> provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor));
    trace_sdk::Provider::SetTracerProvider(provider);
  }

  void CleanupTracer() {
    std::shared_ptr<trace_api::TracerProvider> none;
    trace_sdk::Provider::SetTracerProvider(none);
  }
};

class Handler : public oatpp::web::server::HttpRequestHandler {
public:
  std::shared_ptr<OutgoingResponse> handle(const std::shared_ptr<IncomingRequest>& request) override {
    Logging::Logger().Info() << "Called /rolldice";
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("my-app-tracer");
    auto span = tracer->StartSpan("RollDiceServer");

    int low = 1;
    int high = 7;
    int random = std::rand() % (high - low) + low;
    const std::string response = std::to_string(random);

    Logging::Logger().Info() << "Roll dice ready to return " << random;
    span->End();

    return ResponseFactory::createResponse(Status::CODE_200, response.c_str());
  }
};

void run() {
  auto router = oatpp::web::server::HttpRouter::createShared();
  router->route("GET", "/rolldice", std::make_shared<Handler>());
  auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);
  auto connectionProvider = oatpp::network::tcp::server::ConnectionProvider::createShared({"0.0.0.0", 8080, oatpp::network::Address::IP_4});
  oatpp::network::Server server(connectionProvider, connectionHandler);

  Logging::Logger().Info() << "Server running on port " <<  static_cast<const char*>(connectionProvider->getProperty("port").getData());

  server.run();
}

int main() {
  Logging::Logger().Init("roll_dice.log");

  TracerGuard tracer;

  srand((int)time(0));
  run();
  oatpp::base::Environment::destroy();
}
