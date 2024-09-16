#pragma once
#include <string>

namespace esphome {
namespace ecodan 
{
    // https://github.com/m000c400/Mitsubishi-CN105-Protocol-Decode
    enum class MsgType : uint8_t
    {
        SET_CMD = 0x41,
        SET_RES = 0x61,
        GET_CMD = 0x42,
        GET_RES = 0x62,
        CONNECT_CMD = 0x5A,
        CONNECT_RES = 0x7A,
        GET_CONFIGURATION = 0x5B,
        CONFIGURATION_RES = 0x7B
    };

    enum class SetType : uint8_t
    {
        BASIC_SETTINGS = 0x32,
        CONTROLLER_SETTING = 0x34,
        ROOM_SETTINGS = 0x35,
        REMOTE_ROOM_SETTINGS = 0x07
    };

    enum class CONTROLLER_FLAG : uint8_t
    {
        FORCED_DHW = 0x01,
        HOLIDAY_MODE = 0x02,
        PROHIBIT_DHW = 0x04,
        PROHIBIT_Z1_HEATING = 0x08,
        PROHIBIT_Z1_COOLING = 0x10,
        PROHIBIT_Z2_HEATING = 0x20,
        PROHIBIT_Z2_COOLING = 0x40,
        SERVER_CONTROL = 0x80
    };

#define SET_SETTINGS_FLAG_ZONE_TEMPERATURE 0x80
#define SET_SETTINGS_FLAG_DHW_TEMPERATURE 0x20
#define SET_SETTINGS_FLAG_HP_MODE_ZONE1 0x08
#define SET_SETTINGS_FLAG_HP_MODE_ZONE2 0x10
#define SET_SETTINGS_FLAG_DHW_MODE 0x04
#define SET_SETTINGS_FLAG_MODE_TOGGLE 0x1
#define SET_SETTINGS_FLAG_HOLIDAY_MODE_TOGGLE 0x2


    enum class SetZone
    {
        ZONE_1,
        ZONE_2,
        BOTH
    };

    enum class SetHpMode
    {
        TEMPERATURE_MODE,
        FLOW_CONTROL_MODE,
        COMPENSATION_CURVE_MODE
    };

    enum class GetType : uint8_t
    {
        UNKNOWN_0x01 = 0x01,
        DEFROST_STATE = 0x02,
        ERROR_STATE = 0x03,
        COMPRESSOR_FREQUENCY = 0x04,
        DHW_STATE = 0x05,
        UNKNOWN_0x06 = 0x06,
        HEATING_POWER = 0x07,
        TEMPERATURE_CONFIG = 0x09,
        SH_TEMPERATURE_STATE = 0x0B,
        TEMPERATURE_STATE_A = 0x0C,
        TEMPERATURE_STATE_B = 0x0D,
        TEMPERATURE_STATE_C = 0x0E,
        UNKNOWN_0x0F = 0x0F,
        EXTERNAL_STATE = 0x10,
        UNKNOWN_0x11 = 0x11,
        ACTIVE_TIME = 0x13,
        FLOW_RATE = 0x14,
        PUMP_STATUS = 0x15,
        UNKNOWN_0x16 = 0x16,
        UNKNOWN_0x17 = 0x17,
        UNKNOWN_0x18 = 0x18,
        UNKNOWN_0x19 = 0x19,
        UNKNOWN_0x1A = 0x1A,
        UNKNOWN_0x1B = 0x1B,
        UNKNOWN_0x1C = 0x1C,
        UNKNOWN_0x1D = 0x1D,
        UNKNOWN_0x1E = 0x1E,
        UNKNOWN_0x1F = 0x1F,
        UNKNOWN_0x20 = 0x20,
        MODE_FLAGS_A = 0x26,
        UNKNOWN_0x27 = 0x27, // dhw eco mode setting        
        MODE_FLAGS_B = 0x28,        
        UNKNOWN_0x29 = 0x29, 
        ENERGY_USAGE = 0xA1,
        ENERGY_DELIVERY = 0xA2,
        HARDWARE_CONFIGURATION = 0xC9
    };

    template <class T>
    inline T operator &(const T& lhs, const T& rhs)
    {
        return static_cast<T>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    template <class T>
    inline T operator |(const T& lhs, const T& rhs)
    {
        return static_cast<T>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    const uint8_t HEADER_SIZE = 5;
    const uint8_t PAYLOAD_SIZE = 16;
    const uint8_t CHECKSUM_SIZE = sizeof(uint8_t);
    const uint8_t TOTAL_MSG_SIZE = HEADER_SIZE + PAYLOAD_SIZE + sizeof(uint8_t);
    const uint8_t MSG_TYPE_OFFSET = 1;
    const uint8_t PAYLOAD_SIZE_OFFSET = 4;

    const uint8_t HEADER_MAGIC_A = 0xFC;
    const uint8_t HEADER_MAGIC_B = 0x02;
    const uint8_t HEADER_MAGIC_C = 0x7A;

    struct Message
    {
        Message()
            : cmd_{false}, buffer_{}
        {
        }

        Message(MsgType msgType)
            : cmd_{true}, buffer_{HEADER_MAGIC_A, static_cast<uint8_t>(msgType), HEADER_MAGIC_B, HEADER_MAGIC_C, 0x00}, writeOffset_(HEADER_SIZE)
        {
        }

        Message(MsgType msgType, SetType setType)
            : cmd_{true}, buffer_{HEADER_MAGIC_A, static_cast<uint8_t>(msgType), HEADER_MAGIC_B, HEADER_MAGIC_C, 0x00}, writeOffset_(HEADER_SIZE)
        {
            // All SET_CMD messages have 15-bytes of zero payload.
            char payload[PAYLOAD_SIZE] = {};
            payload[0] = static_cast<uint8_t>(setType);
            write_payload(payload, sizeof(payload));
        }

        Message(MsgType msgType, GetType getType)
            : cmd_{true}, buffer_{HEADER_MAGIC_A, static_cast<uint8_t>(msgType), HEADER_MAGIC_B, HEADER_MAGIC_C, 0x00}, writeOffset_(HEADER_SIZE)
        {
            // All GET_CMD messages have 15-bytes of zero payload.
            char payload[PAYLOAD_SIZE] = {};
            payload[0] = static_cast<uint8_t>(getType);
            write_payload(payload, sizeof(payload));
        }

        Message(Message&& other)
        {
            cmd_ = other.cmd_;
            memcpy(buffer_, other.buffer_, sizeof(buffer_));
            writeOffset_ = other.writeOffset_;
            valid_ = other.valid_;
            other.valid_ = false;
        }

        Message& operator=(Message&& other)
        {
            cmd_ = other.cmd_;
            memcpy(buffer_, other.buffer_, sizeof(buffer_));
            writeOffset_ = other.writeOffset_;
            valid_ = other.valid_;
            other.valid_ = false;

            return *this;
        }

        std::string debug_dump_packet()
        {
            if (!valid_)
                std::string("");
            
            char result[1024];

            snprintf(result, 1024, "%s { .Hdr { %02X, %02X, %02X, %02X, %02X } .Payload { %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X } .Chk { %02X } }",
                    cmd_ ? "CMD" : "RES",
                    buffer_[0], buffer_[1], buffer_[2], buffer_[3], buffer_[4],
                    buffer_[5], buffer_[6], buffer_[7], buffer_[8], buffer_[9],
                    buffer_[10], buffer_[11], buffer_[12], buffer_[13], buffer_[14],
                    buffer_[15], buffer_[16], buffer_[17], buffer_[18], buffer_[19], buffer_[20],
                    buffer_[21]);
            
            return std::string(result);
        }

        bool verify_header()
        {
            if (buffer_[0] != HEADER_MAGIC_A)
                return false;

            if (buffer_[2] != HEADER_MAGIC_B)
                return false;

            if (buffer_[3] != HEADER_MAGIC_C)
                return false;

            if (payload_size() > PAYLOAD_SIZE)
                return false;

            memset(payload(), 0, PAYLOAD_SIZE + CHECKSUM_SIZE);

            return true; // Looks like a valid header!
        }

        MsgType type() const
        {
            return static_cast<MsgType>(buffer_[MSG_TYPE_OFFSET]);
        }

        template<typename T>
        T payload_type() const
        {
            return static_cast<T>(buffer_[HEADER_SIZE]);
        }

        uint8_t* buffer()
        {
            return std::addressof(buffer_[0]);
        }

        size_t size() const
        {
            return HEADER_SIZE + payload_size() + CHECKSUM_SIZE;
        }

        size_t payload_size() const
        {
            return buffer_[PAYLOAD_SIZE_OFFSET];
        }

        uint8_t* payload()
        {
            return buffer_ + HEADER_SIZE;
        }

        bool write_header(const char* data, uint8_t length)
        {
            if (length != HEADER_SIZE)
                return false;

            memcpy(buffer_, data, length);
            writeOffset_ = HEADER_SIZE;
            valid_ = true;
            return true;
        }

        bool write_payload(const uint8_t *data, uint8_t length)
        {
            if (length > PAYLOAD_SIZE)
                return false;

            if (data != nullptr)
            {
                memcpy(payload(), data, length);
            }
            else if (length != 0)
            {
                return false;
            }

            memset(payload() + length, 0, PAYLOAD_SIZE - length);
            buffer_[PAYLOAD_SIZE_OFFSET] = length;
            writeOffset_ = HEADER_SIZE + length;
            valid_ = true;
            return true;
        }
        bool write_payload(const char *data, uint8_t length)
        {
            return write_payload((const uint8_t *)data, length);
        }

        void append_byte(const char data)
        {
            if (writeOffset_ < TOTAL_MSG_SIZE) {
                buffer_[writeOffset_] = data;
                writeOffset_++;
                valid_ = true;
            }
        }

        void set_checksum()
        {
            buffer_[writeOffset_] = calculate_checksum();
        }

        uint8_t get_write_offset()
        {
            return writeOffset_;
        }

        bool verify_checksum()
        {
            uint8_t v = calculate_checksum();
            if (writeOffset_ < size())
                return false;
            if (v == buffer_[size() - 1])
                return true;
            
            return false;
        }

        float get_float24(size_t index)
        {
            float value = uint16_t(payload()[index] << 8) | payload()[index + 1];
            float remainder = payload()[index + 2];
            return value + (remainder / 100.0f);
        }

        // runtime
        float get_float24_v2(size_t index)
        {
            float value = uint16_t(payload()[index + 1] << 8) | payload()[index + 2];
            float remainder = payload()[index];
            return value * 100 + remainder;
        }

        float get_float16(size_t index)
        {
            float value = uint16_t(payload()[index] << 8) | payload()[index + 1];
            return value /= 100.0f;
        }

        float get_float16_signed(size_t index)
        {
            float value = int16_t(payload()[index] << 8) | payload()[index + 1];
            return value /= 100.0f;
        }

        // Used for most single-byte floating point values
        float get_float8(size_t index)
        {
            float value = payload()[index];
            return (value / 2) - 40.0f;
        }

        // Used for DHW temperature dropand  min/max SH flow temperature threshold
        float get_float8_v2(size_t index)
        {
            float value = payload()[index];
            return (value - 40.0f) / 2;
        }

        uint16_t get_u16(size_t index)
        {
            return uint16_t(payload()[index] << 8) | payload()[index + 1];
        }

        void set_float16(float value, size_t index)
        {
            uint16_t u16 = uint16_t(value * 100.0f);

            payload()[index] = u16 >> 8;
            payload()[index + 1] = u16 & 0xff;
        }

        uint8_t& operator[](size_t index)
        {
            return payload()[index];
        }

      private:
        uint8_t calculate_checksum()
        {
            uint8_t checkSum = 0;
            if (writeOffset_ < size() - 1)
                return 0;
            for (size_t i = 0; i < size() - 1; ++i)
                checkSum += buffer_[i];

            checkSum = 0xFC - checkSum;
            return checkSum & 0xFF;
        }

        bool cmd_;
        bool valid_ = false;
        uint8_t buffer_[TOTAL_MSG_SIZE];
        uint8_t writeOffset_ = 0;
    };
} // namespace ecodan
} // namespace esphome
