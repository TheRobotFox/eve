#ifndef ASSET_H_
#define ASSET_H_

namespace reve {
    template<typename T>
    class Asset
    {
        friend AssetGroup<T>;
        friend typename AssetGroup<T>::Derivitive;

        std::shared_ptr<T> m_val;

        const Info &m_info;
        int m_localVersion;
        Asset(std::shared_ptr<T> val, const Info &info)
            : m_val(std::move(val)),
                m_info(info),
                m_localVersion(info.version)
        {}
        static auto load(std::span<const uint8_t> /* ignore */) -> std::optional<T> {
            static_assert(false, "Asset::load was not Implemented for used Type!\nPlease supply via template Specialisation");
        };
        static auto getDefault(Info::State /* ignore */) -> T& {
            static T obj{};
            return obj;
        }
    public:
        auto get() -> const T& {
            if(m_info.state==Info::State::LOADED)
                return *m_val;
            return Asset<T>::getDefault(m_info.state);
        }

        operator const T&() {return get();}
        auto has_changed() const -> bool {return m_localVersion != m_info.version;}
        auto fetch_version()     -> bool
        {
            bool res = this->has_changed();
            m_localVersion = m_info.version;
            return res;
        }
            auto getState() -> Info::State
            {
                return m_info.state;
            }

    };

}

#endif // ASSET_H_
