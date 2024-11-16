#ifndef REVE_H_
#define REVE_H_

#include "../Eve.hpp"
#include "../Reactive.hpp"
#include "FS.hpp"
#include <algorithm>
#include <bits/types/struct_sched_param.h>
#include <chrono>
#include <filesystem>
#include <functional>
#include <source_location>
#include <stacktrace>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>
// #include "Asset.hpp"

namespace reve {

    struct REvent
    {
        enum EventType
        {
            FILECHANGE,
            LOADED,
            CHECK
        };

        using id = EventType;
        const id name;

        struct CheckFile {
            std::filesystem::path file;
        };
        struct readFile {
            std::filesystem::path file;
        };
        struct fileData {
            std::filesystem::path file;
            std::span<char> data;
        };
        using Data1 = std::variant<C, L, Check>;
            struct Data
            {
                Data1 data;
                std::source_location src;
                template<class T> requires (!std::is_same_v<std::remove_cvref_t<T>, Data1>)
                Data(T &&data) : data(std::move(data)), src(std::source_location::current())
                    {
                        std::cout << "Created!" << std::endl;
                    }
                Data(Data && copy)  noexcept : data(std::move(copy.data)), src(copy.src)
                    {
                        std::cout << "Move: " << src.function_name() << " in " << std::stacktrace::current()<< std::endl;
                    }
                Data(const Data & copy) : data(copy.data), src(copy.src)
                    {
                        if(!std::holds_alternative<Check>(data))
                            std::cout << "Copy: " << src.function_name() << " in " << std::stacktrace::current()<< std::endl;
                    }
            };
        Data data;
        // using value_type = Data;

        template<class T>
        auto getData() const -> T
        {
            return std::get<std::remove_cvref_t<T>>(data.data);
        }

        template<class T>
        REvent(id name, T &&data)
            : name(name), data(data)
        {}
        REvent(REvent &&move)
            : name(move.name), data(std::move(move.data))
        {}
        REvent(const REvent &move)
            : name(move.name), data(move.data)
        {
            // std:: cout << "Event copied from: " << std::stacktrace::current() << std::endl;
        }
    };

    using namespace eve;
    using EV = Eve<eve::event_queue::DefaultQueue<REvent>,
        modules::React, modules::Interval, modules::Async>;

    class ReveStandalone : public reactive::Reactive<EV>
    {
        using path = std::filesystem::path;
        using version = std::filesystem::file_time_type;

        struct FSHandle
        {
            std::string name;
            std::unique_ptr<fs::FS> fs;
        };
        std::list<FSHandle> m_sources;

        struct FileInfo
        {
            version version;
            eve::modules::IntervalHandle interval;
            reve::fs::FS *origin;
        };
        std::unordered_map<path, version> m_fileVersions;

        EV ev;

        auto onLoaded(const REvent::L &loaded)
        {
            std::cout << "Loaded: " << loaded.file << std::endl;
            // std::cout << "Loaded: " << str(data.loaded.data) << std::endl;
        }
        auto onFileChanged(const REvent::C &changed)
        {
            std::cout << "Changed: " << changed.file << std::endl;
        }
        auto onCheck(const REvent::Check _)
        {
            std::ranges::for_each(m_fileVersions, [this](auto &pair){
                version now = std::filesystem::last_write_time(pair.first);
                if(now!=pair.second){
                    pair.second = now;
                    ev.emplaceEvent(REvent::FILECHANGE, REvent::C{pair.first});
                }
            });
        }

        public:

            auto addSourceFront(std::string &&name, std::unique_ptr<fs::FS> &&fs)
            {
                m_sources.emplace_front(name, fs);
            }
            auto addSourceBack(std::string &&name, std::unique_ptr<fs::FS> &&fs)
            {
                m_sources.emplace_back(name, fs);
            }
            auto removeSource(std::string_view name)
            {
                m_sources.remove_if([name](auto p){return p.first==name;});
            }
            template<class T>
            auto get(path file) -> void
            {
                m_fileVersions[file] = {};
            }
            ReveStandalone(std::chrono::milliseconds check_interval=std::chrono::milliseconds(400))
                : reactive::Reactive<EV>(ev)
            {
                this->addHandler(REvent::id::FILECHANGE, &::reve::ReveStandalone::onFileChanged);
                this->addHandler(REvent::id::LOADED, &ReveStandalone::onLoaded);
                this->addHandler(REvent::id::CHECK, &ReveStandalone::onCheck);

                ev.addInterval({REvent::CHECK, REvent::Check{}}, check_interval, true);
            }

            auto run() -> void
            {
                ev.step();
            }


    };
}

#endif // REVE_H_
