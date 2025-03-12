#pragma once

#include "CommonIncludes.h"

namespace sockets
{
    template <typename Type>
    struct message_header
    {
        Type id{};
        uint32_t size = 0;
    };

    template <typename Type>
    struct message
    {
        message_header<Type> header{};
        std::vector<uint8_t> body;

        size_t size() const
        {
            return sizeof(message_header<Type>) + body.size();
        }

        friend std::ostream& operator << (std::ostream& stream, const message<Type>& msg)
        {
            stream.write("ID: ", static_cast<int>(msg.header.id)) << " Size: " << msg.header.size;
            return stream;
        }

        template <typename DataType>
        friend message<Type>& operator << (message<Type>& msg, const DataType& data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

            size_t size = msg.body.size();
            msg.body.resize(msg.body.size() + sizeof(DataType));
            memcpy(msg.body.data() + size, &data, sizeof(DataType));
            msg.header.size = static_cast<decltype(msg.header.size)>(msg.size());
            return msg;
        }

        template <typename DataType>
        friend message<Type>& operator >> (message<Type>& msg, DataType& data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
            size_t it = msg.body.size() - sizeof(DataType);
            memcpy(&data, msg.body.data() + it, sizeof(DataType));
            msg.body.resize(it);
            msg.header.size = static_cast<decltype(msg.header.size)>(msg.size());
            return msg;
        }
    };

    template <typename Type>
    class connection;

    template <typename Type>
    struct owned_message
    {
        std::shared_ptr<connection<Type>> remote = nullptr;
        message<Type> msg;

        friend std::ostream& operator << (std::ostream& stream, const owned_message<Type>& msg)
        {
            stream << msg.msg;
            return stream;
        }
    };

};