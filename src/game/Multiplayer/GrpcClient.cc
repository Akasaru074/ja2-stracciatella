#include "GrpcClient.h"
#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

MultiplayerClient::~MultiplayerClient() {
    Disconnect();
}

bool MultiplayerClient::Connect(const std::string& address, const std::string& player_name) {
    if (connected_) return false;

    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    stub_ = ja2::multiplayer::MultiplayerGame::NewStub(channel);

    // 1. Join Match
    ja2::multiplayer::JoinRequest join_request;
    join_request.set_player_name(player_name);

    ClientContext context;
    Status status = stub_->JoinMatch(&context, join_request, &match_state_);

    if (!status.ok() || !match_state_.success()) {
        std::cerr << "Failed to join match: " << status.error_message() << std::endl;
        return false;
    }

    std::cout << "Successfully joined match! " << match_state_.message() << std::endl;

    // 2. Start streaming events in a background thread
    connected_ = true;
    stream_context_ = std::make_unique<ClientContext>();
    ja2::multiplayer::StreamRequest stream_req;
    reader_ = stub_->StateStream(stream_context_.get(), stream_req);

    receive_thread_ = std::thread(&MultiplayerClient::ReceiveEventsLoop, this);

    return true;
}

void MultiplayerClient::Disconnect() {
    if (!connected_) return;

    connected_ = false;
    if (stream_context_) {
        stream_context_->TryCancel();
    }

    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    stub_.reset();
    stream_context_.reset();
    reader_.reset();
}

bool MultiplayerClient::SendInput(const ja2::multiplayer::PlayerInput& input) {
    if (!connected_) return false;

    ja2::multiplayer::InputResponse reply;
    ClientContext context;
    Status status = stub_->SendInput(&context, input, &reply);

    if (!status.ok()) {
        std::cerr << "SendInput failed: " << status.error_message() << std::endl;
        return false;
    }
    return reply.accepted();
}

void MultiplayerClient::ReceiveEventsLoop() {
    ja2::multiplayer::StateEvent event;
    while (connected_ && reader_->Read(&event)) {
        std::lock_guard<std::mutex> lock(event_mutex_);
        event_queue_.push(event);
    }
    std::cout << "Event stream ended." << std::endl;
}

bool MultiplayerClient::PollEvent(ja2::multiplayer::StateEvent& out_event) {
    std::lock_guard<std::mutex> lock(event_mutex_);
    if (event_queue_.empty()) return false;
    
    out_event = event_queue_.front();
    event_queue_.pop();
    return true;
}
