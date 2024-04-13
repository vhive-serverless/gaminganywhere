#include <grpc++/grpc++.h>
#include "gaminganywhere.pb.h"
#include "gaminganywhere.grpc.pb.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerBuilder;
using grpc::ServerContext;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterServiceImpl final : public Greeter::Service {
  	Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
		std::string prefix("Hello ");
		reply->set_message(prefix + request->name());
		return Status::OK;
	}
};
