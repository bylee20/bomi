#ifndef AUDIOFORMAT_HPP
#define AUDIOFORMAT_HPP

class AudioFormat {
public:
    auto operator == (const AudioFormat &rhs) const -> bool
    {
        return m_samplerate == rhs.m_samplerate && m_bitrate == rhs.m_bitrate
                && m_bits == rhs.m_bits && m_channels == rhs.m_channels
                && m_type == rhs.m_type;
    }
    auto operator != (const AudioFormat &rhs) const -> bool
        { return !operator == (rhs); }
    auto samplerate() const -> int { return m_samplerate; }
    auto bitrate() const -> int { return m_bitrate; }
    auto bits() const -> int { return m_bits; }
    auto type() const -> QString { return m_type; }
    auto channels() const -> QString { return m_channels; }
    auto nch() const -> int { return m_nch; }
private:
    friend class AudioController;
    int m_samplerate = 0, m_bitrate = 0, m_bits = 0, m_nch = 0;
    QString m_channels, m_type;
};

#endif // AUDIOFORMAT_HPP
