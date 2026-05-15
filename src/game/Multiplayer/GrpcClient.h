#pragma once

#include <grpcpp/grpcpp.h>
#include "multiplayer.grpc.pb.h"
#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>

class MultiplayerClient {
public:
    static MultiplayerClient& GetInstance() {
        static MultiplayerClient instance;
        return instance;
    }

    bool Connect(const std::string& address, const std::string& player_name);
    void Disconnect();

    bool SendInput(const ja2::multiplayer::PlayerInput& input);
    bool PollEvent(ja2::multiplayer::StateEvent& out_event);

    const ja2::multiplayer::MatchState& GetMatchState() const { return match_state_; }

private:
    MultiplayerClient() = default;
    ~MultiplayerClient();

    void ReceiveEventsLoop();

    std::unique_ptr<ja2::multiplayer::MultiplayerGame::Stub> stub_;
    std::unique_ptr<grpc::ClientContext> stream_context_;
    std::unique_ptr<grpc::ClientReader<ja2::multiplayer::StateEvent>> reader_;

    std::queue<ja2::multiplayer::StateEvent> event_queue_;
    std::mutex event_mutex_;

    std::thread receive_thread_;
    std::atomic<bool> connected_{false};

    ja2::multiplayer::MatchState match_state_;
};
