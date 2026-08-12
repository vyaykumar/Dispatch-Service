#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "transport.h"
#include "../workload/workload_types.h"

namespace protocol {

    using FieldMap = std::pmr::unordered_map<uint8_t, std::vector<uint8_t>>;
    using T_ID = std::string;

    enum class MessageType : uint8_t {
        kTaskSubmit = 1,
        kTaskAck = 2,
        kTaskResult = 3,
        kCancel = 4,
    };

    enum class TaskStatus : uint8_t {
        kPending = 0,
        kRunning = 1,
        kSucceeded = 2,
        kFailed = 3,
        kInProgress = 4,
    };

    enum class FieldId : uint8_t {
        kTaskId = 1,
        kIdempotencyKey = 2,
        kStatus = 3,
        kPayload = 4,
        kWorkload = 5,
    };

    enum class ParseError : uint8_t {
        TruncHeader = 0,
        TruncValue
    };

    // ---- TLV encode/decode ----

    class FieldWriter {
    public:

        void AddField (FieldId id, const uint32_t length) {
            payload_.emplace_back(static_cast<uint8_t>(id));
            for (size_t idx {0}; idx < 4; idx++)
                payload_.emplace_back(length >> (8*(3-idx)) & 0xFF);
        }

        void AddString(const FieldId id, std::string_view value) {
            AddField(id,value.size());

            for (const auto& element : value)
                payload_.emplace_back(static_cast<uint8_t>(element));
        }
        void AddByte(const FieldId id, uint8_t value) {
            AddField(id,1);

            payload_.emplace_back(value);
        }
        void AddRaw(const FieldId id, std::span<const uint8_t> value) {
            AddField(id,value.size());

            payload_.insert(payload_.end(), value.begin(), value.end());
        }

        // Inspection copy.
        [[nodiscard]] const std::vector<uint8_t>& payload() const {
            return payload_;
        }

        // Consumption copy. Do not use FieldWriter after usage.
        [[nodiscard]] std::vector<uint8_t> finish() && {
            return std::move(payload_);
        }

    private:
        std::vector<uint8_t> payload_;
    };

    [[nodiscard]] inline std::expected<FieldMap, ParseError> ParseFields (const uint8_t* input_stream, const std::size_t size) {
        FieldMap output {};

        if (size == 0)
            return output;

        size_t offset {0};

        while (offset < size) {
            if (size - offset < 5)
                return std::unexpected(ParseError::TruncHeader);

            const uint8_t type = input_stream[offset++];

            uint32_t length {};

            std::memcpy(&length, input_stream + offset, 4);
            if constexpr (std::endian::native == std::endian::little)
                length = std::byteswap(length);
            offset += 4;

            if (length > size - offset)
                return std::unexpected(ParseError::TruncValue);

            std::vector value(input_stream + offset, input_stream + offset + length);
            offset += length;

            output.insert_or_assign(type, std::move(value));
        }
        return output;
    }

    inline std::string FieldAsString(const FieldMap& fields, FieldId id) {
        const auto it = fields.find(static_cast<uint8_t>(id));
        if (it == fields.end()) return {};
        return {it->second.begin(), it->second.end()};
    }


    // ---- Message structs ----

    struct TaskSubmit {
        T_ID task_id;
        std::string idempotency_key;
        std::vector<uint8_t> payload;
        workload::Workload workload = workload::Workload::SlowSuccess;
    };

    struct TaskAck {
        T_ID task_id;
    };

    struct TaskResult {
        T_ID task_id;
        TaskStatus status;
        std::vector<uint8_t> payload;  // result data, or error info if failed
    };

    struct Cancel {
        T_ID task_id;
    };


    // ---- Encode: struct -> raw frame, ready for transport::SendFrame ----

    inline bool SendTaskSubmit(const transport::socket_t socket, const TaskSubmit& message) {
        FieldWriter writer;
        writer.AddString(FieldId::kTaskId, message.task_id);
        writer.AddString(FieldId::kIdempotencyKey, message.idempotency_key);
        writer.AddRaw(FieldId::kPayload, message.payload);
        writer.AddByte(FieldId::kWorkload, static_cast<uint8_t>(message.workload));
        return transport::SendFrame(socket, static_cast<uint8_t>(MessageType::kTaskSubmit), std::move(writer).finish());
    }

    inline bool SendTaskAck(transport::socket_t socket, const TaskAck& message) {
        FieldWriter writer;
        writer.AddString(FieldId::kTaskId, message.task_id);
        return transport::SendFrame(socket, static_cast<uint8_t>(MessageType::kTaskAck), std::move(writer).finish());
    }

    inline bool SendTaskResult(transport::socket_t socket, const TaskResult& message) {
        FieldWriter writer;
        writer.AddString(FieldId::kTaskId, message.task_id);
        writer.AddByte(FieldId::kStatus, static_cast<uint8_t>(message.status));
        writer.AddRaw(FieldId::kPayload, message.payload);
        return transport::SendFrame(socket, static_cast<uint8_t>(MessageType::kTaskResult), std::move(writer).finish());
    }

    inline bool SendCancel(transport::socket_t socket, const Cancel& message) {
        FieldWriter writer;
        writer.AddString(FieldId::kTaskId, message.task_id);
        return transport::SendFrame(socket, static_cast<uint8_t>(MessageType::kCancel), std::move(writer).finish());
    }


    // ---- Decode: raw frame -> typed message ----

    struct DecodedMessage {
        MessageType type;
        TaskSubmit t_submit;
        TaskAck t_ack;
        TaskResult t_result;
        Cancel cancel;
    };

    enum class ReceiveError {
        TimeOut,
        ConnectionClosed,
        ParseError
    };

    inline std::optional<DecodedMessage> ReceiveMessage(const transport::socket_t socket) {
        auto raw = transport::RecvFrame(socket);
        if (!raw) return std::nullopt;

        auto fields = ParseFields(raw->payload.data(), raw->payload.size());
        if (!fields) return std::nullopt;

        DecodedMessage message{};
        message.type = static_cast<MessageType>(raw->type);

        switch (message.type) {
            case MessageType::kTaskSubmit: {
                message.t_submit.task_id = FieldAsString(*fields, FieldId::kTaskId);
                message.t_submit.idempotency_key = FieldAsString(*fields, FieldId::kIdempotencyKey);
                if (auto pay_it = fields->find(static_cast<uint8_t>(FieldId::kPayload)); pay_it != fields->end())
                    message.t_submit.payload = pay_it->second;
                if (auto work_it = fields->find(static_cast<uint8_t>(FieldId::kWorkload));
                    work_it != fields->end() and !work_it->second.empty())
                    message.t_submit.workload = static_cast<workload::Workload>(work_it->second[0]);
                break;
            }
            case MessageType::kTaskAck: {
                message.t_ack.task_id = FieldAsString(*fields, FieldId::kTaskId);
                break;
            }
            case MessageType::kTaskResult: {
                message.t_result.task_id = FieldAsString(*fields, FieldId::kTaskId);
                auto statusIt = fields->find(static_cast<uint8_t>(FieldId::kStatus));
                message.t_result.status = statusIt != fields->end() && !statusIt->second.empty()
                                         ? static_cast<TaskStatus>(statusIt->second[0])
                                         : TaskStatus::kFailed;
                auto payloadIt = fields->find(static_cast<uint8_t>(FieldId::kPayload));
                if (payloadIt != fields->end()) message.t_result.payload = payloadIt->second;
                break;
            }
            case MessageType::kCancel: {
                message.cancel.task_id = FieldAsString(*fields, FieldId::kTaskId);
                break;
            }
            default:
                return std::nullopt;  // unknown top-level message type
        }
        return message;
    }

}  // namespace protocol
