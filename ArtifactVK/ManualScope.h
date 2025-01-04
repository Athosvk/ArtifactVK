#pragma once

// Goes against RAII, but needed to handle (un)init order correctly in some cases.
// Alternatively we use optional/unique ptr, but this is a bit more specific.
// This will only allow creation/destruction once
template <typename T> class ManualScope
{
  public:
    ManualScope() = default;
    ~ManualScope()
    {
        assert((m_Initialized || !m_Inner.has_value()) &&
               "Manual scope not manually destroyed. Prefer using a RAII base mechanism instead");
    }
    ManualScope(const ManualScope &) = delete;

    ManualScope(ManualScope &&other)
        : m_Inner(std::exchange(other.m_Inner, std::nullopt)), m_Initialized(other.m_Initialized)
    {
    }

    const T &operator*() const
    {
        return m_Inner.value();
    }

    T &operator*()
    {
        return m_Inner.value();
    }

    T *operator->()
    {
        return &m_Inner.value();
    }

    const T *operator->() const
    {
        return &m_Inner.value();
    }

    template <typename... Args> void ScopeBegin(Args &&...args)
    {
        assert(!m_Initialized && "Scope value recreated");
        m_Inner.emplace(std::forward<Args>(args)...);
        m_Initialized = true;
    }

    void ScopeEnd()
    {
        assert(m_Initialized && "Scope value not yet created or scope ended multiple times");
        m_Inner = std::nullopt;
    }

  private:
    bool m_Initialized;
    std::optional<T> m_Inner;
};

