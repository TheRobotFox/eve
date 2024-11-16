#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <span>
#include <vector>

namespace reve::fs {

    using path = std::filesystem::path;
    using file_time = std::filesystem::file_time_type;

    using fileData = std::span<const char>;
    using filePromise = std::future<fileData>;

    struct FS
    {
        virtual auto get(const path &) const -> filePromise;
        virtual auto free(fileData &) const -> filePromise;
        virtual auto contains(const path &) const -> bool = 0;
    };

    struct TrackedFS
    {
        virtual auto has_changed(const path &file) const -> bool = 0;
    };

    class Storage;
    class StorageFile : public FileData
    {
        friend Storage;
        std::vector<uint8_t> m_data;

        file_time m_time;
    public:
        StorageFile(const path &file)
            : m_data(read(file)),
              m_time(std::filesystem::last_write_time(file))
        {}
        auto get() const -> std::span<const uint8_t> override
        {
            return m_data;
        }
        auto getTime() const -> file_time
        {
            return m_time;
        }
        auto freeData() -> void override
        {
            m_data = {};
        }
        static auto read(path path) -> std::vector<uint8_t>
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            file.unsetf(std::ios::skipws);

            std::size_t length = static_cast<std::size_t>(file.tellg());
            std::vector<uint8_t> res;
            res.reserve(length);
            file.seekg(0, std::ios::beg);
            std::copy(std::istream_iterator<uint8_t>(file),
                      std::istream_iterator<uint8_t>(),
                      std::back_inserter(res));
            return res;
        }
    };

    class Storage : public FS, public TrackedFS
    {
        const path m_directory;

    public:
        Storage(path directory="")
            : m_directory(std::move(directory))
        {}

        auto get(const path &file) const -> std::unique_ptr<FileData> override
        {
            return std::make_unique<StorageFile>(m_directory/file);
        }
        auto has_changed(const path &file, const FileData *from) const -> bool override
        {
            const auto *sf = dynamic_cast<const StorageFile*>(from);
            assert(sf!=nullptr);
            return std::filesystem::last_write_time(file)!=sf->getTime();
        }
        auto contains(const path &file) const -> bool override
        {
            return std::filesystem::exists(file);
        }
    };

    class MemoryFile : public FileData
    {
        std::span<const uint8_t> m_data;
    public:
        MemoryFile(std::span<const uint8_t> &&data)
            : m_data(data)
        {}
        auto get() const -> std::span<const uint8_t> override
        {
            return m_data;
        }
    };

    class Memory : public FS
    {
        struct Block {
            std::size_t offset;
            std::size_t size;
        };

    protected:
        using Header = std::unordered_map<path, Block>;
        const std::optional<std::vector<uint8_t>> m_own;
        Header m_header;
        std::span<const uint8_t> m_data;

        template<class T>
        static auto read(const uint8_t *&ptr) -> T
        {
            T res = *reinterpret_cast<const T*>(ptr);
            ptr+=sizeof(T);
            return res;
        }
        static auto read_name(const uint8_t *&ptr) -> std::string_view
        {
            auto len = read<uint16_t>(ptr);
            std::string_view name(reinterpret_cast<const char*>(ptr), len);
            ptr+=len;
            return name;
        }
        auto from_span(std::span<const uint8_t> data) -> void
        {
            const uint8_t *ptr = data.data();
            auto entries = read<uint16_t>(ptr);

            while(entries-- > 0) {
                auto name = read_name(ptr);
                auto offset = read<std::size_t>(ptr);
                auto size = read<std::size_t>(ptr);

                m_header[name] = {offset, size};
            }
            m_data = {ptr, m_data.end().base()};
        }
    public:

        Memory(const path &file)
            : m_own(StorageFile::read(file))
        {
            from_span(*m_own);
        }
        Memory(std::span<uint8_t> data)
            : m_own()
        {
            from_span(data);
        }

        auto get(const path &file) const -> std::unique_ptr<FileData> override
        {
            const Block &b = m_header.at(file);
            return std::make_unique<MemoryFile>(m_data.subspan(b.offset, b.size));
        }
        auto contains(const path &file) const -> bool override
        {
            return m_header.contains(file);
        }
    };

    class AssetFile : public FileData
    {
        std::unique_ptr<FileData> m_root;
        FS *m_origin;
    public:
        AssetFile(std::unique_ptr<FileData> &&root, FS *source)
            : m_root(std::move(root)),
              m_origin(source)
        {}
        void freeData() override
        {
            m_root->freeData();
        }
        auto get() const -> std::span<const uint8_t> override
        {
            return m_root->get();
        }
        auto unwrap() const -> const FileData*
        {
            return m_root.get();
        }
        auto getOrigin() const -> FS*
        {
            return m_origin;
        }
    };

    class AssetFiles : public FS, public TrackedFS
    {
        // maybe setting to keep initial FS from cache
        struct FSInfo {
            std::string name;
            std::unique_ptr<FS> fs;
        };
        std::list<FSInfo> m_sources;
        auto getFileFS(const path &file) const
        {
            auto it = std::ranges::find_if(m_sources, [&file](auto &info){return info.fs->contains(file);});
            assert(it!=m_sources.end());
            return it;
        }
    public:
        template<class F>
            requires std::is_base_of_v<FS, F>
        auto add_top(std::string name, std::unique_ptr<F> &&fs) -> void
        {
            m_sources.emplace_back(std::move(name), std::move(fs));
        }
        template<class F>
            requires std::is_base_of_v<FS, F>
        auto add_bottom(std::string name, std::unique_ptr<F> &&fs) -> void
        {
            m_sources.emplace_front(std::move(name), std::move(fs));
        }
        auto remove(const std::string &name) -> void
        {
            auto it = std::ranges::find_if(m_sources, [&name](FSInfo &info){return info.name==name;});
            if(it==m_sources.end()) return;

            m_sources.erase(it);
        }

        auto get(const path &file) const -> std::unique_ptr<FileData> override
        {
            auto it = getFileFS(file);
            return std::make_unique<AssetFile>(it->fs->get(file), it->fs.get());
        }
        auto has_changed(const path &file, const FileData *from) const -> bool override
        {
            const auto *loaded = dynamic_cast<const AssetFile*>(from);
            assert(loaded!=nullptr);

            auto it = getFileFS(file);

            if(loaded->getOrigin()!=it->fs.get()) return true; // if origin changes prop best to assume data changed

            if(auto *t = dynamic_cast<TrackedFS*>(it->fs.get()); t!=nullptr)
                return t->has_changed(file, loaded->unwrap());
            return false;
        }
        auto contains(const path &file) const -> bool override
        {
            auto it = std::ranges::find_if(m_sources, [&file](auto &info){return info.fs->contains(file);});
            return it!=m_sources.end();
        }
    };

    /*class DependencyTracker : public FS
    {
        const path m_save;
        std::unordered_set<path> dependencies; // TODO Thread safety
    public:
        DependencyTracker(path save = "")
            : m_save(std::move(save))
        {}
        auto get() const -> std::vector<path>;
        auto save() const -> void;
        ~DependencyTracker() override;
        };*/
}
