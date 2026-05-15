#pragma once

#include <grpcpp/grpcpp.h>
#include "multiplayer.grpc.pb.h"
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <condition_variable>

class GrpcServerImpl final : public ja2::multiplayer::MultiplayerGame::Service {
public:
    grpc::Status JoinMatch(grpc::ServerContext* context, const ja2::multiplayer::JoinRequest* request, ja2::multiplayer::MatchState* reply) override;
    grpc::Status SendInput(grpc::ServerContext* context, const ja2::multiplayer::PlayerInput* request, ja2::multiplayer::InputResponse* reply) override;
    grpc::Status StateStream(grpc::ServerContext* context, const ja2::multiplayer::StreamRequest* request, grpc::ServerWriter<ja2::multiplayer::StateEvent>* writer) override;

    void PushStateEvent(const ja2::multiplayer::StateEvent& event);
    bool PopInput(ja2::multiplayer::PlayerInput& out_input);
    void Stop();

private:
    std::queue<ja2::multiplayer::PlayerInput> input_queue_;
    std::mutex input_mutex_;

    std::queue<ja2::multiplayer::StateEvent> event_queue_;
    std::mutex event_mutex_;
    std::condition_variable event_cv_;
    bool stopped_ = false;
};

class MultiplayerServer {
public:
    static MultiplayerServer& GetInstance() {
        static MultiplayerServer instance;
        return instance;
    }
    
    void Start(int port);
    void Stop();

    void BroadcastStateEvent(const ja2::multiplayer::StateEvent& event);
    bool PollInput(ja2::multiplayer::PlayerInput& out_input);

private:
    MultiplayerServer() = default;
    ~MultiplayerServer();

    std::unique_ptr<grpc::Server> server_;
    GrpcServerImpl service_;
};
