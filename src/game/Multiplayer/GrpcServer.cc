#include "GrpcServer.h"
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

grpc::Status GrpcServerImpl::JoinMatch(ServerContext* context, const ja2::multiplayer::JoinRequest* request, ja2::multiplayer::MatchState* reply) {
    std::cout << "Client joined: " << request->player_name() << std::endl;
    reply->set_success(true);
    reply->set_message("Welcome to JA2 Stracciatella Multiplayer!");
    // In a real implementation, we would set the correct random_seed, host_team, client_team here based on GameInit state.
    reply->set_random_seed(12345);
    reply->set_host_team(0); // OUR_TEAM
    reply->set_client_team(1); // ENEMY_TEAM
    return Status::OK;
}

grpc::Status GrpcServerImpl::SendInput(ServerContext* context, const ja2::multiplayer::PlayerInput* request, ja2::multiplayer::InputResponse* reply) {
    std::lock_guard<std::mutex> lock(input_mutex_);
    input_queue_.push(*request);
    reply->set_accepted(true);
    return Status::OK;
}

grpc::Status GrpcServerImpl::StateStream(ServerContext* context, const ja2::multiplayer::StreamRequest* request, ServerWriter<ja2::multiplayer::StateEvent>* writer) {
    std::cout << "Client connected to StateStream" << std::endl;
    while (!context->IsCancelled()) {
        ja2::multiplayer::StateEvent event;
        {
            std::unique_lock<std::mutex> lock(event_mutex_);
            event_cv_.wait(lock, [this, context] { 
                return !event_queue_.empty() || stopped_ || context->IsCancelled(); 
            });
            
            if (stopped_ || context->IsCancelled()) {
                break;
            }
            event = event_queue_.front();
            event_queue_.pop();
        }
        writer->Write(event);
    }
    std::cout << "Client disconnected from StateStream" << std::endl;
    return Status::OK;
}

void GrpcServerImpl::PushStateEvent(const ja2::multiplayer::StateEvent& event) {
    {
        std::lock_guard<std::mutex> lock(event_mutex_);
        event_queue_.push(event);
    }
    event_cv_.notify_all();
}

bool GrpcServerImpl::PopInput(ja2::multiplayer::PlayerInput& out_input) {
    std::lock_guard<std::mutex> lock(input_mutex_);
    if (input_queue_.empty()) return false;
    out_input = input_queue_.front();
    input_queue_.pop();
    return true;
}

void GrpcServerImpl::Stop() {
    {
        std::lock_guard<std::mutex> lock(event_mutex_);
        stopped_ = true;
    }
    event_cv_.notify_all();
}

MultiplayerServer::~MultiplayerServer() {
    Stop();
}

void MultiplayerServer::Start(int port) {
    std::string server_address("0.0.0.0:" + std::to_string(port));
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;
}

void MultiplayerServer::Stop() {
    service_.Stop();
    if (server_) {
        server_->Shutdown();
        server_.reset();
    }
}

void MultiplayerServer::BroadcastStateEvent(const ja2::multiplayer::StateEvent& event) {
    service_.PushStateEvent(event);
}

bool MultiplayerServer::PollInput(ja2::multiplayer::PlayerInput& out_input) {
    return service_.PopInput(out_input);
}
