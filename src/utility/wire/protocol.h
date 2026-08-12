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
    using TaskId = std::string;

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
        std::string taskId;
        std::string idempotencyKey;
        std::vector<uint8_t> payload;
        work_l::Workload workload = work_l::Workload::SlowSuccess;
    };

    struct TaskAck {
        std::string taskId;
    };

    struct TaskResult {
        std::string t_id;
        TaskStatus status;
        std::vector<uint8_t> payload;  // result data, or error info if failed
    };

    struct Cancel {
        std::string taskId;
    };


    // ---- Encode: struct -> raw frame, ready for transport::SendFrame ----

    inline bool SendTaskSubmit(transport::socket_t s, const TaskSubmit& msg) {
        FieldWriter w;
        w.AddString(FieldId::kTaskId, msg.taskId);
        w.AddString(FieldId::kIdempotencyKey, msg.idempotencyKey);
        w.AddRaw(FieldId::kPayload, msg.payload);
        w.AddByte(FieldId::kWorkload, static_cast<uint8_t>(msg.workload));
        return transport::SendFrame(s, static_cast<uint8_t>(MessageType::kTaskSubmit), std::move(w).finish());
    }

    inline bool SendTaskAck(transport::socket_t s, const TaskAck& msg) {
        FieldWriter w;
        w.AddString(FieldId::kTaskId, msg.taskId);
        return transport::SendFrame(s, static_cast<uint8_t>(MessageType::kTaskAck), std::move(w).finish());
    }

    inline bool SendTaskResult(transport::socket_t s, const TaskResult& msg) {
        FieldWriter w;
        w.AddString(FieldId::kTaskId, msg.t_id);
        w.AddByte(FieldId::kStatus, static_cast<uint8_t>(msg.status));
        w.AddRaw(FieldId::kPayload, msg.payload);
        return transport::SendFrame(s, static_cast<uint8_t>(MessageType::kTaskResult), std::move(w).finish());
    }

    inline bool SendCancel(transport::socket_t s, const Cancel& msg) {
        FieldWriter w;
        w.AddString(FieldId::kTaskId, msg.taskId);
        return transport::SendFrame(s, static_cast<uint8_t>(MessageType::kCancel), std::move(w).finish());
    }


    // ---- Decode: raw frame -> typed message ----

    struct DecodedMessage {
        MessageType type;
        TaskSubmit submit;
        TaskAck ack;
        TaskResult result;
        Cancel cancel;
    };

    enum class ReceiveError {
        TimeOut,
        ConnectionClosed,
        ParseError
    };

    inline std::optional<DecodedMessage> ReceiveMessage(const transport::socket_t s) {
        auto raw = transport::RecvFrame(s);
        if (!raw) return std::nullopt;

        auto fields = ParseFields(raw->payload.data(), raw->payload.size());
        if (!fields) return std::nullopt;

        DecodedMessage msg{};
        msg.type = static_cast<MessageType>(raw->type);

        switch (msg.type) {
            case MessageType::kTaskSubmit: {
                msg.submit.taskId = FieldAsString(*fields, FieldId::kTaskId);
                msg.submit.idempotencyKey = FieldAsString(*fields, FieldId::kIdempotencyKey);
                if (auto pay_it = fields->find(static_cast<uint8_t>(FieldId::kPayload)); pay_it != fields->end())
                    msg.submit.payload = pay_it->second;
                if (auto work_it = fields->find(static_cast<uint8_t>(FieldId::kWorkload));
                    work_it != fields->end() and !work_it->second.empty())
                    msg.submit.workload = static_cast<work_l::Workload>(work_it->second[0]);
                break;
            }
            case MessageType::kTaskAck: {
                msg.ack.taskId = FieldAsString(*fields, FieldId::kTaskId);
                break;
            }
            case MessageType::kTaskResult: {
                msg.result.t_id = FieldAsString(*fields, FieldId::kTaskId);
                auto statusIt = fields->find(static_cast<uint8_t>(FieldId::kStatus));
                msg.result.status = statusIt != fields->end() && !statusIt->second.empty()
                                         ? static_cast<TaskStatus>(statusIt->second[0])
                                         : TaskStatus::kFailed;
                auto payloadIt = fields->find(static_cast<uint8_t>(FieldId::kPayload));
                if (payloadIt != fields->end()) msg.result.payload = payloadIt->second;
                break;
            }
            case MessageType::kCancel: {
                msg.cancel.taskId = FieldAsString(*fields, FieldId::kTaskId);
                break;
            }
            default:
                return std::nullopt;  // unknown top-level message type
        }
        return msg;
    }

}  // namespace protocol
