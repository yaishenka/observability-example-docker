#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

#include "opentelemetry/exporters/otlp/otlp_grpc_client_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_options.h"

#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/provider.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"

#include "opentelemetry/logs/provider.h"
#include "opentelemetry/sdk/logs/exporter.h"
#include "opentelemetry/sdk/logs/logger_provider.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/processor.h"
#include "opentelemetry/sdk/logs/provider.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"

#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "opentelemetry/sdk/metrics/meter.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
#include "opentelemetry/sdk/metrics/provider.h"
#include "opentelemetry/sdk/metrics/view/instrument_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/meter_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/view_factory.h"
#include "opentelemetry/sdk/metrics/view/view_registry_factory.h"

#include <argparse/argparse.hpp>

#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

namespace trace       = opentelemetry::trace;
namespace sdk         = opentelemetry::sdk;
namespace trace_sdk   = opentelemetry::sdk::trace;
namespace logs_sdk    = opentelemetry::sdk::logs;
namespace otlp        = opentelemetry::exporter::otlp;
namespace logs        = opentelemetry::logs;
namespace metric_sdk  = opentelemetry::sdk::metrics;
namespace common      = opentelemetry::common;
namespace metrics_api = opentelemetry::metrics;

class TracerGuard {
public:
  TracerGuard(std::string grpcEndpoint) {
    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
    opts.endpoint = std::move(grpcEndpoint);

    std::shared_ptr<otlp::OtlpGrpcClient> sharedClient = otlp::OtlpGrpcClientFactory::Create(opts);
    sdk::resource::ResourceAttributes attributes = {{"service.name", "roll-dice-service"}};
    auto resource = sdk::resource::Resource::Create(attributes);

    InitLogger(sharedClient, resource);
    InitTracer(opts, sharedClient, resource);
    InitMetrics(sharedClient, resource);
  }

  ~TracerGuard() {
    CleanupTracer();
    CleanupLogger();
    CleanupMetrics();
  }

private:
  void InitTracer(
    const opentelemetry::exporter::otlp::OtlpGrpcExporterOptions& opts,
    const std::shared_ptr<otlp::OtlpGrpcClient>& sharedClient,
    const sdk::resource::Resource& resource) {
    auto exporter  = otlp::OtlpGrpcExporterFactory::Create(opts, sharedClient);
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

    tracerProvider = trace_sdk::TracerProviderFactory::Create(std::move(processor), resource);

    std::shared_ptr<opentelemetry::trace::TracerProvider> apiProvider = tracerProvider;
    trace_sdk::Provider::SetTracerProvider(apiProvider);
  }

  void CleanupTracer() {
    if (tracerProvider) {
      tracerProvider->ForceFlush();
    }

    tracerProvider.reset();
    std::shared_ptr<opentelemetry::trace::TracerProvider> none;
    trace_sdk::Provider::SetTracerProvider(none);
  }

  void InitLogger(
    const std::shared_ptr<otlp::OtlpGrpcClient>& sharedClient,
    const sdk::resource::Resource& resource) {
    opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporterOptions logOpts;

    auto exporter   = otlp::OtlpGrpcLogRecordExporterFactory::Create(logOpts, sharedClient);
    auto processor  = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));

    loggerProvider = logs_sdk::LoggerProviderFactory::Create(std::move(processor), resource);

    std::shared_ptr<opentelemetry::logs::LoggerProvider> apiProvider = loggerProvider;
    logs_sdk::Provider::SetLoggerProvider(apiProvider);
  }

  void CleanupLogger() {
    if (loggerProvider) {
      loggerProvider->ForceFlush();
    }

    loggerProvider.reset();
    std::shared_ptr<logs::LoggerProvider> none;
    logs_sdk::Provider::SetLoggerProvider(none);
  }

  void InitMetrics(
    const std::shared_ptr<otlp::OtlpGrpcClient>& sharedClient,
    const sdk::resource::Resource& resource) {
    otlp::OtlpGrpcMetricExporterOptions exporterOptions;
    auto exporter = otlp::OtlpGrpcMetricExporterFactory::Create(exporterOptions, sharedClient);

    std::string version{"1.2.0"};
    std::string schema{"https://opentelemetry.io/schemas/1.2.0"};

    metric_sdk::PeriodicExportingMetricReaderOptions readerOptions;
    readerOptions.export_interval_millis = std::chrono::milliseconds(1000);
    readerOptions.export_timeout_millis  = std::chrono::milliseconds(500);

    auto reader =
        metric_sdk::PeriodicExportingMetricReaderFactory::Create(std::move(exporter), readerOptions);

    auto views = metric_sdk::ViewRegistryFactory::Create();
    auto context = metric_sdk::MeterContextFactory::Create(std::move(views), resource);
    context->AddMetricReader(std::move(reader));

    auto provider = metric_sdk::MeterProviderFactory::Create(std::move(context));

    std::shared_ptr<opentelemetry::metrics::MeterProvider> apiProvider(std::move(provider));

    metric_sdk::Provider::SetMeterProvider(apiProvider);
  }

  void CleanupMetrics() {
    std::shared_ptr<metrics_api::MeterProvider> none;
    metric_sdk::Provider::SetMeterProvider(none);
  }


  std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> tracerProvider;
  std::shared_ptr<opentelemetry::sdk::logs::LoggerProvider> loggerProvider;
};

class Handler : public oatpp::web::server::HttpRequestHandler {
public:
  std::shared_ptr<OutgoingResponse> handle(const std::shared_ptr<IncomingRequest>& request) override {
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("roll-dice");
    auto handle_span = tracer->StartSpan("rolldice");
    auto handle_scope = tracer->WithActiveSpan(handle_span);

    auto logger =  logs::Provider::GetLoggerProvider()->GetLogger("roll-dice-logger", "roll-dice");
    logger->Info("Handle /rolldice");

    std::string counterName = "rolldice_counter";
    auto provider = metrics_api::Provider::GetMeterProvider();
    opentelemetry::nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("rolldice", "1.2.0");
    auto counter = meter->CreateUInt64Counter(counterName);
    counter->Add(1);

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
    logger->Info(std::format("Roll dice ready to return {}", random));

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

  std::cerr << "Server running on port " <<  static_cast<const char*>(connectionProvider->getProperty("port").getData());

  server.run();
}

int main(int argc, char** argv) {
  argparse::ArgumentParser program("dice-server");

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

  TracerGuard tracer(program.get<std::string>("--trace-url"));

  srand((int)time(0));
  run();
  oatpp::base::Environment::destroy();
}
