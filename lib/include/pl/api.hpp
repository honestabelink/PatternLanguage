#pragma once

#include <pl/core/token.hpp>
#include <pl/helpers/types.hpp>
#include <pl/helpers/fs.hpp>
#include <pl/helpers/file.hpp>

#include <cmath>
#include <vector>
#include <functional>
#include <string>
#include <optional>

namespace pl {

    class PatternLanguage;

    namespace core { class Evaluator; }

}

namespace pl::api {

    using PragmaHandler = std::function<bool(PatternLanguage&, const std::string &)>;
    using DataSourceHandler = std::function<void(u64, const u8 *, size_t)>;

    constexpr static u64 SKIP = 0xFFFF'FFFF'FFFF'FFFF;

    class Section {
    public: 
        Section(const std::string& name) : m_name(name), m_data({})  {}
        std::string m_name;

        std::vector<u8>& data() { return m_data; };
        const pl::u8 * dataPtr() const { return m_data.data(); };
        virtual size_t size() const = 0;
        virtual void resize(size_t size) = 0;
        virtual void readData(u64 address, u8* buffer, size_t size) = 0;
        virtual void writeData(u64 srcOffset, u64 dstOffset, const u8* buffer, size_t size) = 0;
    protected : 
        std::vector<u8> m_data;
    };

    class SectionMemoryBacked : public Section {
    public: 
        SectionMemoryBacked(const std::string& name) : Section(name)  {}
        
        size_t size() const override { return m_data.size(); };
        void resize(size_t size) override { m_data.resize(size); };
        void readData(u64 address, u8* buffer, size_t size) override {
            std::memcpy(buffer, dataPtr() + address, size);
        }
        void writeData([[maybe_unused]] u64 srcOffset, u64 dstOffset, const u8* buffer, size_t size) override {
            std::memcpy(&m_data[0] + dstOffset, buffer, size);
        }
    };

    class SectionDataSourceBacked : public Section {
    public :
        SectionDataSourceBacked(const std::string& name, size_t size, DataSourceHandler readFunction, DataSourceHandler writeFunction) 
            : Section(name), m_size(size), m_readFunction(readFunction), m_writeFunction(writeFunction) {}

        size_t size() const override { return m_size; }
        void resize(size_t size) override { m_data.resize(size); };
        void readData(u64 address, u8* buffer, size_t size) override {
            m_readFunction(address, buffer, size);
        }
        void writeData(u64 srcOffset, u64 dstOffset, const u8* buffer, size_t size) override {
            if (srcOffset != SKIP) m_readFunction(srcOffset, buffer, size);
            if (dstOffset != SKIP) m_writeFunction(dstOffset, buffer, size);
        }
    private :
        size_t m_size;
        DataSourceHandler m_readFunction;
        DataSourceHandler m_writeFunction;
    };

    class SectionFileBacked : public Section {
    public : 
        SectionFileBacked(const std::string& name, const std::string& path) : Section(name){
            m_file = hlp::fs::File(path, hlp::fs::File::Mode::Write);
        }
        size_t size() const override { return m_file.getSize(); }
        void resize(size_t size) override { m_file.setSize(size); };
        void readData(u64 address, u8* buffer, size_t size) override {
            m_file.seek(address);
            m_file.readBuffer(buffer, size);
        }
        void writeData(u64 srcOffset, u64 dstOffset, [[maybe_unused]] const u8* buffer, size_t size) override {
            m_file.seek(srcOffset);
            std::vector<u8> data(size, 0x00);
            if (srcOffset != SKIP) {
                m_file.readBuffer(data.data(), size);
            }
            m_file.seek(dstOffset);
            if (dstOffset != SKIP) m_file.write(data.data(), size);
        }
    private :
        hlp::fs::File m_file;
    };


    struct FunctionParameterCount {
        FunctionParameterCount() = default;

        constexpr bool operator==(const FunctionParameterCount &other) const {
            return this->min == other.min && this->max == other.max;
        }

        [[nodiscard]] static FunctionParameterCount unlimited() {
            return FunctionParameterCount { 0, 0xFFFF'FFFF };
        }

        [[nodiscard]] static FunctionParameterCount none() {
            return FunctionParameterCount { 0, 0 };
        }

        [[nodiscard]] static FunctionParameterCount exactly(u32 value) {
            return FunctionParameterCount { value, value };
        }

        [[nodiscard]] static FunctionParameterCount moreThan(u32 value) {
            return FunctionParameterCount { value + 1, 0xFFFF'FFFF };
        }

        [[nodiscard]] static FunctionParameterCount lessThan(u32 value) {
            return FunctionParameterCount { 0, u32(std::max<i64>(i64(value) - 1, 0)) };
        }

        [[nodiscard]] static FunctionParameterCount atLeast(u32 value) {
            return FunctionParameterCount { value, 0xFFFF'FFFF };
        }

        [[nodiscard]] static FunctionParameterCount between(u32 min, u32 max) {
            return FunctionParameterCount { min, max };
        }

        u32 min = 0, max = 0;
    private:
        FunctionParameterCount(u32 min, u32 max) : min(min), max(max) { }
    };

    using Namespace = std::vector<std::string>;
    using FunctionCallback  = std::function<std::optional<core::Token::Literal>(core::Evaluator *, const std::vector<core::Token::Literal> &)>;

    struct Function {
        FunctionParameterCount parameterCount;
        std::vector<core::Token::Literal> defaultParameters;
        FunctionCallback func;
        bool dangerous;
    };

}