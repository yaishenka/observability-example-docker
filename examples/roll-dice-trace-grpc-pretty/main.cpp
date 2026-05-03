#include "log.hpp"

#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/provider.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"

#include <argparse/argparse.hpp>

#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

namespace trace     = opentelemetry::trace;
namespace sdk       = opentelemetry::sdk;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace otlp      = opentelemetry::exporter::otlp;

class TracerGuard {
public:
  TracerGuard(std::string grpcEndpoint) {
    InitTracer(std::move(grpcEndpoint));
  }

  ~TracerGuard() {
    CleanupTracer();
  }

private:
  void InitTracer(std::string grpcEndpoint) {
    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
    opts.endpoint = grpcEndpoint;
    auto exporter  = otlp::OtlpGrpcExporterFactory::Create(opts);
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

    sdk::resource::ResourceAttributes attributes = {{"service.name", "roll-dice-service"}};
    auto resource = sdk::resource::Resource::Create(attributes);
    std::shared_ptr<opentelemetry::trace::TracerProvider> provider  = trace_sdk::TracerProviderFactory::Create(std::move(processor), resource);

    trace_api::Provider::SetTracerProvider(provider);
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

    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("roll-dice");
    auto handle_span = tracer->StartSpan("rolldice");
    auto handle_scope = tracer->WithActiveSpan(handle_span);
    int random = 0;
    {
      auto roll_span = tracer->StartSpan("roll");
      auto roll_scope = tracer->WithActiveSpan(roll_span);

      int low = 1;
      int high = 7;
      random = rand() % (high - low) + low;

      roll_span->SetAttribute("roll_value", random);
    }

    const std::string response = std::to_string(random);
    Logging::Logger().Info() << "Roll dice ready to return " << random;
    handle_span->SetAttribute("response", response);
    handle_span->End();

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

int main(int argc, char** argv) {
  argparse::ArgumentParser program("dice-server");

  program.add_argument("--log")
    .default_value(std::string("roll_dice.log"))
    .help("Path to store log");

  program.add_argument("--trace-url")
    .help("Path to send traces (grpc endpoint)");

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  Logging::Logger().Init(program.get<std::string>("--log"));

  TracerGuard tracer(program.get<std::string>("--trace-url"));

  srand((int)time(0));
  run();
  oatpp::base::Environment::destroy();
}
